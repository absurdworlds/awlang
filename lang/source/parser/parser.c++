/*
 * Copyright (C) 2022 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <aw/script/parser/parser.h>
#include <aw/script/utility/wrap.h>

#include <aw/algorithm/in.h>
#include <aw/types/support/enum.h>

#include <charconv>

#include "errors.h"

using namespace std::string_view_literals;

namespace aw::script {

// TODO: evaluate idea
// sub-parsers, for each language construct
// with parse and bail-out methods
// bail-out: discard invalid source until you can parse again
// for example, func my_func( garbage_hsfjkflnkjashnlfkjl ) { }
// would discard `garbage_hsfjkflnkjashnlfkjl` and try to parse everything else

parser::parser(dependencies deps)
	: lex(deps.lexer), diag(deps.diag)
{
	advance();
}

bool parser::advance()
{
	tok = lex.next();
	skip_comments();
	return true; // TODO:
}

void parser::skip_comments()
{
	while (in(tok, token_kind::line_comment, token_kind::block_comment))
		tok = lex.next();
}

token parser::skip_illegal_tokens()
{
	auto first_illegal_token = tok;
	while (tok == token_kind::illegal)
		tok = lex.next();
	skip_comments();
	return first_illegal_token;
}

bool parser::advance(token_kind expected)
{
	assert(tok == expected);
	return advance();
}

bool parser::advance(string_view identifier)
{
	assert(tok == token_kind::identifier &&
	       tok == identifier);
	return advance();
}

struct parser::save_point {
	lexer::save_point lex_state;
	token tok;
};

auto parser::save_state() -> save_point
{
	return { lex.save_state(), tok };
}

void parser::restore_state(save_point sp)
{
	lex.restore_state(sp.lex_state);
	tok = sp.tok;
}

bool parser::match(token_kind expected)
{
	if (tok.kind != expected)
		return false;

	// consume token
	advance();
	return true;
}

bool parser::match(string_view identifier)
{
	if (tok.kind != token_kind::identifier)
		return false;

	if (tok.data != identifier)
		return false;

	// consume token
	advance();
	return true;
}

//! Expects the next token is of a specified kind
bool parser::expect(token_kind expected)
{
	if (!match(expected))
		return error_unexpected_token(diag, tok, expected);
	return true;
}

bool parser::expect(string_view identifier)
{
	if (!match(identifier))
		return error_unexpected_token(diag, tok, identifier);
	return true;
}

//! Opposite of 'expect': expects that should NOT be a specific token
bool parser::unmatch(string_view identifier, diagnostic_id msg)
{
	if (tok == identifier) {
		error(diag, msg, tok);
		advance();
		return false;
	}
	return true;
}

bool parser::check_eof(token_kind expect)
{
	if (tok == token_kind::eof) {
		error(diag, diagnostic_id::unexpected_eof, tok.loc, expect);
		return true;
	}
	return false;
}

bool parser::match_id(string_view identifier)
{
	assert(tok.kind == token_kind::identifier);

	if (tok.data != identifier)
		return false;

	// consume token
	advance();
	return true;
}

auto parser::parse_identifier() -> std::optional<std::string_view>
{
	if (tok != token_kind::identifier)
		return error(diag, diagnostic_id::expected_identifier, tok);

	auto name = tok.data;
	advance();

	assert(!name.empty());
	return name;
}

auto parser::parse_qualified_identifier() -> std::optional<ast::identifier>
{
	auto name = parse_identifier();
	if (!name)
		return {};

	ast::identifier id {
		.name = std::string(*name)
	};

	while (match(token_kind::double_colon)) {
		id.path.push_back(std::move(id.name));
		auto name = parse_identifier();
		if (!name) {
			id.name.clear();
			break; // TODO: better recovery
		}
		id.name = *name;
	}
	return id;
}

std::optional<size_t> parser::parse_usize()
{
	if (tok != token_kind::numeric_constant)
		return error(diag, diagnostic_id::expected_a_number, tok);

	if (tok.data.find('.') != tok.data.npos)
		return error(diag, diagnostic_id::non_integer_array_size, tok);

	size_t number = 0;

	// TODO: bases
	auto result = std::from_chars(tok.data.begin(), tok.data.end(), number, 10);
	advance();

	return number;
}

bool parser::parse_type_specifier(ast::type& type, ast::type default_type)
{
	if (!match(token_kind::colon)) {
		type = std::move(default_type);
		return true;
	}

	auto opt_type = parse_type();
	if (!opt_type)
		return false;
	type = std::move(*opt_type);
	return true;
}

auto parser::parse_array_size() -> std::optional<size_t>
{
	if (match(token_kind::r_square))
		return {};
	auto result = parse_usize();
	expect(token_kind::r_square);
	return result;
}

auto parser::parse_type() -> std::optional<ast::type>
{
	auto name = parse_qualified_identifier();
	if (!name)
		return {};

	if (match(token_kind::ast)) {
		return ast::pointer_type{
			.pointee = *name
		};
	}

	if (match(token_kind::l_square)) {
		return ast::array_type{
			.elem = *name,
			.size = parse_array_size(),
		};
	}

	return ast::regular_type{
		.name = *name
	};
}

/********************** Statements **********************/
auto parser::parse_statement_block() -> std::optional<ast::statement>
{
	if (!match(token_kind::l_brace))
		return error_unexpected_token(diag, tok, token_kind::l_brace);

	ast::statement_block statements;
	while (!match(token_kind::r_brace)) {
		if (check_eof(token_kind::r_brace))
			break;
		auto statement = parse_statement();
		if (!statement)
			continue;

		statements.push_back(std::move(*statement));
	}

	return std::move(statements);
}

