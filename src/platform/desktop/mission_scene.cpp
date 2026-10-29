#include "mission_scene.hpp"
#include "mission_spatial.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/gameplay/subset_cursors.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mission scene input");return *p;}
struct Cursor final:RenderCursor {
    EntitySubset::Cursor cursor;
    explicit Cursor(EntitySubset& group):cursor(group.cursor()){}
    std::optional<ComponentSlots*> next()override{
        while(const auto entry=cursor.next()){
            const auto* effect=dynamic_cast<EffectComponent*>((*entry)->get(26));
            if(effect&&effect->type==3)continue; // SAGA_STUB(mission-collection-labels): retain native effect/lifetime; numeric glyph rendering is unbound.
            if(effect&&effect->type==7)continue; // SAGA_STUB(mission-mp-notices): retain native no-Depth notice and lifetime; dedicated output is unbound.
            return entry;
        }return std::nullopt;
    }
};
struct MovementWorld final:PassInitializationWorld,ModificationInitializationWorld,EmitterInitializationWorld {
    MissionEntities& world;
    explicit MovementWorld(MissionEntities& value):world(value){}
    EntitySubset* subset(const std::vector<std::int32_t>& slots)override{return world.subset(slots);}
    EntitySubset* modification_subset(const EventIntegers& slots)override{return world.subset(slots);}
    EntitySubset* emitter_subset(const EventIntegers& slots)override{return world.subset(slots);}
    void subscribe_impact(EntityEvents::Single,std::optional<EventHashes>)override{
        throw std::logic_error("mission impact dispatch is not bound");
    }
};
struct ExpiringCursor final:GarbageCursor {
    MissionEntities& world;EntitySubset::Cursor cursor;
    ExpiringCursor(MissionEntities& owner,EntitySubset& group):world(owner),cursor(group.cursor()){}
    std::optional<GarbageEntity> next()override{if(const auto entity=cursor.next())return GarbageEntity{world.handle(**entity),*entity};return std::nullopt;}
};
}
MissionScene::MissionScene(MissionEntities& world,MissionApplication& app,EntitySubset& visible,MissionView& view,MapComponent* map)
    :world_(world),app_(app),visible_(visible),view_(view),map_(map),form_{form_state_,nullptr,this}{
    initialize_render(renderer_,this);initialize_animation(animations_,&world_);
    MovementWorld movement(world_);initialize_movement(movers_,&movement);initialize_collisions(collisions_,&movement);initialize_garbage(garbage_entities_,&movement);
    initialize_battle(battle_,&movement);
    initialize_modifications(modifications_,&movement);
    initialize_emitters(emitters_,&movement);
    initialize_parameter_experience(experience_,this);initialize_parameters(this);
    initialize_effects(effects_,this);
}
void MissionScene::subscribe_dropoff(EntityEvents::Single receiver,std::optional<EventHashes> hashes){world_.events().subscribe(16,std::move(receiver),hashes);}
void MissionScene::subscribe_parameter_experience(ParameterExperienceInitializationWorld::Receiver receiver,std::optional<EventHashes> hashes){
    // Fresh scene Init has one event21 receiver. General typed EventManager removal/restoration is unbound.
    if(hashes||experience_receiver_)throw std::logic_error("parameter experience requires fresh scene initialization");experience_receiver_=std::move(receiver);
}
void MissionScene::send_parameter_experience(ComponentSlots* e,std::int32_t key){experience_receiver_(e,key);}
void MissionScene::advance_battle(std::int32_t pass,bool enabled){if(gameplay_needs_update(2,enabled,pass)){update_battle(battle_);++battle_steps_;}}
void MissionScene::advance_parameter_experience(std::int32_t pass,bool enabled){if(gameplay_needs_update(9,enabled,pass)){update_parameter_experience(experience_);++experience_steps_;}}
void MissionScene::advance_parameters(std::int32_t pass,bool enabled){if(gameplay_needs_update(14,enabled,pass)){update_parameters();++parameter_steps_;}}
void MissionScene::advance_effects(std::int32_t pass,bool enabled){if(gameplay_needs_update(23,enabled,pass)){update_effects(effects_,*this);++effect_steps_;}}
void MissionScene::write_pass_report(std::ostream& out){
    out<<",\"battle_steps\":"<<battle_steps_;
    std::size_t pending=0;for(const auto& queue:*experience_.queues)pending+=queue->size();
    out<<",\"parameter_experience_steps\":"<<experience_steps_<<",\"parameter_steps\":"<<parameter_steps_<<",\"parameter_experience_pending\":"<<pending<<",\"effect_steps\":"<<effect_steps_<<",\"effect_updates\":[";
    bool first=true;auto cursor=required(effects_.subset).cursor();while(const auto entity=cursor.next()){
        const auto& effect=required(dynamic_cast<EffectComponent*>((*entity)->get(26)));if(!first)out<<',';first=false;
        out<<"{\"id\":"<<world_.id(**entity)<<",\"type\":"<<effect.type<<",\"frame\":"<<effect.frame<<",\"max_frame\":"<<effect.max_frame<<'}';
    }out<<']';
}
void MissionScene::advance_animations(SharedAnimationGates gates){update_animations(animations_,gates);}
void MissionScene::advance_movement(std::int32_t pass,bool enabled){
    if(gameplay_needs_update(11,enabled,pass))move_subset(required(movers_));
}
void MissionScene::advance_garbage(std::int32_t pass,bool enabled){
    if(gameplay_needs_update(31,enabled,pass))garbage_pass_.update(*this);
}
void MissionScene::advance_modifications(std::int32_t pass,bool enabled){if(gameplay_needs_update(18,enabled,pass))update_modifications(modifications_,*this);}
void MissionScene::advance_emitters(std::int32_t pass,bool enabled){if(gameplay_needs_update(15,enabled,pass))update_emitters(emitters_,*this);}
void MissionScene::advance_collisions(std::int32_t pass,bool enabled){if(gameplay_needs_update(12,enabled,pass))collisions_.pass.value().update(*this);}
std::unique_ptr<CollisionCursor> MissionScene::open_collision_cursor(){return open_collision_subset(required(collisions_.subset));}
void MissionScene::send_collision(std::int32_t event,ComponentSlots& a,ComponentSlots& b){
    const auto area=[](const ComponentSlots& e){return e.contains(33)&&!e.contains(29);};
    const bool area_contact=(area(a)&&b.contains(24))||(area(b)&&a.contains(24));
    world_.events().send(event,&a,&b);++collision_events_;if(area_contact)++area_collision_events_;
}
std::unique_ptr<GarbageCursor> MissionScene::open_garbage_cursor(){return std::make_unique<ExpiringCursor>(world_,required(garbage_entities_));}
void MissionScene::bind_spatial(MissionSpatial& value){spatial_=&value;map_=&value.map();initialize_render(renderer_,this);}
RenderQueue& MissionScene::render(){faults_.clear();clear_render_queue(queue_);update_render(renderer_,spatial_?spatial_->camera_host():*this,*this);return queue_;}
std::unique_ptr<RenderCursor> MissionScene::visible_render_cursor(){return std::make_unique<Cursor>(spatial_?spatial_->visible():visible_);}
AnimationResource* MissionScene::render_resource(RenderQueue&,std::int32_t id){auto* r=get_application_resource(app_,id);return r?r->animations:nullptr;}
EventInstruction MissionScene::render_sprite(SharedAnimationClip& clip,std::int32_t frame,std::int32_t layer){return seb_sprite(required(clip.sprites),sprite_work_,frame,layer);}
std::int32_t MissionScene::render_layer_count(SharedAnimationClip& clip){return static_cast<std::int32_t>(required(required(clip.sprites).groups.get()).size());}
void MissionScene::render_error(ComponentSlots& e,const std::exception& error){faults_.push_back({world_.id(e),error.what()});}
QuestCameraForm* MissionScene::current_camera_form(){form_state_.scale=view_.scale;form_.x=view_.camera_x;form_.y=view_.camera_y;return &form_;}
CameraFormation* MissionScene::current_camera_members(){throw std::logic_error("selected mission scene has no formation camera binding");}
std::int32_t MissionScene::camera_touch_count(){throw std::logic_error("selected mission scene has no touch input binding");}
EventInstruction MissionScene::camera_saved_integers(){throw std::logic_error("selected mission scene has no decoded camera-save slots");}
} // namespace saga::desktop
