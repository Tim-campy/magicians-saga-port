#include "current_culture.hpp"
namespace saga::core {
// Thread.GetCurrentCultureNoAppX 0x1ecd268; UserDefaultCulture 0x1e2a8e0.
// Preserve both TLS accesses on the hit branch, even if the second returns null.
std::shared_ptr<CultureInstance> resolve_current_culture(CurrentCultureState& state,
    ThreadCultureHost& thread,CurrentCultureHost& user_default) {
    if(thread.thread_culture())return thread.thread_culture();
    auto selected=thread.default_thread_culture();
    if(selected)return selected;
    return construct_current_culture(state,user_default);
}
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
