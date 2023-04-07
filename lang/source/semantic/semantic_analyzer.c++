#include "aw/script/semantic/semantic_analyzer.h"

#include "context.h"
#include "convert_to_middle.h"

#include <aw/script/semantic/scope.h>
#include <aw/script/utility/wrap.h>

#include <aw/script/diag/error_t.h>

#include <aw/utility/ranges/paired.h>

namespace aw::script {
auto create_type(std::string_view name) -> std::unique_ptr<ir::type>
{
	ir::type type;
	type.name = std::string(name);
	return wrap(std::move(type));
}

static auto create_builtin_types()
{
	std::vector<std::unique_ptr<ir::type>> types;

	types.push_back(create_type("void"));
	types.push_back(create_type("bool"));

	auto create_int = [&types] (char kind, int size, std::string_view name = {}) {
		std::string type_name;
		if (name.empty()) {
			type_name += kind;
			type_name += std::to_string(size);
		} else {
			type_name = name;
		}

		types.push_back(wrap(ir::type{
			type_name,
			ir::integer_type{
				.size = size,
				.is_signed = (kind != 'u')
			}
		}));

		return types.back().get();
	};

	auto u8 = create_int('u', 8);
	create_int('u', 16);
	create_int('u', 32);
	create_int('u', 64);
	create_int('u', 64, "usize");
	create_int('u', 128);

	create_int('i', 8);
	create_int('i', 16);
	create_int('i', 32);
	create_int('i', 32, "int"); // TODO: alias for 32
	create_int('i', 64);
	create_int('i', 64, "isize");
	create_int('i', 128);

	types.push_back(wrap(ir::type{"float",  ir::fp_type{ 32 }})); // TODO: aliases
	types.push_back(wrap(ir::type{"double", ir::fp_type{ 64 }})); // TODO: aliases
	types.push_back(wrap(ir::type{"f32",    ir::fp_type{ 32 }}));
	types.push_back(wrap(ir::type{"f64",    ir::fp_type{ 64 }}));

	types.push_back(wrap(ir::type{"u8*",  ir::pointer_type{
		.base_type = u8,
		.is_mutable = false,
	}}));

	types.push_back(create_type("string_literal"));


	return types;
}

semantic_analyzer::semantic_analyzer()
	: builtin_types(create_builtin_types())
{
}


auto semantic_analyzer::lower(const ast::module& in_mod) -> middle::module
{
	context ctx;

	for (const auto& type : builtin_types)
	{
		ctx.default_scope.add_type(type->name, type.get());
	}


	middle::module mod = convert_to_middle(ctx, in_mod);

	ctx.push_scope();
	for (const auto& decl_ptr : mod.decls)
	{
		if (auto decl = std::get_if<middle::variable>(decl_ptr.get()))
			ctx.current_scope()->add_symbol(decl->name, decl);
		if (auto decl = std::get_if<middle::function>(decl_ptr.get()))
			ctx.current_scope()->add_symbol(decl->name, decl);
	}

	for (const auto& decl_ptr : mod.decls)
	{
		visit(ctx, *decl_ptr);
	}

	// TODO: store them directly in the module instead of moving after the fact
	for (auto& ty : ctx.types)
		mod.types.push_back(std::move(ty));

	return mod;
}

/*
 * Declarations
 */
void semantic_analyzer::visit(context& ctx, middle::declaration& decl)
{
	auto ast_visitor = [this, &ctx] (auto& decl)
	{
		return visit(ctx, decl);
	};

	return std::visit(ast_visitor, decl);
}

void semantic_analyzer::visit(context& ctx, middle::function& func)
{
	for (auto& var : func.args)
	{
		visit(ctx, *var);
	}

	if (func.body) {
		// TODO: local ctx instead of push/pop scope
		ctx.push_scope();
		ctx.current_scope()->add_symbol("$func", &func); // hack!
		visit_stmt(ctx, *func.body);
		ctx.pop_scope();
	}
}

void semantic_analyzer::visit(context& ctx, middle::variable& var)
{
	ctx.current_scope()->add_symbol(var.name, &var);
	if (var.value) {
		visit_expr(ctx, *var.value);
		if (!var.type)
			var.type = infer_type(ctx, *var.value);
		else
			propagate_type(ctx, var.type, *var.value);
	}
}

/*
 * Statements
 */
void semantic_analyzer::visit_stmt(context& ctx, middle::statement& in_stmt)
{
	auto stmt_visitor = [this, &ctx] (auto& stmt)
	{
		return visit_stmt(ctx, stmt);
	};
	std::visit(stmt_visitor, in_stmt);
}

void semantic_analyzer::visit_stmt(context& ctx, middle::decl_statement& in_stmt)
{
	visit(ctx, *in_stmt.decl);
}

void semantic_analyzer::visit_stmt(context& ctx, middle::statement_block& in_block)
{
	ctx.push_scope();
	for (auto& stmt : in_block)
	{
		visit_stmt(ctx, stmt);
	}
	ctx.pop_scope();
}

void semantic_analyzer::visit_stmt(context& ctx, middle::if_else_statement& stmt)
{
	if (stmt.if_expr) {
		visit_expr(ctx, *stmt.if_expr);
		propagate_type(ctx, ctx.find_type("bool"), *stmt.if_expr);
	}
	if (stmt.if_body)
		visit_stmt(ctx, *stmt.if_body);
	if (stmt.else_body)
		visit_stmt(ctx, *stmt.else_body);
}

void semantic_analyzer::visit_stmt(context& ctx, middle::return_statement& stmt)
{
	if (stmt.value)
	{
		visit_expr(ctx, *stmt.value);

		if (auto func = ctx.current_scope()->find_func("$func")) // hack!
			propagate_type(ctx, func->return_type, *stmt.value);
	}
}

void semantic_analyzer::visit_stmt(context& ctx, middle::empty_statement& /*stmt*/)
{
}

void semantic_analyzer::visit_stmt(context& ctx, middle::expression& expr)
{
	visit_expr(ctx, expr);
	if (expr.type)
		propagate_type(ctx, expr.type, expr);
	else
		expr.type = infer_type(ctx, expr);
}

/*
 * Expressions
 */
void semantic_analyzer::visit_expr(context& ctx, middle::expression& expr)
{
	auto expr_visitor = [this, &ctx] (auto& expr)
	{
		return visit_expr(ctx, expr);
	};
	return std::visit(expr_visitor, expr);
}

void semantic_analyzer::visit_expr(context& ctx, middle::unary_expression& in_expr)
{
	visit_expr(ctx, *in_expr.lhs);
}

void semantic_analyzer::visit_expr(context& ctx, middle::binary_expression& in_expr)
{
	visit_expr(ctx, *in_expr.lhs);
	visit_expr(ctx, *in_expr.rhs);
}

void semantic_analyzer::visit_expr(context& ctx, middle::call_expression& call)
{
	for (auto& arg : call.args)
	{
		visit_expr(ctx, arg);
	}
	call.func = ctx.current_scope()->find_func(call.func_name);
}

void semantic_analyzer::visit_expr(context& ctx, middle::if_expression& in_expr)
{
	if (in_expr.if_expr)
		visit_expr(ctx, *in_expr.if_expr);
	if (in_expr.if_body)
		visit_expr(ctx, *in_expr.if_body);
	if (in_expr.else_body)
		visit_expr(ctx, *in_expr.else_body);
}

void semantic_analyzer::visit_expr(context& ctx, middle::value_expression& expr)
{
	expr.ref = ctx.current_scope()->find_var(expr.name);
}

void semantic_analyzer::visit_expr(context& ctx, middle::numeric_literal& in_expr)
{
}

void semantic_analyzer::visit_expr(context& ctx, middle::string_literal& in_expr)
{
}

void visit_op(context& ctx, ir::type* ty, middle::binary_expression& expr)
{
	using enum ir::binary_operator;

	if (auto* fp = std::get_if<ir::fp_type>(&ty->kind)) {
		switch(expr.op) {
		case less:
			expr.op = less_fp;
			break;
		case greater:
			expr.op = greater_fp;
			break;
		case divide:
			expr.op = divide_fp;
			break;
		default:
			break;
		}
	}

	if (auto* integer = std::get_if<ir::integer_type>(&ty->kind)) {
		const bool is_signed = integer->is_signed;

		switch(expr.op) {
		case less:
			expr.op = is_signed ? less : less_unsigned;
			break;
		case greater:
			expr.op = is_signed ? greater : greater_unsigned;
			break;
		case divide:
			expr.op = is_signed ? divide : divide_unsigned;
			break;
		default:
			break;
		}
	}
}

/*
 * Type inference
 */
auto semantic_analyzer::common_type(ir::type* a, ir::type* b) -> ir::type*
{
	if (!a || !b)
		return nullptr;

	if (a != b)
		return error_t(); // TODO

	return a;
}

auto semantic_analyzer::common_type(context& ctx, middle::expression& lhs, middle::expression& rhs) -> ir::type*
{
	auto* lhs_type = infer_type(ctx, lhs);
	auto* rhs_type = infer_type(ctx, rhs);

	if (!lhs_type && rhs_type)
		lhs_type = propagate_type(ctx, rhs_type, lhs);
	else if (lhs_type && !rhs_type)
		rhs_type = propagate_type(ctx, lhs_type, rhs);

	return common_type(lhs_type, rhs_type);
}

auto semantic_analyzer::common_type(context& ctx, ir::type* type, middle::expression& lhs, middle::expression& rhs) -> ir::type*
{
	auto* lhs_type = infer_type(ctx, lhs);
	auto* rhs_type = infer_type(ctx, rhs);

	if (!lhs_type || !rhs_type) {
		if (rhs_type)
			lhs_type = propagate_type(ctx, rhs_type, lhs);
		else if (lhs_type)
			rhs_type = propagate_type(ctx, lhs_type, rhs);
		else {
			lhs_type = propagate_type(ctx, type, lhs);
			lhs_type = propagate_type(ctx, type, rhs);
		}
	}

	return common_type(lhs_type, rhs_type);
}


auto semantic_analyzer::infer_type(context& ctx, middle::expression& expr) -> ir::type*
{
	auto expr_visitor = [this, &ctx] (auto& expr) -> ir::type*
	{
		return infer_type(ctx, expr);
	};

	expr.type = std::visit(expr_visitor, expr);

	return expr.type;
}

auto semantic_analyzer::infer_type(context& ctx, middle::unary_expression& expr) -> ir::type*
{
	return infer_type(ctx, *expr.lhs);
}

auto semantic_analyzer::infer_type(context& ctx, middle::binary_expression& expr) -> ir::type*
{
	auto ty = common_type(ctx, *expr.lhs, *expr.rhs);

	// TODO: move out of here
	visit_op(ctx, ty, expr);

	return ty;
}

auto semantic_analyzer::infer_type(context& ctx, middle::call_expression& expr) -> ir::type*
{
	if (expr.func)
	{
		auto& params = expr.func->args;

		for (const auto& [expr, param] : aw::paired(expr.args, params))
			propagate_type(ctx, param->type, expr);
		return expr.func->return_type;
	}
	return nullptr;
}

auto semantic_analyzer::infer_type(context& ctx, middle::if_expression& expr) -> ir::type*
{
	if (!expr.if_body || !expr.else_body)
		return ctx.find_type("void");

	return common_type(ctx, *expr.if_body, *expr.else_body);
}

auto semantic_analyzer::infer_type(context& ctx, middle::value_expression& expr) -> ir::type*
{
	if (auto var = expr.ref)
	{
		return var->type;
	}
	return nullptr;
}
auto semantic_analyzer::infer_type(context& ctx, middle::numeric_literal& expr) -> ir::type*
{
	return expr.type;
}
auto semantic_analyzer::infer_type(context& ctx, middle::string_literal& expr) -> ir::type*
{
	return ctx.find_type("string_literal");
}

/*
 * Type propagation
 */
auto semantic_analyzer::propagate_type(context& ctx, ir::type* type, middle::expression& expr) -> ir::type*
{
	auto expr_visitor = [this, type, &ctx] (auto& expr) -> ir::type*
	{
		return propagate_type(ctx, type, expr);
	};

	expr.type = std::visit(expr_visitor, expr);

	return expr.type;
}

auto semantic_analyzer::propagate_type(context& ctx, ir::type* type, middle::unary_expression& expr) -> ir::type*
{
	return propagate_type(ctx, type, *expr.lhs);
}

auto semantic_analyzer::propagate_type(context& ctx, ir::type* type, middle::binary_expression& expr) -> ir::type*
{
	auto ty = common_type(ctx, type, *expr.lhs, *expr.rhs);

	// TODO: move out of here
	visit_op(ctx, ty, expr);

	return ty;
}

auto semantic_analyzer::propagate_type(context& ctx, ir::type* type, middle::call_expression& expr) -> ir::type*
{
	return infer_type(ctx, expr);
}

auto semantic_analyzer::propagate_type(context& ctx, ir::type* type, middle::if_expression& expr) -> ir::type*
{
	propagate_type(ctx, ctx.find_type("bool"), *expr.if_expr);

	if (!expr.else_body)
	{
		propagate_type(ctx, type, *expr.if_body);
		return ctx.find_type("void");
	}

	return common_type(ctx, type, *expr.if_body, *expr.else_body);
}

auto semantic_analyzer::propagate_type(context& ctx, ir::type* type, middle::value_expression& expr) -> ir::type*
{
	// TODO: if the referred-to variable has no type, it should be back-propagated
	return common_type(type, infer_type(ctx, expr));
}

auto semantic_analyzer::propagate_type(context& ctx, ir::type* type, middle::numeric_literal& expr) -> ir::type*
{
	if (!expr.type)
		expr.type = type;
	return common_type(expr.type, type);
}

auto semantic_analyzer::propagate_type(context& ctx, ir::type* type, middle::string_literal& expr) -> ir::type*
{
	return common_type(type, ctx.find_type("u8*")); // TODO: should be string_literal
}

} // namespace aw::script
