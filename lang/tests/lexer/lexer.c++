#include <aw/script/lexer/lexer.h>

#include <aw/test/test.h>

#include <aw/algorithm/join.h>

TestFile("lexer-test")

using namespace std::string_view_literals;

namespace aw::script {

auto lex_single_token(string_view string)
{
	source_buffer buf(string);
	return lexer(&buf).next();
}

Test(string_token_must_include_quotes) {
	const auto string = R"("this-is-a-string")"sv;
	const auto tok = lex_single_token(string);
	TestAssert(!tok.incomplete);
	TestAssert(tok.kind == token_kind::string_literal);
	TestEqual(tok.data, string);
}

Test(adjacent_string_tokens_must_be_separate) {
	Checks {
		source_buffer buf(R"("s1""s2")");
		lexer lex(&buf);

		std::vector<string_view> tokens;
		for (auto tok = lex.next(); tok != token_kind::eof; tok = lex.next())
		{
			TestAssert(!tok.incomplete);
			TestAssert(tok.kind == token_kind::string_literal);
			tokens.push_back(tok.data);
		}

		TestEqual(tokens, decltype(tokens){ R"("s1")", R"("s2")" });
	}

	Checks {
		source_buffer buf(R"("s1" "s2")");
		lexer lex(&buf);

		std::vector<string_view> tokens;
		for (auto tok = lex.next(); tok != token_kind::eof; tok = lex.next())
		{
			TestAssert(!tok.incomplete);
			TestAssert(tok.kind == token_kind::string_literal);
			tokens.push_back(tok.data);
		}

		TestEqual(tokens, decltype(tokens){ R"("s1")", R"("s2")" });
	}
}

Test(incomplete_string_should_be_marked_as_such) {
	const auto string = R"("this string is ...)"sv;

	source_buffer buf(string);
	lexer lex(&buf);

	const auto tok = lex.next();
	TestAssert(tok.incomplete);
	TestAssert(tok.kind == token_kind::string_literal);
	TestEqual(tok.data, string);
}

Test(escaped_quote_does_not_end_a_string) {
	const auto string = R"("a\"b")"sv;
	const auto tok = lex_single_token(string);
	TestAssert(!tok.incomplete);
	TestAssert(tok.kind == token_kind::string_literal);
	TestEqual(tok.data, string);
}

Test(escaped_backslash_does_not_escape_the_quote) {
	Checks {
		// the string ends with a backslash, escaped by another one
		const auto string = R"("a\\")"sv;
		const auto tok = lex_single_token(string);
		TestAssert(!tok.incomplete);
		TestEqual(tok.data, string);
	}

	Checks {
		// three backslashes: the last one escapes the quote, so the
		// string carries on to the next one
		const auto string = R"("a\\\"b")"sv;
		const auto tok = lex_single_token(string);
		TestAssert(!tok.incomplete);
		TestEqual(tok.data, string);
	}
}

Test(string_ending_in_a_lone_escape_is_incomplete) {
	const auto string = R"("abc\)"sv;
	const auto tok = lex_single_token(string);
	TestAssert(tok.incomplete);
	TestAssert(tok.kind == token_kind::string_literal);
	TestEqual(tok.data, string);
}

Test(comments_should_lex_as_regular_tokens) {
	Checks {
		const auto s = "//comment"sv;
		const auto tok = lex_single_token(s);
		TestAssert(tok.kind == token_kind::line_comment);
		TestEqual(tok.data, s);
	}

	Checks {
		const auto s = "/*block comment*/"sv;
		const auto tok = lex_single_token(s);
		TestAssert(tok.kind == token_kind::block_comment);
		TestEqual(tok.data, s);
	}
}

Test(each_comment_is_one_token) {
	source_buffer buf(R"(
	// a comment...
	/// another comment
	/*
	 * a block comment
	 */
)");
	lexer lex(&buf);

	{
		const auto tok = lex.next();
		TestAssert(!tok.incomplete);
		TestAssert(tok.kind == token_kind::line_comment);
		TestEqual(tok.data, "// a comment..."sv);
	}
	{
		const auto tok = lex.next();
		TestAssert(!tok.incomplete);
		TestAssert(tok.kind == token_kind::line_comment);
		TestEqual(tok.data, "/// another comment"sv);
	}
	{
		const auto tok = lex.next();
		TestAssert(!tok.incomplete);
		TestAssert(tok.kind == token_kind::block_comment);
		TestEqual(tok.data, R"(/*
	 * a block comment
	 */)"sv);
	}
	{
		const auto tok = lex.next();
		TestAssert(tok.kind == token_kind::eof);
	}
}

// this isn't some kind of a grammar rule - just an observation which can be proven mathematically
Test(block_comments_are_at_least_4_chars_long) {
	Checks {
		const auto s = "/*/"sv;
		const auto tok = lex_single_token(s);
		TestAssert(tok.incomplete);
		TestAssert(tok.kind == token_kind::block_comment);
		TestEqual(tok.data, s);
	}
	Checks {
		const auto s = "/**/ "sv;
		const auto tok = lex_single_token(s);
		TestAssert(!tok.incomplete);
		TestAssert(tok.kind == token_kind::block_comment);
		TestEqual(tok.data, "/**/"sv);
	}
}

Test(all_defined_operators_should_be_lexed) {
	using namespace std::string_literals;

	const std::vector punct_tokens = {
#define TOKEN(...)
#define PUNCT(x, y) token_kind::x,
#define KEYWORD(...)
#include <aw/script/lexer/tokens.h>
	};

	const std::vector punct_spellings = {
#define TOKEN(...)
#define PUNCT(x, y) y##sv,
#define KEYWORD(...)
#include <aw/script/lexer/tokens.h>
	};

	auto string = aw::join(begin(punct_spellings), end(punct_spellings), " "s);

	int index = 0;
	source_buffer buf(string);
	lexer lex(&buf);
	std::vector<string_view> tokens;
	for (auto tok = lex.next(); tok != token_kind::eof; tok = lex.next())
	{
		TestAssert(!tok.incomplete);
		TestEqual(tok.data, punct_spellings[index]);
		TestAssert(tok.kind == punct_tokens[index++]);
	}

}
} // namespace aw::script
