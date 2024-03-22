#include "convert_to_middle.h"

#include <aw/script/utility/wrap.h>
#include <aw/script/utility/location.h>
#include <aw/script/diag/error_t.h>

#include <aw/utility/ranges/paired.h>

#include <charconv>

namespace aw::script {

namespace {
auto convert_operator(ast::unary_operator op) -> ir::unary_operator
{
	using unop = ir::unary_operator;
	switch(op) {
		using enum ast::unary_operator;
	case minus:
		return unop::minus;
	case plus:
		return unop::plus;
	case logical_negation:
	case binary_negation:
		return unop::negation;
	case reference:
		return unop::reference;
	}
	assert(!"Corrupt value!");
};

auto convert_operator(ast::binary_operator op) -> ir::binary_operator
{
	using binop = ir::binary_operator;
	switch(op) {
		using enum ast::binary_operator;
	case minus:
		return binop::minus;
	case plus:
		return binop::plus;
	case multiply:
		return binop::multiply;
	case divide:
		return binop::divide;
	case modulo:
		return binop::modulo;

	case bitwise_and:
		return binop::bitwise_and;
	case bitwise_xor:
		return binop::bitwise_xor;
	case bitwise_or:
		return binop::bitwise_or;
	case shift_left:
		return binop::shift_left;
	case shift_right:
		return binop::shift_right;

	case less:
		return binop::less;
	case less_equal:
		return binop::less_equal;
	case greater:
		return binop::greater;
	case greater_equal:
		return binop::greater_equal;
	case equal:
		return binop::equal;
	case not_equal:
		return binop::not_equal;
	case logical_and:
		return binop::logical_and;
	case logical_or:
		return binop::logical_or;

	case assignment:
	case assign_minus:
	case assign_plus:
	case assign_multiply:
	case assign_divide:
	case assign_modulo:
	case assign_or:
	case assign_xor:
	case assign_and:
	case assign_shift_left:
	case assign_shift_right:
		return binop::assignment;

	case compare:
	case range:
	case access:
		assert(!"Unhandled special-case operator!");
		break;
	}

	assert(!"Corrupt value!");
};

auto break_composite_op(ast::binary_operator op) -> std::optional<ir::binary_operator>
{
	using binop = ir::binary_operator;
	switch(op) {
		using enum ast::binary_operator;
	case assign_minus:
		return binop::minus;
	case assign_plus:
		return binop::plus;
	case assign_multiply:
		return binop::multiply;
	case assign_divide:
		return binop::divide;
	case assign_modulo:
		return binop::modulo;
	case assign_or:
		return binop::bitwise_or;
	case assign_xor:
		return binop::bitwise_xor;
	case assign_and:
		return binop::bitwise_and;
	case assign_shift_left:
		return binop::shift_left;
	case assign_shift_right:
		return binop::shift_right;
	default:
		return std::nullopt;
	}
}

struct convert_to_middle_visitor {
	context& ctx;
	diagnostics_engine& diag;

	void convert_decl(const ast::declaration& in_decl, middle::module& mod)
	{
		if (auto foreign = get_if<ast::foreign_block>(&in_decl)) {
			for (const auto& decl : foreign->decls) {
				convert_decl(decl, mod);
			}
		}
		else if (auto mod_hdr = get_if<ast::module_header>(&in_decl)) {
			if (mod.name.empty()) {
				mod.name = mod_hdr->name;
			} else {
				error(diag, diagnostic_id::module_redeclaration, location());
			}

			if (!mod.decls.empty()) {
				error(diag, diagnostic_id::module_bad_loc, location());
			}
		} else {
			mod.decls.push_back(wrap(convert_decl(in_decl)));
		}
	}

	auto convert_decl(const ast::declaration& in_decl) -> middle::declaration
	{
		auto wrapper = [this] (const auto& decl) -> middle::declaration
		{
			return convert_decl(decl);
		};

		return std::visit(wrapper, in_decl);
	}

