
#include "aw/script/diag/diagnostics_engine.h"
#include "aw/script/lexer/lexer.h"
#include "aw/script/lexer/source_manager.h"
#include "aw/script/parser/parser.h"

#include <aw/test/test.h>

TestFile("parser-comments")

using namespace std::string_view_literals;

namespace aw::script {
Test(decl_comment) {
	aw::script::source_manager srcman;

	auto id = srcman.add_buffer(R"(
	// a test function
	func test_func(var x: int) : int;

	// another function
	func test_func2(x: int)
	{
		// print something
		print( "1" )
	}

	// one more function:
	// does nothing
	func test_func3(x: int)
	{
	}
)");

	aw::script::diagnostics_engine diag(srcman);

	aw::script::lexer lexer(srcman.get_buffer(id));

	aw::script::parser parser({
		.lexer = lexer,
		.diag = diag
	});

	{
		auto decl = parser.parse_top_level();
		TestAssert(decl.has_value());
		TestEqual(decl->comment, "a test function");
	}

	{
		auto decl = parser.parse_top_level();
		TestAssert(decl.has_value());
		TestEqual(decl->comment, "another function");
	}

	{
		auto decl = parser.parse_top_level();
		TestAssert(decl.has_value());
		TestEqual(decl->comment, "one more function:\ndoes nothing");
	}
}

Test(stray_comment_not_leaked) {
	aw::script::source_manager srcman;

	// The block comment inside the initializer isn't a leading comment for
	// anything -- it shouldn't leak into var y's comment slot.
	auto id = srcman.add_buffer(R"(
	var x: int = 1 /* stray */ + 2;

	// leading comment for y
	var y: int = 3;
)");

	aw::script::diagnostics_engine diag(srcman);

	aw::script::lexer lexer(srcman.get_buffer(id));

	aw::script::parser parser({
		.lexer = lexer,
		.diag = diag
	});

	{
		auto decl = parser.parse_top_level();
		TestAssert(decl.has_value());
		TestEqual(decl->comment, "");
	}

	{
		auto decl = parser.parse_top_level();
		TestAssert(decl.has_value());
		TestEqual(decl->comment, "leading comment for y");
	}
}
} // namespace aw::script
