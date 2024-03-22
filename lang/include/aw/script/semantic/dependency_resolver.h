#ifndef aw_script_semantic_resolve_dependencies_h
#define aw_script_semantic_resolve_dependencies_h

#include <aw/script/diag/diagnostics_engine.h>
#include <aw/script/ast/declaration.h>

namespace aw::script {
struct module_depenency {
	ast::identifier module_id;
	bool import_all = false;
	std::vector<ast::unqual_id> imported_ids;
};
auto get_module_dependencies(diagnostics_engine& diag, const ast::module& mod)
	-> std::vector<module_depenency>;

} // namespace aw::script

#endif // aw_script_semantic_resolve_dependencies_h
