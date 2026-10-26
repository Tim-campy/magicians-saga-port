#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace saga::core {
struct ApplyCatalogItem {
    std::optional<std::u16string> name,provision_type,provision_text;
    std::int32_t provision_value;
};
using ApplyCatalog=std::vector<std::shared_ptr<ApplyCatalogItem>>;
// ApplyTask.Init name-resolution branch 0x11bb5a4..0x11bb71c.
// Returns the selected record; null means retain the existing empty task name.
// It neither applies an item nor establishes payment/receipt validity.
std::shared_ptr<ApplyCatalogItem> resolve_apply_name_item(const std::shared_ptr<ApplyCatalog>&,
    const std::u16string& provision_type,const std::u16string& provision_text,std::int32_t provision_value);
}
