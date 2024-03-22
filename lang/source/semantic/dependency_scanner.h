#ifndef aw_script_semantic_dependency_scanner_h
#define aw_script_semantic_dependency_scanner_h

#include <aw/script/semantic/dependency_resolver.h>

#include <aw/script/ast/statement.h>
#include <aw/script/ast/expression.h>

#include <map>

namespace aw::script {

struct dependency_scanner {
	diagnostics_engine& diag;

	std::vector<module_depenency> deps;
	std::map<ast::identifier, size_t> map;

	module_depenency& add_module_dependency(const ast::identifier& id)
	{
		auto it = map.lower_bound(id);
		if (it == map.end() || (id < it->first))
		{
			auto& dep = deps.emplace_back();
			dep.module_id = id;
			map.emplace_hint(it, id, deps.size() - 1);
			return dep;
		}

		return deps[it->second];
	}

	/*
	 * Identifier
	 */
	void visit_identifier(const ast::identifier& id)
	{
		if (!id.is_qualified())
			return;

		// TODO: implmenet optional check for redundant qualifiers
		ast::identifier mod_id {
			.path = id.path,
			.name = id.path.back()
		};

		mod_id.path.pop_back();

		auto& mod = add_module_dependency(mod_id);
		mod.imported_ids.push_back(id.name);
	}

	/*
	 * Types
	 */
	void visit_type(const ast::type& type)
	{
		auto type_visitor = [this] (auto& type) { return visit_type(type); };

		return std::visit(type_visitor, type);
	}

	void visit_type(const ast::unknown_type& /*type*/) {}
	void visit_type(const ast::regular_type& type)
	{
		visit_identifier(type.name);
	}
	void visit_type(const ast::reference_type& type)
	{
		visit_identifier(type.pointee);
	}
	void visit_type(const ast::pointer_type& type)
	{
		visit_identifier(type.pointee);
	}
	void visit_type(const ast::array_type& type)
	{
		visit_identifier(type.elem);
	}

	/*
	 * Declarations
	 */
	void visit_decl(const ast::declaration& decl)
	{
		auto ast_visitor = [this] (auto& decl) { return visit_decl(decl); };

		return std::visit(ast_visitor, decl);
	}

	void visit_decl(const ast::function& func)
	{
		for (auto& var : func.params)
			visit_decl(var);

		if (func.body)
			visit_stmt(*func.body);
	}

	void visit_decl(const ast::variable& var)
	{
		visit_type(var.type);
		if (var.value)
			visit_expr(*var.value);
	}

	void visit_decl(const ast::struct_decl& decl)
	{
		for (auto& field : decl.members)
			visit_decl(field);
	}

	void visit_decl(const ast::foreign_block& /*decl*/)
	{
	}

	void visit_decl(const ast::module_header& /*decl*/)
	{
	}

	void visit_decl(const ast::import_decl& decl)
	{
		// TODO: handle duplicates
		auto& mod = add_module_dependency(decl.mod_id);
		for (const auto& id : decl.decl_ids) {
			mod.imported_ids.push_back(id);
		}
	}

	/*
	 * Statements
	 */
	void visit_stmt(const ast::statement& stmt)
	{
		auto stmt_visitor = [this] (auto& stmt)
		{
			return visit_stmt(stmt);
		};
		std::visit(stmt_visitor, stmt);
	}

	void visit_stmt(const ast::decl_statement& stmt)
	{
		visit_decl(*stmt.decl);
	}

	void visit_stmt(const ast::statement_block& block)
	{
		for (auto& stmt : block)
			visit_stmt(stmt);
	}

	void visit_stmt(const ast::if_else_statement& stmt)
	{
		if (stmt.if_expr)
			visit_stmt(*stmt.if_expr);
		if (stmt.if_body)
			visit_stmt(*stmt.if_body);
		if (stmt.else_body)
			visit_stmt(*stmt.else_body);
	}

	void visit_stmt(const ast::while_statement& stmt)
	{
		if (stmt.cond_expr)
			visit_expr(*stmt.cond_expr);
		if (stmt.loop_body)
			visit_stmt(*stmt.loop_body);
	}

	void visit_stmt(const ast::return_statement& stmt)
	{
		if (stmt.value)
			visit_expr(*stmt.value);

	}

	void visit_stmt(const ast::empty_statement& /*stmt*/)
	{
	}

	void visit_stmt(const ast::expression& expr)
	{
		visit_expr(expr);
	}

	/*
	 * Expressions
	 */
	void visit_expr(const ast::expression& expr)
	{
		auto expr_visitor = [this] (auto& expr)
		{
			return visit_expr(expr);
		};
		std::visit(expr_visitor, expr);
	}

	void visit_expr(const ast::paren_expression& expr)
	{
		visit_expr(*expr.inner);
	}

	void visit_expr(const ast::cast_expression& expr)
	{
		visit_expr(*expr.lhs);
		visit_type(expr.to_type);
	}

	void visit_expr(const ast::unary_expression& expr)
	{
		visit_expr(*expr.lhs);
	}

	void visit_expr(const ast::binary_expression& expr)
	{
		visit_expr(*expr.lhs);
		visit_expr(*expr.rhs);
	}

	void visit_expr(const ast::call_expression& call)
	{
		visit_identifier(call.func);
		for (auto& arg : call.args)
			visit_expr(arg);
	}

	void visit_expr(const ast::subscript_expression& expr)
	{
		visit_expr(*expr.lhs);
		for (const auto& arg : expr.args)
			visit_expr(arg);

	}

	void visit_expr(const ast::field_expression& expr)
	{
		visit_expr(*expr.lhs);
	}

	void visit_expr(const ast::if_expression& expr)
	{
		if (expr.if_expr)
			visit_expr(*expr.if_expr);
		if (expr.if_body)
			visit_expr(*expr.if_body);
		if (expr.else_body)
			visit_expr(*expr.else_body);
	}

	void visit_expr(const ast::value_expression& expr)
	{
		visit_identifier(expr.name);
	}

	void visit_expr(const ast::struct_literal& expr)
	{
		for (auto& field : expr.fields)
			visit_expr(*field.value);
	}

	void visit_expr(const ast::numeric_literal& /*expr*/) {}
	void visit_expr(const ast::string_literal& /*expr*/) {}
};

} // namespace aw::script


#endif // aw_script_semantic_dependency_scanner_h