	auto convert_decl(const ast::struct_decl& in_struct) -> middle::struct_decl
	{
		middle::struct_decl st;
		st.name = std::string(in_struct.name);
		for (const auto& var : in_struct.members)
			st.members.push_back(wrap(convert_decl(var)));
		return st;
	}

	auto convert_decl(const ast::foreign_block& /*unused*/) -> middle::declaration
	{
		assert(!"Unreachable");
		return {};
	}

	auto convert_decl(const ast::module_header& /*unused*/) -> middle::declaration
	{
		assert(!"Unreachable");
		return {};
	}

	/*
	 * Variable
	 */
	auto convert_decl(const ast::variable& in_var) -> middle::variable
	{
		middle::variable var;
		var.name = in_var.name;
		var.access = in_var.access;
		if (!get_if<ast::unknown_type>(&in_var.type))
			var.type = ctx.create_type(in_var.type);
		if (in_var.value)
			var.value = wrap(convert_expr(*in_var.value));

		return var;
	}

	/*
	 * Function
	 */
	auto convert_decl(const ast::function& in_func) -> middle::function
	{
		middle::function func;
		func.name = in_func.name;
		func.return_type = ctx.create_type(in_func.return_type);
		if (in_func.body)
			func.body = wrap(convert_stmt(*in_func.body));

		func.args.resize(in_func.params.size());
		for (auto&& [arg, in_arg] : paired(func.args, in_func.params))
		{
			arg = wrap(convert_decl(in_arg));
		}
		func.is_variadic = in_func.is_variadic();
		return func;
	}

	/*
	 * Statements
	 */
	auto convert_stmt(const ast::statement& in_stmt) -> middle::statement
	{
		auto stmt_visitor = [this] (const auto& stmt) -> middle::statement
		{
			return convert_stmt(stmt);
		};
		return std::visit(stmt_visitor, in_stmt);
	}

	auto convert_stmt(const ast::decl_statement& in_stmt) -> middle::decl_statement
	{
		return { convert_decl(*in_stmt.decl) };
	}

	auto convert_stmt(const ast::statement_block& in_block) -> middle::statement_block
	{
		middle::statement_block block;
		for (const auto& stmt : in_block)
		{
			if (std::holds_alternative<ast::empty_statement>(stmt))
				continue;
			block.push_back(convert_stmt(stmt));
		}
		return block;

	}

	auto convert_stmt(const ast::if_else_statement& in_stmt) -> middle::if_else_statement
	{
		middle::if_else_statement stmt;
		if (in_stmt.if_expr)
			stmt.if_expr = wrap(convert_expr(*in_stmt.if_expr));
		if (in_stmt.if_body)
			stmt.if_body = wrap(convert_stmt(*in_stmt.if_body));
		if (in_stmt.else_body)
			stmt.else_body = wrap(convert_stmt(*in_stmt.else_body));
		return stmt;
	}

	auto convert_stmt(const ast::while_statement& in_stmt) -> middle::while_statement
	{
		middle::while_statement stmt;
		if (in_stmt.cond_expr)
			stmt.cond_expr = wrap(convert_expr(*in_stmt.cond_expr));
		if (in_stmt.loop_body)
			stmt.loop_body = wrap(convert_stmt(*in_stmt.loop_body));
		return stmt;
	}

	auto convert_stmt(const ast::return_statement& in_stmt) -> middle::return_statement
	{
		middle::return_statement stmt;
		if (in_stmt.value)
			stmt.value = wrap(convert_expr(*in_stmt.value));
		return stmt;
	}

	auto convert_stmt(const ast::empty_statement& /*in_stmt*/) -> middle::empty_statement
	{
		return {};
	}

	auto convert_stmt(const ast::expression& in_expr) -> middle::expression
	{
		return convert_expr(in_expr);
	}

	/*
	 * Expressions
	 */
	auto convert_expr(const ast::expression& in_expr) -> middle::expression
	{
		auto expr_visitor = [this] (const auto& expr) -> middle::expression
		{
			return convert_expr(expr);
		};
		return std::visit(expr_visitor, in_expr);
	}

