/*
 * Copyright (C) 2015-2022 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_ast_decl_h
#define aw_script_ast_decl_h

#include "aw/script/lexer/token.h"
#include <aw/script/ast/mutability.h>
#include <aw/script/ast/statement.h>
#include <aw/script/ast/type.h>

#include <aw/types/string_view.h>

#include <memory>
#include <variant>
#include <vector>
#include <optional>

#include <cassert>

namespace aw::script::ast {

struct variable {
	std::string name;
	ast::type type;
	ast::access access;
	std::optional<expression> value;
};

struct variadic_parameter {
	std::string_view name;
};

struct parameter_list : std::vector<variable>
{
	std::optional<variadic_parameter> variadic;
};

struct function {
	std::string name;

	ast::type return_type;
	parameter_list params;

	std::unique_ptr<statement> body;

	bool is_variadic() const { return params.variadic.has_value(); }
};

using var_list = noncopyable<std::vector<variable>>;
struct struct_decl {
	std::string_view name;

	var_list members;
};

struct declaration;
using decl_list = noncopyable<std::vector<declaration>>;

struct foreign_block {
	std::string_view lang;

	enum type {
		import
	} kind;

	decl_list decls;
};

struct module_header {
	std::string_view name;
};

struct module {
	std::string path;
	std::string_view name;
	decl_list decls;
};

struct import_decl {
	// Module that is imported
	identifier mod_id;
	// List of declarations to import
	std::vector<unqual_id> decl_ids;
};

using declaration_variant = std::variant<
	variable,
	function,
	struct_decl,
	foreign_block,
	module_header,
	import_decl
>;

struct declaration : public declaration_variant {
	using declaration_variant::declaration_variant;
	using declaration_variant::operator=;
	token start_token;
};

} // namespace aw::script::ast

#endif//aw_script_ast_decl_h
