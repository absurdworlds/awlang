#include "type_inference.h"

#include "aw/script/ast/middle/module.h"
#include <aw/script/diag/error_t.h>
#include <aw/script/utility/location.h>

#include <aw/algorithm/in.h>

#include <aw/utility/ranges/paired.h>

#include <iostream>

namespace aw::script {
using namespace middle;

bool is_logical_operation(ir::binary_operator op)
{
	using enum ir::binary_operator;
	return in(op, logical_and, logical_or);
}

bool is_comparison(ir::binary_operator op)
{
	using enum ir::binary_operator;
	return in(op, less, less_equal, greater, greater_equal, equal, not_equal);
}

bool is_number(ir::type* type)
{
	return std::holds_alternative<ir::integer_type>(type->kind) ||
	       std::holds_alternative<ir::fp_type>(type->kind);
}

struct type_inference_visitor
{
	context& ctx;
	diagnostics_engine& diag;

	auto common_type(ir::type* a, ir::type* b) -> ir::type*
	{
		if (!a || !b)
			return nullptr;

		if (a != b) {
			// TODO: add convesion tables for types
			if (a->name == "u8*" && b->name == "string_literal")
				return a;
			if (a->name == "string_literal" && b->name == "u8*")
				return b;
			if (is_number(a) && b->name == "numeric_literal")
				return a;
			if (a->name == "numeric_literal" && is_number(b))
				return b;
			return error(diag, diagnostic_id::type_mismathch, location(), a->name, b->name);
		}

		return a;
	}

	auto common_type(expression& lhs, expression& rhs) -> ir::type*
	{
		auto lhs_type = infer_type(lhs);
		auto rhs_type = infer_type(rhs);

		if (rhs_type.is_ambiguous)
			rhs_type = propagate_type(lhs_type, rhs);
		if (lhs_type.is_ambiguous)
			lhs_type = propagate_type(rhs_type, lhs);

		return common_type(lhs_type, rhs_type);
	}


	auto infer_type(expression& expr) -> type_hint
	{
		auto expr_visitor = [this] (auto& expr) -> type_hint
		{
			return infer_type(expr);
		};

		auto hint = std::visit(expr_visitor, expr);

		expr.type = hint.type;

		return hint;
	}

	auto infer_type(cast_expression& expr) -> type_hint
	{
		auto from_type = infer_type(*expr.lhs);
		if (from_type.is_ambiguous)
			propagate_type(expr.to_type, *expr.lhs);
		return expr.to_type;
	}

	auto infer_type(unary_expression& expr) -> type_hint
	{
		auto hint = infer_type(*expr.lhs);
		if (expr.op == ir::unary_operator::logical_not)
			return ctx.find_type("bool");
		if (expr.op == ir::unary_operator::reference)
			return ctx.add_type(ir::type{
				.name = hint.type->name + "&",
				.kind = ir::reference_type{
					.base_type = hint.type,
					.is_mutable = false,
				},
			});
		return hint;
	}

	auto infer_type(binary_expression& expr) -> type_hint
	{
		auto type = common_type(*expr.lhs, *expr.rhs);
		if (is_logical_operation(expr.op) || is_comparison(expr.op))
			return ctx.find_type("bool");
		return type;
	}

	auto infer_type(call_expression& expr) -> type_hint
	{
		if (!expr.func)
			return nullptr;

		auto& params = expr.func->args;

		for (const auto& [expr, param] : aw::paired(expr.args, params)) {
			auto type = infer_type(expr);
			if (!type)
				propagate_type(param->type, expr);
		}

		for (size_t i = params.size(), e = expr.args.size(); i<e; ++i)
			infer_type(expr.args[i]);

		return expr.func->return_type;
	}

	auto infer_type(field_expression& expr) -> type_hint
	{
		if (!expr.type) {
			auto hint = infer_type(*expr.lhs);
			if (!hint) {
				error(diag, diagnostic_id::is_not_a_struct_type, location());
				return nullptr;
			}

			expr.type = get_if<ir::struct_type>(&hint.type->kind);
			if (!expr.type) {
				error(diag, diagnostic_id::is_not_a_struct_type, location());
				return nullptr;
			}
		}

		auto* type = expr.type;
		if (auto* field = type->find_field(expr.name))
			return field->type;

		error(diag, diagnostic_id::no_such_field, location(), type->decl->name, expr.name);
		return nullptr;
	}

	auto infer_type(subscript_expression& expr) -> type_hint
	{
		for (auto& arg : expr.args) {
			auto hint = infer_type(arg);
			if (hint.is_ambiguous)
				propagate_type(ctx.find_type("isize"), arg);
		}

		auto hint = infer_type(*expr.lhs);
		if (hint) {
			if (auto arr_type = get_if<ir::array_type>(&hint.type->kind))
				return arr_type->base_type;
		}
		return nullptr;
	}

	auto infer_type(if_expression& expr) -> type_hint
	{
		auto type = infer_type(*expr.if_expr);
		if (type.is_ambiguous)
			propagate_type(ctx.find_type("bool"), *expr.if_expr);
		if (!expr.else_body)
			return ctx.find_type("void");
		return common_type(*expr.if_body, *expr.else_body);
	}

	auto infer_type(value_expression& expr) -> type_hint
	{
		if (auto var = expr.ref)
			return var->type;
		return nullptr;
	}

	auto infer_type(numeric_literal& expr) -> type_hint
	{
		if (expr.type)
			return expr.type;
		return { ctx.find_type("numeric_literal"), true };
	}

