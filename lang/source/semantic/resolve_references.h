#ifndef aw_script_resolve_references_h
#define aw_script_resolve_references_h

#include "context.h"

#include "aw/script/diag/diagnostics_engine.h"

namespace aw::script {
namespace middle {
struct module;
} // namespace middle;

void resolve_references(context& ctx, diagnostics_engine& diag, middle::module& mod);

} // namespace aw::script

#endif // aw_script_resolve_references_h
