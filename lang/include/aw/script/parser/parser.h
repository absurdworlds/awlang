/*
 * Copyright (C) 2022 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_parser_h
#define aw_script_parser_h

#include <memory>
#include <optional>

#include <aw/script/lexer/lexer.h>

#include <aw/script/parser/operators.h>

#include <aw/script/diag/diagnostics_engine.h>

#include <aw/script/ast/declaration.h>
#include <aw/script/ast/expression.h>
#include <aw/script/ast/statement.h>

namespace aw::script {

// TODO: split it into declaration_parser, statement_parser and expression_parser

class parser {
public:
	struct dependencies {
		script::lexer& lexer;
		diagnostics_engine& diag;
	};

	parser(dependencies deps);

	~parser() = default;

	auto parse_top_level() -> std::optional<ast::declaration>;

private:
	bool advance();
	bool advance(token_kind expected);
	bool advance(string_view identifier);

	struct save_point;

	auto save_state() -> save_point;

	void restore_state(save_point sp);

	void skip_comments();
	token skip_illegal_tokens();
	bool match(token_kind expected);
	bool match(string_view identifier);
	bool expect(token_kind expected);
	bool expect(string_view identifier);
	bool unmatch(string_view identifier, diagnostic_id msg);

	bool check_eof(token_kind expect);

	/// Matches an identifier. Expects that the token kind was already checked.
	bool match_id(string_view identifier);

	auto parse_identifier() -> std::optional<std::string_view>;
	auto parse_qualified_identifier() -> std::optional<ast::identifier>;
	std::optional<size_t> parse_usize();

	bool parse_type_specifier(ast::type& type, ast::type default_type);
	auto parse_array_size() -> std::optional<size_t>;
	auto parse_type() -> std::optional<ast::type>;

	// Type of a decl_context
	enum class decl_type {
		top_level,
		foreign,
		nested_module,
	};

	// Declaration parse context,
	// Holds information necessary for giving correct diagnostics
	struct decl_context {
		decl_type type;
		token start_token;
	};

	auto parse_declaration(decl_type type) -> std::optional<ast::declaration>;
	auto parse_declaration(decl_context context) -> std::optional<ast::declaration>;
	auto parse_declaration_list(decl_type type) -> ast::decl_list;

	auto parse_variable_declaration(ast::access access) -> std::optional<ast::variable>;
	auto parse_initializer_field() -> std::optional<ast::struct_literal::field>;
	auto parse_struct_initializer() -> std::optional<ast::expression>;
	auto parse_variable_initializer() -> std::optional<ast::expression>;

	auto parse_struct_declaration() -> std::optional<ast::declaration>;
	auto parse_class_declaration() -> std::optional<ast::declaration>;

	auto parse_module_declaration(decl_context context) -> std::optional<ast::declaration>;
	auto parse_inline_module_declaration(decl_context context, std::optional<string_view> name)
		-> std::optional<ast::declaration>;
	auto parse_submodule_declaration() -> std::optional<ast::declaration>;

	auto parse_import_declaration() -> std::optional<ast::declaration>;

	auto parse_foreign_declaration() -> std::optional<ast::declaration>;
	auto parse_foreign_block(ast::foreign_block::type kind) -> std::optional<ast::declaration>;

	auto parse_function_declaration() -> std::optional<ast::function>;
	auto parse_function_prototype() -> std::optional<ast::function>;
	bool parse_variadic_parameter(ast::function& func);
	auto parse_variable_start() -> std::optional<ast::access>;
	bool parse_function_arguments(ast::function& func);
	bool parse_function_return_type(ast::function& func);
	auto parse_function_body() -> std::optional<ast::statement>;

	auto parse_statement() -> std::optional<ast::statement>;
	auto parse_statement_inner() -> std::optional<ast::statement>;
	auto parse_statement_block() -> std::optional<ast::statement>;
	auto parse_if_statement() -> std::optional<ast::statement>;
	auto parse_for_statement() -> std::optional<ast::statement>;
	auto parse_while_statement() -> std::optional<ast::statement>;
	auto parse_return_statement() -> std::optional<ast::statement>;
	auto parse_nested_function() -> std::optional<ast::decl_statement>;
	auto parse_local_variable(ast::access access) -> std::optional<ast::decl_statement>;

	auto parse_expression() -> std::optional<ast::expression>;
	auto parse_if_expression() -> std::optional<ast::expression>;
	auto parse_postfix_expression() -> std::optional<ast::expression>;
	auto parse_postfix_expression(ast::expression lhs) -> ast::expression;
	auto parse_unary_expression() -> std::optional<ast::expression>;
	auto parse_field_expression(ast::expression lhs) -> ast::expression;
	auto parse_cast_expression(ast::expression lhs) -> ast::expression;
	auto parse_array_subscript(ast::expression lhs) -> ast::expression;
	auto parse_call_expression(ast::identifier name) -> std::optional<ast::expression>;
	auto parse_binary_expression(ast::expression lhs, precedence min_prec) -> ast::expression;

	auto parse_primary_expression() -> std::optional<ast::expression>;

	auto parse_paren_expression() -> std::optional<ast::expression>;
	auto parse_identifier_expression() -> std::optional<ast::expression>;
	auto parse_string_literal_expression() -> std::optional<ast::expression>;
	auto parse_numeric_literal_expression() -> std::optional<ast::expression>;

	auto parse_unary_operator(token tok) -> std::optional<ast::unary_operator>;
	auto parse_binary_operator(token tok) -> std::optional<ast::binary_operator>;

private:
	/*! Current lookahead (peek) token. */
	token tok;

	/*! Lexer which provides the stream of tokens */
	lexer& lex;

	/*! Diagnostics engine for error reporting */
	diagnostics_engine& diag;
};

} // namespace aw::script

#endif // aw_script_parser_h
