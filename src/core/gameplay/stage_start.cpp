#include "stage_start.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing stage startup state");return *p;}
std::int32_t multiply(std::int32_t a,std::int32_t b){const auto v=static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
}
void change_attack_button_state(AttackButtonState& button,std::int32_t state){button.state=state;button.frame=0;}
void reset_stage_timer(StageStartState& state,StageTimerHost& host){
    const auto time=required(state.quest.get()).time_limit;
    state.remaining_time=multiply(time,host.frames_per_second());
}
void start_stage(StageStartState& state,EventObject area,std::shared_ptr<StageQuest> quest,
                 std::int32_t stage,RandomSource& random,StageStartHost& host){
    state.area=std::move(area);state.quest=quest;state.stage=stage;
    state.replace_rewards(std::make_shared<StageRewards>(StageRewards{0,0,0,{}}));
    cleanup_quest_stage(host.cleanup_host());
    reset_stage_timer(state,host);
    state.boss_started=false;state.collection.boss_defeated=false;state.collection.reward_box_picked_up=false;
    state.move_front_pushed=false;state.retried=false;state.total_damage=0;state.state=0;
    state.min_z_rate=0;state.max_z_rate=95;state.frame=0;
    required(host.current_map_system()).load_map(state.quest);
    required(state.humans);
    auto& members=required(state.members);
    start_party(members,host.party_host(),[quest]{return (static_cast<std::uint32_t>(required(quest.get()).flags)&2U)!=0;});
    required(state.enemies).start_enemies(quest);
    auto& current_members=required(state.members);
    const auto count=required(current_members.members.get()).entities().size();
    required(host.quest_form()).scale=count>3?75:100;
    place_stage_drops(state,random,host);
    for(std::size_t i=0;i<required(state.attack_buttons.get()).size();++i){
        auto& button=required(state.attack_buttons->at(i));
        change_attack_button_state(button,i==0?1:0);button.frame=4;
    }
    state.last_active_button=0;
    const auto events=required(state.quest.get()).events;
    const auto first=required(events.get()).at(0);
    state.event_states=std::make_shared<std::vector<std::uint8_t>>(required(first.get()).size(),0);
    if(host.systems_paused())host.resume_systems();
    host.play_stage_bgm();host.play_stage_sound();
}
} // namespace saga::core
