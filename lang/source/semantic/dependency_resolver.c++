
#include "dependency_scanner.h"

#include <aw/script/semantic/dependency_resolver.h>

namespace aw::script {
auto get_module_dependencies(diagnostics_engine& diag, const ast::module& mod)
	-> std::vector<module_depenency>
{
	dependency_scanner visitor(diag);
	for (const auto& decl : mod.decls)
		visitor.visit_decl(decl);

	for (auto& dep : visitor.deps)
	{
		auto& ids = dep.imported_ids;
		std::sort(ids.begin(), ids.end());
		auto new_end = std::unique(ids.begin(), ids.end());
		ids.erase(new_end, ids.end());
	}

	return visitor.deps;
}
} // namespace aw::script
