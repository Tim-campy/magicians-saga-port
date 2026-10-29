#include "resource_catalog.hpp"

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
