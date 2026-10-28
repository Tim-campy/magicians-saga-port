#include "stage_update.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing stage update input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t subtract(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t coordinate(float z){ // FCVTZS plus IL2CPP's explicit positive-infinity override.
    if(std::isnan(z))return 0;
    if(z==std::numeric_limits<float>::infinity()||z<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(z>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(z);
}
void change_stage(StageStartState& state,std::int32_t next){state.state=next;state.frame=0;}
ComponentSlots* front(StageStartState& s){return front_line_human(&required(required(s.members).members.get()).entities());}
void upgrade(ComponentSlots& hero,EquipmentSlot slot,StageUpdateHost& host){
    const auto ids=component<EquipmentRecordLinks>(hero,8).ids;
    const auto id=required(ids.get()).at(static_cast<std::size_t>(slot));
    auto& level=required(host.equipment_level(slot,id));
    if(slot==EquipmentSlot::cloth)raise_stage_cloth_level(level,1);else raise_stage_equipment_level(level);
}
void celebrate(StageStartState& s){
    const auto list=required(s.members).members;required(list.get());const auto version=list->version();
    for(std::size_t i=0;;++i){
        if(list->version()!=version)throw std::runtime_error("party changed during stage celebration");
        if(i>=list->entities().size())break;
        auto& e=required(list->entities()[i]);
        if(stage_human_alive(component<HumanRecordLink>(e,14)))
            change_human_state(e,6,required(required(s.humans).transitions));
    }
    change_stage(s,3);
}
}
bool stage_human_alive(const HumanRecordLink& h){return (static_cast<std::uint32_t>(h.state)&0xfffffffeU)!=4U;}
void complete_stage_quest(StageQuest& quest){quest.completed=1;}
void raise_stage_equipment_level(std::int32_t& level){level=std::min(add(level,1),100);}
void raise_stage_cloth_level(std::int32_t& level,std::int32_t amount){level=std::min(add(level,amount),100);}
std::int32_t stage_progress_rate(StageStartState& s,float z,StagePlacementHost& host){
    const auto height=stage_quest_height(required(s.quest.get()));
    const auto first_step=required(host.stage_map()).step_z;
    const auto second_step=required(host.stage_map()).step_z;
    const auto base=multiply(first_step,add(height,-3));
    const auto distance=std::max(add(subtract(base,coordinate(z)),second_step/2),0);
    const auto numerator=multiply(distance,100);
    // The bounded body emits SDIV directly: zero -> 0, INT_MIN/-1 -> INT_MIN.
    const auto rate=base==0?0:(base==-1&&numerator==std::numeric_limits<std::int32_t>::min()?numerator:numerator/base);
    return std::max(rate,0);
}
std::int32_t stage_progress_rate(StageStartState& s,StagePlacementHost& host){
    auto* e=front(s);if(!e)return 0;
    return stage_progress_rate(s,component<ContactPosition>(*e,0).z,host);
}
bool execute_stage_events(StageStartState& s,StageUpdateHost& host){
    const auto& q=required(s.quest.get());
    if((q.flags&2)!=0||q.completed!=0)return false;
    const auto groups=required(s.quest.get()).events;
    const auto rows=required(groups.get()).at(0);required(rows.get());bool executed=false;
    for(std::size_t i=0;i<rows->size();++i){
        if(required(s.event_states.get()).at(i)!=0)continue;
        const auto row=rows->at(i);required(row.get());
        const auto id=row->at(0),threshold=row->at(1);
        if(stage_progress_rate(s,host)<threshold)continue;
        const bool result=host.execute_stage_event(id);
        // Re-read the current flags after the callback, including when it returned null.
        required(s.event_states.get()).at(i)=1;
        executed=executed||result;
    }
    return executed;
}
void update_stage_attack_buttons(StageStartState& s){
    const auto buttons=s.attack_buttons;required(buttons.get());
    for(auto* p:*buttons){auto& b=required(p);b.frame=std::min(add(b.frame,1),4);}
}
std::shared_ptr<StageDialog> make_stage_retry_dialog(std::int32_t reason,StageUpdateHost& host){
    auto result=host.make_retry_form(100);required(result.get()).reason=reason;result->kind=98;return result;
}
void stage_game_over(StageStartState& s,std::int32_t reason,StageUpdateHost& host){
    if(s.retried)change_stage(s,reason==2?6:5);
    else {s.dialog=make_stage_retry_dialog(reason,host);required(host.stage_form_stack()).push(s.dialog.get());}
    host.stop_stage_bgm();
    auto* stack=host.stage_form_stack();const auto form=host.make_defeat_form(reason);
    required(stack).push(form.get());
}
void update_stage(StageStartState& s,StageUpdateHost& host){
    if(const auto dialog=s.dialog){
        if(dialog->kind==98){
            if(dialog->answer==0){
                if(host.check_crystals(100)){
                    host.subtract_crystals(100);host.retry_stage(s);s.dialog.reset();return;
                }
                const auto reason=required(s.dialog.get()).reason;
                s.dialog=make_stage_retry_dialog(reason,host);
                required(host.stage_form_stack()).push(s.dialog.get());
                auto* stack=host.stage_form_stack();const auto notice=host.make_insufficient_crystals_form();
                required(stack).push(notice.get());return;
            }
            if(dialog->answer==1){
                auto* stack=host.stage_form_stack();const auto ad=host.make_interstitial_ad();
                required(stack).push(ad.get());host.retry_stage(s);s.dialog.reset();return;
            }
            change_stage(s,dialog->reason==2?6:5);s.dialog.reset();return;
        }
        if(dialog->kind==99&&dialog->answer==0){return_to_town(3,host.return_host());s.dialog.reset();return;}
        s.dialog.reset();
    }
    const auto branch=s.state;
    s.loop_counter=add(s.loop_counter,1)%std::numeric_limits<std::int32_t>::max();
    switch(branch){
    case 0:if(required(host.update_quest_form()).fade_frame>=20)change_stage(s,1);return;
    case 1:
        collect_drops(s.collection,host.collection_host());
        update_collecting_drops(s.collection,host.collection_host());
        if(execute_stage_events(s,host)){release_stage_movement(host.movement_host());return;}
        if(!s.boss_started&&stage_progress_rate(s,host)>=90)host.start_boss_battle(s);
        if(s.collection.boss_defeated&&s.collection.reward_box_picked_up){celebrate(s);return;}
        update_stage_attack_buttons(s);
        if(!s.collection.boss_defeated){
            s.remaining_time=std::max(add(s.remaining_time,-1),0);
            if(s.remaining_time<=0){stage_game_over(s,2,host);return;}
        }
        if(host.key_pulse(0x2000000)){host.pause_systems(true);change_stage(s,2);}return;
    case 2:
        if(host.key_pulse(0x4000000)){host.pause_systems(false);change_stage(s,1);return;}
        if(host.key_pulse(0x8000000)){
            s.dialog=host.make_exit_form();required(s.dialog.get()).kind=99;
            required(host.stage_form_stack()).push(s.dialog.get());
        }return;
    case 3:{
        s.frame=add(s.frame,1);if(!host.key_pulse(0x100000))return;
        host.show_quest_result();required(s.members);
        auto& hero=required(member_hero(host.update_world()));
        upgrade(hero,EquipmentSlot::weapon,host);upgrade(hero,EquipmentSlot::hat,host);upgrade(hero,EquipmentSlot::cloth,host);
        change_stage(s,4);return;
    }
    case 4:complete_stage_quest(required(s.quest.get()));return_to_town(0,host.return_host());return;
    case 5:return_to_town(1,host.return_host());return;
    case 6:return_to_town(2,host.return_host());return;
    default:return;
    }
}
} // namespace saga::core
