#ifndef aw_script_ast_middle_module_h
#define aw_script_ast_middle_module_h

#include <aw/script/ast/middle/declaration.h>

namespace aw::script::middle {

struct declaration;

enum class module_id;

struct module_import {
	module_id id;
	bool import_all = false;
	std::vector<ast::unqual_id> symbols;
};

// TODO: proper modules
struct module {
	std::string name;
	std::string dir_path;
	// Imported modules
	std::vector<module_import> imports;
	// Items declared in the module
	std::vector<std::unique_ptr<declaration>> decls;
	// Types instantiated in the module
	std::vector<std::unique_ptr<ir::type>> types;
};

} // namespace aw::script::middle

#endif // aw_script_ast_middle_module_h
