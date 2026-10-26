#include "current_culture.hpp"
namespace saga::core {
// CultureInfo.ConstructCurrentCulture 0x1e249b0, LSDA 0x948d70.
// Runtime class initialization/allocation identities are outside this decoded model.
std::shared_ptr<CultureInstance> construct_current_culture(CurrentCultureState& state,CurrentCultureHost& host) {
    if(state.cached)return state.cached;
    if(host.invariant_mode())return host.invariant_culture(); // No cache write on this branch.
    const auto name=host.native_locale_name(); // Outside the managed catch.
    std::shared_ptr<CultureInstance> result;
    if(name) {
        try {result=host.create_specific(*name);}
        catch(const CultureManagedError&) {} // Original managed-object filter, not catch-all C++.
    }
    if(result) {
        result->read_only=true;
        result->use_user_override=true;
    } else result=host.invariant_culture(); // Failure here propagates without a cache store.
    state.cached=result;
    return result;
}
}
