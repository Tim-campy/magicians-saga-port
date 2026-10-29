#include "mission_scene.hpp"
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
}
MissionScene::MissionScene(MissionEntities& world,MissionApplication& app,EntitySubset& visible,MissionView& view,MapComponent* map)
    :world_(world),app_(app),visible_(visible),view_(view),map_(map),form_{form_state_,nullptr,this}{
    initialize_render(renderer_,this);initialize_animation(animations_,&world_);
}
void MissionScene::advance_animations(SharedAnimationGates gates){update_animations(animations_,gates);}
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
