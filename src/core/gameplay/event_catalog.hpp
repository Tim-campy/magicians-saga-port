#pragma once
#include "event_conditions.hpp"
#include <functional>
#include <string_view>

namespace saga::core {
using EventCatalog = ConditionCatalog<EventConditionRecord>;
using EventIntegerParser = std::function<std::int32_t(std::string_view)>;
// Input is decoded UTF-8 text. Invalid UTF-8 policy and culture-sensitive
// Int32.Parse belong to the caller. CRLF normalization is an explicit version gate.
EventCatalog load_event_catalog(std::string_view text, bool normalize_crlf,
                                const EventIntegerParser& integer);
// Observed table subset only: ASCII decimal, optional sign and ASCII whitespace.
std::int32_t parse_event_decimal(std::string_view token);
// AppData.GetDelayId: fallback must perform the original +0x1e8 scan/diagnostic
// or fail explicitly when that context is unavailable; result remains -1.
std::int32_t identify_event_script(const EventCatalog&, const EventScript&,
                                 const std::function<void()>& fallback);
EventScript resolve_event_script(const EventCatalog&, std::int32_t id);
}
