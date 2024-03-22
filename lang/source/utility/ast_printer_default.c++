#include "aw/script/utility/ast_printer_default.h"
#include "aw/script/utility/type_name.h"

// TODO: for now it prints into iostream
// abstract it out later
#include <iostream>

namespace aw::script {
ast_printer_default::ast_printer_default()
{

}

ast_printer_default::~ast_printer_default()
{
	std::cout.flush();
}

void ast_printer_default::print_declaration(const ast::declaration& decl)
{
	std::visit([this] (auto&& node) { print_decl(node); }, decl);
	end_line();
}

void ast_printer_default::start(string_view name, scope_type type)
{
	print_inline("(");
	if (name.empty())
		state = node_start;
	else
		std::cout << name;
	if (type == block_scope)
		start_line();
	scope_stack.push_back(type);
}

void ast_printer_default::end()
{
	const auto type = scope_stack.back();
	scope_stack.pop_back();
	print_indent();
	std::cout << ')';
	if (type != inline_scope)
		start_line();
}

void ast_printer_default::start_line()
{
	state = line_start;
	std::cout << '\n';
}

void ast_printer_default::end_line()
{
	if (state == line_middle) {
		state = line_start;
		std::cout << '\n';
	}
}

void ast_printer_default::print_inline(string_view text)
{
	if (state == line_start)
		print_indent();
	if (state == line_middle)
		std::cout << ' ';
	state = line_middle;
	std::cout << text;
}

void ast_printer_default::print_indent()
{
	if (state != line_start)
		return;
	const auto depth = scope_stack.size();
	if (depth > 0)
		std::cout << std::string(depth*2, ' ');
}

void ast_printer_default::print_decl(const ast::function& decl)
{
	start("func", mixed_scope);

	print_inline(decl.name);
	{
		start("fn-return", inline_scope);
		print_type(decl.return_type);
		end();
	}
	start_line();
	{
		start("fn-params");
		for (const auto& param : decl.params)
		{
			print_decl(param);
			start_line();
		}
		end();
	}
	if (decl.body) {
		start("fn-body");
		print(*decl.body);
		end();
	}
	end();
}

void ast_printer_default::print_decl(const ast::variable& var)
{
	auto kind = var.access == ast::access::variable ? "var" : "const";
	start(kind, inline_scope);
	print_type(var.type);
	print_inline(var.name);
	if (var.value)
		print_expr(*var.value);
	end();
}

void ast_printer_default::print_decl(const ast::struct_decl& st)
{
	start("struct", mixed_scope);
	print_inline(st.name);
	{
		start_line();
		start("members");
		for (const auto& var : st.members) {
			print_decl(var);
			start_line();
		}
		end();
	}

	end();
}

void ast_printer_default::print_decl(const ast::import_decl& decl)
{
	start("import", inline_scope);
	print_inline(decl.mod_id.name); // TODO
	end();
}

void ast_printer_default::print_decl(const ast::module_header& mod)
{
	start("module", inline_scope);
	print_inline(mod.name);
	end();
}

void ast_printer_default::print_decl(const ast::foreign_block& block)
{
	start("foreign", mixed_scope);
	print_inline(block.kind == block.import ? "import" : "export");
	print_inline(block.lang);
	{
		start_line();
		start("decls");
		for (const auto& decl : block.decls) {
			print_declaration(decl);
		}
		end();
	}

	end();
}

void ast_printer_default::print_identifier(const ast::identifier& id)
{
	print_inline(to_string(id));
}

void ast_printer_default::print_type(const ast::type& type)
{
	print_inline(type_name(type));
}

void ast_printer_default::print(const ast::statement& stmt)
{
	std::visit([this] (auto&& node) { print_stmt(node); }, stmt);
}

void ast_printer_default::print_stmt(const ast::decl_statement& stmt)
{
	if (stmt.decl)
		print_declaration(*stmt.decl);
}

void ast_printer_default::print_stmt(const ast::return_statement& stmt)
{
	start("return", inline_scope);
	print_expr(stmt.value);
	end();
}

void ast_printer_default::print_stmt(const ast::statement_list& list)
{
	start("stmt-list");
	for (const auto& stmt : list)
	{
		print(stmt);
	}
	end();
}

void ast_printer_default::print_stmt(const ast::if_else_statement& stmt)
{
	start("if", mixed_scope);

	print_expr(stmt.if_expr);
	start_line();

	{
		start("then", block_scope);
		if (stmt.if_body)
			print(*stmt.if_body);
		end();
	}

	if (stmt.else_body)
	{
		start("else", block_scope);
		print(*stmt.else_body);
		end();
	}

	end();
}

void ast_printer_default::print_stmt(const ast::while_statement& stmt)
{
	start("while", mixed_scope);

	print_expr(stmt.cond_expr);
	start_line();
	{
		start("do", block_scope);
		if (stmt.loop_body)
			print(*stmt.loop_body);
		end();
	}
	end();
}

void ast_printer_default::print_stmt(const ast::empty_statement& stmt)
{
	start("", inline_scope);
	end();
}

void ast_printer_default::print_stmt(const ast::expression& expr)
{
	print_expr(expr);
	start_line();
}

void ast_printer_default::print_expr(const std::unique_ptr<ast::expression>& expr)
{
	if (expr)
		print_expr(*expr);
	else
		print_inline("<invalid-expr>");
}

void ast_printer_default::print_expr(const ast::expression& expr)
{
	std::visit([this] (auto&& node) { print_expr(node); }, expr);
}

void ast_printer_default::print_expr(const ast::paren_expression& expr)
{
	start("paren", inline_scope);

	print_expr(expr.inner);

	end();
}

void ast_printer_default::print_expr(const ast::cast_expression& expr)
{
	start("as", inline_scope);

	print_type(expr.to_type);
	print_expr(expr.lhs);

	end();
}

void ast_printer_default::print_expr(const ast::field_expression& expr)
{
	start(".", inline_scope);
	print_expr(expr.lhs);
	print_inline(expr.name);
	end();
}

void ast_printer_default::print_expr(const ast::if_expression& expr)
{
	start("if", inline_scope);

	print_expr(expr.if_expr);
	start_line();

	{
		start("then", inline_scope);
		if (expr.if_body)
			print_expr(*expr.if_body);
		end();
	}

	{
		start("else", inline_scope);
		if (expr.else_body)
			print_expr(*expr.else_body);
		end();
	}

	end();
}

std::string_view spell_operator(ast::unary_operator op)
{
	switch(op) {
	case ast::unary_operator::minus:
		return "-";
	case ast::unary_operator::plus:
		return "+";
	case ast::unary_operator::logical_negation:
		return "!";
	case ast::unary_operator::binary_negation:
		return "~";
	case ast::unary_operator::reference:
		return "&";
	}
	return "<invalid-unary-op>";
}

void ast_printer_default::print_expr(const ast::unary_expression& expr)
{
	start(spell_operator(expr.op), inline_scope);
	print_expr(expr.lhs);
	end();
}

std::string_view spell_operator(ast::binary_operator op)
{
	switch(op) {
		using enum ast::binary_operator;
	case minus:
		return "-";
	case plus:
		return "+";
	case multiply:
		return "*";
	case divide:
		return "/";
	case modulo:
		return "%";
	case bitwise_and:
		return "&";
	case bitwise_xor:
		return "^";
	case bitwise_or:
		return "|";
	case shift_left:
		return "<<";
	case shift_right:
		return ">>";
	case equal:
		return "==";
	case not_equal:
		return "!=";
	case less:
		return "<";
	case less_equal:
		return "<=";
	case greater:
		return ">";
	case greater_equal:
		return ">=";
	case compare:
		break; // NIY
	case assignment:
		return "=";
	case logical_or:
		return "||";
	case logical_and:
		return "&&";
	case assign_minus:
		return "-=";
	case assign_plus:
		return "+=";
	case assign_multiply:
		return "*=";
	case assign_divide:
		return "/=";
	case assign_modulo:
		return "%=";
	case assign_or:
		return "|=";
	case assign_xor:
		return "^=";
	case assign_and:
		return "&=";
	case assign_shift_left:
		return "<<=";
	case assign_shift_right:
		return ">>=";
	case access:
		return ".";
	case range:
		return "..";
	}
	return "<invalid-binary-op>";
}

void ast_printer_default::print_expr(const ast::binary_expression& expr)
{
	start(spell_operator(expr.op), inline_scope);
	print_expr(expr.lhs);
	print_expr(expr.rhs);
	end();
}

void ast_printer_default::print_expr(const ast::call_expression& expr)
{
	start("call", inline_scope);
	print_identifier(expr.func);
	if (!expr.args.empty())
	{
		start("args", inline_scope);
		for (const auto& arg : expr.args)
			print_expr(arg);
		end();
	}
	end();
}

void ast_printer_default::print_expr(const ast::subscript_expression& expr)
{
	start("subscript", inline_scope);
	print_expr(expr.lhs);
	{
		start({}, inline_scope);
		for (const auto& arg : expr.args)
			print_expr(arg);
		end();
	}
	end();
}

void ast_printer_default::print_expr(const ast::value_expression& expr)
{
	print_identifier(expr.name);
}

void ast_printer_default::print_expr(const ast::numeric_literal& expr)
{
	print_inline(expr.value);
}

void ast_printer_default::print_expr(const ast::string_literal& expr)
{
	// TODO: escape special chars
	std::string tmp;
	tmp += '"';
	for (char c : expr.value)
	{
		if (c == '"')
			tmp += "\\\"";
		if (c == '\n')
			tmp += "\\n";
		else
			tmp += c;
	}
	tmp += '"';
	print_inline(tmp);
}

void ast_printer_default::print_expr(const ast::struct_literal& init)
{
	start("init", inline_scope);
	for (const auto& field : init.fields) {
		start(field.name, inline_scope);
		print_expr(field.value);
		end();
	}
	end();
}

} // namespace aw::script
