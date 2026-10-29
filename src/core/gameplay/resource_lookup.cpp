#include "resource_lookup.hpp"
#include "presentation_text.hpp"

#include <stdexcept>
#include <utility>

namespace saga::core {
namespace {
template<class T> T& required(T* value) {
    if (!value) throw std::invalid_argument("missing resource lookup input");
    return *value;
}
const std::u16string& value(const CatalogText& text) {
    static const std::u16string empty;
    return text ? *text : empty; // String.Concat treats null as empty.
}
CatalogText text(std::u16string s) { return std::make_shared<const std::u16string>(std::move(s)); }
void replace_all(std::u16string& s, std::u16string_view from, std::u16string_view to) {
    std::size_t start = 0;
    while ((start = s.find(from, start)) != s.npos) {
        s.replace(start, from.size(), to);
        start += to.size();
    }
}
} // namespace

CatalogText convert_resource_extension(CatalogText name, std::int32_t selector) {
    if (!name || (selector != 2 && selector != 3 && selector != 4)) return name;
    auto result = *name;
    replace_all(result, u".gif", u".png");
    replace_all(result, u".mld", u".ogg");
    return result == *name ? name : text(std::move(result));
}

CatalogRows resource_access_files(ResourceLookupState& state, CatalogText name, ResourceLookupHost& host) {
    if (!state.country) host.initialize(state);
    name = convert_resource_extension(std::move(name), host.extension_selector());
    const auto parts = split_trimmed_text_fields(name.get(), u'/');
    const auto& filename = parts.at(parts.size() - 1); // Empty split faults, not empty candidates.
    std::u16string directory;
    for (std::size_t i = 0; i + 1 < parts.size(); ++i) directory += parts[i] + u"/";

    std::vector<CatalogText> folders;
    if (state.language_pack_folders && host.use_resource_language()) {
        const auto packs = state.language_pack_folders; // Reread after the getter.
        for (const auto& pack : required(packs.get())) folders.push_back(text(value(pack) + u"/"));
    }
    folders.push_back(text(value(host.language_folder()) + u"/"));
    folders.push_back(text(u""));

    auto result = std::make_shared<std::vector<CatalogText>>();
    // Native captures the starting int32 but reloads the folder array for each
    // bounds/index operation. Keep invalid indices and the end index distinct.
    for (std::int64_t i = state.dpi; i < static_cast<std::int64_t>(required(state.dpi_folders.get()).size()); ++i) {
        for (const auto& folder : folders) {
            const auto prefix = required(state.dpi_folders.get()).at(static_cast<std::size_t>(i));
            result->push_back(text(directory + value(prefix) + u"/" + value(folder) + filename));
        }
    }
    for (std::int64_t i = static_cast<std::int64_t>(state.dpi) - 1; i >= 0; --i) {
        for (const auto& folder : folders) {
            const auto prefix = required(state.dpi_folders.get()).at(static_cast<std::size_t>(i));
            result->push_back(text(directory + value(prefix) + u"/" + value(folder) + filename));
        }
    }
    for (const auto& folder : folders) result->push_back(text(directory + value(folder) + filename));
    return result;
}

std::optional<std::size_t> search_resource_member(JarResourceLookup& jar, ResourceLookupState& state,
                                                CatalogText name, JarResourceHost& host) {
    if (!name || !jar.dictionary) return std::nullopt;
    name = host.trim(*name);
    name = host.convert_jar_extension(std::move(name));
    name = host.lower_case(required(name.get()));
    const auto candidates = resource_access_files(state, std::move(name), host);
    for (const auto& candidate : *candidates) {
        const auto& original = required(candidate.get());
        auto* dictionary = jar.dictionary; // Captured before lower-case call.
        const auto query = host.lower_case(original);
        const auto found = required(dictionary).find(required(query.get()));
        if (found) return found;
    }
    return std::nullopt;
}

} // namespace saga::core
