#pragma once
#include "resource_archive.hpp"
#include "mission_text_layout.hpp"
#include "core/gameplay/event_dispatch.hpp"
#include "core/gameplay/event_execution.hpp"
#include "core/gameplay/form_push.hpp"
#include "core/gameplay/presentation_forms.hpp"
#include "core/gameplay/text_reveal.hpp"
#include <map>
#include <ostream>

namespace saga::desktop {
struct MissionEventScenario {
    core::NullablePresentationText save_string0;
    bool short_words=false,held_advance=false,print=false;
    bool new_game_context=false;
    // Provisional TextLayout provider outputs, not raw/source text lengths.
    std::map<std::int32_t,std::vector<std::int32_t>> page_lengths;
    std::optional<MissionFontScenario> font;
};
std::u16string decode_mission_text(std::string_view);
class KingHallServices {
public:
    virtual ~KingHallServices()=default;
    virtual core::NullablePresentationText king_voice()=0;
    virtual void king_grant()=0;
    virtual void king_sound()=0;
    virtual void king_save_game()=0;
    virtual void king_save_system()=0;
};
class MissionEvents final:private core::EventEffectsHost,private core::EventFlowHost,
    private core::PresentationFormHost,private core::FormStackHost,private core::TextRevealClock,private core::TalkPageHost,private core::EventNoticeHost,private core::PopupContentsHost,private core::KingHallHost {
public:
    MissionEvents(ResourceArchive&,MissionEventScenario,std::function<void()> clear_input);
    bool execute(std::int32_t);
    core::EventObject make_king_hall(KingHallServices&);
    bool request_king_back();
    core::EventObject execute_unit(const core::EventInstruction& row,const core::EventContext& context){return dispatch(row,context);}
    void push_script_result(core::EventObject form){push_form(std::move(form));}
    core::FormText format_popup_value(std::string_view,std::int64_t);
    core::EventObject make_popup_contents(std::int32_t,core::FormText,core::FormText,bool);
    core::EventDelayQueue& delays(){return delays_;}
    const core::EventDelayQueue& delays()const{return delays_;}
    core::DecodedEventExecutionHost& executor(){return executor_;}
    core::FormPushState form_owner()const{return root_;}
    // The application dispatches Push through the current form's virtual method.
    // Town supplies its SelectForm flag writes; mission retains its existing base Push.
    void bind_application_push(std::function<void(core::EventObject)> callback){application_push_=std::move(callback);}
    void bind_normal_save(core::EventInstruction values){normal_save_=std::move(values);}
    void bind_numeric_effect(std::function<void(const core::EventIntegers&)> callback){numeric_effect_=std::move(callback);}
    void bind_clear_input(std::function<void()> callback){clear_input_=std::move(callback);}
    bool pending()const{return !forms_.empty();}
    // True when the selected modal consumes this dispatch; false permits the underlying Quest update.
    bool advance(std::uint64_t frame,std::int32_t tick_ms,bool pulse,bool held);
    void write_report(std::ostream&)const;
    std::string title()const;
    std::shared_ptr<DesktopTextFont> font_provider()const{return font_provider_;}
    const std::optional<MissionFontScenario>& font_scenario()const{return scenario_.font;}
private:
    friend struct MissionCheckpoint;
    struct PopupContents {
        core::PopupProductionState production;
        std::int32_t kind=3;
        bool option=false;
        std::string title,text;
    };
    struct KingHall {
        core::KingHallState state;
        KingHallServices* services=nullptr;
        std::uint64_t news_updates=0;
    };
    struct Talk {
        std::int32_t id=0,state=0,frame=0,finish_frame=-1;
        core::PreparedTalkForm prepared;
        std::optional<core::PreparedEventForm> event_notice;
        std::optional<PopupContents> popup;
        std::optional<KingHall> king;
        std::int32_t event_page=0,event_frame=0;
        bool event_resource_ready=false;
        core::TextRevealState reveal{};
        std::unique_ptr<MissionTextLayout> layout;
    };
    ResourceArchive& archive_; // Source fingerprint is owned by the profile; decoded message cache is immutable.
    std::optional<core::EventMessageTable> event_messages_;
    MissionEventScenario scenario_;
    core::TalkTable talks_;
    core::DecodedEventExecutionState execution_;
    core::EventDelayQueue delays_;
    core::DecodedEventExecutionHost executor_;
    std::vector<std::shared_ptr<Talk>> forms_;
    std::shared_ptr<Talk> creating_,updating_;
    core::FormPushState root_;
    std::uint64_t ticks_=0,opened_=0,closed_=0,flushes_=0,pages_=0;
    std::int64_t now_=0;
    std::int32_t total_frames_=0;
    bool pulse_=false,held_=false;
    bool king_back_requested_=false,king_back_pulse_=false;
    std::vector<std::int32_t> executed_;
    std::vector<std::string> lifecycle_;
    std::function<void()> clear_input_;
    std::function<void(core::EventObject)> application_push_;
    core::EventInstruction normal_save_;
    std::function<void(const core::EventIntegers&)> numeric_effect_;
    std::shared_ptr<DesktopTextFont> font_provider_;
    std::uint64_t layout_measurements_=0;
    struct LayoutSample {std::int32_t talk,page,length,height;std::size_t blocks;};
    std::vector<LayoutSample> layout_samples_;
    core::EventProgression& progression()override;
    std::int32_t stamina_limit()override;
    std::int64_t read_stamina()override;
    void write_stamina(std::int64_t)override;
    core::EventObject make_talk_form(std::int32_t,core::EventObject)override;
    core::EventObject make_event_form(std::int32_t,core::EventObject)override;
    std::int32_t parse_event_type(const std::u16string&)override;
    bool debug_command_enabled()override{return scenario_.held_advance;}
    bool held_advance()override{return held_;}
    void frame_resource()override{} // SAGA_STUB(event-notice-presentation): native common Seb.Frame has cosmetic effects only.
    void set_event(std::int32_t)override;
    bool cancel_pressed()override{return false;} // SAGA_STUB(event-notice-touch): Enter/pulse is bound; softlabel3 hit routing remains unbound.
    core::NullablePresentationText king_hall_voice()override;
    void king_hall_initialize_news(std::int32_t)override;
    std::int32_t king_hall_easing(std::int32_t)override;
    void execute_king_rewards()override;
    bool king_hall_confirm_pulse()override{return pulse_;}
    void king_hall_play_sound()override;
    void king_hall_save_game()override;
    void king_hall_save_system()override;
    bool king_hall_soft_label(std::int32_t id)override{return id==3&&king_back_pulse_;}
    void king_hall_push_notes()override;
    void king_hall_push_tapjoy()override;
    void king_hall_pop()override;
    void king_hall_update_news()override;
    void draw_event_notice_semantics(Talk&);
    std::u16string translated_popup_literal(std::string_view);
    bool popup_close_pulse()override{return pulse_;} // Desktop Enter/replay pulse maps to native mask0x02000000 for type19.
    void pop_popup()override;
    void push_form(core::EventObject)override;
    void push(core::EventObject,core::EventObject,bool)override;
    core::EventObject dispatch(const core::EventInstruction&,const core::EventContext&)override;
    std::int32_t identify_script(const core::EventScript&)override;
    core::EventScript resolve_script(std::int32_t)override;
    std::int32_t read_talk_character()override;
    void restore_talk_character(std::size_t,std::int32_t)override;
    core::NullablePresentationText translate_argument(const std::u16string&)override;
    bool remove_duplicate_honorific()override{return false;}
    std::int64_t current_time_millis()override{return now_;}
    std::int32_t total_frames()override{return total_frames_;}
    void text_frame()override;
    void portrait_frame()override; // Presentation-only boundary.
    bool advance_pulse()override{return pulse_;}
    bool held_advance_enabled()override{return scenario_.held_advance;}
    bool advance_held()override{return held_;}
    bool text_ended()override;
    void flush_text()override;
    void set_text(const std::u16string&)override;
    void continue_talk(core::PreparedTalkForm&)override;
    void set_page(Talk&);
    bool finish_talk(Talk&);
};
}