auto parser::parse_statement() -> std::optional<ast::statement>
{
	const auto start_tok = tok;

	auto stmt = parse_statement_inner();

	// skip a token to avoid looping forever
	if (!stmt && start_tok == tok)
		advance();

	while (match(token_kind::semicolon));

	return stmt;
}

auto parser::parse_statement_inner() -> std::optional<ast::statement>
{
	switch (tok.kind) {
	case token_kind::semicolon:
		return ast::empty_statement();

	case token_kind::l_brace:
		return parse_statement_block();

	case token_kind::identifier:
		if (tok == "function"sv)
			return parse_nested_function();
		if (tok == "const"sv)
			return parse_local_variable(ast::access::constant);
		if (tok == "var"sv)
			return parse_local_variable(ast::access::variable);
		if (tok == "if"sv)
			return parse_if_statement();
		if (tok == "match"sv)
			return {};
		if (tok == "loop"sv)
			return {};
		if (tok == "while"sv)
			return parse_while_statement();
		if (tok == "for"sv)
			return parse_for_statement();
		if (tok == "continue"sv)
			return {};
		if (tok == "break"sv)
			return {};
		if (tok == "return"sv)
			return parse_return_statement();

		[[fallthrough]];
	default:
		return parse_expression();
	}
}

auto parser::parse_if_statement() -> std::optional<ast::statement>
{
	advance("if");

	ast::if_else_statement stmt;

	if (unmatch("then", diagnostic_id::expected_expression)) {
		stmt.if_expr = wrap(parse_expression());
		if (!stmt.if_expr)
			error_expected_expression(diag, tok);
	}

	match("then");

	stmt.if_body = wrap(parse_statement());

	if (match("else"))
		stmt.else_body = wrap(parse_statement());

	return stmt;
}

auto parser::parse_for_statement() -> std::optional<ast::statement>
{
	return {};
}

auto parser::parse_while_statement() -> std::optional<ast::statement>
{
	advance("while");

	auto cond = parse_expression();
	if (!cond)
		error_expected_expression(diag, tok);

	auto body = parse_statement();
	if (!body)
		error(diag, diagnostic_id::expected_a_statement, tok);

	return ast::while_statement {
		.cond_expr = wrap(std::move(cond)),
		.loop_body = wrap(std::move(body)),
	};
}

auto parser::parse_return_statement() -> std::optional<ast::statement>
{
	advance("return");

	ast::return_statement stmt;
	if (!match(token_kind::semicolon))
	{
		stmt.value = wrap(parse_expression());
		if (!stmt.value)
			return {};
	}

	return stmt;
}

