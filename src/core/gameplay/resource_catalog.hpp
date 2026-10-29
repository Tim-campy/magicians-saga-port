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

// LoadStart row parsing slice, 0x1ba6f04..0x1ba7008. ID defaults to -1;
// successful explicit IDs strip the prefix in place, ignoring further fields.
// The host parser here must implement JInteger.ParseInt for the supplied input.
std::vector<std::int32_t> parse_resource_catalog_rows(const CatalogRows&, CatalogTextHost&);
class ResourceNameHost : public CatalogTextHost {
public:
    // String.StartsWith(string) uses the current culture; the core must not
    // substitute an ordinal comparison for arbitrary Unicode strings.
    virtual bool starts_with(const std::u16string&, std::u16string_view) = 0;
};
// ResourceManager.GetFileName, 0x1ba7fcc; prefix check precedes public Trim.
CatalogText resource_file_name(CatalogText, ResourceNameHost&);
// Image.GetOptionFile, 0x1b91e48; comma split is untrimmed; absent dot faults.
CatalogText image_option_file(CatalogText);
struct SebLoadOptions {CatalogText filename;std::int32_t reverse_u=0,reverse_v=0,cache_size=0;};
// LoadStart SEB slice 0x1baaa10..0x1baaaa4: case-sensitive first characters;
// u/v set flags, c parses the remaining text, unknown first characters ignored.
SebLoadOptions seb_load_options(CatalogText,CatalogTextHost&);

} // namespace saga::core
