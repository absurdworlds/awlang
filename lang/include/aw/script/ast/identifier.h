#ifndef aw_lang_ast_indentifier_h
#define aw_lang_ast_indentifier_h

#include <string>
#include <string_view>
#include <vector>

namespace aw::script::ast {

using unqual_id = std::string;

struct identifier {
	std::vector<std::string> path;
	unqual_id name;

	bool is_qualified() const { return !path.empty(); }
};

inline bool operator==(const identifier& id, const std::string_view& sv)
{
	return !id.is_qualified() && id.name == sv;
}

inline bool operator==(const identifier& lhs, const identifier& rhs)
{
	return lhs.path == rhs.path &&
	       lhs.name == rhs.name;
}
inline bool operator<(const identifier& lhs, const identifier& rhs)
{
	if (lhs.path == rhs.path)
		return lhs.name < rhs.name;
	return lhs.path < rhs.path;
}

inline auto to_string(const identifier& id) -> std::string
{
	// TODO: reserve
	std::string result;
	for (const auto& elem : id.path) {
		result += elem;
		result += "::";
	}
	result += id.name;
	return result;
}


} // namespace aw::script::ast

#endif // aw_lang_ast_indentifier_h
