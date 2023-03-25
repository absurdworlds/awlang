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

#include <vector>
#include <memory>
#include <variant>

#include <aw/script/ast/expression.h>

namespace aw::script::ast {

struct statement;

using statement_list = std::vector<statement>;

struct empty_statement {
};

struct return_statement {
	std::unique_ptr<expression> value;
};

struct if_else_statement {
	std::unique_ptr<expression> if_expr;
	std::unique_ptr<statement>  if_body;
	std::unique_ptr<statement>  else_body;
};

// struct is needed due to clang's shenanigans
struct statement_block : statement_list{
	statement_block() = default;

	statement_block(const statement_block&) = delete;
	statement_block(statement_block&&) = default;

	statement_block& operator=(const statement_block&) = default;
	statement_block& operator=(statement_block&&) = default;
};

using statement_variant = std::variant<
	statement_block,
	if_else_statement,
	return_statement,
	expression,
	empty_statement
>;

struct statement : statement_variant
{
	using statement_variant::statement_variant;
	using statement_variant::operator=;
};

} // namespace aw::script::ast

#endif//aw_script_ast_statement_h
