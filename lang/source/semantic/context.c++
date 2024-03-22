#include "context.h"

#include "aw/script/utility/wrap.h"
#include "aw/script/utility/type_name.h"

namespace aw::script {

// TODO: use composite keys instead of strings?
// TODO: mangle name?
auto context::create_type(const ast::type &type) -> ir::type*
{
	if (auto _ = get_if<ast::unknown_type>(&type)) {
		assert(!"Unknown type must be inferred");
		return nullptr;
	}

	if (auto* ty = get_if<ast::regular_type>(&type)) {
		auto existing_ty = find_type(type_name(*ty));
		if (existing_ty)
			return existing_ty;

		return add_type(ir::type{ type_name(*ty) });
	}

	if (auto* arr = get_if<ast::array_type>(&type)) {
		auto name = type_name(*arr);

		if (auto existing_ty = find_type(name))
			return existing_ty;

		auto elem = find_type(type_name(arr->elem));

		// TODO: convert unsized to array_slice
		return add_type(ir::type{
			.name = name,
			.kind = ir::array_type{
				.base_type = elem,
				.size = arr->size,
			}
		});
	}

	if (auto ptr = get_if<ast::pointer_type>(&type)) {
		auto name = type_name(*ptr);

		if (auto existing_ty = find_type(name))
			return existing_ty;

		auto pointee = find_type(type_name(ptr->pointee));

		return add_type(ir::type{
			.name = name,
			.kind = ir::pointer_type{
				.base_type = pointee,
				.is_mutable = false, // for now
			}
		});
	}

	// TODO
	assert(!"Not implemented");
	return nullptr;
}

auto context::add_type(ir::type&& type) -> ir::type*
{
	auto existing_ty = find_type(type.name);
	if (existing_ty) {
		if (get_if<ir::unknown_type>(&existing_ty->kind))
			*existing_ty = std::move(type);
		return existing_ty;
	}

	auto new_type = wrap(std::move(type));
	auto type_ref = new_type.get();

	types.push_back(std::move(new_type));

	default_scope.add_type(type_ref->name, type_ref);

	return type_ref;
}


} // namespace aw::script
