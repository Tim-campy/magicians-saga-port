#include "current_culture.hpp"
namespace saga::core {
// NumberFormatInfo.Clone 0x1e147c0 and ReadOnly 0x1e14450.
std::shared_ptr<CultureNumberFormat> clone_number_format(const std::shared_ptr<CultureNumberFormat>& value) {
    if(!value)throw CultureManagedException("null number format");
    auto result=std::make_shared<CultureNumberFormat>(*value);
    result->read_only=false;
    return result;
}
std::shared_ptr<CultureNumberFormat> readonly_number_format(const std::shared_ptr<CultureNumberFormat>& value) {
    if(!value)throw CultureManagedException("null number format");
    if(value->read_only)return value;
    auto result=std::make_shared<CultureNumberFormat>(*value);
    result->read_only=true;
    return result;
}
// InvariantInfo 0x1e10458: default constructor, invariant flag, ReadOnly copy.
std::shared_ptr<CultureNumberFormat> make_invariant_number_format() {
    auto value=std::make_shared<CultureNumberFormat>();
    value->invariant=true;
    return readonly_number_format(value);
}
// CultureInfo .ctor 0x1e275c8 / ConstructInvariant 0x1e27468.
std::shared_ptr<CultureInstance> make_invariant_culture(bool user_override,bool read_only,
    const std::shared_ptr<CultureNumberFormat>& invariant_format) {
    auto value=std::make_shared<CultureInstance>();
    value->identity.lcid=127;
    value->identity.constructed=true;
    // Fresh zero-initialized territory stays null; name is String.Empty.
    // No native number-record lookup occurs on this path.
    value->use_user_override=user_override;
    value->read_only=read_only;
    value->number_format=read_only?invariant_format:clone_number_format(invariant_format);
    return value;
}
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
