/*
 * Copyright (C) 2015   Hedede <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_AST_CallExpr_
#define aw_script_AST_CallExpr_
#include <vector>
#include <string>
#include <aw/script/ast/expression.h>

namespace aw {
namespace script {
namespace ast {
/*! AST node for a function call */
class CallExpr : public Expression {
public:
	CallExpr(std::string_view callee,
	         std::vector<std::unique_ptr<Expression>> arguments)
		: callee(callee), arguments(std::move(arguments))
	{
	}

	virtual ~CallExpr() = default;

	virtual std::string_view getFunction() const
	{
		return callee;
	}

	virtual std::vector<uptr<Expression>>& getArguments()
	{
		return arguments;
	}

	virtual void accept(ast::Visitor& visitor)
	{
		visitor.visit(*this);
	}
private:
	//Function* callee;
	std::string_view callee;
	std::vector<std::unique_ptr<Expression>> arguments;
};

} // namespace ast
} // namespace script
} // namespace aw
#endif//aw_script_AST_CallExpr_
