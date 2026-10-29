#include "mission_spatial.hpp"
#include "mission_scene.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
MissionSpatial::MissionSpatial(MissionEntities& world,MapComponent& map,MissionView& view,StageQuestForm& state,
    std::shared_ptr<StageQuest> quest,EventInstruction images)
    :world_(world),map_(map),view_(view),formation_(*this),form_{state,this,this},
     visible_([](auto* e){return e!=nullptr;},[&world](const auto& e){return world.id(e);}),cell_view_(*this,scratch_){
    if(!quest)throw std::invalid_argument("mission map requires a live quest");
    state.scale=view.scale;quest->chip_images=std::move(images);
    initialize_mission_map(map_system_,*this);
    initialize_quest_camera(camera_,this);
    initialize_cells(cells_,this);initialize_cell_index(index_,this,*this);
    // LoadMap writes both camera-save slots before reading them; fresh scratch is explicit scenario state.
    load_mission_map(map_system_,std::move(quest),*this);
}
void MissionSpatial::advance_camera(std::int32_t pass,bool enabled){if(gameplay_needs_update(8,enabled,pass)){update_quest_camera(camera_,*this);++camera_steps_;}}
void MissionSpatial::advance_cells(std::int32_t pass,bool enabled){if(gameplay_needs_update(17,enabled,pass)){update_cells(cells_,*this);++cell_steps_;}}
void MissionSpatial::advance_visibility(std::int32_t pass,bool enabled){if(gameplay_needs_update(20,enabled,pass)){update_cell_visibility(index_,*this);++visibility_steps_;}}
std::int32_t MissionSpatial::camera_view_width(){return view_.width;}
std::int32_t MissionSpatial::camera_touch_count(){throw std::logic_error("mission camera touch input is unbound");}
std::int32_t MissionSpatial::quest_view_x(){return view_.margin_x;}
std::int32_t MissionSpatial::quest_view_y(){return view_.margin_y;}
std::int32_t MissionSpatial::game_width(){return view_.width;}
std::int32_t MissionSpatial::game_height(){return view_.height;}
void MissionSpatial::Notifications::added(ComponentSlots& e,std::int32_t i,Component* c){if(previous)previous->added(e,i,c);events.added(e,i,c);}
void MissionSpatial::Notifications::removed(ComponentSlots& e,std::int32_t i,Component* c){if(previous)previous->removed(e,i,c);events.removed(e,i,c);}
void MissionSpatial::Notifications::changed(ComponentSlots& e,std::int32_t i,Component* a,Component* b){if(previous)previous->changed(e,i,a,b);events.changed(e,i,a,b);}
void MissionSpatial::bind_notifications(EntitySubset& group){
    if(observed_&&observed_!=&group)throw std::logic_error("mission cell observer requires the canonical Cell group");
    if(!observed_){observed_=&group;notifications_.group=&group;notifications_.previous=group.notifications;group.notifications=&notifications_;}
}
void MissionSpatial::subscribe_cell_added(EntitySubset& group,ComponentReceiver f){
    bind_notifications(group);observers_.emplace_back(std::move(f),true);notifications_.events.subscribe_added(observers_.back());
}
void MissionSpatial::subscribe_cell_removed(EntitySubset& group,ComponentReceiver f){
    bind_notifications(group);observers_.emplace_back(std::move(f),false);notifications_.events.subscribe_removed(observers_.back());
}
void MissionSpatial::subscribe_cell_change(Receiver f,std::optional<EventHashes>){
    // Typed event14 projection: immutable delivery snapshot, no per-send copy/allocation.
    auto next=cell_receivers_?std::make_shared<std::vector<Receiver>>(*cell_receivers_):std::make_shared<std::vector<Receiver>>();
    next->push_back(std::move(f));cell_receivers_=std::move(next);
}
void MissionSpatial::send_cell_change(ComponentSlots* e,std::int32_t old_key){
    const auto receivers=cell_receivers_;if(receivers)for(const auto& f:*receivers)f(e,old_key);++changes_;
}
void MissionSpatial::report(std::ostream& out){
    out<<",\"camera_steps\":"<<camera_steps_<<",\"cell_steps\":"<<cell_steps_<<",\"visibility_steps\":"<<visibility_steps_
       <<",\"cell_change_events\":"<<changes_<<",\"visible_count\":"<<visible_count_;
    out<<",\"camera\":{\"x\":"<<form_.x<<",\"y\":"<<form_.y<<",\"saved_x\":"<<save_->at(17)<<",\"saved_y\":"<<save_->at(18)<<",\"scale\":"<<form_.state.scale<<'}';
    out<<",\"mission_map\":{\"width\":"<<map_.width<<",\"height\":"<<map_.height<<",\"size_width\":"<<map_.size_width
       <<",\"size_height\":"<<map_.size_height<<",\"step_x\":"<<map_.step_x<<",\"step_z\":"<<map_.step_z<<",\"chips_live\":"<<world_.subset({9})->size()<<'}';
    out<<",\"visible_entities\":[";bool first=true;auto cursor=visible_.cursor();while(const auto e=cursor.next()){
        if(!first)out<<',';first=false;out<<"{\"id\":"<<world_.id(**e)<<",\"destroyed\":"<<(world_.destroyed(**e)?"true":"false")<<'}';}out<<']';
}
}
