#pragma once
#include "presentation_text.hpp"
#include <array>
#include <cstdint>

namespace saga::core {
struct InitialFormSelection {
    std::int32_t type;
    std::int32_t x;
    std::int32_t y;
    std::int32_t selection;
};
// Prepared data from SubForm.MakeTalkForm36177, not a renderer/TextLayout clone.
struct PreparedTalkForm {
    InitialFormSelection initial;
    std::int32_t character_index;
    std::vector<std::u16string> texts;
    std::int32_t text_index;
    std::int32_t next_index;
    NullablePresentationText arguments;
};
struct PreparedEventForm {
    InitialFormSelection initial;
    std::vector<std::int32_t> event_ids;
    std::vector<std::u16string> texts;
    std::array<std::int32_t, 4> animation;
};
class PresentationFormHost : public PresentationTextHost {
public:
    // Boundary for JInteger.ParseInt/System.Int32.Parse. No guessed culture or
    // replacement of malformed/empty values by zero inside the preparation.
    virtual std::int32_t parse_event_type(const std::u16string& text) = 0;
};
PreparedTalkForm prepare_talk_form(std::int32_t character_index,
                                 NullablePresentationText text,
                                 NullablePresentationText arguments,
                                 PresentationTextHost& host);
// Input is the encoded text passed to MakeEvent36179, after the outer overload's
// type + '&' + text construction. Null/empty input can yield an empty form.
PreparedEventForm prepare_event_form(NullablePresentationText text,
                                    NullablePresentationText arguments,
                                    PresentationFormHost& host);
}
