#pragma once
#include "presentation_text.hpp"
#include "event_catalog.hpp"
#include <array>
#include <cstdint>
#include <memory>

namespace saga::core {
using PresentationStringDecoder = std::function<std::u16string(std::string_view)>;
struct TalkDefinition {
    std::int32_t id=-1,character_index=0;
    std::shared_ptr<std::vector<std::u16string>> texts;
};
using TalkTable = std::vector<std::optional<TalkDefinition>>;
// TalkData.Load: immediate scalar stores; the counted array is assigned only
// after every string has been read. Decoding policy belongs to the caller.
TalkDefinition& load_talk_definition(TalkDefinition&,std::optional<std::string_view>,
                                    const EventIntegerParser&,const PresentationStringDecoder&);
TalkTable load_talk_table(std::string_view,bool normalize_crlf,
                         const EventIntegerParser&,const PresentationStringDecoder&);
// BootForm's talk delegate joins the array with TAB. Null arrays fail rather
// than being replaced by an empty page; empty fragments remain distinct.
std::u16string join_talk_text(const TalkDefinition&);
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
class TalkPageHost {
public:
    virtual ~TalkPageHost()=default;
    virtual void text_frame()=0;
    virtual void portrait_frame()=0;
    virtual bool advance_pulse()=0; // Canvas pulse mask0x100000.
    virtual bool held_advance_enabled()=0; // Config static byte+0xbb.
    virtual bool advance_held()=0; // Canvas held mask0x40.
    virtual bool text_ended()=0;
    virtual void flush_text()=0;
    virtual void set_text(const std::u16string&)=0;
    virtual void continue_talk(PreparedTalkForm&)=0; // Explicit non-(-1) nextIndex boundary.
};
// SubForm.Update type0 only, after the caller's opening/closing guards.
void update_talk_page(PreparedTalkForm&,std::int32_t& close_counter,TalkPageHost&);
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
