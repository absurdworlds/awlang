#include <aw/script/driver/main.h>

#include <aw/utility/argv_parser.h>
#include <aw/algorithm/in.h>

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
	using aw::in;
	std::vector<std::string> parameters;

	aw::utils::argv_parser parser(argv+1);

	aw::script::driver::options options;
	while(true) {
		auto arg = parser.parse_argument();
		if (!arg)
			break;

		if (arg->type == arg->argument) {
			options.input_files.push_back(arg->value);
			continue;
		}

		auto name = arg->name;
		auto value = arg->value;

		if (name.empty()) {
			std::cerr << "Invalid parameters.";
			return EXIT_FAILURE;
		}

		if (in(name, "o", "output")) {
			if (value.empty())
				value = parser.get_param();
			options.output_file = value;
		} else if (in(name, "dump-ast")) {
			options.dump_ast = true;
		} else if (in(name, "dump-sem-tree")) {
			options.dump_sem = true;
		} else if (in(name, "dump-ir")) {
			options.dump_ir = true;
		} else if (in(name, "g", "gen-debug")) {
			options.gen_debug_info = true;
		} else if (in(name, "n", "dry-run")) {
			options.mode = aw::script::driver::mode::dry_run;
		} else if (in(name, "c", "no-link")) {
			options.mode = aw::script::driver::mode::make_obj;
		} else if (in(name, "O")) {
			if (value.empty())
				value = parser.get_param();
			if (value == "0")
				options.opt_level = aw::script::optimization_level::O0;
			if (value == "1")
				options.opt_level = aw::script::optimization_level::O1;
			if (value == "2")
				options.opt_level = aw::script::optimization_level::O2;
			if (value == "3")
				options.opt_level = aw::script::optimization_level::O3;
		}
	}

	return aw::script::driver::run_compiler(options);
}
