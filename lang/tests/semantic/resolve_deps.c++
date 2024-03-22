#include <aw/script/ast/declaration.h>
#include <aw/script/semantic/dependency_resolver.h>
#include <aw/script/lexer/source_manager.h>
#include <aw/script/utility/wrap.h>

#include <aw/test/test.h>
#include <aw/utility/string/split.h>

TestFile("resolve-deps-test")

namespace aw::script {

static auto make_test_module() -> ast::module
{
	auto mod = ast::module{
		.name = "main"
	};

	mod.decls.emplace_back(ast::import_decl{
		.mod_id = { .name = "test1" },
		.decl_ids = { "func1", "func2" }
	});


	auto f_body = ast::statement_block{};
	f_body.emplace_back(ast::call_expression{
		.func = { .name = "func1" }
	});
	f_body.emplace_back(ast::call_expression{
		.func = { .path = {"test1"}, .name = "func2" }
	});
	f_body.emplace_back(ast::call_expression{
		.func = { .path = {"test2"}, .name = "func2" }
	});

	auto f = ast::function{
		.name = "f",
		.return_type = ast::regular_type{
			.name = {
				.path = { "aw", "math" },
				.name = "vec3"
			}
		},
		.body = wrap(ast::statement(std::move(f_body))),
	};

	f.params.emplace_back(ast::variable{
		.name = "a",
		.type = ast::regular_type{
			.name = {
				.path = { "aw", "math" },
				.name = "vec3"
			}
		}
	});

	mod.decls.push_back(std::move(f));

	return mod;
}

auto make_mod_id(const std::string_view str)
{
	ast::identifier id;
	const auto parts = aw::string::split(str, "::");
	if (parts.empty())
		return id;

	if (parts.size() == 1) {
		id.name = parts.back();
		return id;
	}

	for (const auto& part : parts) {
		id.path.emplace_back(part);
	}
	id.name = id.path.back();
	id.path.pop_back();
	return id;
}

auto module_id_equal(const std::string_view str)
{
	const auto id = make_mod_id(str);
	return [id] (const module_depenency& val) { return val.module_id == id; };
}

Test(predefined_ast) {
	auto mod = make_test_module();

	source_manager srcman;
	diagnostics_engine diag(srcman);
	auto deps = get_module_dependencies(diag, mod);

	auto dep1 = std::find_if(deps.begin(), deps.end(), module_id_equal("test1"));
	auto dep2 = std::find_if(deps.begin(), deps.end(), module_id_equal("test2"));
	auto dep3 = std::find_if(deps.begin(), deps.end(), module_id_equal("aw::math"));

	TestEqual(deps.size(), 3);
	TestAssert(dep1 != deps.end());
	TestAssert(dep2 != deps.end());
	TestAssert(dep3 != deps.end());

	TestEqual(dep1->imported_ids.size(), 2);
	TestEqual(dep2->imported_ids.size(), 1);
	TestEqual(dep3->imported_ids.size(), 1);

	const auto contains = [] (auto&& vec, auto&& f) {
		return std::find(vec.begin(), vec.end(), f) != vec.end();
	};

	TestAssert(contains(dep1->imported_ids, "func1"));
	TestAssert(contains(dep1->imported_ids, "func2"));
	TestAssert(contains(dep2->imported_ids, "func2"));
	TestAssert(contains(dep3->imported_ids, "vec3"));
}

} // namespace aw::script
