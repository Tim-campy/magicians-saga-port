#pragma once
#include "culture_locale.hpp"
#include <memory>
#include <stdexcept>
namespace saga::core {
struct CultureInstance {
    CultureIdentity identity;
    bool read_only=false;
    bool use_user_override=false;
};
// Host-translated managed throws. Unrelated C++ failures are not swallowed.
class CultureManagedError:public std::runtime_error {public:using std::runtime_error::runtime_error;};
struct CurrentCultureState {std::shared_ptr<CultureInstance> cached;};
class CurrentCultureHost {
public:
    virtual ~CurrentCultureHost() = default;
    virtual bool invariant_mode() = 0; // Shipped GlobalizationMode initializes false.
    virtual std::optional<std::u16string> native_locale_name() = 0;
    virtual std::shared_ptr<CultureInstance> create_specific(const std::u16string&) = 0;
    virtual std::shared_ptr<CultureInstance> invariant_culture() = 0;
};
std::shared_ptr<CultureInstance> construct_current_culture(CurrentCultureState&,CurrentCultureHost&);
}
