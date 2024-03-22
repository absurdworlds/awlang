/*
 * Copyright (C) 2015-2022 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_ast_statement_h
#define aw_script_ast_statement_h

#include <memory>
#include <variant>
#include <vector>

#include <aw/script/ast/expression.h>
#include <aw/script/utility/hard_alias.h>

namespace aw::script::ast {

struct statement;

using statement_list = std::vector<statement>;

struct empty_statement {
};

struct return_statement {
	std::unique_ptr<expression> value;
};

struct declaration;
struct decl_statement {
	decl_statement(declaration&& decl);
	decl_statement(std::unique_ptr<declaration> decl);

	decl_statement(const decl_statement& other) = delete;
	decl_statement(decl_statement&& other) noexcept;

	~decl_statement();

	decl_statement& operator=(const decl_statement& other) = delete;
	decl_statement& operator=(decl_statement&& other) noexcept;

	std::unique_ptr<declaration> decl;
};

struct if_else_statement {
	std::unique_ptr<expression> if_expr;
	std::unique_ptr<statement>  if_body;
	std::unique_ptr<statement>  else_body;
};

struct while_statement {
	std::unique_ptr<expression> cond_expr;
	std::unique_ptr<statement>  loop_body;
};

using statement_block = noncopyable<statement_list>;

using statement_variant = std::variant<
	empty_statement,
	statement_block,
	if_else_statement,
	while_statement,
	return_statement,
	expression,
	decl_statement
>;

struct statement : statement_variant
{
	using statement_variant::statement_variant;
	using statement_variant::operator=;
};

} // namespace aw::script::ast

#endif//aw_script_ast_statement_h
