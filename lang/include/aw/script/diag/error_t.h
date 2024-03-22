/*
 * Copyright (C) 2015-2022 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_diag_error_t_h
#define aw_script_diag_error_t_h

#include <aw/script/diag/diagnostics_engine.h>

#include <memory>
#include <optional>

namespace aw::script {

struct error_t {
	constexpr operator bool() const { return false; }

	constexpr operator std::string_view() const { return {}; }

	template <typename T>
	constexpr operator std::unique_ptr<T>() const { return {}; }

	template <typename T>
	constexpr operator T*() const { return nullptr; }

	template <typename T>
	constexpr operator std::optional<T>() const { return std::nullopt; }
};

template<typename...Args>
inline error_t error(diagnostics_engine& diag, diagnostic_id id, location loc, Args&&... args)
{
	diag.report(diagnostic(id, loc, std::forward<Args>(args)...));
	return {};
}

template<typename...Args>
inline error_t error(diagnostics_engine& diag, diagnostic_id id, token tok, Args&&... args)
{
	return error(diag, id, tok.loc, tok.data, std::forward<Args>(args)...);
}

}//namespace aw::script
#endif//aw_script_diag_error_t_h
