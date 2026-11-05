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
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <utility>
#include "core/gameplay/combat_rules.hpp"

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
    :archive_(archive),scenario_(std::move(scenario)),executor_(execution_,*this),root_{std::make_shared<int>(0),this,{}},clear_input_(std::move(clear_input)){
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
std::u16string MissionEvents::translated_popup_literal(std::string_view source){
    // SAGA_STUB(town-gift-translation): reuse the selected text.txt exact lookup;
    // concatenated/localized transformations beyond this literal remain unbound.
    if(const auto data=archive_.data("text.txt")){
        std::istringstream rows(std::string(data->bytes.begin(),data->bytes.end()));
        for(std::string line;std::getline(rows,line);){
            if(!line.empty()&&line.back()=='\r')line.pop_back();const auto tab=line.find('\t');
            if(tab==std::string::npos||std::string_view(line).substr(0,tab)!=source)continue;
            const auto end=line.find('\t',tab+1);const auto display=line.substr(tab+1,end==std::string::npos?end:end-tab-1);
            if(!display.empty())return decode_mission_text(display);
            break;
        }
    }
    return decode_mission_text(source);
}
FormText MissionEvents::format_popup_value(std::string_view pattern,std::int64_t amount){
    // MyFormBase.LT0x11d8220: TranslateText(true), JString.ValueOf(int64), then StringUtil.Replace.
    auto translated=translated_popup_literal(pattern);
    const auto text=substitute_form_arguments(std::move(translated),decode_mission_text(std::to_string(amount)),*this);
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> converter;
    return std::make_shared<const std::string>(converter.to_bytes(*text));
}
EventObject MissionEvents::make_popup_contents(std::int32_t kind,FormText title,FormText text,bool option){
    if(kind!=3||option||!title||!text)throw std::invalid_argument("unsupported resident popup content");
    auto form=std::make_shared<Talk>();form->id=-1;form->prepared={};form->prepared.initial.type=19;
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> converter;
    form->popup=PopupContents{{1,0},kind,option,converter.to_bytes(translated_popup_literal(*title)),*text};
    // SAGA_STUB(town-gift-popup-output): title acknowledgement replaces native
    // window/glyph/SEB output and its soft-label25 hit target; domain clocks run.
    creating_=form;return form;
}
void MissionEvents::pop_popup(){
    if(!updating_||!updating_->popup)throw std::logic_error("popup Pop requires selected type19 form");
    updating_->state=4;lifecycle_.push_back("pop:"+std::to_string(updating_->id));
}
EventObject MissionEvents::make_shop(ShopFormHost& host,std::int32_t kind){
    auto form=std::make_shared<Talk>();form->id=-101;form->prepared={};
    form->magic_shop=MagicShop{{},&host,{}};form->magic_shop->state.kind=kind;creating_=form;return form;
}
const ShopFormState* MissionEvents::active_shop()const{
    return forms_.empty()||!forms_.back()->magic_shop?nullptr:&forms_.back()->magic_shop->state;
}
void MissionEvents::select_shop_row(std::int32_t value){
    if(!active_shop())throw std::logic_error("shop selection requires selected shop");
    forms_.back()->magic_shop->state.selection=value; // ShopForm.SetTouchValue0x1211d50, mask0x20000.
}
EventObject MissionEvents::make_magic_shop_notice(const MagicShopBuyRequest& request,EventObject character,std::function<void()> sound){
    auto form=std::make_shared<Talk>();form->id=-14;form->prepared={};form->prepared.initial.type=14;
    form->magic_notice=MagicShopNotice{{},request,std::move(character),std::move(sound)};
    // SAGA_STUB(town-magic-purchase-output): title replaces native celebration art/audio.
    creating_=form;return form;
}
EventObject MissionEvents::make_equipment_purchase(std::int32_t kind,std::int32_t id,std::int32_t option,std::function<void()> sound){
    // SubForm.MakeShopPurchase0x11f61e4; SubForm13 shares14's completion controller.
    auto form=std::make_shared<Talk>();form->id=-13;form->prepared={};form->prepared.initial.type=13;
    form->magic_notice=MagicShopNotice{{},{kind,0,0,0,0},{},std::move(sound),id,option};
    // SAGA_STUB(town-magic-purchase-output): title acknowledgement and audio provider.
    creating_=form;return form;
}
bool MissionEvents::pop_shop(){
    if(!updating_||!updating_->magic_shop)throw std::logic_error("shop return requires selected shop");
    updating_->state=4;lifecycle_.push_back("return:-101");return true;
}
std::optional<std::int32_t> MissionEvents::take_shop_child_result(){
    if(!updating_||!updating_->magic_shop)throw std::logic_error("shop child result requires selected parent");
    return std::exchange(updating_->magic_shop->child_result,std::nullopt);
}
EventObject MissionEvents::make_build_list(std::int32_t slot,ConstructionServices& services){
    auto form=std::make_shared<Talk>();form->id=-12;form->prepared={};form->prepared.initial.type=12;
    form->build_list=BuildList{{},&services};form->build_list->state.town_slot=slot;creating_=form;return form;
}
const BuildListState* MissionEvents::active_build_list()const{return forms_.empty()||!forms_.back()->build_list?nullptr:&forms_.back()->build_list->state;}
const BuildingCompleteState* MissionEvents::active_building_complete()const{return forms_.empty()||!forms_.back()->building_complete?nullptr:&forms_.back()->building_complete->state;}
void MissionEvents::select_build_entry(std::int32_t value){
    if(forms_.empty()||!forms_.back()->build_list)throw std::logic_error("build selection requires selected list");
    forms_.back()->build_list->state.selected=value; // SubForm.SetTouchValue(mask0x20000).
}
void MissionEvents::push_building_complete(std::int32_t id){
    auto form=std::make_shared<Talk>();form->id=-17;form->prepared={};form->prepared.initial.type=17;
    form->building_complete=BuildingComplete{{id,0,0},updating_->build_list->services};
    creating_=form;push_form(form);
}
void MissionEvents::pop_build_list(){updating_->state=4;lifecycle_.push_back("pop:-12");}
void MissionEvents::pop_building_complete(){
    updating_->state=4;lifecycle_.push_back("pop:"+std::to_string(updating_->id));
}
void MissionEvents::push_build_tapjoy(){unbound("optional construction Tapjoy child");}
EventObject MissionEvents::make_king_hall(KingHallServices& services){
    auto form=std::make_shared<Talk>();form->id=-15;form->prepared={};form->prepared.initial.type=15;
    form->king=KingHall{{},&services,0};creating_=form;return form;
}
bool MissionEvents::request_form_back(){
    if(forms_.empty()||(!forms_.back()->king&&!forms_.back()->build_list&&!forms_.back()->magic_shop))return false;
    form_back_requested_=true;return true;
}
NullablePresentationText MissionEvents::king_hall_voice(){return updating_->king->services->king_voice();}
void MissionEvents::king_hall_initialize_news(std::int32_t value){
    if(value!=60)throw std::invalid_argument("unsupported king news initialization");
    // SAGA_STUB(town-king-news): offline NewsBanner(60), no online feed or output.
    updating_->king->news_updates=0;
}
std::int32_t MissionEvents::king_hall_easing(std::int32_t frame){return easing(-40,80,40,frame,0);}
void MissionEvents::execute_king_rewards(){updating_->king->services->king_grant();}
void MissionEvents::king_hall_play_sound(){updating_->king->services->king_sound();}
void MissionEvents::king_hall_save_game(){updating_->king->services->king_save_game();}
void MissionEvents::king_hall_save_system(){updating_->king->services->king_save_system();}
void MissionEvents::king_hall_push_notes(){unbound("king notes form5");}
void MissionEvents::king_hall_push_tapjoy(){unbound("king optional Tapjoy child");}
void MissionEvents::king_hall_pop(){updating_->state=4;lifecycle_.push_back("pop:-15");}
void MissionEvents::king_hall_update_news(){++updating_->king->news_updates;}
bool MissionEvents::execute(std::int32_t id){
    const auto result=execute_indexed_event(id,{},delays_,executor_);executed_.push_back(id);return static_cast<bool>(result);
}
bool MissionEvents::advance(std::uint64_t frame,std::int32_t tick_ms,bool pulse,bool held){
    if(forms_.empty())return false;
    form_back_pulse_=std::exchange(form_back_requested_,false);
    construction_pulses_=std::exchange(pending_construction_pulses_,0);
    now_=static_cast<std::int64_t>(frame)*tick_ms;total_frames_=static_cast<std::int32_t>(frame);pulse_=pulse;held_=held;
    // Same-thread, synchronous manager projection: process pending Init before selecting the top.
    for(auto& form:forms_)if(form->state==0){
        // SAGA_STUB(mission-talk-common-ui): non-gameplay Init providers, softlabels, blur and resource release.
        if(form->event_notice){updating_=form;set_event(0);updating_.reset();}
        if(form->popup){form->popup->production={1,0};form->event_frame=0;}
        if(form->build_list){updating_=form;form->build_list->state.frame=0;initialize_build_list(form->build_list->state,*this);updating_.reset();}
        if(form->building_complete){form->building_complete->state.frame=0;initialize_building_complete(form->building_complete->state);}
        if(form->magic_shop){updating_=form;initialize_shop_form(form->magic_shop->state,*form->magic_shop->host);updating_.reset();}
        if(form->magic_notice){form->magic_notice->state={1,0};}
        if(form->king){updating_=form;form->king->state.frame=0;initialize_king_hall(form->king->state,*this);updating_.reset();}
        form->state=1;clear_input_();pulse_=held_=form_back_pulse_=false;construction_pulses_=0;
        lifecycle_.push_back("init:"+std::to_string(form->id));
    }
    while(!forms_.empty()){
        updating_=forms_.back();auto& talk=*updating_;
        if(talk.state==1){clear_input_();pulse_=held_=form_back_pulse_=false;construction_pulses_=0;talk.state=2;lifecycle_.push_back("resume:"+std::to_string(talk.id));} // Manager.Resume, no form Resume callback.
        ++ticks_;
        if(talk.build_list||talk.building_complete||talk.magic_shop||talk.magic_notice){
            if(talk.magic_shop)update_shop_form(talk.magic_shop->state,*talk.magic_shop->host);
            else if(talk.magic_notice)update_magic_shop_notice(talk.magic_notice->state,*this);
            else if(talk.build_list)update_build_list(talk.build_list->state,*this);
            else update_building_complete(talk.building_complete->state,*this);
            if(talk.state==4){
                if(!finish_talk(talk)||talk.state!=4)return true;
                const auto dying=updating_;
                lifecycle_.push_back("remove:"+std::to_string(talk.id));
                const auto position=std::find(forms_.begin(),forms_.end(),dying);
                if(position==forms_.end())throw std::logic_error("construction Pop lost parent identity");
                if(talk.magic_shop){
                    // ChangeCurrentForm retires every shop descendant. A Buy+Back
                    // can push a fresh notice in this same Update; it must not survive.
                    for(auto child=std::next(position);child!=forms_.end();++child){
                        finish_talk(**child);lifecycle_.push_back("remove:"+std::to_string((*child)->id));++closed_;
                    }
                    forms_.erase(position,forms_.end());
                }else forms_.erase(position);
                ++closed_;updating_.reset();clear_input_();pulse_=held_=form_back_pulse_=false;construction_pulses_=0;
                // BuildList pushes Complete before Pop(self). Its state0 child
                // survives and receives Init on the following manager dispatch.
                if(!forms_.empty()&&forms_.back()->state==0)return true;
                if(!forms_.empty()&&forms_.back()->state==3)forms_.back()->state=1;
                continue;
            }
            talk.state=3;updating_.reset();return true;
        }
        if(talk.king){
            // SAGA_STUB(town-king-output): title/Backspace is the declared idle
            // window/soft-label provider; the original domain controller executes.
            talk.frame=advance_select_loop_counter(talk.frame);
            update_king_hall(talk.king->state,*this);
            if(talk.state==4){
                if(!finish_talk(talk)||talk.state!=4)return true;
                lifecycle_.push_back("remove:-15");forms_.pop_back();++closed_;
                updating_.reset();clear_input_();pulse_=held_=form_back_pulse_=false;construction_pulses_=0;
                if(!forms_.empty()&&forms_.back()->state==3)forms_.back()->state=1;
                continue;
            }
            talk.state=3;updating_.reset();return true;
        }
        if(talk.popup){
            // Type19 config[3]=-1 skips the generic five-pass window gate.
            talk.frame=advance_select_loop_counter(talk.frame);
            update_popup_contents(talk.event_frame,talk.popup->production,*this);
            if(talk.state==4){
                if(!finish_talk(talk)||talk.state!=4)return true;
                lifecycle_.push_back("remove:"+std::to_string(talk.id));forms_.pop_back();++closed_;
                updating_.reset();clear_input_();pulse_=held_=form_back_pulse_=false;construction_pulses_=0;
                if(!forms_.empty()&&forms_.back()->state==3)forms_.back()->state=1;
                continue;
            }
            talk.state=3;updating_.reset();return true;
        }
        if(talk.finish_frame>=0){
            --talk.finish_frame;
            if(talk.finish_frame<=0){
                talk.state=4; // Pop callback marks4; it does not erase the form.
                lifecycle_.push_back("pop:"+std::to_string(talk.id));
                // SubForm.Finish's fresh null-delay/null-share path returns true.
                // The manager removes only after that Finish and clears Canvas input.
                if(!finish_talk(talk)||talk.state!=4)return true;
                lifecycle_.push_back("remove:"+std::to_string(talk.id));
                forms_.pop_back();++closed_;updating_.reset();clear_input_();pulse_=held_=form_back_pulse_=false;construction_pulses_=0;
                if(!forms_.empty()&&forms_.back()->state==3)forms_.back()->state=1;
                continue; // No manager wait: select the underlying form in this same dispatch.
            }
        }else{
            talk.frame=advance_select_loop_counter(talk.frame);
            // SubForm.configData[0][3] is2; IsDrawWindowEnd permits its branch after5.
            if(talk.frame>5){
                if(talk.event_notice)update_event_notice(*talk.event_notice,talk.event_page,talk.event_frame,talk.finish_frame,*this);
                else update_talk_page(talk.prepared,talk.finish_frame,*this);
            }
        }
        if(talk.event_notice)draw_event_notice_semantics(talk);
        talk.state=3;updating_.reset();return true;
    }
    return false;
}
EventObject MissionEvents::dispatch(const EventInstruction& row,const EventContext& context){
    if(numeric_effect_&&row&&!row->empty()){
        const auto opcode=row->at(0);
        if(opcode==1||opcode==2||opcode==4||opcode==15){numeric_effect_(resolve_event_arguments(*row,context.argi));return {};}
    }
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
void MissionEvents::push_form(EventObject form){
    if(updating_){FormPushState parent{updating_,this,{}};push_base_form(parent,std::move(form));}
    else if(application_push_)application_push_(std::move(form));else push_base_form(root_,std::move(form));
}
void MissionEvents::push(EventObject parent,EventObject child,bool option){
    auto existing=std::find_if(forms_.begin(),forms_.end(),[&](const auto& form){return child==form;});
    const auto parent_exists=parent==root_.identity||std::any_of(forms_.begin(),forms_.end(),[&](const auto& form){return parent==form;});
    if(!parent_exists||option||child==parent||(child!=creating_&&existing==forms_.end()))throw std::invalid_argument("unsupported mission form push");
    const auto fresh=existing==forms_.end();auto form=fresh?creating_:*existing;
    if(!fresh)forms_.erase(existing);
    // PushCallback removes/reinserts known identities, preserving their native
    // state. SelectForm's second Push of ExecEvent's result is intentional.
    if(parent!=root_.identity){
        const auto position=std::find_if(forms_.begin(),forms_.end(),[&](const auto& value){return parent==value;});
        forms_.insert(std::next(position),form);
    }else if(application_push_)forms_.insert(forms_.begin(),form);else forms_.push_back(form);
    lifecycle_.push_back(std::string(fresh?"push:":"reinsert:")+std::to_string(form->id));
    if(child==creating_)creating_.reset();if(fresh)++opened_;

}
bool MissionEvents::finish_talk(Talk& talk){
    // SubForm.Finish 0x120488c: this fresh factory owns no delay/share continuation.
    // SAGA_STUB(mission-talk-common-ui): base UI/resource cleanup and parent release hook.
    lifecycle_.push_back("finish:"+std::to_string(talk.id));
    const auto position=std::find_if(forms_.begin(),forms_.end(),[&](const auto& form){return form.get()==&talk;});
    if(position!=forms_.begin()&&position!=forms_.end()&&(*std::prev(position))->magic_shop){
        // The shop consumes any returned MyFormBase child, including event8's
        // ordinary Talk. Reached factories retain Init's result-1.
        (*std::prev(position))->magic_shop->child_result=-1;
    }
    return true;
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
std::int32_t MissionEvents::parse_event_type(const std::u16string& text){
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> converter;
    return parse_event_decimal(converter.to_bytes(text));
}
EventObject MissionEvents::make_event_form(std::int32_t id,EventObject args){
    if(!event_messages_){
        const auto data=archive_.data("EventMsg.txt");if(!data)throw std::invalid_argument("event notice requires EventMsg.txt");
        event_messages_=load_event_message_table(std::string_view(reinterpret_cast<const char*>(data->bytes.data()),data->bytes.size()),false,parse_event_decimal,decode_mission_text);
    }
    const auto& row=event_messages_->at(static_cast<std::size_t>(id));if(!row)throw std::invalid_argument("null event-message record");
    const auto type=std::to_string(row->type);std::u16string source(type.begin(),type.end());source+=u'&';source+=row->text;
    const auto arguments=std::static_pointer_cast<std::u16string>(args);
    auto form=std::make_shared<Talk>();form->id=id;form->prepared={};form->prepared.initial.type=4;
    form->event_notice=prepare_event_form(std::move(source),arguments?NullablePresentationText(*arguments):std::nullopt,*this);
    creating_=form;return form;
}
void MissionEvents::set_event(std::int32_t page){
    if(!updating_||!updating_->event_notice)throw std::logic_error("notice setup requires current event form");
    auto& form=*updating_;const auto kind=form.event_notice->event_ids.at(static_cast<std::size_t>(page));
    if(form.event_resource_ready)return;
    // SAGA_STUB(event-notice-presentation): offline resource/window/audio provider.
    // Native first setup loads RecordStore(1,5), resets the clock/four cosmetic
    // counters and requests HAPPY for kinds0/2 or SAD for1; no progression effect.
    static_cast<void>(kind);form.event_frame=0;form.event_notice->animation.fill(0);form.event_resource_ready=true;
}
void MissionEvents::draw_event_notice_semantics(Talk& form){
    // SAGA_STUB(event-notice-window): title backend has no animated window.
    // Foreground is ready while not closing; one semantic Draw per dispatch.
    // Native DrawWindow must reject closing BEFORE the possibly end-page lookup.
    if(form.finish_frame>=0)return;
    const auto kind=form.event_notice->event_ids.at(static_cast<std::size_t>(form.event_page));
    if(!form.event_resource_ready)throw std::logic_error("notice Draw requires initialized resource provider");
    const auto frame_bits=static_cast<std::uint32_t>(form.event_frame)+1U;
    form.event_frame=frame_bits<=0x7fffffffU?static_cast<std::int32_t>(frame_bits):-1-static_cast<std::int32_t>(0xffffffffU-frame_bits);
    if(kind==0||kind==1)for(auto& tick:form.event_notice->animation){
        const auto bits=static_cast<std::uint32_t>(tick)+1U;tick=bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
    }
    // SAGA_STUB(event-notice-touch): full-viewport AddTouch(2); physical routing unbound.
}
EventProgression& MissionEvents::progression(){unbound("live numeric event progression");}
std::int32_t MissionEvents::stamina_limit(){unbound("saved stamina item state");}
std::int64_t MissionEvents::read_stamina(){unbound("saved stamina");}
void MissionEvents::write_stamina(std::int64_t){unbound("saved stamina");}
std::int32_t MissionEvents::identify_script(const EventScript& s){return executor_.identify_script(s);}
EventScript MissionEvents::resolve_script(std::int32_t id){return executor_.resolve_script(id);}
std::int32_t MissionEvents::read_talk_character(){
    if(!normal_save_)unbound("saved talk character slot7");return normal_save_->at(7);
}
void MissionEvents::restore_talk_character(std::size_t index,std::int32_t value){
    if(!normal_save_)unbound("saved talk character slot7");normal_save_->at(index)=value;
}
NullablePresentationText MissionEvents::translate_argument(const std::u16string& text){
    // SAGA_STUB(mission-text-translation): explicit invariant-language service, no translation/honorific rewriting.
    return text;
}
std::string MissionEvents::title()const{
    if(forms_.empty())return {};
    const auto& talk=*forms_.back();
    if(talk.build_list)return "Construction - select an item twice; Backspace returns";
    if(talk.magic_shop)return talk.magic_shop->state.kind==1?"Magic shop - select a row, then Buy; Backspace returns":talk.magic_shop->state.kind==5?"Recruitment - select a character, then Buy; Backspace returns":"Equipment shop - select an item, then Buy; Backspace returns";
    if(talk.magic_notice)return talk.prepared.initial.type==13?(talk.magic_notice->request.kind==5?"Character recruited - Enter continues":"Equipment purchased - Enter continues"):"Magic improved - Enter continues";
    if(talk.building_complete)return "Construction complete - Enter continues";
    if(talk.king)return "King hall - Backspace returns to town";
    if(talk.popup)return talk.popup->title+" - Enter continues";
    if(talk.finish_frame>=0)return talk.event_notice?"Notice - closing":"Dialogue - closing";
    if(talk.event_notice)return "Notice "+std::to_string(talk.event_page+1)+"/"+std::to_string(talk.event_notice->texts.size())+" - Enter continues";
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
    if(pending()&&forms_.back()->event_notice){const auto& form=*forms_.back();
        out<<",\"event_notice\":{\"id\":"<<form.id<<",\"page\":"<<form.event_page<<",\"frame\":"<<form.event_frame
           <<",\"pages\":"<<form.event_notice->event_ids.size()<<",\"window_provider\":\"provisional ready foreground\",\"full_view_touch\":2}";
    }
    if(pending()&&forms_.back()->popup){const auto& form=*forms_.back();const auto& popup=*form.popup;
        out<<",\"popup_contents\":{\"type\":19,\"kind\":"<<popup.kind<<",\"option\":"<<(popup.option?"true":"false")
           <<",\"production_state\":"<<popup.production.step<<",\"production_frame\":"<<popup.production.frame
           <<",\"animation_frame\":"<<form.event_frame<<",\"title\":"<<std::quoted(popup.title)<<",\"text\":"<<std::quoted(popup.text)
           <<",\"close_pulse\":33554432,\"presentation\":\"provisional title acknowledgement\"}";
    }
    if(pending()&&forms_.back()->king){const auto& hall=*forms_.back()->king;const auto& k=hall.state;
        out<<",\"king_hall\":{\"production_state\":"<<k.production_step<<",\"frame\":"<<k.frame
           <<",\"movement_frame\":"<<k.movement_frame<<",\"entrance_x\":"<<k.entrance_x<<",\"selection\":"<<k.selection
           <<",\"speech_present\":"<<(k.speech?"true":"false")<<",\"news_updates\":"<<hall.news_updates<<'}';
    }
    if(const auto* state=active_shop()){
        out<<(state->kind==1?",\"magic_shop\":{\"state\":":state->kind==5?",\"recruitment_shop\":{\"state\":":",\"equipment_shop\":{\"state\":")<<state->state<<",\"selected\":"<<state->selection<<",\"page\":"<<state->page
           <<",\"page_count\":"<<state->page_count<<",\"frame\":"<<state->frame<<",\"crossfade\":"<<state->crossfade<<",\"witch_x\":"<<state->witch_x<<",\"entries\":[";
        if(state->entries)for(std::size_t i=0;i<state->entries->size();++i){if(i)out<<',';out<<state->entries->at(i);}out<<']';
        if(state->kind!=1)out<<",\"kind\":"<<state->kind;
        out<<'}';
    }
    if(pending()&&forms_.back()->magic_notice){const auto& n=*forms_.back()->magic_notice;
        if(forms_.back()->prepared.initial.type==13)out<<",\"equipment_purchase\":{\"kind\":"<<n.request.kind<<",\"item\":"<<n.item<<",\"option\":"<<n.option;
        else out<<",\"magic_purchase\":{\"parameter\":"<<n.request.parameter_key<<",\"add\":"<<n.request.add_value<<",\"total\":"<<n.request.total;
        out
           <<",\"frame\":"<<n.state.frame<<",\"production_state\":"<<n.state.production_step<<'}';
    }
    if(const auto* state=active_build_list()){
        out<<",\"build_list\":{\"slot\":"<<state->town_slot<<",\"selected\":"<<state->selected<<",\"production_state\":"<<state->production_step<<",\"frame\":"<<state->frame<<",\"entries\":[";
        if(state->entries)for(std::size_t i=0;i<state->entries->size();++i){if(i)out<<',';out<<state->entries->at(i);}out<<"]}";
    }
    if(const auto* state=active_building_complete())out<<",\"building_complete\":{\"building\":"<<state->building_id<<",\"production_state\":"<<state->production_step<<",\"frame\":"<<state->frame<<'}';
    out<<",\"talk_lifecycle\":[";for(std::size_t i=0;i<lifecycle_.size();++i){if(i)out<<',';out<<'"'<<lifecycle_[i]<<'"';}out<<']';
    out<<",\"event_execution_counts\":[";bool first=true;
    for(std::size_t i=0;i<execution_.catalog->size();++i){const auto& r=execution_.catalog->at(i);if(r&&r->execution_count!=0){if(!first)out<<',';first=false;out<<'['<<i<<','<<r->execution_count<<']';}}out<<']';
}
}
