#pragma once
#include <string>
#include <vector>

namespace saga::core {
// StringUtil.Split14533/14535, used AFTER replacement in SubForm factories.
// Operates on UTF-16 code units like the original; null/empty input => no parts.
// A nonempty input retains empty segments, including after a final delimiter.
std::vector<std::u16string> split_form_text(const std::u16string* text);
// The inner MakeEvent split uses '&', without trim or escape processing.
std::vector<std::u16string> split_event_text_fields(const std::u16string* text);
}