auto parser::parse_nested_function() -> std::optional<ast::decl_statement>
{
	advance("function");

	auto func = parse_function_declaration();
	if (func)
		return ast::decl_statement(std::move(*func));
	return {};
}

auto parser::parse_local_variable(ast::access access) -> std::optional<ast::decl_statement>
{
	advance(access == ast::access::variable ? "var" : "const");

	auto var = parse_variable_declaration(access);
	if (var)
		return ast::decl_statement(std::move(*var));
	return {};
}

/********************** Expressions **********************/
auto parser::parse_expression() -> std::optional<ast::expression>
{
	auto lhs = parse_postfix_expression();
	if (!lhs)
		return {};

	return parse_binary_expression(std::move(*lhs), precedence::unknown);
}

auto parser::parse_postfix_expression() -> std::optional<ast::expression>
{
	auto expr = parse_unary_expression();
	if (!expr)
		return expr;

	return parse_postfix_expression(std::move(*expr));
}

auto parser::parse_cast_expression(ast::expression lhs) -> ast::expression
{
	advance("as");

	auto type = parse_type();
	if (!type) {
		error(diag, diagnostic_id::expected_a_type, tok);
		return lhs;
	}

	return ast::cast_expression{
		.to_type = std::move(*type),
		.lhs = wrap(std::move(lhs)),
	};
}

auto parser::parse_array_subscript(ast::expression lhs) -> ast::expression
{
	advance(token_kind::l_square);

	ast::subscript_expression expr;

	expr.lhs = wrap(std::move(lhs));

	if (!match(token_kind::r_square)) {
		while (true) {
			auto arg = parse_expression();
			if (!arg) {
				error_expected_expression(diag, tok);
				break;
			}

			expr.args.push_back(std::move(*arg));

			if (match(token_kind::r_square))
				break;

			expect(token_kind::comma);
		}
	}

	return expr;
}

auto parser::parse_postfix_expression(ast::expression lhs) -> ast::expression
{
	while (true) {
		if (tok == token_kind::l_square)
			lhs = parse_array_subscript(std::move(lhs));
		else
			break;
	}

	if (tok == "as"sv)
		lhs = parse_cast_expression(std::move(lhs));

	return lhs;
}

auto parser::parse_unary_expression() -> std::optional<ast::expression>
{
	auto op = parse_unary_operator(tok);
	if (!op)
		return parse_primary_expression();

	ast::unary_expression expr {
		.op = *op,
		.lhs = wrap(parse_unary_expression())
	};

	if (!expr.lhs)
		return {};

	return expr;
}

auto parser::parse_field_expression(ast::expression lhs) -> ast::expression
{
	auto name = parse_identifier();
	if (!name)
		return {};

#if 0
	if (match(token_kind::l_paren))
		return parse_call_expression(name);
#endif

	return parse_postfix_expression(ast::field_expression{
		.lhs = wrap(std::move(lhs)),
		.name = std::string(*name),
	});
}

static precedence increment_prec(precedence prec, int count)
{
	auto result = precedence( to_underlying(prec) + count );
	assert(result < precedence::max);
	return result;
}

auto parser::parse_binary_expression(ast::expression lhs, precedence min_prec) -> ast::expression
{
	while (true) {
		precedence cur_prec = token_precedence(tok);
		if (cur_prec < min_prec)
			break;

		auto op = parse_binary_operator(tok);
		assert(op && "token_precedence implemented incorrectly!");

		if (op == ast::binary_operator::access)
			return parse_field_expression(std::move(lhs));

		auto rhs = parse_postfix_expression();
		if (!rhs) {
			error_expected_expression(diag, tok);
			break;
		}

		const precedence next_prec = token_precedence(tok);
		const bool is_right_assoc = is_right_associative(tok);

		if (cur_prec < next_prec ||
		   (cur_prec == next_prec && is_right_assoc)) {
			rhs = parse_binary_expression(
				std::move(*rhs),
				increment_prec(cur_prec, !is_right_assoc)
			);
		}

		lhs = ast::binary_expression {
			.op = *op,
			.lhs = wrap(std::move(lhs)),
			.rhs = wrap(std::move(*rhs)),
		};
	}

	return lhs;
}

