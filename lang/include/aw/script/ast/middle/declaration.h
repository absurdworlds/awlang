#ifndef aw_script_ast_middle_declaration_h
#define aw_script_ast_middle_declaration_h

#include <aw/script/ast/middle/statement.h>

#include <aw/script/ast/mutability.h>
#include <aw/script/ir/type.h>

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace aw::script::middle {
struct variable {
	std::string name;
	ir::type* type = nullptr;
	ast::access access;
	std::unique_ptr<expression> value;
};

using parameter_list = std::vector<std::unique_ptr<variable>>;

struct function {
	std::string name;

	ir::type* return_type = nullptr;

	parameter_list args;
	bool is_variadic = false;

	std::unique_ptr<statement> body;
};

struct struct_decl {
	std::string name;

	using member_list = std::vector<value_ptr<variable>>;
	member_list members;
};

using declaration_variant = std::variant<
	variable,
	function,
	struct_decl
>;

struct declaration : public declaration_variant
{
	using declaration_variant::declaration_variant;
	using declaration_variant::operator=;

#if 0
	auto name() const -> std::string_view
	{
		auto visitor = [] (const auto& decl) -> std::string_view { return decl.name; };
		return std::visit(visitor, *this);
	}
#endif
};

} // namespace aw::script::middle

#endif // aw_script_ast_middle_declaration_h
