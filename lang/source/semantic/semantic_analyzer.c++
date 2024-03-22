#include "aw/script/semantic/semantic_analyzer.h"

#include "context.h"
#include "convert_to_middle.h"
#include "type_inference.h"
#include "resolve_references.h"

#include <aw/script/semantic/scope.h>
#include <aw/script/utility/wrap.h>

#include <aw/script/diag/error_t.h>

#include <aw/utility/ranges/paired.h>
#include <aw/utility/ranges/ipairs.h>

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
	types.push_back(wrap(ir::type{ "bool", ir::simple_type::boolean }));

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

	types.push_back(create_type("numeric_literal"));
	types.push_back(create_type("string_literal"));
	types.push_back(create_type("float_literal"));


	return types;
}

semantic_analyzer::semantic_analyzer(diagnostics_engine& diag)
	: builtin_types(create_builtin_types())
	, diag(diag)
{
}

// Overview:
// 1. Load imports
// 2. Prepare the symbol table
// 3. Resolve references
// 4. Infer and check types
// 5. Resolve overloaded operators and functions

auto semantic_analyzer::lower(const ast::module& in_mod) -> middle::module
{
	context ctx;

	for (const auto& type : builtin_types)
	{
		ctx.default_scope.add_type(type->name, type.get());
	}


	middle::module mod = convert_to_middle(ctx, diag, in_mod);

	ctx.push_scope();
	for (const auto& decl : mod.decls)
	{
		auto* decl_ptr = decl.get();
		if (auto decl = std::get_if<middle::variable>(decl_ptr))
			ctx.current_scope()->add_symbol(decl->name, decl);
		if (auto decl = std::get_if<middle::function>(decl_ptr))
			ctx.current_scope()->add_symbol(decl->name, decl);
		//if (auto decl = std::get_if<middle::struct_decl>(decl_ptr)>)
	}

	resolve_references(ctx, diag, mod);

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
		ctx.push_scope();
		ctx.current_scope()->add_symbol("$func", &func); // hack!

		if (func.is_variadic) {
			using namespace std::string_view_literals;
			error(diag, diagnostic_id::not_implemented_yet, location(), "variadic functions"sv);
			return;
		}

		visit_stmt(ctx, *func.body);

		ctx.pop_scope();
	}
}

void infer_array_size(middle::variable& var)
{
	auto arr = get_if<ir::array_type>(&var.type->kind);
	if (!arr)
		return;
	if (arr->size.has_value())
		return;
	if (auto init = get_if<middle::list_literal>(var.value.get()))
	{
		arr->size = init->fields.size();
	}
}

void semantic_analyzer::visit(context& ctx, middle::variable& var)
{
	if (var.value) {
		auto hint = infer_type(ctx, diag, *var.value);
		if (!var.type) {
			if (hint.type->name == "numeric_literal")
				var.type = ctx.find_type("isize");
			else if (hint.type->name == "float_literal")
				var.type = ctx.find_type("f64");
			else
				var.type = hint.type;
		}

		if (hint.is_ambiguous)
			propagate_type(ctx, diag, var.type, *var.value);

		visit_expr(ctx, *var.value);
		infer_array_size(var);
	}
}

void semantic_analyzer::visit(context& ctx, middle::struct_decl& decl)
{
	ir::struct_type type{
		.decl = &decl
	};

	ctx.push_scope();
	for (auto [i,field] : ipairs(decl.members)) {
		visit(ctx, *field);
		type.fields[i] = ir::struct_type::field{
			.name = field->name,
			.type = field->type,
			.init = field->value.get(),
		};
		type.field_map[field->name] = i;
	}
	ctx.pop_scope();

	ctx.add_type(ir::type{ decl.name, std::move(type) });
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
	if (stmt.if_expr)
		visit_stmt(ctx, *stmt.if_expr);
	if (stmt.if_body)
		visit_stmt(ctx, *stmt.if_body);
	if (stmt.else_body)
		visit_stmt(ctx, *stmt.else_body);
}

void semantic_analyzer::visit_stmt(context &ctx, middle::while_statement& stmt)
{
	if (stmt.cond_expr) {
		infer_type(ctx, diag, *stmt.cond_expr);
		visit_expr(ctx, *stmt.cond_expr);
	}
	if (stmt.loop_body)
		visit_stmt(ctx, *stmt.loop_body);
}

