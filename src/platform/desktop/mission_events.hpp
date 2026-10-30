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
class MissionEvents final:private core::EventEffectsHost,private core::EventFlowHost,
    private core::PresentationTextHost,private core::FormStackHost,private core::TextRevealClock,private core::TalkPageHost {
public:
    MissionEvents(ResourceArchive&,MissionEventScenario,std::function<void()> clear_input);
    bool execute(std::int32_t);
    bool pending()const{return !forms_.empty();}
    // True when the selected modal consumes this dispatch; false permits the underlying Quest update.
    bool advance(std::uint64_t frame,std::int32_t tick_ms,bool pulse,bool held);
    void write_report(std::ostream&)const;
    std::string title()const;
private:
    struct Talk {
        std::int32_t id=0,state=0,frame=0,finish_frame=-1;
        core::PreparedTalkForm prepared;
        core::TextRevealState reveal{};
        std::unique_ptr<MissionTextLayout> layout;
    };
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
    std::vector<std::int32_t> executed_;
    std::vector<std::string> lifecycle_;
    std::function<void()> clear_input_;
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
