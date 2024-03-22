/*
 * Copyright (C) 2015  Hedede <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_diagnostic_h
#define aw_script_diagnostic_h
#include <utility>
#include <vector>

#include <aw/script/utility/location.h>
#include <aw/script/utility/print_token.h>
#include <aw/script/ast/identifier.h>
namespace aw::script {
class diagnostics_engine;

enum class diagnostic_id {
#define DIAG(type, id, msg) id,
#include <aw/script/diag/messages.h>
};

/*!
 * This class is used to compose diagnostic messages
 */
class diagnostic {
public:
	diagnostic(location loc, diagnostic_id id)
		: loc(loc), id(id)
	{
	}

	template<typename... Args>
	diagnostic(diagnostic_id id, location loc, Args&&... args)
		: loc(loc), id(id),
		  args{ arg(std::forward<Args>(args)) ... }
	{
	}

private:
	friend class diagnostics_engine;

	static std::string arg(std::string str)
	{
		return str;
	}

	static std::string arg(std::string_view str)
	{
		return std::string(str);
	}

	static std::string arg(token_kind type)
	{
		return spell_token(type);
	}

	static std::string arg(const ast::identifier& id)
	{
		return to_string(id);
	}

	location loc;
	diagnostic_id id;
	std::vector<std::string> args;
};

std::string diag_message_template(diagnostic_id id);

} // namespace aw::script
#endif//aw_script_diagnostic_h
