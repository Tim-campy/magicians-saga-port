#include "presentation_forms.hpp"
#include <utility>

namespace saga::core {
PreparedTalkForm prepare_talk_form(std::int32_t character_index,
                                 NullablePresentationText text,
                                 NullablePresentationText arguments,
                                 PresentationTextHost& host) {
    // Original overload skips Replace altogether when arguments is null.
    if (arguments) text = substitute_form_arguments(std::move(text), arguments, host);
    auto parts = split_form_text(text ? &*text : nullptr);
    // The original loads textAry_[0] for TextLayout.SetText. No fabricated page
    // is supplied for a missing first element. Layout side effects remain open.
    (void)parts.at(0);
    return {{0, 0, 0, 0}, character_index, std::move(parts), 0, -1, std::move(arguments)};
}
PreparedEventForm prepare_event_form(NullablePresentationText text,
                                    NullablePresentationText arguments,
                                    PresentationFormHost& host) {
    text = substitute_form_arguments(std::move(text), std::move(arguments), host);
    const auto parts = split_form_text(text ? &*text : nullptr);
    PreparedEventForm result{{4, 0, 0, 0}, std::vector<std::int32_t>(parts.size()),
                             std::vector<std::u16string>(parts.size()), {0, 0, 0, 0}};
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const auto fields = split_event_text_fields(&parts[i]);
        result.event_ids[i] = host.parse_event_type(fields.at(0));
        // The original parses first; only then checks whether field1 exists.
        result.texts[i] = fields.at(1);
        // Fields2+ are not consumed by this factory.
    }
    return result;
}
}
