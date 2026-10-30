#ifdef _MSC_VER
// This portable desktop adapter deliberately targets the C++17 conversion API.
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif
#include "mission_events.hpp"
#include "core/gameplay/select_event_pass.hpp"
#include <codecvt>
#include <locale>
#include <limits>
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
[[noreturn]]void unbound(const char* name){throw std::logic_error(std::string("mission event service unbound: ")+name);}
}
std::u16string decode_mission_text(std::string_view text){
    // C++17's public UTF-8/UTF-16 converter; no reconstruction of Mono Unicode internals.
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> converter;
    return converter.from_bytes(text.data(),text.data()+text.size());
}
MissionEvents::MissionEvents(ResourceArchive& archive,MissionEventScenario scenario,std::function<void()> clear_input)
    :scenario_(std::move(scenario)),executor_(execution_,*this),root_{std::make_shared<int>(0),this,{}},clear_input_(std::move(clear_input)){
    auto read=[&](const char* name){auto data=archive.data(name);if(!data)throw std::invalid_argument(std::string("missing mission event table: ")+name);return std::string(data->bytes.begin(),data->bytes.end());};
    execution_.catalog=std::make_shared<EventCatalog>(load_event_catalog(read("Event.txt"),false,parse_event_decimal));
    talks_=load_talk_table(read("Talk.txt"),false,parse_event_decimal,decode_mission_text);
    if(scenario_.font)font_provider_=std::make_shared<DesktopTextFont>(scenario_.font->path);
    execution_.normal_save_present=true;
    execution_.normal_save_strings=std::make_shared<std::vector<EventObject>>(1);
    if(scenario_.save_string0)execution_.normal_save_strings->at(0)=std::make_shared<std::u16string>(*scenario_.save_string0);
    execution_.print_enabled=scenario_.print;
    execution_.diagnostic=[](auto,std::int32_t){ /* SAGA_STUB(mission-event-diagnostic): Config.PRINT logging only. */ };
}
bool MissionEvents::execute(std::int32_t id){
    const auto result=execute_indexed_event(id,{},delays_,executor_);executed_.push_back(id);return static_cast<bool>(result);
}
bool MissionEvents::advance(std::uint64_t frame,std::int32_t tick_ms,bool pulse,bool held){
    if(forms_.empty())return false;
    now_=static_cast<std::int64_t>(frame)*tick_ms;total_frames_=static_cast<std::int32_t>(frame);pulse_=pulse;held_=held;
    // Same-thread, synchronous manager projection: process pending Init before selecting the top.
    for(auto& form:forms_)if(form->state==0){
        // SAGA_STUB(mission-talk-common-ui): non-gameplay Init providers, softlabels, blur and resource release.
        form->state=1;clear_input_();pulse_=held_=false;
        lifecycle_.push_back("init:"+std::to_string(form->id));
    }
    while(!forms_.empty()){
        updating_=forms_.back();auto& talk=*updating_;
        if(talk.state==1){clear_input_();pulse_=held_=false;talk.state=2;lifecycle_.push_back("resume:"+std::to_string(talk.id));} // Manager.Resume, no form Resume callback.
        ++ticks_;
        if(talk.finish_frame>=0){
            --talk.finish_frame;
            if(talk.finish_frame<=0){
                talk.state=4; // Pop callback marks4; it does not erase the form.
                lifecycle_.push_back("pop:"+std::to_string(talk.id));
                // SubForm.Finish's fresh null-delay/null-share path returns true.
                // The manager removes only after that Finish and clears Canvas input.
                if(!finish_talk(talk)||talk.state!=4)return true;
                lifecycle_.push_back("remove:"+std::to_string(talk.id));
                forms_.pop_back();++closed_;updating_.reset();clear_input_();pulse_=held_=false;
                if(!forms_.empty()&&forms_.back()->state==3)forms_.back()->state=1;
                continue; // No manager wait: select the underlying form in this same dispatch.
            }
        }else{
            talk.frame=advance_select_loop_counter(talk.frame);
            // SubForm.configData[0][3] is2; IsDrawWindowEnd permits its branch after5.
            if(talk.frame>5)update_talk_page(talk.prepared,talk.finish_frame,*this);
        }
        talk.state=3;updating_.reset();return true;
    }
    return false;
}
EventObject MissionEvents::dispatch(const EventInstruction& row,const EventContext& context){
    const auto result=dispatch_known_event_instruction(row,context,*this);
    if(!result.handled)unbound("effect opcode");return result.value;
}
EventObject MissionEvents::make_talk_form(std::int32_t id,EventObject args){
    const auto& record=talks_.at(static_cast<std::size_t>(id));if(!record)throw std::invalid_argument("null talk record");
    const auto arguments=std::static_pointer_cast<std::u16string>(args);
    auto talk=std::make_shared<Talk>();talk->id=id;
    talk->prepared=prepare_talk_form(record->character_index,join_talk_text(*record),arguments?NullablePresentationText(*arguments):std::nullopt,*this);
    talk->reveal.speed=configured_reveal_speed(1500,scenario_.short_words);
    set_page(*talk);creating_=talk;return talk;
}
void MissionEvents::push_form(EventObject form){push_base_form(root_,std::move(form));}
void MissionEvents::push(EventObject parent,EventObject child,bool option){
    if(parent!=root_.identity||option||child!=creating_)throw std::invalid_argument("unsupported mission form push");
    // RunOnUiThread's same-UI-thread path invokes Push's callback synchronously.
    // The callback inserts state0; Init/Finish are owned by the next manager dispatch.
    forms_.push_back(creating_);lifecycle_.push_back("push:"+std::to_string(creating_->id));creating_.reset();++opened_;
}
bool MissionEvents::finish_talk(Talk& talk){
    // SubForm.Finish 0x120488c: this fresh factory owns no delay/share continuation.
    // SAGA_STUB(mission-talk-common-ui): base UI/resource cleanup and parent release hook.
    lifecycle_.push_back("finish:"+std::to_string(talk.id));return true;
}
void MissionEvents::set_page(Talk& talk){
    if(scenario_.font){
        if(!talk.layout)talk.layout=std::make_unique<MissionTextLayout>(*scenario_.font,font_provider_);
        restart_text_reveal_for_new_text(talk.reveal); // SetText resets timing before font/block preparation.
        const auto before=talk.layout->measurements();
        talk.layout->set_text(talk.prepared.texts.at(static_cast<std::size_t>(talk.prepared.text_index)));
        layout_measurements_+=talk.layout->measurements()-before;
        talk.reveal.length=talk.layout->length();
        layout_samples_.push_back({talk.id,talk.prepared.text_index,talk.reveal.length,talk.layout->height(),talk.layout->blocks().blocks.size()});
        return;
    }
    const auto lengths=scenario_.page_lengths.find(talk.id);
    if(lengths==scenario_.page_lengths.end()||lengths->second.size()!=talk.prepared.texts.size())
        throw std::invalid_argument("talk_layout requires one explicit provider length per prepared page for talk "+std::to_string(talk.id));
    talk.reveal.length=lengths->second.at(static_cast<std::size_t>(talk.prepared.text_index));
    restart_text_reveal_for_new_text(talk.reveal);
}
void MissionEvents::text_frame(){advance_text_reveal_timing(updating_->reveal,*this);}
void MissionEvents::portrait_frame(){ /* SAGA_STUB(mission-talk-presentation): portrait Seb.Frame and drawing are unbound. */ }
bool MissionEvents::text_ended(){return text_reveal_ended(updating_->reveal);}
void MissionEvents::flush_text(){flush_text_reveal(updating_->reveal,*this);++flushes_;}
void MissionEvents::set_text(const std::u16string&){set_page(*updating_);++pages_;}
void MissionEvents::continue_talk(PreparedTalkForm&){unbound("non-factory talk continuation");}
EventObject MissionEvents::make_event_form(std::int32_t,EventObject){unbound("type4 event form");}
EventProgression& MissionEvents::progression(){unbound("live numeric event progression");}
std::int32_t MissionEvents::stamina_limit(){unbound("saved stamina item state");}
std::int64_t MissionEvents::read_stamina(){unbound("saved stamina");}
void MissionEvents::write_stamina(std::int64_t){unbound("saved stamina");}
std::int32_t MissionEvents::identify_script(const EventScript& s){return executor_.identify_script(s);}
EventScript MissionEvents::resolve_script(std::int32_t id){return executor_.resolve_script(id);}
std::int32_t MissionEvents::read_talk_character(){unbound("saved talk character slot7");}
void MissionEvents::restore_talk_character(std::size_t,std::int32_t){unbound("saved talk character slot7");}
NullablePresentationText MissionEvents::translate_argument(const std::u16string& text){
    // SAGA_STUB(mission-text-translation): explicit invariant-language service, no translation/honorific rewriting.
    return text;
}
std::string MissionEvents::title()const{
    if(forms_.empty())return {};
    const auto& talk=*forms_.back();
    if(talk.finish_frame>=0)return "Dialogue - closing";
    return "Dialogue "+std::to_string(talk.prepared.text_index+1)+"/"+std::to_string(talk.prepared.texts.size())+" - Enter reveals or advances";
}
void MissionEvents::write_report(std::ostream& out)const{
    out<<",\"new_game_event_context\":"<<(scenario_.new_game_context?"true":"false")
       <<",\"event_save_string0_kind\":\""<<(!scenario_.save_string0?"null":scenario_.save_string0->empty()?"empty":"text")<<'"';
    out<<",\"talk_font_bound\":"<<(scenario_.font?"true":"false")<<",\"talk_font_measurements\":"<<layout_measurements_<<",\"talk_layout_samples\":[";
    for(std::size_t i=0;i<layout_samples_.size();++i){if(i)out<<',';const auto& s=layout_samples_[i];out<<'['<<s.talk<<','<<s.page<<','<<s.length<<','<<s.height<<','<<s.blocks<<']';}out<<']';
    out<<",\"mission_events_bound\":true,\"talk_forms_opened\":"<<opened_<<",\"talk_forms_closed\":"<<closed_
       <<",\"talk_update_calls\":"<<ticks_<<",\"talk_flushes\":"<<flushes_<<",\"talk_page_changes\":"<<pages_<<",\"executed_stage_events\":[";
    for(std::size_t i=0;i<executed_.size();++i){if(i)out<<',';out<<executed_[i];}out<<"],\"talk_pending\":"<<(pending()?"true":"false");
    if(pending()){
        const auto& t=*forms_.back();out<<",\"talk\":{\"id\":"<<t.id<<",\"character\":"<<t.prepared.character_index<<",\"page\":"<<t.prepared.text_index
            <<",\"pages\":"<<t.prepared.texts.size()<<",\"frame\":"<<t.frame<<",\"close_counter\":"<<t.finish_frame
            <<",\"form_state\":"<<t.state<<",\"reveal_length\":"<<t.reveal.length<<",\"reveal_position\":"<<t.reveal.position
            <<",\"layout_provider\":\""<<(scenario_.font?"native ASCII blocks; public FreeType metrics unverified":"provisional scenario")<<"\"}";
    }
    out<<",\"talk_lifecycle\":[";for(std::size_t i=0;i<lifecycle_.size();++i){if(i)out<<',';out<<'"'<<lifecycle_[i]<<'"';}out<<']';
    out<<",\"event_execution_counts\":[";bool first=true;
    for(std::size_t i=0;i<execution_.catalog->size();++i){const auto& r=execution_.catalog->at(i);if(r&&r->execution_count!=0){if(!first)out<<',';first=false;out<<'['<<i<<','<<r->execution_count<<']';}}out<<']';
}
}
