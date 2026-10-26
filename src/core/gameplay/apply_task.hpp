#pragma once
#include "select_notifications.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>
namespace saga::core {
struct ApplyCatalogItem {
    std::optional<std::u16string> name,provision_type,provision_text;
    std::int32_t provision_value;
};
using ApplyCatalog=std::vector<std::shared_ptr<ApplyCatalogItem>>;
class ApplyManagedError:public std::runtime_error {public:using std::runtime_error::runtime_error;};
enum class ApplyNumberFailure { Format,Overflow };
class ApplyNumberError final:public ApplyManagedError {
public:
    explicit ApplyNumberError(ApplyNumberFailure value):ApplyManagedError("invalid Int32 input"),kind(value) {}
    ApplyNumberFailure kind;
};
struct ApplyNumberSigns {std::u16string positive,negative;};
// Non-null UTF-16 input, NumberStyles.Integer (7), supplied current-culture signs.
std::int32_t parse_apply_integer(const std::u16string&,const ApplyNumberSigns&);
struct ApplyGiftState {
    std::u16string gift,product_id,provision_type,provision_text;
    std::optional<std::u16string> name;
    bool notice,locked;
    std::int32_t type,provision_value,talk_character;
    std::optional<std::u16string> talk_text;
};
class ApplyGiftHost {
public:
    virtual ~ApplyGiftHost() = default;
    virtual ApplyNumberSigns current_number_signs() = 0;
    virtual bool starts_special_gift(const std::u16string&) = 0; // Original StartsWith("@") semantics.
    virtual std::shared_ptr<ApplyCatalog> catalog() = 0;
};
// ApplyTask.Init with null item; the string constructor supplies type=0, notice=true.
// Caller retains item identity separately. Existing talk fields survive non-TAB routes.
void initialize_apply_gift(ApplyGiftState&,std::optional<std::u16string> gift,std::int32_t type,bool notice,ApplyGiftHost&);
std::shared_ptr<NotificationApplyTask> make_notification_gift(NotificationText,ApplyGiftHost&);
// ApplyTask.Init name-resolution branch 0x11bb5a4..0x11bb71c.
// Returns the selected record; null means retain the existing empty task name.
// It neither applies an item nor establishes payment/receipt validity.
std::shared_ptr<ApplyCatalogItem> resolve_apply_name_item(const std::shared_ptr<ApplyCatalog>&,
    const std::u16string& provision_type,const std::u16string& provision_text,std::int32_t provision_value);
}
