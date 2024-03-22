/*
 * Copyright (C) 2023  Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_driver_options_h
#define aw_script_driver_options_h

#include <aw/script/codegen/optimization_level.h>

#include <string>
#include <vector>

namespace aw::script::driver {

enum class mode {
	dry_run,
	assemble,
	make_obj,
	link,
};

struct options {
	driver::mode mode = mode::link;
	std::vector<std::string> input_files;
	std::string output_file;
	optimization_level opt_level = optimization_level::O0;
	bool gen_debug_info = false;
	bool dump_ast = false;
	bool dump_sem = false;
	bool dump_ir = false;
};

} // namespace aw::script::driver

#endif // aw_script_driver_options_h
