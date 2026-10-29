#pragma once

#include "resource_catalog.hpp"
#include <initializer_list>
#include <optional>

namespace saga::core {

struct ResourceLookupState {
    CatalogText country; // Null triggers AssetReader._init; empty is initialized.
    std::int32_t dpi = 0;
    CatalogRows dpi_folders = std::make_shared<std::vector<CatalogText>>(
        std::initializer_list<CatalogText>{
            std::make_shared<const std::u16string>(u"drawable-ldpi"),
            std::make_shared<const std::u16string>(u"drawable-mdpi"),
            std::make_shared<const std::u16string>(u"drawable-hdpi"),
            std::make_shared<const std::u16string>(u"drawable-xhdpi")});
    CatalogRows language_pack_folders; // Language static +0xb8; null != empty.
};

class ResourceLookupHost {
public:
    virtual ~ResourceLookupHost() = default;
    virtual void initialize(ResourceLookupState&) = 0; // AssetReader._init.
    virtual std::int32_t extension_selector() = 0; // Live Config static +0.
    virtual bool use_resource_language() = 0;
    virtual CatalogText language_folder() = 0; // Language.GetFolder, always called.
};

// Config.ConvertExtension, 0x1bece1c. Literal ordinal replacement, not suffixes.
CatalogText convert_resource_extension(CatalogText, std::int32_t selector);
// AssetReader.GetAccessFilesList, 0x1c8fe9c. Order/duplicates/slashes retained.
CatalogRows resource_access_files(ResourceLookupState&, CatalogText, ResourceLookupHost&);

class ResourceDictionary {
public:
    virtual ~ResourceDictionary() = default;
    virtual std::optional<std::size_t> find(const std::u16string&) = 0;
};
struct JarResourceLookup {
    ResourceDictionary* dictionary = nullptr;
};
class JarResourceHost : public ResourceLookupHost {
public:
    // Public text/path behavior stays external, including current-culture lower
    // casing. This is not an ASCII substitute for production .NET semantics.
    virtual CatalogText trim(const std::u16string&) = 0;
    virtual CatalogText lower_case(const std::u16string&) = 0;
    virtual CatalogText convert_jar_extension(CatalogText) = 0;
};
// JarInflater._search, 0x1bfc468. Returns the host member index at first match.
std::optional<std::size_t> search_resource_member(JarResourceLookup&, ResourceLookupState&,
                                                CatalogText, JarResourceHost&);

} // namespace saga::core
