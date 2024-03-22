#include <aw/script/utility/ast_printer_default.h>

#include <aw/script/driver/main.h>

#include <aw/io/file.h>

#include <iostream>

int main(int /*argc*/, char** argv)
{
	using namespace aw::script;

	if (argv[1] == nullptr)
		return 1;

	std::cout << argv[1] << std::endl;

	driver::options options;
	options.input_files.emplace_back(argv[1]);
	options.mode = driver::mode::dry_run;
	options.dump_ast = true;

	return driver::run_compiler(options);
}
