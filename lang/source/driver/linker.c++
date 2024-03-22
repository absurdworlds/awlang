#include "linker.h"

#include <aw/config.h>

#include <aw/io/process.h>

namespace aw::script {

// TODO: actually detect linkers
auto enum_linkers() -> std::vector<std::string>
{
#if AW_PLATFORM == AW_PLATFORM_WIN32
	return { "link" };
#else
	return { "g++", "ld" };
#endif
}

bool run_linker(const linker_parameters& params)
{
	std::string linker;
	std::vector<std::string> linker_args;

	if (params.linker == "link") {
		linker = "link.exe";
		linker_args = {
			"/machine:x64", "/subsystem:console",
			"/manifest", "/manifest:embed",
			"/debug",
			"/OUT:", params.output_file,
			"msvcrtd.lib", "kernel32.lib", "legacy_stdio_definitions.lib",
		};
		// TODO: research which libraries are actually needed
		// msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib
		// shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

	} else if (params.linker == "g++") {
		linker = "g++";
		linker_args = {
			"-o", params.output_file,
		};
	} else if (params.linker == "ld") {
		linker = "ld";
		linker_args = {
			"-m", "elf_x86_64",
			"-pie",
			"-dynamic-linker", "/usr/lib/ld-linux-x86-64.so.2",
			"-lc",
			"-o ", params.output_file,
			" /usr/lib/Scrt1.o"
		};
	}

	linker_args.insert(begin(linker_args), begin(params.objects), end(params.objects));

	return io::run(linker, linker_args) == io::wait_status::finished;
}

std::string default_linker()
{
#if AW_PLATFORM == AW_PLATFORM_WIN32
	return "link";
#else
	return "g++";
#endif
}

} // namespace aw::script