	auto convert_expr(const ast::cast_expression& in_expr) -> middle::cast_expression
	{
		return middle::cast_expression{
			.to_type = ctx.create_type(in_expr.to_type),
			.lhs = wrap(convert_expr(*in_expr.lhs)),
		};
	}

	auto convert_expr(const ast::paren_expression& in_expr) -> middle::expression
	{
		return convert_expr(*in_expr.inner);
	}

	auto convert_expr(const ast::unary_expression& in_expr) -> middle::expression
	{
		auto expr = middle::expression(middle::unary_expression {
			.op = convert_operator(in_expr.op),
			.lhs = wrap(convert_expr(*in_expr.lhs)),
		});

		return expr;
	}

	static bool can_chain(const ast::binary_operator& in_op)
	{
		using enum ast::binary_operator;
		return in(in_op,
			less,
			less_equal,
			greater,
			greater_equal
		);
	}

	static bool can_chain(const ast::binary_expression& in_expr)
	{
		if (can_chain(in_expr.op)) {
			// For left-associative expresions, the parser always produces
			// a tree of form (+ (+ a b) c), so we can skip checking the rhs
			return get_if<ast::binary_expression>(in_expr.lhs.get()) != nullptr;
		}

		return false;
	}

	auto convert_expr(const ast::binary_expression& in_expr) -> middle::expression
	{
		if (can_chain(in_expr))
			return convert_chain_expr(in_expr);

		return convert_binary_expr(in_expr);
	}

	auto make_stack(const ast::binary_expression& expr)
	{
		std::vector<const ast::binary_expression*> stack{ &expr };

		auto* lhs = stack.back();
		while (true) {
			lhs = get_if<ast::binary_expression>(lhs->lhs.get());
			if (!lhs || !can_chain(lhs->op))
				break;
			stack.push_back(lhs);
		}

		return stack;
	}


	auto convert_chain_expr(const ast::binary_expression& in_expr) -> middle::binary_expression
	{
		auto stack = make_stack(in_expr);

		middle::binary_expression expr;

		value_ptr<middle::expression> lhs;

		while (!stack.empty()) {
			auto* top = stack.back();
			stack.pop_back();

			middle::binary_expression current_expr{
				.op = convert_operator(top->op),
				.lhs = lhs ? std::move(lhs) : wrap(convert_expr(*top->lhs)),
				.rhs = wrap(convert_expr(*top->rhs)),
			};

			lhs = clone(current_expr.rhs);

			if (!expr.lhs) {
				expr = std::move(current_expr);
			} else {
				expr = middle::binary_expression{
					.op = ir::binary_operator::logical_and,
					.lhs = std::move(expr),
					.rhs = std::move(current_expr),
				};
			}

		}

		return expr;
	}

	auto convert_binary_expr(const ast::binary_expression& in_expr) -> middle::binary_expression
	{
		auto op = convert_operator(in_expr.op);
		auto lhs = convert_expr(*in_expr.lhs);
		auto rhs = convert_expr(*in_expr.rhs);
		if (auto second_op = break_composite_op(in_expr.op)) {
			rhs = middle::binary_expression{
				.op = *second_op,
				.lhs = wrap(lhs),
				.rhs = wrap(std::move(rhs)),
			};
		}

		return middle::binary_expression{
			.op = op,
			.lhs = wrap(std::move(lhs)),
			.rhs = wrap(std::move(rhs)),
		};
	}

	auto convert_expr(const ast::field_expression& in_expr) -> middle::field_expression
	{
		return middle::field_expression{
			.lhs = wrap(convert_expr(*in_expr.lhs)),
			.name = std::string(in_expr.name),
		};
	}

	auto convert_expr(const ast::call_expression& in_expr) -> middle::call_expression
	{
		middle::call_expression call;
		call.func_name = in_expr.func;
		for (const auto& arg : in_expr.args)
		{
			call.args.push_back(convert_expr(arg));
		}
		return call;
	}

	auto convert_expr(const ast::subscript_expression& in_expr) -> middle::subscript_expression
	{
		middle::subscript_expression expr;
		expr.lhs = wrap(convert_expr(*in_expr.lhs));
		for (const auto& arg : in_expr.args)
		{
			expr.args.push_back(convert_expr(arg));
		}
		return expr;
	}

