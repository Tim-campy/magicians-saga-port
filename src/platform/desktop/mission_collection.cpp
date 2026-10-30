#include "mission_humans.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
ComponentSlots* MissionHumans::CollectionHost::front_line_human(){
    if(!owner.members_.members)throw std::invalid_argument("missing collection member list");
    return core::front_line_human(&owner.members_.members->entities());
}
void MissionHumans::enable_collection(){
    if(collection_enabled_||!notification_scenario_||!world_.reward_catalog_ready())throw std::invalid_argument("collection fresh requires one scenario, stage_notifications and Treasure catalog");
    if(!native_state_){
        stage_.replace_rewards(std::make_shared<StageRewards>(StageRewards{0,0,0,{}}));
        stage_.collection.collecting=std::make_shared<std::vector<ComponentSlots*>>();
        stage_.collection.reward_box_picked_up=false;
    }
    collection_enabled_=true;
}
void MissionHumans::advance_collection(std::int32_t pass,bool enabled){
    if(!collection_enabled_||!gameplay_needs_update(3,enabled,pass)||stage_.dialog||stage_.state!=1)return;
    collect_drops(stage_.collection,collection_host_);update_collecting_drops(stage_.collection,collection_host_);++collection_steps_;
}
void MissionHumans::write_collection_report(std::ostream& out){
    out<<",\"collection_enabled\":"<<(collection_enabled_?"true":"false");if(!collection_enabled_)return;
    const auto& rewards=*stage_.rewards();
    out<<",\"collection_steps\":"<<collection_steps_<<",\"collecting_drops\":"<<stage_.collection.collecting->size()
       <<",\"reward_box_picked_up\":"<<(stage_.collection.reward_box_picked_up?"true":"false")
       <<",\"collection_sounds\":"<<collection_sounds_<<",\"collection_jingles\":"<<collection_jingles_
       <<",\"stage_rewards\":{\"experience\":"<<rewards.experience<<",\"money\":"<<rewards.money<<",\"prisoners\":"<<rewards.prisoners<<",\"treasures\":[";
    bool first=true;for(const auto id:rewards.treasure_ids){if(!first)out<<',';first=false;out<<id;}out<<"]}";
    std::size_t labels=0;auto cursor=world_.subset({26})->cursor();while(const auto entry=cursor.next()){
        const auto* effect=dynamic_cast<EffectComponent*>((*entry)->get(26));if(effect&&effect->type==3)++labels;
    }out<<",\"numeric_effects_live\":"<<labels;
}
} // namespace saga::desktop
