#include "linker.h"

#include <aw/config.h>

#include <aw/io/process.h>
#include <aw/utility/string/lazy_split.h>

#include <cstdlib>
#include <filesystem>

namespace aw::script {

namespace {

#if AW_PLATFORM != AW_PLATFORM_WIN32
bool is_executable(const std::filesystem::path& path)
{
	std::error_code ec;
	auto status = std::filesystem::status(path, ec);
	if (ec || !std::filesystem::is_regular_file(status))
		return false;

	using std::filesystem::perms;
	return (status.permissions() & perms::owner_exec) != perms::none;
}

bool exists_on_path(std::string_view name)
{
	const char* path_env = std::getenv("PATH");
	if (!path_env)
		return false;

	for (auto dir : aw::string::lazy::split_by(path_env, ":"))
	{
		if (is_executable(std::filesystem::path(dir) / std::string(name)))
			return true;
	}
	return false;
}
#endif

} // namespace

auto enum_linkers() -> std::vector<std::string>
{
#if AW_PLATFORM == AW_PLATFORM_WIN32
	return { "link" };
#else
	return { "cc", "ld" };
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

	} else if (params.linker == "cc") {
		// Hand off to the system C compiler driver, same as rustc does:
		// it already knows how to find and invoke the right linker for
		// the target, so awscript doesn't have to duplicate that logic.
		linker = "cc";
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
			"-o", params.output_file,
			"/usr/lib/Scrt1.o"
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
	for (const auto& candidate : enum_linkers())
	{
		if (exists_on_path(candidate))
			return candidate;
	}
	// Nothing found on PATH; return the preferred candidate anyway and
	// let run_linker() surface a clear spawn failure.
	return enum_linkers().front();
#endif
}

} // namespace aw::script
