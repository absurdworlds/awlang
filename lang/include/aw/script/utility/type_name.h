#ifndef aw_script_utility_type_name_h
#define aw_script_utility_type_name_h

#include "aw/script/ast/type.h"

#include <cassert>
#include <string>

namespace aw::script {
inline auto type_name(const ast::identifier& id) -> std::string
{
	return to_string(id);
}

inline auto type_name(const ast::unknown_type& /*type*/) -> std::string
{
	return "<inferred type>";
}

inline auto type_name(const ast::regular_type& type) -> std::string
{
	return type_name(type.name);
}

inline auto type_name(const ast::pointer_type& type) -> std::string
{
	return type_name(type.pointee) + "*";
}

inline auto type_name(const ast::reference_type& type) -> std::string
{
	return type_name(type.pointee) + "&";
}

inline auto type_name(const ast::array_type& type) -> std::string
{
	auto arr_size = [] (std::optional<size_t> opt) {
		std::string result = "[";
		if (opt)
			result += std::to_string(*opt);
		result += "]";
		return result;
	};
	return type_name(type.elem) + arr_size(type.size);
}

inline auto type_name(const ast::type& type) -> std::string
{
	const auto visitor = [] (auto& v) -> std::string { return type_name(v); };
	return std::visit(visitor, type);
}

} // namespace aw::script

#endif // aw_script_utility_type_name_h
