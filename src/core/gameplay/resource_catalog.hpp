#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace saga::core {

using CatalogText = std::shared_ptr<const std::u16string>;
using CatalogRows = std::shared_ptr<std::vector<CatalogText>>;

// Existing public text/numeric implementations supply these operations. No
// culture, Unicode classification or runtime parsing implementation is rebuilt.
class CatalogTextHost {
public:
    virtual ~CatalogTextHost() = default;
    virtual CatalogText trim(const std::u16string&) = 0;
    virtual std::int32_t parse_id(std::u16string_view) = 0; // System.Int32.Parse.
};

// ResourceManager.LoadReady's selected-array normalization. The outer loading,
// Language.UseResourceLanguage gate, exception logging and totals are external.
void trim_resource_catalog(const CatalogRows&, CatalogTextHost&);

// LoadReady's fallback merge, 0x1ba6794..0x1ba6a2c. selected was normalized above.
// Each accepted row publishes a new array; aliases keep the old array. base is
// captured by value, including when it initially aliases selected.
// max_mask_slots is a preparation memory policy, not a limit in the original.
void merge_resource_catalog(CatalogRows& selected, CatalogRows base,
                            CatalogTextHost&, std::size_t max_mask_slots = 1048576);

} // namespace saga::core
