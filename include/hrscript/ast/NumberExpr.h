/*
 * Copyright (C) 2015   Hedede <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef _hrscript_AST_NumberExpr_
#define _hrscript_AST_NumberExpr_
#include <hrscript/ast/Expression.h>

namespace hrscript {
namespace ast {
/*! Expression for numeric literals */
class NumberExpr : public Expression {
public:
	NumberExpr(std::string value);

	virtual ~NumberExpr();

	virtual accept(ast::Visitor& visitor) = 0;
private:
	std::string value;
};

class FPNumberExpr : NumberExpr {
	virtual ~FPNumberExpr();

	virtual accept(ast::Visitor& visitor) = 0;
}

class IntNumberExpr : NumberExpr {
	virtual ~IntNumberExpr();

	virtual accept(ast::Visitor& visitor) = 0;
}

} // namespace ast
} // namespace hrscript
#endif//_hrscript_AST_NumberExpr_