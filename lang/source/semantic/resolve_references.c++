#include "resolve_references.h"

#include "aw/script/ast/middle/module.h"
#include "aw/script/diag/error_t.h"

namespace aw::script {
using namespace middle;

struct resolver {
	context& ctx;
	diagnostics_engine& diag;

	/*
	 * Declarations
	 */
	void visit_decl(declaration& decl)
	{
		auto ast_visitor = [this] (auto& decl) { return visit_decl(decl); };

		return std::visit(ast_visitor, decl);
	}

	void visit_decl(function& func)
	{
		ctx.current_scope()->add_symbol(func.name, &func);

		// TODO: local ctx instead of push/pop scope
		ctx.push_scope();
		ctx.current_scope()->add_symbol("$func", &func); // hack!

		for (auto& var : func.args)
			visit_decl(*var);

		if (func.body)
			visit_stmt(*func.body);

		ctx.pop_scope();
	}

	void visit_decl(variable& var)
	{
		ctx.current_scope()->add_symbol(var.name, &var);
		if (var.value)
			visit_expr(*var.value);
	}

	void visit_decl(struct_decl& decl)
	{
		ctx.push_scope();
		for (auto& field : decl.members)
			visit_decl(*field);
		ctx.pop_scope();
	}

	/*
	 * Statements
	 */
	void visit_stmt(statement& stmt)
	{
		auto stmt_visitor = [this] (auto& stmt)
		{
			return visit_stmt(stmt);
		};
		std::visit(stmt_visitor, stmt);
	}

	void visit_stmt(decl_statement& stmt)
	{
		visit_decl(*stmt.decl);
	}

	void visit_stmt(statement_block& block)
	{
		for (auto& stmt : block)
			visit_stmt(stmt);
	}

	void visit_stmt(if_else_statement& stmt)
	{
		if (stmt.if_expr)
			visit_stmt(*stmt.if_expr);
		if (stmt.if_body)
			visit_stmt(*stmt.if_body);
		if (stmt.else_body)
			visit_stmt(*stmt.else_body);
	}

	void visit_stmt(while_statement& stmt)
	{
		if (stmt.cond_expr)
			visit_expr(*stmt.cond_expr);
		if (stmt.loop_body)
			visit_stmt(*stmt.loop_body);
	}

	void visit_stmt(return_statement& stmt)
	{
		if (stmt.value)
			visit_expr(*stmt.value);

	}

	void visit_stmt(empty_statement& /*stmt*/)
	{
	}

	void visit_stmt(expression& expr)
	{
		visit_expr(expr);
	}

	/*
	 * Expressions
	 */
	void visit_expr(expression& expr)
	{
		auto expr_visitor = [this] (auto& expr)
		{
			return visit_expr(expr);
		};
		std::visit(expr_visitor, expr);
	}

	void visit_expr(cast_expression& expr)
	{
		visit_expr(*expr.lhs);
	}

	void visit_expr(unary_expression& expr)
	{
		visit_expr(*expr.lhs);
	}

	void visit_expr(binary_expression& expr)
	{
		visit_expr(*expr.lhs);
		visit_expr(*expr.rhs);
	}

	void visit_expr(call_expression& call)
	{
		for (auto& arg : call.args)
			visit_expr(arg);

		assert(call.func_name.path.empty()); // TODO
		call.func = ctx.current_scope()->find_func(call.func_name.name);
		if (!call.func)
			error(diag, diagnostic_id::undefined_function, location(), call.func_name);
	}

	void visit_expr(subscript_expression& expr)
	{
		visit_expr(*expr.lhs);
		for (auto& arg : expr.args)
			visit_expr(arg);

	}

	void visit_expr(field_expression& expr)
	{
		visit_expr(*expr.lhs);
	}

	void visit_expr(if_expression& expr)
	{
		if (expr.if_expr)
			visit_expr(*expr.if_expr);
		if (expr.if_body)
			visit_expr(*expr.if_body);
		if (expr.else_body)
			visit_expr(*expr.else_body);
	}

	void visit_expr(value_expression& expr)
	{
		assert(expr.name.path.empty());
		expr.ref = ctx.current_scope()->find_var(expr.name.name);
		if (!expr.ref)
			error(diag, diagnostic_id::undefined_variable, location(), expr.name);
	}

	void visit_expr(list_literal& expr)
	{
		for (auto& field : expr.fields)
			visit_expr(field);
	}

	void visit_expr(numbered_list_literal& expr)
	{
		for (auto& field : expr.fields)
			visit_expr(*field.value);
	}

	void visit_expr(struct_literal& expr)
	{
		for (auto& field : expr.fields)
			visit_expr(*field.value);
	}

	void visit_expr(bool_literal& /*expr*/) {}
	void visit_expr(numeric_literal& /*expr*/) {}
	void visit_expr(float_literal& /*expr*/) {}
	void visit_expr(string_literal& /*expr*/) {}
};

void resolve_references(context& ctx, diagnostics_engine& diag, module& mod)
{
	resolver visitor{ .ctx = ctx, .diag = diag };
	for (auto& decl : mod.decls)
		visitor.visit_decl(*decl);
}

} // namespace aw::script
