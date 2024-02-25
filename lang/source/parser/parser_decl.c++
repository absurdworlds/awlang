/*
 * Copyright (C) 2022-2023 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <aw/script/parser/parser.h>
#include <aw/script/utility/wrap.h>

#include "errors.h"

using namespace std::string_view_literals;

namespace aw::script {
/********************** Declarations **********************/
std::optional<ast::declaration> parser::parse_top_level()
{
	if (tok == token_kind::eof)
		return {};

	if (tok != token_kind::identifier)
		return error(diag, diagnostic_id::expected_declaration, tok);

	std::optional<ast::declaration> decl = parse_declaration();

	/* TODO: do not forget about global variables*/
	while (match(token_kind::semicolon));

	return decl;
}

std::optional<ast::declaration> parser::parse_declaration()
{
	// awscript employs context-sensitive keywords
	if (match_id("var"sv))
		return parse_variable_declaration(ast::access::variable);

	if (match_id("const"sv))
		return parse_variable_declaration(ast::access::constant);

	if (match_id("function"sv))
		return parse_function_declaration();

	if (match_id("struct"sv))
		return parse_struct_declaration();

	if (match_id("class"sv))
		return parse_class_declaration();

	if (match_id("foreign"sv))
		return parse_foreign_declaration();

	return error(diag, diagnostic_id::expected_declaration, tok);
}


auto parser::parse_variable_declaration(ast::access access) -> std::optional<ast::variable>
{
	ast::variable var{ .access = access };
	var.name = parse_identifier();
	if (var.name.empty())
		return {};

	if (!parse_type_specifier(var.type, ast::unknown_type{}))
		return {};

	if (match(token_kind::equal))
		var.value = parse_variable_initializer();

	return var;
}

auto parser::parse_initializer_field() -> std::optional<ast::struct_literal::field>
{
	using field = ast::struct_literal::field;

	field result;
	if (match(token_kind::dot)) {
		if (!in(tok, token_kind::identifier, token_kind::number))
			return error(diag, diagnostic_id::expected_identifier, tok);

		result.name = tok.data;
		result.kind = tok == token_kind::identifier ?
			ast::struct_literal::named :
			ast::struct_literal::numbered;

		expect(token_kind::equal);
	}

	result.value = wrap(parse_expression());
	if (!result.value)
		return {};
	return result;
}

auto parser::parse_struct_initializer() -> std::optional<ast::expression>
{
	ast::struct_literal init;
	while (!match(token_kind::r_brace)) {
		if (check_eof(token_kind::r_brace))
			break;

		auto field = parse_initializer_field();
		if (!field)
			continue;

		init.fields.push_back(std::move(*field));

		if (match(token_kind::r_brace))
			break;

		expect(token_kind::comma);
	}

	return init;
}

auto parser::parse_variable_initializer() -> std::optional<ast::expression>
{
	if (match(token_kind::l_brace))
		return parse_struct_initializer();

	// TODO: parse until ';' or a reasonable place to start parsing the next thing
	return parse_expression();
}

auto parser::parse_function_declaration() -> std::optional<ast::function>
{
	auto func = parse_function_prototype();
	if (func) {
		func->body = wrap(parse_function_body());
	}

	return func;
}

auto parser::parse_struct_declaration() -> std::optional<ast::declaration>
{
	ast::struct_decl st;
	st.name = parse_identifier();
	if (st.name.empty())
		return {};

	if (!match(token_kind::l_brace))
		return error_unexpected_token(diag, tok, token_kind::l_brace);

	while (tok != token_kind::r_brace) {
		if (check_eof(token_kind::r_brace))
			break;

		auto var_kind = parse_variable_start();
		if (!var_kind)
			return {};

		auto var = parse_variable_declaration(*var_kind);
		if (!var)
			return {};

		st.members.push_back(std::move(*var));

		if (tok == token_kind::r_brace)
			break;

		expect(token_kind::semicolon);
	}

	expect(token_kind::r_brace);

	return st;
}

auto parser::parse_class_declaration() -> std::optional<ast::declaration>
{
	return error_not_implemented_yet(diag, tok);
}

auto parser::parse_function_prototype() -> std::optional<ast::function>
{
	ast::function func;
	func.name = parse_identifier();

	expect(token_kind::l_paren);

	parse_function_arguments(func);

	expect(token_kind::r_paren);

	parse_function_return_type(func);

	return func;
}

bool parser::parse_variadic_parameter(ast::function& func)
{
	if (match(token_kind::ellipsis)) {
		ast::variadic_parameter vparam;
		if (tok == token_kind::identifier) {
			vparam.name = tok.data;
			advance();
		}
		func.params.variadic = vparam;
		return true;
	}

	return false;
}

auto parser::parse_variable_start() -> std::optional<ast::access>
{
	if (tok != token_kind::identifier)
		return {};
	if (match_id("var"sv))
		return ast::access::variable;
	match_id("const"sv);
	return ast::access::constant;
}

bool parser::parse_function_arguments(ast::function& func)
{
	while (tok != token_kind::r_paren) {
		if (check_eof(token_kind::r_paren))
			break;

		auto var_kind = parse_variable_start();
		if (!var_kind)
			return false;

		if (parse_variadic_parameter(func)) {
			if (tok != token_kind::r_paren)
				error(diag, diagnostic_id::variadic_parameter, tok.loc);
			// TODO: skip rest of parameters
			break;
		}

		auto arg = parse_variable_declaration(*var_kind);
		if (!arg)
			return false;

		func.params.push_back(std::move(*arg));

		if (tok == token_kind::r_paren)
			break;

		expect(token_kind::comma);
	}

	return true;
}

bool parser::parse_function_return_type(ast::function& func)
{
	return parse_type_specifier(func.return_type, ast::regular_type{ .name = "void" });
}

auto parser::parse_function_body() -> std::optional<ast::statement>
{
	if (match(token_kind::semicolon))
		return {};
	if (tok == token_kind::l_brace)
		return parse_statement_block();
	if (tok == "return")
		return parse_return_statement();

#if AW_SCRIPT_FEATURE_IMPLICIT_RETURN
	if (is_declaration_start(tok)) // TODO
		return nullptr;

	ast::return_statement stmt;
	stmt.value = parse_expression();
	if (stmt.value)
		return stmt;

#endif
	return {};
}



auto parser::parse_foreign_declaration() -> std::optional<ast::declaration>
{
	if (match_id("import"sv))
		return parse_foreign_block(ast::foreign_block::import);
	if (match_id("export"sv))
		return error_not_implemented_yet(diag, tok);
	if (match_id("module"sv))
		return error_not_implemented_yet(diag, tok);

	return error_unexpected_token(diag, tok, token_kind::identifier);
}

auto parser::parse_foreign_block(ast::foreign_block::type kind) -> std::optional<ast::declaration>
{
	ast::foreign_block block;
	block.kind = kind;

	expect(token_kind::colon);

	block.lang = parse_identifier();

	if (!expect(token_kind::l_brace)) {
		// Foreign blocks without a body are not allowed,
		// but we treat them as empty blocks when recovering
		if (match(token_kind::semicolon))
			return block;
	}

	while (true) {
		auto decl = parse_top_level();
		if (!decl)
			break;
		block.decls.push_back(std::move(*decl));
		if (tok == token_kind::r_brace)
			break;
	}

	expect(token_kind::r_brace);

	return block;
}

} // namespace aw::script
