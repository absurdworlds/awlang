#pragma once

#include "ast_printer.h"

#include <aw/script/ast/decl/function.h>

#include <vector>

namespace aw::script {
class ast_printer_default : public ast_printer
{
public:
	ast_printer_default();

	void print_declaration(const ast::declaration& decl) override;

private:
	void print(const ast::function& decl);
	void print(const ast::variable& var);
	void print(const ast::statement& stmt);

	void print_type(const ast::type* type);

	void print_stmt(const ast::statement_list& list);
	void print_stmt(const ast::if_else_statement& stmt);
	void print_stmt(const ast::expression& expr);

	void print_expr(const std::unique_ptr<ast::expression>& expr);
	void print_expr(const ast::expression& expr);
	void print_expr(const ast::unary_expression& expr);
	void print_expr(const ast::binary_expression& expr);
	void print_expr(const ast::call_expression& expr);
	void print_expr(const ast::value_expression& expr);
	void print_expr(const ast::numeric_literal& expr);
	void print_expr(const ast::string_literal& expr);

	enum scope_type {
		// (scope
		//   content
		//   more_content
		// )
		block_scope,
		// (scope content more_content)
		inline_scope,
		// (scope content
		//   more_content
		// )
		mixed_scope,
	};

	enum state {
		line_start,
		node_start,
		line_middle,
		line_end,
	};

	void start_line();
	void start(string_view name, scope_type type = block_scope);
	void end();
	void print_indent();
	void print_inline(string_view text);

	std::vector<scope_type> scope_stack;
	state state;
};

} // namespace aw::script