	auto convert_expr(const ast::if_expression& in_expr) -> middle::if_expression
	{
		middle::if_expression expr;
		if (in_expr.if_expr)
			expr.if_expr = wrap(convert_expr(*in_expr.if_expr));
		if (in_expr.if_body)
			expr.if_body = wrap(convert_expr(*in_expr.if_body));
		if (in_expr.else_body)
			expr.else_body = wrap(convert_expr(*in_expr.else_body));
		return expr;
	}

	auto convert_expr(const ast::value_expression& in_expr) -> middle::expression
	{
		using namespace std::string_view_literals;
		// TODO: transform at a later stage to allow (true) and (false) refer
		//       to a variable?
		if (in_expr.name == "true"sv)
			return middle::bool_literal{ .value = true };

		if (in_expr.name == "false"sv)
			return middle::bool_literal{ .value = false };

		return middle::value_expression{
			.name = in_expr.name
		};
	}

	auto convert_expr(const ast::numeric_literal& in_expr) -> middle::expression
	{
		if (in_expr.value.find('.') != in_expr.value.npos)
			return middle::float_literal{ .value = in_expr.value };
		return middle::numeric_literal{
			.value = in_expr.value,
			.base = in_expr.base,
		};
	}

	static auto convert_expr(const ast::string_literal& in_expr) -> middle::string_literal
	{
		return { .value = in_expr.value };
	}

	size_t convert_to_index(std::string_view str)
	{
		size_t number = 0;
		auto result = std::from_chars(str.begin(), str.end(), number, 10);
		assert(result.ec == std::errc{});
		return number;
	}

	auto convert_indexed_literal(const ast::struct_literal& in_init)
	{
		middle::numbered_list_literal result;
		bool is_mixed = false;
		for (const auto& field : in_init.fields) {
			if (field.kind != ast::struct_literal::numbered) {
				is_mixed = true;
				continue;
			}
			result.fields.push_back({
				.index = convert_to_index(field.name),
				.value = wrap(convert_expr(*field.value)),
			});
		}
		if (is_mixed)
			error(diag, diagnostic_id::mixed_initializer, location());
		return result;
	}

	auto convert_named_literal(const ast::struct_literal& in_init)
	{
		middle::struct_literal result;
		bool is_mixed = false;
		for (const auto& field : in_init.fields) {
			if (field.kind != ast::struct_literal::named) {
				is_mixed = true;
				continue;
			}
			result.fields.push_back({
				.name = std::string(field.name),
				.value = wrap(convert_expr(*field.value)),
			});
		}
		if (is_mixed)
			error(diag, diagnostic_id::mixed_initializer, location());
		return result;
	}

	auto convert_array_literal(const ast::struct_literal& in_init)
	{
		middle::list_literal result;
		bool is_mixed = false;
		for (const auto& field : in_init.fields) {
			if (field.kind != ast::struct_literal::positional)
				is_mixed = true;
			result.fields.push_back(convert_expr(*field.value));
		}
		if (is_mixed)
			error(diag, diagnostic_id::mixed_initializer, location());
		return result;
	}

	auto convert_expr(const ast::struct_literal& in_init) -> middle::expression
	{
		if (!in_init.fields.empty()) {
			auto kind = in_init.fields.front().kind;
			if (kind == ast::struct_literal::numbered)
				return convert_indexed_literal(in_init);
			else if (kind == ast::struct_literal::named)
				return convert_named_literal(in_init);
		}

		return convert_array_literal(in_init);
	}

};

} // namespace

auto convert_to_middle(
	context& ctx,
	diagnostics_engine& diag,
	const ast::module& in_mod) -> middle::module
{
	middle::module mod;

	convert_to_middle_visitor visitor{ .ctx = ctx, .diag = diag };
	for (const auto& in_decl : in_mod.decls)
	{
		visitor.convert_decl(in_decl, mod);
	}

	return mod;
}

} // namespace aw::script
