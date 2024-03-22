#ifndef aw_script_ast_linkage_h
#define aw_script_ast_linkage_h

namespace aw::script::ast {

enum class linkage {
	internal,
	imported,
	exported
};

} // namespace aw::script::ast

#endif // aw_script_ast_linkage_h
