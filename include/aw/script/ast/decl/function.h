/*
 * Copyright (C) 2015   Hedede <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_ast_function_h
#define aw_script_ast_function_h
#include <aw/script/ast/statement_block.h>
#include <aw/script/ast/declaration.h>
#include <aw/script/ast/decl/variable.h>
namespace aw {
namespace script {
namespace ast {
typedef std::vector<std::unique_ptr<Variable>> ArgList;

// TODO: merge with Function
class FunctionProto : public Declaration {
public:
	static uptr<FunctionProto>
	create(std::string_view id,
	       std::string_view returnType,
	       ArgList args)
	{
		uptr<FunctionProto> tmp(
		        new FunctionProto(id, returnType, std::move(args)));
		return std::move(tmp);
	}

	virtual ~FunctionProto() = default;

	virtual std::string_view getName() const
	{
		return name;
	}

	virtual std::string_view getReturnType() const
	{
		return returnType;
	}

	ArgList& getArguments()
	{
		return args;
	}

	virtual void accept(ast::Visitor& visitor)
	{
		visitor.visit(*this);
	}
private:
	FunctionProto(std::string_view id, std::string_view returnType, ArgList args)
		: Declaration(Declaration::FunctionProto),
		  name(id),
		  returnType(returnType)
	{
	}

	std::string_view name;
	std::string_view returnType;
	ArgList args;
};

class Function : public Declaration {
public:
	static uptr<Function>
	create(uptr<ast::FunctionProto> proto,
	       uptr<StatementBlock> body)
	{
		uptr<Function> tmp(new Function(
		         std::move(proto), std::move(body)));
		return std::move(tmp);
	}

	virtual ~Function() = default;

	ast::FunctionProto& getPrototype() const
	{
		return *proto;
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
	Function(uptr<ast::FunctionProto> proto,
		 uptr<StatementBlock> body)
		: Declaration(Declaration::Function),
		  proto(std::move(proto)),
		  body(std::move(body))
	{
	}

	uptr<ast::FunctionProto> proto;
	uptr<StatementBlock> body;
};
} // namespace ast
} // namespace script
} // namespace aw
#endif//aw_script_ast_function_h
