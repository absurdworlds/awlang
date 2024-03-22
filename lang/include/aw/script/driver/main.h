/*
 * Copyright (C) 2023  Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_driver_main_h
#define aw_script_driver_main_h

#include <aw/script/driver/options.h>

namespace aw::script::driver {

int run_compiler(const options& options);

} // namespace aw::script::driver

#endif // aw_script_driver_main_h
