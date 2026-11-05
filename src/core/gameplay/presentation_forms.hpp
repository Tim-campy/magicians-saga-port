#pragma once
#include "presentation_text.hpp"
#include "event_catalog.hpp"
#include <array>
#include <cstdint>
#include <memory>

namespace saga::core {
using PresentationStringDecoder = std::function<std::u16string(std::string_view)>;
struct EventMessageDefinition {
    std::int32_t id=-1,type=0;
    std::u16string text;
};
using EventMessageTable = std::vector<std::optional<EventMessageDefinition>>;
// EventMsgData.Load: id and type stores precede decoding/assigning text.
EventMessageDefinition& load_event_message_definition(EventMessageDefinition&,
    std::optional<std::string_view>,const EventIntegerParser&,const PresentationStringDecoder&);
EventMessageTable load_event_message_table(std::string_view,bool normalize_crlf,
    const EventIntegerParser&,const PresentationStringDecoder&);
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
class EventNoticeHost {
public:
    virtual ~EventNoticeHost()=default;
    virtual bool debug_command_enabled()=0; // Config static byte+0xbb, read afresh.
    virtual bool held_advance()=0; // Canvas held mask0x40.
    virtual void frame_resource()=0;
    virtual bool advance_pulse()=0; // Canvas pulse mask0x100000.
    virtual void set_event(std::int32_t page)=0;
    virtual bool cancel_pressed()=0; // AppData key3.
};
// Selected SubForm.Update type4 branch only. Draw owns animation-frame advance.
void update_event_notice(const PreparedEventForm&,std::int32_t& page,
    std::int32_t& animation_frame,std::int32_t& close_counter,EventNoticeHost&);
// SubForm type19 Init supplies step1; the managed production counter starts0.
struct PopupProductionState {
    std::int32_t step=1;
    std::int32_t frame=0;
};
class PopupContentsHost {
public:
    virtual ~PopupContentsHost()=default;
    virtual bool popup_close_pulse()=0; // Canvas pulse mask0x02000000.
    virtual void pop_popup()=0; // FormBase.Pop; its return value is ignored.
};
// SubForm.UpdateWindowProduction 0x11fa02c. True for entry step1/2, false otherwise.
bool update_popup_production(PopupProductionState&,PopupContentsHost&);
// SubForm._update type19, after the enclosing Update guards. False for entry
// step1/2, true otherwise; this return is not a request to remove the form.
bool update_popup_contents(std::int32_t& animation_frame,PopupProductionState&,PopupContentsHost&);
struct KingHallState {
    std::int32_t production_step=0;
    std::int32_t frame=0;
    std::int32_t movement_frame=0;
    std::int32_t entrance_x=0;
    std::int32_t selection=0;
    NullablePresentationText speech;
};
class KingHallHost {
public:
    virtual ~KingHallHost()=default;
    // Data.voiceData.FirstOrDefault(type==2), required, then GetRandomText.
    virtual NullablePresentationText king_hall_voice()=0;
    virtual void king_hall_initialize_news(std::int32_t interval)=0;
    virtual std::int32_t king_hall_easing(std::int32_t frame)=0; // Graph.Easing(-40,80,40,frame,0).
    virtual void execute_king_rewards()=0; // AppData.CheckExeKingEvent(true).
    virtual bool king_hall_confirm_pulse()=0; // Canvas pulse0x100000.
    virtual void king_hall_play_sound()=0;
    virtual void king_hall_save_game()=0; // SaveRecorder.SaveGame(0,true).
    virtual void king_hall_save_system()=0;
    virtual bool king_hall_soft_label(std::int32_t)=0;
    virtual void king_hall_push_notes()=0; // Fresh SubForm(5), FormBase.Push.
    virtual void king_hall_push_tapjoy()=0; // MakeTapJoy(0), FormBase.Push.
    virtual void king_hall_pop()=0;
    virtual void king_hall_update_news()=0;
};
// SubForm.Init type15 block only, after common Init has reset frame to0.
// The other counters/selection survive reinitialization; fresh allocation supplies0.
void initialize_king_hall(KingHallState&,KingHallHost&);
// SubForm._update type15 after the enclosing Update guards; returns true.
bool update_king_hall(KingHallState&,KingHallHost&);
struct BuildListState {
    std::int32_t selected=0,town_slot=0,production_step=0,frame=0;
    std::shared_ptr<std::vector<std::int32_t>> entries;
};
class BuildListHost {
public:
    virtual ~BuildListHost()=default;
    // Fill the already-published list using the native catalogue-order eligibility
    // kernel: nonnull, !FLAG_DISABLE_BUILDING, have>=1, stop after six IDs.
    virtual void fill_build_list(std::vector<std::int32_t>&)=0;
    virtual bool build_key_pulse(std::int32_t mask)=0;
    virtual std::int32_t build_price(std::int32_t building)=0;
    virtual bool check_build_crystals(std::int64_t price)=0;
    virtual void subtract_build_crystals(std::int32_t price)=0;
    virtual void install_building(std::int32_t town_slot,std::int32_t building)=0;
    virtual void build_sound()=0;
    virtual void push_building_complete(std::int32_t building)=0;
    virtual std::int32_t& construction_stock(std::int32_t building)=0;
    virtual void pop_build_list()=0; // Pop this parent identity, even after pushing its child.
    virtual bool build_back_pressed()=0; // AppData softlabel3.
    virtual void push_build_tapjoy()=0; // MakeTapJoy(0), FormBase.Push.
};
// Reached SubForm.Init type12 block, after common Init's frame reset.
void initialize_build_list(BuildListState&,BuildListHost&);
// SubForm._update type12: production does not suppress input; insufficient
// crystals produce no dialog and still reach optional-action/back handling.
bool update_build_list(BuildListState&,BuildListHost&);
struct BuildingCompleteState {
    std::int32_t building_id=0,production_step=0,frame=0;
};
class BuildingCompleteHost {
public:
    virtual ~BuildingCompleteHost()=default;
    virtual void completion_sound()=0;
    virtual bool completion_close_pulse()=0; // Canvas pulse0x02000000.
    virtual void pop_building_complete()=0;
};
// SubForm.Init type17 sets only production_step; common Init owns frame0.
void initialize_building_complete(BuildingCompleteState&);
bool update_building_complete(BuildingCompleteState&,BuildingCompleteHost&);
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