auto parser::parse_primary_expression() -> std::optional<ast::expression>
{
	switch(tok.kind) {
	case token_kind::l_paren:
		return parse_paren_expression();
	case token_kind::identifier:
		return parse_identifier_expression();
	case token_kind::numeric_constant:
		return parse_numeric_literal_expression();
	case token_kind::string_literal:
		return parse_string_literal_expression();
	case token_kind::illegal:
		return error(diag, diagnostic_id::illegal_token, skip_illegal_tokens());
	default:
		return {}; // expected expression
	}
}

auto parser::parse_paren_expression() -> std::optional<ast::expression>
{
	advance(token_kind::l_paren);

	auto expr = parse_expression();

	expect(token_kind::r_paren);

	return ast::paren_expression{
		.inner = wrap(std::move(expr))
	};
}

auto parser::parse_if_expression() -> std::optional<ast::expression>
{
	advance("if");

	ast::if_expression expr;

	if (unmatch("then", diagnostic_id::expected_expression)) {
		expr.if_expr = wrap(parse_expression());
		if (!expr.if_expr)
			error_expected_expression(diag, tok);
	}

	match("then");

	expr.if_body = wrap(parse_expression());

	expect("else"sv);

	expr.else_body = wrap(parse_expression());

	return expr;
}

auto parser::parse_identifier_expression() -> std::optional<ast::expression>
{
	const auto sp = save_state();

	const auto id = parse_qualified_identifier();
	if (!id)
		return {};

	if (*id == "if"sv) {
		if (tok != token_kind::r_paren)
		{
			// Restoring state isn't really necessary
			// (and even slightly detrimental for performance)
			// but I think it's more elegant
			restore_state(sp);
			return parse_if_expression();
		}
	}

	if (match(token_kind::l_paren))
		return parse_call_expression(*id);

	ast::value_expression expr{ .name = std::move(*id) };

	return expr;
}

char escape_char(char c)
{
	switch(c) {
	case 'n':
		return '\n';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	case 'r':
		return '\r';
	case '\\':
	case '"':
	default:
		return c;
	}
}

static std::string parse_string(string_view s)
{
	std::string result;
	result.reserve(s.size());

	const auto str = s.substr(1, s.size() - 2);

	bool slash = false;
	for (char c : str)
	{
		if (c == '\\') {
			slash = true;
			continue;
		}

		if (slash) {
			slash = false;
			if (c == '\n')
				continue;
			c = escape_char(c);
		}

		result += c;
	}

	return result;
}

auto parser::parse_string_literal_expression() -> std::optional<ast::expression>
{
	assert(tok == token_kind::string_literal &&
	       "parse_string_literal_expression called when there's no string literal!");

	ast::string_literal str{ .value = parse_string(tok.data) };

	// consume string
	advance();

	return str;
}

auto decode_base(std::string_view num) -> ast::number_base
{
	using enum ast::number_base;
	if (num.starts_with('0')) {
		if (num.starts_with("0x"))
			return hexadecimal;
		if (num.starts_with("0b"))
			return binary;
		if (num.starts_with("0o"))
			return octal;
	}
	return decimal;
}

auto parser::parse_numeric_literal_expression() -> std::optional<ast::expression>
{
	assert(tok == token_kind::numeric_constant &&
	       "parse_numeric_literal_expression called when there's no number!");

	ast::numeric_literal num {
		.value = tok.data,
		.base = decode_base(tok.data)
	};

	if (num.base != ast::number_base::decimal)
		num.value.remove_prefix(2);

	// consume number
	advance();

	return num;
}

auto parser::parse_call_expression(ast::identifier name) -> std::optional<ast::expression>
{
	ast::call_expression expr;

	expr.func = std::move(name);

	if (!match(token_kind::r_paren)) {
		while (true) {
			auto arg = parse_expression();
			if (!arg) {
				error_expected_expression(diag, tok);
				break;
			}

			expr.args.push_back(std::move(*arg));

			if (match(token_kind::r_paren))
				break;

			expect(token_kind::comma);
		}
	}

	return expr;
}
} // namespace aw::script
