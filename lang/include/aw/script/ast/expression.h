/*
 * Copyright (C) 2015-2023 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_ast_expression_h
#define aw_script_ast_expression_h

#include <aw/script/ast/number_base.h>
#include <aw/script/ast/operators.h>
#include <aw/script/ast/type.h>
#include <aw/script/utility/hard_alias.h>

#include <aw/types/string_view.h>

#include <memory>
#include <variant>
#include <vector>

namespace aw::script::ast {

class expression;

struct paren_expression {
	std::unique_ptr<expression> inner;
};

struct cast_expression {
	ast::type to_type;
	std::unique_ptr<expression> lhs;
};

struct unary_expression {
	unary_operator op;
	std::unique_ptr<expression> lhs;
};

struct binary_expression {
	binary_operator op;
	std::unique_ptr<expression> lhs;
	std::unique_ptr<expression> rhs;
};

struct field_expression {
	std::unique_ptr<expression> lhs;
	std::string name;
};

// TODO: tell apart lvalue and rvalue
struct value_expression {
	ast::identifier name;
};

using argument_list = noncopyable<std::vector<expression>>;

struct call_expression {
	ast::identifier func;
	argument_list args;
};

struct subscript_expression {
	std::unique_ptr<expression> lhs;
	argument_list args;
};

struct numeric_literal {
	std::string_view value;
	number_base base = number_base::decimal;
};

struct string_literal {
	std::string value;
};

struct struct_literal {
	enum kind {
		positional,
		named,
		numbered,
	};

	struct field {
		struct_literal::kind kind = positional;
		std::string_view name;
		std::unique_ptr<expression> value;
	};
	using field_vec = noncopyable<std::vector<field>>;
	field_vec fields;
};


struct if_expression {
	std::unique_ptr<expression> if_expr;
	std::unique_ptr<expression> if_body;
	std::unique_ptr<expression> else_body;
};

using expression_variant = std::variant<
	cast_expression,
	paren_expression,
	unary_expression,
	binary_expression,
	call_expression,
	subscript_expression,
	if_expression,
	field_expression,
	value_expression,
	numeric_literal,
	string_literal,
	struct_literal
>;

struct expression : expression_variant
{
	using expression_variant::expression_variant;
	using expression_variant::operator=;
};

} // namespace aw::script::ast

#endif//aw_script_ast_expression_h
