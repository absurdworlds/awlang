#ifndef aw_script_driver_linker_h
#define aw_script_driver_linker_h

#include <vector>
#include <string>

namespace aw::script {

struct linker_parameters {
	std::string linker;
	std::string output_file;
	std::vector<std::string> objects;
};

auto enum_linkers() -> std::vector<std::string>;
auto default_linker() -> std::string;
bool run_linker(const linker_parameters& params);

} // namespace aw::script


#endif // aw_script_driver_linker_h
