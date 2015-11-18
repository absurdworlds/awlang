/*
 * Copyright (C) 2015   Hedede <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef _hrscript_AST_FunctionDeclaration_
#define _hrscript_AST_FunctionDeclaration_
#include <hrscript/ast/StatementBlock.h>
#include <hrscript/ast/Declaration.h>
#include <hrscript/ast/decl/Variable.h>
namespace hrscript {
namespace ast {
class FuncDeclaration : public Declaration {
public:
	FuncDeclaration(std::string id, std::string returnType,
	                std::vector<std::unique_ptr<VariableDeclaration>> args)
		: name(id), returnType(returnType), args(std::move(args))
	{
	}

	virtual ~FuncDeclaration() = default;

	virtual std::string getName() const
	{
		return name;
	}

	virtual std::string getReturnType() const
	{
		return returnType;
	}

	std::vector<uptr<VariableDeclaration>>& getArguments()
	{
		return args;
	}

	virtual void accept(ast::Visitor& visitor)
	{
		visitor.visit(*this);
	}
private:
	std::string name;
	std::string returnType;
	std::vector<std::unique_ptr<VariableDeclaration>> args;
};

class FuncDefinition : public Declaration {
public:
	FuncDefinition(uptr<FuncDeclaration> proto, uptr<StatementBlock> body)
		: prototype(std::move(proto)), body(std::move(body))
	{
	}

	virtual ~FuncDefinition() = default;

	FuncDeclaration& getPrototype()
	{
		return *prototype;
	}

	StatementBlock& getBody()
	{
		return *body;
	}

	virtual void accept(ast::Visitor& visitor)
	{
		visitor.visit(*this);
	}
private:
	uptr<FuncDeclaration> prototype;
	uptr<StatementBlock> body;
};
} // namespace ast
} // namespace hrscript
#endif//_hrscript_AST_FuncDeclaration_