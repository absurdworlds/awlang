/*
 * Copyright (C) 2015   Hedede <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_ast_decl_variable_h
#define aw_script_ast_decl_variable_h
#include <memory>
#include <string>
#include <aw/script/ast/declaration.h>
#include <aw/script/ast/expression.h>
namespace aw {
namespace script {
namespace ast {
class Variable : public Declaration {
public:
	static uptr<Variable>
	create(std::string_view id)
	{
		uptr<Variable> tmp(new Variable(id));
		return std::move(tmp);
	}

	virtual ~Variable() = default;

	virtual void accept(ast::Visitor& visitor)
	{
		visitor.visit(*this);
	}

	virtual Expression* getInitializer()
	{
		return val.get();
	}

	virtual bool isWriteable(bool writeable)
	{
		return writeAccessible;
	}

	virtual void set_const(bool is_const)
	{
		writeAccessible = !is_const;
	}

	virtual void set_initialier(std::unique_ptr<Expression> newVal)
	{
		val = std::move(newVal);
	}

	std::string_view getName() const
	{
		return name;
	}
private:
	Variable(std::string_view id)
		: Declaration(Declaration::Variable),
		  name(id)
	{
	}

	std::string_view name;
	std::unique_ptr<Expression> val;
	bool writeAccessible = true;
};
} // namespace ast
} // namespace script
} // namespace aw
#endif//aw_script_ast_decl_variable_h
