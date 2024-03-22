#pragma once

#include <aw/script/ast/ast_printer.h>

#include <vector>

namespace aw::script {
class ast_printer_default : public ast_printer
{
public:
	ast_printer_default();
	~ast_printer_default();

	void print_declaration(const ast::declaration& decl) override;

private:
	void print_decl(const ast::function& decl);
	void print_var(const ast::variable& var);
	void print_decl(const ast::variable& var);
	void print_decl(const ast::struct_decl& var);
	void print_decl(const ast::import_decl& decl);
	void print_decl(const ast::module_header& var);
	void print_decl(const ast::foreign_block& var);

	void print_identifier(const ast::identifier& id);
	void print_type(const ast::type& type);

	void print(const ast::statement& stmt);
	void print_stmt(const ast::decl_statement& stmt);
	void print_stmt(const ast::return_statement& stmt);
	void print_stmt(const ast::statement_list& list);
	void print_stmt(const ast::if_else_statement& stmt);
	void print_stmt(const ast::while_statement& stmt);
	void print_stmt(const ast::empty_statement& stmt);
	void print_stmt(const ast::expression& expr);

	void print_expr(const std::unique_ptr<ast::expression>& expr);
	void print_expr(const ast::expression& expr);
	void print_expr(const ast::paren_expression& expr);
	void print_expr(const ast::cast_expression& expr);
	void print_expr(const ast::field_expression& expr);
	void print_expr(const ast::unary_expression& expr);
	void print_expr(const ast::if_expression& expr);
	void print_expr(const ast::binary_expression& expr);
	void print_expr(const ast::call_expression& expr);
	void print_expr(const ast::subscript_expression& expr);
	void print_expr(const ast::value_expression& expr);
	void print_expr(const ast::numeric_literal& expr);
	void print_expr(const ast::string_literal& expr);
	void print_expr(const ast::struct_literal& expr);

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
	void end_line();
	void start(string_view name, scope_type type = block_scope);
	void end();
	void print_indent();
	void print_inline(string_view text);

	std::vector<scope_type> scope_stack;
	state state = line_start;
};

} // namespace aw::script
