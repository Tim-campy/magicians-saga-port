#include "mission_scene.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mission scene input");return *p;}
struct Cursor final:RenderCursor {
    EntitySubset::Cursor cursor;
    explicit Cursor(EntitySubset& group):cursor(group.cursor()){}
    std::optional<ComponentSlots*> next()override{return cursor.next();}
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
    MovementWorld movement(world_);initialize_movement(movers_,&movement);initialize_garbage(garbage_entities_,&movement);
    initialize_modifications(modifications_,&movement);
    initialize_emitters(emitters_,&movement);
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
std::unique_ptr<GarbageCursor> MissionScene::open_garbage_cursor(){return std::make_unique<ExpiringCursor>(world_,required(garbage_entities_));}
RenderQueue& MissionScene::render(){faults_.clear();clear_render_queue(queue_);update_render(renderer_,*this,*this);return queue_;}
std::unique_ptr<RenderCursor> MissionScene::visible_render_cursor(){return std::make_unique<Cursor>(visible_);}
AnimationResource* MissionScene::render_resource(RenderQueue&,std::int32_t id){auto* r=get_application_resource(app_,id);return r?r->animations:nullptr;}
EventInstruction MissionScene::render_sprite(SharedAnimationClip& clip,std::int32_t frame,std::int32_t layer){return seb_sprite(required(clip.sprites),sprite_work_,frame,layer);}
std::int32_t MissionScene::render_layer_count(SharedAnimationClip& clip){return static_cast<std::int32_t>(required(required(clip.sprites).groups.get()).size());}
void MissionScene::render_error(ComponentSlots& e,const std::exception& error){faults_.push_back({world_.id(e),error.what()});}
QuestCameraForm* MissionScene::current_camera_form(){form_state_.scale=view_.scale;form_.x=view_.camera_x;form_.y=view_.camera_y;return &form_;}
CameraFormation* MissionScene::current_camera_members(){throw std::logic_error("selected mission scene has no formation camera binding");}
std::int32_t MissionScene::camera_touch_count(){throw std::logic_error("selected mission scene has no touch input binding");}
EventInstruction MissionScene::camera_saved_integers(){throw std::logic_error("selected mission scene has no decoded camera-save slots");}
} // namespace saga::desktop
