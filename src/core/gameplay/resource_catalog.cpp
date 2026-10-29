#include "resource_catalog.hpp"
#include "presentation_text.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace saga::core {
namespace {
template<class T> T& required(T* value) {
    if (!value) throw std::invalid_argument("missing resource catalog input");
    return *value;
}

std::int32_t increment(std::int32_t value) {
    return value == std::numeric_limits<std::int32_t>::max()
        ? std::numeric_limits<std::int32_t>::min() : value + 1;
}

std::int32_t row_id(const std::u16string& row, std::int32_t current, CatalogTextHost& host) {
    const auto tab = row.find(u'\t');
    return tab == std::u16string::npos ? current
        : host.parse_id(std::u16string_view(row).substr(0, tab));
}
} // namespace

std::vector<std::int32_t> parse_resource_catalog_rows(const CatalogRows& rows, CatalogTextHost& host) {
    auto captured = rows;
    auto& values = required(captured.get());
    std::vector<std::int32_t> ids(values.size(), -1);
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto& original = required(values.at(i).get());
        const auto fields = split_trimmed_text_fields(&original, u'\t');
        if (fields.size() >= 2) {
            ids.at(i) = host.parse_id(fields[0]);
            values.at(i) = std::make_shared<const std::u16string>(fields[1]);
        }
    }
    return ids;
}

CatalogText resource_file_name(CatalogText name, ResourceNameHost& host) {
    const auto& original = required(name.get());
    if (host.starts_with(original, u"..\\") || host.starts_with(original, u"../")) {
        auto value = original;
        auto last = value.find_last_of(u'\\');
        if (last != value.npos) value = value.substr(last + 1);
        last = value.find_last_of(u'/');
        if (last != value.npos) value = value.substr(last + 1);
        return host.trim(value);
    }
    return host.trim(original);
}

CatalogText image_option_file(CatalogText name) {
    if (!name) return nullptr;
    auto value = name->substr(0, name->find(u','));
    const auto dot = value.find_last_of(u'.');
    if (dot == value.npos) throw std::out_of_range("image option filename has no dot");
    return std::make_shared<const std::u16string>(value.substr(0, dot) + u".opt");
}

SebLoadOptions seb_load_options(CatalogText spec,CatalogTextHost& host) {
    const auto fields=split_trimmed_text_fields(spec.get(),u',');
    SebLoadOptions result{std::make_shared<const std::u16string>(fields.at(0)),0,0,0};
    for(std::size_t i=1;i<fields.size();++i){
        const auto& option=fields[i];
        const auto first=option.at(0);
        if(first==u'u')result.reverse_u=1;
        else if(first==u'v')result.reverse_v=1;
        else if(first==u'c')result.cache_size=host.parse_id(std::u16string_view(option).substr(1));
    }
    return result;
}

void trim_resource_catalog(const CatalogRows& rows, CatalogTextHost& host) {
    auto captured = rows;
    auto& values = required(captured.get());
    for (auto& row : values) row = host.trim(required(row.get()));
}

void merge_resource_catalog(CatalogRows& selected, CatalogRows base,
                            CatalogTextHost& host, std::size_t max_mask_slots) {
    if (!base) return;
    if (max_mask_slots < 256) throw std::length_error("resource catalog mask budget exceeded");
    std::vector<std::uint8_t> occupied(256, 0);
    auto primary = selected;
    std::int32_t current = 0;
    for (const auto& row : required(primary.get())) {
        if (!row || row->empty()) continue;
        current = row_id(*row, current, host);
        if (current >= static_cast<std::int64_t>(occupied.size())) {
            const auto length = increment(current);
            if (length < 0 || static_cast<std::size_t>(length) > max_mask_slots)
                throw std::length_error("resource catalog mask budget exceeded");
            occupied.resize(static_cast<std::size_t>(length), 0);
        }
        occupied.at(static_cast<std::size_t>(current)) = 1;
        current = increment(current);
    }

    current = 0;
    for (const auto& row : *base) {
        if (!row) continue;
        const auto trimmed = host.trim(*row);
        if (!trimmed || trimmed->empty()) continue;
        // The native call searches a second Trim result, but takes Substring
        // from the first one. Keep that order without rebuilding String.Trim.
        const auto searched = host.trim(*trimmed);
        const auto tab = required(searched.get()).find(u'\t');
        if (tab != std::u16string::npos)
            current = host.parse_id(std::u16string_view(*trimmed).substr(0, tab));
        const bool present = current < static_cast<std::int64_t>(occupied.size())
            && occupied.at(static_cast<std::size_t>(current)) != 0;
        if (!present) {
            auto appended = std::make_shared<std::vector<CatalogText>>(required(selected.get()));
            appended->push_back(trimmed);
            selected = std::move(appended);
            // Deliberately do not mark occupied: base duplicates are retained.
        }
        current = increment(current);
    }
}

} // namespace saga::core
