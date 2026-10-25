#pragma once
#include <string>
#include <optional>
#include <vector>

namespace saga::core {
// StringUtil.Split14533/14535, used AFTER replacement in SubForm factories.
// Operates on UTF-16 code units like the original; null/empty input => no parts.
// A nonempty input retains empty segments, including after a final delimiter.
std::vector<std::u16string> split_form_text(const std::u16string* text);
// The inner MakeEvent split uses '&', without trim or escape processing.
std::vector<std::u16string> split_event_text_fields(const std::u16string* text);
// StringUtil.Split14533 with comma, used by the color parameter fallback.
std::vector<std::u16string> split_text_color_fields(const std::u16string* text);

using NullablePresentationText = std::optional<std::u16string>;
class PresentationTextHost {
public:
    virtual ~PresentationTextHost() = default;
    // Language.TranslateText(argument, false); no assumed identity translator.
    virtual NullablePresentationText translate_argument(const std::u16string& text) = 0;
    // Read after EACH translation, as StringUtil.SanSanRemove may change.
    virtual bool remove_duplicate_honorific() = 0;
};
// StringUtil.Replace14531/14532: value-level UTF-16 transformation, not managed
// string reference identity, decoding, or the Language translation service.
NullablePresentationText substitute_form_arguments(NullablePresentationText text,
                                                   NullablePresentationText args,
                                                   PresentationTextHost& host);
}