	auto infer_type(float_literal& expr) -> type_hint
	{
		if (expr.type)
			return expr.type;
		return { ctx.find_type("float_literal"), true };
	}

	auto infer_type(bool_literal& /*expr*/) -> type_hint
	{
		return ctx.find_type("bool");
	}

	auto infer_type(string_literal& /*expr*/) -> type_hint
	{
		return ctx.find_type("string_literal");
	}

	auto infer_type(list_literal& /*expr*/) -> type_hint
	{
		return nullptr;
	}

	auto infer_type(numbered_list_literal& /*expr*/) -> type_hint
	{
		return nullptr;
	}

	auto infer_type(struct_literal& /*expr*/) -> type_hint
	{
		return nullptr;
	}

	auto propagate_type(ir::type* type, expression& expr) -> ir::type*
	{
		auto expr_visitor = [this, type] (auto& expr) -> ir::type*
		{
			return propagate_type(type, expr);
		};

		auto res = std::visit(expr_visitor, expr);
		if (res)
			expr.type = res;

		return expr.type;
	}

	auto propagate_type(ir::type* /*type*/, cast_expression& expr) -> ir::type*
	{
		return expr.to_type;
	}

	auto propagate_type(ir::type* type, unary_expression& expr) -> ir::type*
	{
		return propagate_type(type, *expr.lhs);
	}

	auto propagate_type(ir::type* type, if_expression& expr) -> ir::type*
	{
		auto* lhs = propagate_type(type, *expr.if_body);
		if (!expr.else_body)
			return ctx.find_type("void");
		auto* rhs = propagate_type(type, *expr.else_body);
		return common_type(lhs, rhs);
	}

	auto propagate_type(ir::type* type, binary_expression& expr) -> ir::type*
	{
		auto* lhs = propagate_type(type, *expr.lhs);
		auto* rhs = propagate_type(type, *expr.rhs);
		return common_type(lhs, rhs);
	}

	auto propagate_type(ir::type* type, call_expression& expr) -> ir::type*
	{
		return nullptr;
	}

	auto propagate_type(ir::type* type, subscript_expression& expr) -> ir::type*
	{
		return nullptr;
	}

	auto propagate_type(ir::type* type, field_expression& expr) -> ir::type*
	{
		return nullptr;
	}

	auto propagate_type(ir::type* type, value_expression& expr) -> ir::type*
	{
		return nullptr;
	}

	auto propagate_type(ir::type* type, bool_literal& expr) -> ir::type*
	{
		return nullptr;
	}

	auto propagate_type(ir::type* type, string_literal& expr) -> ir::type*
	{
		return nullptr;
	}

	auto propagate_type(ir::type* type, float_literal& expr) -> ir::type*
	{
		return (expr.type = type);
	}

	auto propagate_type(ir::type* type, numeric_literal& expr) -> ir::type*
	{
		return (expr.type = type);
	}

	auto propagate_type(ir::type* type, list_literal& expr) -> ir::type*
	{
		assert(type);
		if (expr.type) // TODO: check if same
			return nullptr;

		expr.type = type;
		if (auto arr_type = get_if<ir::array_type>(&type->kind)) {
			for (auto& field : expr.fields)
				propagate_type(arr_type->base_type, field);
		} else if (auto struct_type = get_if<ir::struct_type>(&type->kind)) {
			auto pairs = paired(struct_type->fields, expr.fields);
			for (auto [struct_field, field] : pairs)
				propagate_type(struct_field.type, field);
		}

		return expr.type;
	}

	auto propagate_type(ir::type* type, numbered_list_literal& expr) -> ir::type*
	{
		assert(type);
		if (expr.type) // TODO: check if same
			return nullptr;

		auto arr_type = get_if<ir::array_type>(&type->kind);
		if (!arr_type) {
			error(diag, diagnostic_id::is_not_an_array_type, location(), type->name);
			return nullptr;
		}

		expr.type = type;

		for (auto& field : expr.fields) {
			propagate_type(arr_type->base_type, *field.value);
		}

		return expr.type;
	}

	auto propagate_type(ir::type* type, struct_literal& expr) -> ir::type*
	{
		assert(type);
		if (expr.type) // TODO: check if same
			return nullptr;

		expr.type = get_if<ir::struct_type>(&type->kind);
		if (!expr.type) {
			error(diag, diagnostic_id::is_not_a_struct_type, location(), type->name);
			return nullptr;
		}

		auto& struct_type = *expr.type;

		for (auto& field : expr.fields) {
			ir::type* type = infer_type(*field.value);
			if (type)
				continue;

			auto* struct_field = struct_type.find_field(field.name);
			if (!struct_field) {
				error(diag, diagnostic_id::no_such_field, location(), type->name, field.name);
				continue;
			}

			propagate_type(struct_field->type, *field.value);
		}

		return type;
	}


};

void infer_types(context& ctx, diagnostics_engine& diag, middle::module& mod)
{


}

auto infer_type(context& ctx, diagnostics_engine& diag, middle::expression& expr) -> type_hint
{
	type_inference_visitor visitor{ .ctx = ctx, .diag = diag };
	return visitor.infer_type(expr);
}

auto propagate_type(context& ctx, diagnostics_engine& diag, ir::type* type, middle::expression& expr) -> ir::type*
{
	type_inference_visitor visitor{ .ctx = ctx, .diag = diag };
	return visitor.propagate_type(type, expr);
}

} // namespace aw::script
