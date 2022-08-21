/*
 * Copyright (C) 2022 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_symbol_table_h
#define aw_script_symbol_table_h

#include <memory>
#include <unordered_map>
#include <string_view>
#include <vector>

namespace aw::script {
namespace ast {
class declaration;
class type;
class scope;
} // namespace ast

struct unresolved_type {
	ast::scope* scope;
	ast::type** type;
	std::string_view name;
};

class symbol_table {
public:
	std::unique_ptr<ast::scope> create_scope();

	void pop_scope();

	// TODO: create a dedicated structure to represent a symbol name
	ast::declaration* lookup(std::string_view name);

	ast::scope* current_scope();

	void add_unresolved(std::string_view type_name, ast::type** type);

private:
	std::unique_ptr<ast::scope> top_scope;

	std::vector<ast::scope*> stack;

	std::vector<std::unique_ptr<ast::declaration>> types;

	std::vector<unresolved_type> unresolved_types;
};
} // namespace aw::script

#endif//aw_script_symbol_table_h