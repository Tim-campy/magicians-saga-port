#include "mission_scene.hpp"
#include "mission_spatial.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/gameplay/subset_cursors.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>
#include <algorithm>

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
struct EnemyGaugeHost final:EnemyGaugeDrawHost {
    MissionEntities& world;QuestCameraHost& camera;const std::function<void(const ParameterGaugeRequest&)>& draw;
    EnemyGaugeHost(MissionEntities& w,QuestCameraHost& c,const std::function<void(const ParameterGaugeRequest&)>& d):world(w),camera(c),draw(d){}
    const EnemyRecord* gauge_enemy_record(ComponentSlots& e)override{
        const auto& link=required(dynamic_cast<EnemyRecordLink*>(e.get(24)));
        const auto records=world.enemy_records();const auto& row=required(records.get()).at(static_cast<std::size_t>(link.enemy_id));
        return row?&*row:nullptr;
    }
    std::int32_t gauge_enemy_defeats(const EnemyRecord& r)override{return world.saved_enemy_defeats(r.id);}
    GaugePoint gauge_screen_position(const ContactPosition* p)override{
        if(!p)return {};
        const auto x=factory_cell_coordinate(render_isometric_x(p->x,p->z),1);
        const auto y=factory_cell_coordinate(render_isometric_y(p->x,p->z)-p->y,1);
        const auto& form=required(camera.current_camera_form());
        const auto sub=[](std::int32_t a,std::int32_t b){const auto v=static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b);return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);};
        return {sub(x,form.x),sub(y,form.y)};
    }
    std::int32_t enemy_gauge_palette(std::int32_t attribute)override{
        static constexpr std::array<std::int32_t,5> ids{{1,5,6,1,1}}; // EnemySystem..cctor 0x11517e8.
        return ids.at(static_cast<std::size_t>(attribute));
    }
    void parameter_gauge(const ParameterGaugeRequest& r)override{draw(r);}
};
}
MissionScene::MissionScene(MissionEntities& world,MissionApplication& app,EntitySubset& visible,MissionView& view,MapComponent* map,bool defer)
    :world_(world),app_(app),visible_(visible),view_(view),map_(map),form_{form_state_,nullptr,this}{
    if(!defer)for(const auto id:{21,19,11,12,31,2,16,30,22,25,26,13,18,15,9,14,23})initialize_system(id);
}
void MissionScene::initialize_system(std::int32_t id){
    MovementWorld movement(world_);
    switch(id){
    case 2:initialize_battle(battle_,&movement);break;
    case 9:initialize_parameter_experience(experience_,this);break;
    case 11:initialize_movement(movers_,&movement);break;
    case 12:initialize_collisions(collisions_,&movement);break;
    case 13:initialize_rotation(rotating_,&movement);break;
    case 14:initialize_parameters(this);break;
    case 15:initialize_emitters(emitters_,&movement);break;
    case 16:initialize_bubbles(bubbles_,this);break;
    case 18:initialize_modifications(modifications_,&movement);break;
    case 19:initialize_animation(animations_,&world_);break;
    case 21:initialize_render(renderer_,this);break;
    case 22:initialize_gauges(gauges_,this);break;
    case 23:initialize_effects(effects_,this);break;
    case 25:initialize_info(info_,this,*this);break;
    case 26:initialize_buttons(buttons_,this,*this);break;
    case 30:initialize_debug_visualization(debug_visualization_,this);break;
    case 31:initialize_garbage(garbage_entities_,&movement);break;
    default:throw std::invalid_argument("unsupported mission scene initializer");
    }
}
QuestCameraHost& MissionScene::gauge_camera_host(){return spatial_?spatial_->camera_host():*this;}
void MissionScene::draw_enemy_gauges(const std::function<void(const ParameterGaugeRequest&)>& draw){
    EnemyGaugeHost host(world_,gauge_camera_host(),draw);core::draw_enemy_gauges(required(world_.subset({24})),host);
}
void MissionScene::subscribe_dropoff(EntityEvents::Single receiver,std::optional<EventHashes> hashes){world_.events().subscribe(16,std::move(receiver),hashes);}
void MissionScene::subscribe_parameter_experience(ParameterExperienceInitializationWorld::Receiver receiver,std::optional<EventHashes> hashes){
    // Fresh scene Init has one event21 receiver. General typed EventManager removal/restoration is unbound.
    if(hashes||experience_receiver_)throw std::logic_error("parameter experience requires fresh scene initialization");experience_receiver_=std::move(receiver);
}
void MissionScene::send_parameter_experience(ComponentSlots* e,std::int32_t key){experience_receiver_(e,key);}
void MissionScene::advance_battle(std::int32_t pass,bool enabled){if(gameplay_needs_update(2,enabled,pass)){update_battle(battle_);++battle_steps_;}}
void MissionScene::advance_bubbles(std::int32_t pass,bool enabled){if(gameplay_needs_update(16,enabled,pass)){update_bubbles(bubbles_,*this);++bubble_steps_;}}
void MissionScene::advance_gauges(GaugeDrawState& scratch,GaugeDrawingHost& images,std::int32_t pass,bool enabled){
    if(gameplay_needs_update(22,enabled,pass)){update_gauges(gauges_,scratch,*this,images);++gauge_steps_;}
}
void MissionScene::advance_debug(bool application_debug,std::int32_t pass,bool enabled){
    if(gameplay_needs_update(30,enabled,pass)){update_debug_visualization(debug_visualization_,application_debug);++debug_steps_;}
} // SAGA_STUB(mission-debug-draw): native Update is inert; diagnostic overlays/path drawing remain unbound.
void MissionScene::advance_buttons(std::int32_t pass,bool enabled){if(gameplay_needs_update(26,enabled,pass)){update_buttons(buttons_,*this);++button_steps_;}}
void MissionScene::deliver_button_touch(const InputTouch* touch){required(button_touch_receiver_?&button_touch_receiver_:nullptr)(touch);}
void MissionScene::send_button_event(const InputTouch* touch,ComponentSlots* target){
    info_button_(touch,target);++info_button_events_; // Original Info receiver deliberately ignores the target.
    const auto receivers=auxiliary_buttons_;if(receivers)for(const auto& f:*receivers)f.callback(touch,target);
    last_button_target_=target;last_button_event_type_=required(touch).type;++button_events_;
}
void MissionScene::observe_button_event(std::int32_t factory,AuxiliaryInitializationWorld::Button receiver){
    auto next=auxiliary_buttons_?std::make_shared<std::vector<ButtonObserver>>(*auxiliary_buttons_):std::make_shared<std::vector<ButtonObserver>>();
    next->push_back({factory,std::move(receiver)});auxiliary_buttons_=std::move(next);
}
void MissionScene::remove_button_observer(std::int32_t factory){
    if(!auxiliary_buttons_)return;
    auto next=std::make_shared<std::vector<ButtonObserver>>(*auxiliary_buttons_);
    const auto it=std::find_if(next->begin(),next->end(),[=](const auto& entry){return entry.factory==factory;});
    if(it!=next->end())next->erase(it);
    auxiliary_buttons_=std::move(next);
}
void MissionScene::advance_parameter_experience(std::int32_t pass,bool enabled){if(gameplay_needs_update(9,enabled,pass)){update_parameter_experience(experience_);++experience_steps_;}}
void MissionScene::advance_parameters(std::int32_t pass,bool enabled){if(gameplay_needs_update(14,enabled,pass)){update_parameters();++parameter_steps_;}}
void MissionScene::advance_effects(std::int32_t pass,bool enabled){if(gameplay_needs_update(23,enabled,pass)){update_effects(effects_,*this);++effect_steps_;}}
void MissionScene::write_pass_report(std::ostream& out){
    out<<",\"gauge_steps\":"<<gauge_steps_<<",\"gauge_destroy_calls\":"<<gauge_destroy_calls_<<",\"gauge_pending\":"<<gauges_.pending->size();
    out<<",\"gauges\":[";bool first_gauge=true;auto gauge_cursor=world_.subset({19})->cursor();
    while(const auto item=gauge_cursor.next()){
        auto& e=required(*item);auto& b=required(dynamic_cast<EntityGauge*>(e.get(19)));auto& p=required(dynamic_cast<ContactPosition*>(e.get(0)));
        auto& c=required(dynamic_cast<CellComponent*>(e.get(5)));auto& a=required(dynamic_cast<EntityImage*>(e.get(7))).arrays;
        if(!first_gauge)out<<',';first_gauge=false;
        out<<"{\"entity\":"<<world_.id(e)<<",\"type\":"<<b.type<<",\"frame\":"<<b.frame<<",\"frame_max\":"<<b.frame_max
           <<",\"parent\":"<<(b.parent?world_.id(*b.parent):-1)<<",\"x\":"<<p.x<<",\"y\":"<<p.y<<",\"z\":"<<p.z<<",\"cell_xi\":"<<c.xi<<",\"cell_yi\":"<<c.yi;
        for(const auto& field:std::vector<std::pair<const char*,ImageArray>>{{"u",a.u},{"v",a.v},{"width",a.width},{"height",a.height}}){
            out<<",\""<<field.first<<"\":[";for(std::size_t i=0;i<field.second->size();++i){if(i)out<<',';out<<field.second->at(i);}out<<']';
        }out<<'}';
    }out<<']';
    out<<",\"bubble_steps\":"<<bubble_steps_<<",\"bubble_destroy_calls\":"<<bubble_destroy_calls_
       <<",\"bubble_pending\":"<<bubbles_.pending->size()<<",\"bubble_seen\":"<<bubbles_.seen->size()
       <<",\"debug_visualization_steps\":"<<debug_steps_<<",\"debug_visualization_flags\":"<<debug_visualization_.flags;
    out<<",\"bubbles\":[";bool first_bubble=true;auto bubble_cursor=world_.subset({18})->cursor();
    while(const auto item=bubble_cursor.next()){
        auto& e=required(*item);auto& b=required(dynamic_cast<EntityBubble*>(e.get(18)));auto& p=required(dynamic_cast<ContactPosition*>(e.get(0)));
        auto& c=required(dynamic_cast<CellComponent*>(e.get(5)));if(!first_bubble)out<<',';first_bubble=false;
        out<<"{\"entity\":"<<world_.id(e)<<",\"data_id\":"<<b.data_id<<",\"type\":"<<b.type<<",\"frame\":"<<b.frame<<",\"frame_max\":"<<b.frame_max
           <<",\"parent\":"<<(b.parent?world_.id(*b.parent):-1)<<",\"x\":"<<p.x<<",\"y\":"<<p.y<<",\"z\":"<<p.z<<",\"cell_xi\":"<<c.xi<<",\"cell_yi\":"<<c.yi<<'}';
    }out<<']';
    out<<",\"info_steps\":"<<info_steps_<<",\"info_clicks\":"<<info_clicks_<<",\"info_button_events\":"<<info_button_events_<<",\"info_popup_disposals\":"<<info_popup_disposals_
       <<",\"info_emphasize_frame\":"<<info_.emphasize_frame<<",\"info_emphasized_id\":"<<(info_.emphasized?info_.emphasized->id:-1)
       <<",\"info_offset_bottom\":"<<info_.offset_bottom<<",\"info_dialog_pending\":"<<(info_.dialog?"true":"false");
    out<<",\"info_popups\":[";
    for(std::int32_t i=0;i<info_.popups->size();++i){auto b=info_.popups->at(i);if(i)out<<',';
        out<<"{\"id\":"<<b->id<<",\"y\":"<<b->y<<",\"bottom\":"<<b->bottom<<",\"frame\":"<<b->frame<<",\"state_frame\":"<<b->state_frame<<",\"state\":"<<b->state<<'}';}out<<']';
    out<<",\"info_slides\":[";
    for(std::int32_t i=0;i<info_.slides->size();++i){auto b=info_.slides->at(i);if(i)out<<',';
        out<<"{\"id\":"<<b->id<<",\"x\":"<<b->x<<",\"y\":"<<b->y<<",\"frame\":"<<b->frame<<",\"state_frame\":"<<b->state_frame<<",\"state\":"<<b->state<<'}';}out<<']';
    out<<",\"button_steps\":"<<button_steps_<<",\"button_events\":"<<button_events_<<",\"button_removals\":"<<button_removals_
       <<",\"last_button_target\":"<<(last_button_target_?world_.id(*last_button_target_):-1)<<",\"last_button_event_type\":"<<last_button_event_type_;
    out<<",\"entity_buttons\":[";bool first_button=true;auto button_cursor=world_.subset({17})->cursor();
    while(const auto item=button_cursor.next()){
        auto& e=required(*item);auto& b=required(dynamic_cast<EntityButton*>(e.get(17)));
        auto& p=required(dynamic_cast<ContactPosition*>(e.get(0)));
        if(!first_button)out<<',';first_button=false;
        out<<"{\"entity\":"<<world_.id(e)<<",\"group\":"<<b.id<<",\"value\":"<<b.value<<",\"x\":"<<p.x<<",\"y\":"<<p.y
           <<",\"parent\":"<<(b.parent?world_.id(*b.parent):-1)<<",\"dynamic\":"<<(b.dynamic?"true":"false")<<",\"hover\":"<<(b.hover?"true":"false")<<'}';
    }out<<']';
    out<<",\"rotation_steps\":"<<rotation_steps_;
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
void MissionScene::advance_rotation(std::int32_t pass,bool enabled){
    if(gameplay_needs_update(13,enabled,pass)){rotate_subset(required(rotating_));++rotation_steps_;}
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
void MissionScene::bind_spatial(MissionSpatial& value,bool initialize){spatial_=&value;map_=&value.map();if(initialize)initialize_render(renderer_,this);}
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
