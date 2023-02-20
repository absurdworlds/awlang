/*
 * Copyright (C) 2015   Hedede <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_AST_StringExpr_
#define aw_script_AST_StringExpr_
#include <aw/script/ast/expression.h>

namespace aw {
namespace script {
namespace ast {
/*! Expression for numeric literals */
class StringExpr : public Expression {
public:
	StringExpr(std::string_view value)
		: value(value)
	{
	}

	virtual ~StringExpr() = default;

	virtual void accept(ast::Visitor& visitor)
	{
		visitor.visit(*this);
	}

	std::string_view getValue() const
	{
		return value;
	}
private:
	std::string_view value;
};

} // namespace ast
} // namespace script
} // namespace aw
#endif//aw_script_AST_StringExpr_