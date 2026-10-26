#pragma once
#include "culture_locale.hpp"
#include <memory>
#include <stdexcept>
namespace saga::core {
// NumberFormatInfo projection for integer parsing, not the full formatting API.
struct CultureNumberFormat {
    std::u16string positive=u"+",negative=u"-";
    bool read_only=false,invariant=false;
};
std::shared_ptr<CultureNumberFormat> clone_number_format(const std::shared_ptr<CultureNumberFormat>&);
std::shared_ptr<CultureNumberFormat> readonly_number_format(const std::shared_ptr<CultureNumberFormat>&);
std::shared_ptr<CultureNumberFormat> make_invariant_number_format();
struct CultureInstance {
    CultureIdentity identity;
    std::u16string name;
    std::optional<std::size_t> number_index;
    std::shared_ptr<CultureNumberFormat> number_format;
    bool inherited=false;
    bool read_only=false;
    bool use_user_override=false;
};
// Host-translated managed throws. Unrelated C++ failures are not swallowed.
class CultureManagedError:public std::runtime_error {public:using std::runtime_error::runtime_error;};
// System.Exception subset; the current-culture factory catches System.Object,
// while ApplyTask.Init catches only System.Exception in its provision branch.
class CultureManagedException:public CultureManagedError {public:using CultureManagedError::CultureManagedError;};
struct CurrentCultureState {std::shared_ptr<CultureInstance> cached;};
// Decoded culture and integer-sign projection; calendar/other formatting fields are omitted.
std::shared_ptr<CultureInstance> make_invariant_culture(bool user_override,bool read_only,
    const std::shared_ptr<CultureNumberFormat>& invariant_format);
struct InvariantCultureState {
    std::shared_ptr<CultureNumberFormat> number_format=make_invariant_number_format();
    // CultureInfo .cctor constructs (127, false, true) once per managed domain.
    std::shared_ptr<CultureInstance> instance=make_invariant_culture(false,true,number_format);
};
class CurrentCultureHost {
public:
    virtual ~CurrentCultureHost() = default;
    virtual bool invariant_mode() = 0; // Shipped GlobalizationMode initializes false.
    virtual std::optional<std::u16string> native_locale_name() = 0;
    virtual std::shared_ptr<CultureInstance> create_specific(const std::u16string&) = 0;
    virtual std::shared_ptr<CultureInstance> invariant_culture() = 0;
};
std::shared_ptr<CultureInstance> construct_current_culture(CurrentCultureState&,CurrentCultureHost&);
class ThreadCultureHost {
public:
    virtual ~ThreadCultureHost() = default;
    // Live reads of Thread's thread-static +8 and CultureInfo static +0x20.
    // The latter includes the runtime's read barrier; setters are not modeled yet.
    virtual std::shared_ptr<CultureInstance> thread_culture() = 0;
    virtual std::shared_ptr<CultureInstance> default_thread_culture() = 0;
};
std::shared_ptr<CultureInstance> resolve_current_culture(CurrentCultureState&,
    ThreadCultureHost&,CurrentCultureHost&);
}