void semantic_analyzer::visit_stmt(context& ctx, middle::return_statement& stmt)
{
	if (stmt.value) {
		auto hint = infer_type(ctx, diag, *stmt.value);
		visit_expr(ctx, *stmt.value);

		if (hint.is_ambiguous) {
			auto func = ctx.current_scope()->find_func("$func");
			assert(func);
			propagate_type(ctx, diag, func->return_type, *stmt.value);
		}

	}
}

void semantic_analyzer::visit_stmt(context& ctx, middle::empty_statement& /*stmt*/)
{
}

void semantic_analyzer::visit_stmt(context& ctx, middle::expression& expr)
{
	if (!expr.type)
		expr.type = infer_type(ctx, diag, expr);
	visit_expr(ctx, expr);
}

/*
 * Expressions
 */
auto convert_to_fp(ir::binary_operator op) -> ir::binary_operator
{
	switch(op) {
		using enum ir::binary_operator;
	case plus:
		return plus_fp;
	case minus:
		return minus_fp;
	case multiply:
		return multiply_fp;
	case divide:
		return divide_fp;
	case modulo:
		return modulo_fp;
	case less:
		return less_fp;
	case greater:
		return greater_fp;
	case less_equal:
		return less_equal_fp;
	case greater_equal:
		return greater_equal_fp;
	case equal:
		return equal_fp;
	case not_equal:
		return not_equal_fp;
	default:
		return op;
	}
}

auto convert_to_uint(ir::binary_operator op) -> ir::binary_operator
{
	switch(op) {
		using enum ir::binary_operator;
	case divide:
		return divide_unsigned;
	case shift_right:
		return shift_right_unsigned;
	case less:
		return less_unsigned;
	case greater:
		return greater_unsigned;
	case less_equal:
		return less_equal_unsigned;
	case greater_equal:
		return greater_equal_unsigned;
	default:
		return op;
	}
}

void visit_op(context& ctx, ir::type* type, ir::binary_operator& op)
{
	if (!type)
		return;

	if (auto* fp = std::get_if<ir::fp_type>(&type->kind)) {
		op = convert_to_fp(op);
		return;
	}

	if (auto* integer = std::get_if<ir::integer_type>(&type->kind)) {
		if (!integer->is_signed)
			op = convert_to_uint(op);
		return;
	}
}

void visit_op(context& ctx, ir::type* ty, ir::unary_operator& op)
{
	if (auto* fp = std::get_if<ir::fp_type>(&ty->kind)) {
		if (op == ir::unary_operator::minus)
			op = ir::unary_operator::minus_fp;
		return;
	}
}

void semantic_analyzer::visit_expr(context& ctx, middle::expression& expr)
{
	auto expr_visitor = [this, &ctx] (auto& expr)
	{
		return visit_expr(ctx, expr);
	};

	if (auto bin_expr = get_if<middle::binary_expression>(&expr)) {
		visit_op(ctx, expr.type, bin_expr->op);
	} else if (auto un_expr = get_if<middle::unary_expression>(&expr)) {
		visit_op(ctx, expr.type, un_expr->op);
	}

	std::visit(expr_visitor, expr);
}

void semantic_analyzer::visit_expr(context& ctx, middle::cast_expression& in_expr)
{
	visit_expr(ctx, *in_expr.lhs);
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
		visit_expr(ctx, arg);

	if (call.func && !call.func->is_variadic) {
		const auto n_params = call.func->args.size();
		const auto n_args = call.args.size();
		if (n_params != n_args) {
			// TODO: distinguish "not enough" and "too many"
			error(diag, diagnostic_id::not_enough_arguments, location(),
				call.func_name, std::to_string(n_params), std::to_string(n_args));
		}
	}
}

void semantic_analyzer::visit_expr(context& ctx, middle::field_expression& expr)
{
	visit_expr(ctx, *expr.lhs);

	ir::type* type = infer_type(ctx, diag, *expr.lhs);
	if (!type) {
		// TODO: error
		return;
	}
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
}

void semantic_analyzer::visit_expr(context& ctx, middle::subscript_expression& expr)
{
	visit_expr(ctx, *expr.lhs);
	for (auto& arg : expr.args)
		visit_expr(ctx, arg);
}

void semantic_analyzer::visit_expr(context& ctx, middle::struct_literal& expr)
{
	for (auto& field : expr.fields)
		visit_expr(ctx, *field.value);
}

void semantic_analyzer::visit_expr(context& ctx, middle::numbered_list_literal& expr)
{
	for (auto& field : expr.fields)
		visit_expr(ctx, *field.value);
}

void semantic_analyzer::visit_expr(context& ctx, middle::list_literal& expr)
{
	for (auto& field : expr.fields)
		visit_expr(ctx, field);
}


} // namespace aw::script
