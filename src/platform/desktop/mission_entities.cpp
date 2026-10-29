#include "mission_entities.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mission entity input");return *p;}
template<class T>T& acquire(std::deque<T>& pool){pool.emplace_back();return pool.back();}
}
MissionEntities::MissionEntities(const GameplayCatalog& data,MissionProgress& saved,std::int32_t slots,StageGeometry* map,Resources resources,ResourceIds names)
    :catalog_(data),progress_(saved),slots_(slots),map_(map),resources_(std::move(resources)),resource_ids_(std::move(names)),
     groups_(registry_,[this](const ComponentSlots& e){return id(e);}),bullet_world_{ids_,static_cast<BulletFactoryHost&>(*this)},effect_world_{ids_,static_cast<EffectFactoryHost&>(*this)},enemy_world_{ids_,static_cast<EnemyFactoryHost&>(*this)},
     party_rows_(data.characters.size()),party_(std::make_shared<std::vector<PartyCharacter*>>(data.characters.size(),nullptr)),
     hat_rows_(data.hats.size()),hats_(std::make_shared<std::vector<HumanHat*>>(data.hats.size(),nullptr)),shop_(data.shop.size()){
    if(!resources_||!resource_ids_)throw std::invalid_argument("mission entities need resource providers");
    for(std::size_t i=0;i<data.hats.size();++i)if(data.hats[i]){
        hat_rows_[i]=HumanHat{data.hats[i]->flags,data.hats[i]->image};hats_->at(i)=&*hat_rows_[i];
    }
}
ComponentSlots& MissionEntities::human(const HumanCreation& args){
    if(progress_.characters.find(args.character)==progress_.characters.end())throw std::invalid_argument("unbound saved character state: "+std::to_string(args.character));
    return create_human(args,ids_,*this);
}
ComponentSlots& MissionEntities::by_handle(QuestMemberId key){return handles_.at(static_cast<std::size_t>(key))->components;}
ComponentSlots* MissionEntities::enemy(const EnemyCreation& args){return create_enemy(args,ids_,*this);}
ComponentSlots& MissionEntities::bullet(const BulletCreation& args){return create_bullet(args,*this);}
ComponentSlots& MissionEntities::effect(ProjectilePoint p,std::int32_t clip,std::int32_t image,bool loop){return create_visual_effect(p,clip,image,loop,ids_,*this);}
ComponentSlots* MissionEntities::drop(std::int32_t type,std::int32_t value,ProjectilePoint at,std::int32_t scale){
    if(!catalog_.treasures)throw std::invalid_argument("reward scene requires the Treasure catalog");
    return create_drop(type,value,at,scale,*catalog_.treasures,ids_,*this);
}
const TreasureRecord& MissionEntities::treasure(std::int32_t id)const{
    if(!catalog_.treasures)throw std::invalid_argument("collection requires the Treasure catalog");
    const auto& row=catalog_.treasures->at(static_cast<std::size_t>(id));return required(row?&*row:nullptr);
}
ComponentSlots& MissionEntities::emitter(EmitterCreation args){return create_emitter(args,*this);}
DropComponent& MissionEntities::acquire_drop(){return acquire(drops_);}
ComponentSlots& MissionEntities::raw_entity(std::int32_t type){return create_entity(ids_,type,false,static_cast<BulletFactoryHost&>(*this));}
ComponentSlots& MissionEntities::button(const ButtonCreation& args){return create_button(args,ids_,*this);}
EntityButton& MissionEntities::acquire_button(){return acquire(buttons_);}
EntityBubble& MissionEntities::acquire_bubble(){return acquire(bubbles_);}
EntityGauge& MissionEntities::acquire_gauge(){return acquire(gauges_);}
ComponentSlots& MissionEntities::gauge(GaugeCreation args,GaugeDrawState& scratch,GaugeDrawingHost& images){
    return create_gauge(args,ids_,*this,scratch,images);
}
ComponentSlots& MissionEntities::bubble(ComponentSlots* parent,std::int32_t data_id,std::int32_t type,
                                      std::int32_t maximum,std::int32_t resource_id,std::int32_t clip,ProjectilePoint initial){
    // Explicit diagnostic composition, not a recovered native bubble producer.
    auto& e=create_z_object(initial,resource_id,clip,ids_,static_cast<EffectFactoryHost&>(*this));
    return add_bubble(e,data_id,type,parent,maximum,*this);
}
MapComponent& MissionEntities::create_initial_map(){
    core::create_map(initial_mission_map_creation(),ids_,static_cast<MapFactoryHost&>(*this));
    map_=&map_component_;return map_component_;
}
ComponentSlots* MissionEntities::create_camera(){return &create_quest_camera(ids_,*this);}
QuestCameraComponent& MissionEntities::allocate_camera_component(){return acquire(cameras_);}
MapChipComponent& MissionEntities::acquire_map_chip(){return acquire(map_chips_);}
ProjectileLink& MissionEntities::acquire_projectile(){return acquire(projectiles_);}
ProjectileModification& MissionEntities::acquire_modification(){return acquire(modifications_);}
void MissionEntities::return_component(std::int32_t slot,Component* value){retired_.emplace_back(slot,value);}
void MissionEntities::add_animation(ComponentSlots& e,std::int32_t rate,std::int32_t backup){add_animation_component(e,rate,backup,static_cast<EffectFactoryHost&>(*this));}
std::int32_t MissionEntities::id(const ComponentSlots& e)const{return node_index_.at(&e)->id;}
QuestMemberId MissionEntities::handle(const ComponentSlots& e)const{return node_index_.at(&e)->handle;}
bool MissionEntities::destroyed(const ComponentSlots& e)const{return node_index_.at(&e)->lifetime.destroyed;}
void MissionEntities::destroy(ComponentSlots& e,bool remove){destroy_quest_entity(handle(e),remove,*this);}
QuestEntityLifetime& MissionEntities::entity_lifetime(QuestMemberId key){return handles_.at(static_cast<std::size_t>(key))->lifetime;}
void MissionEntities::delete_from_manager(QuestMemberId key,bool remove){delete_entity_from_manager(key,remove,*this);}
void MissionEntities::remove_all_components(QuestMemberId key){handles_.at(static_cast<std::size_t>(key))->components.remove_all();}
void MissionEntities::dispose_entity_events(QuestMemberId key){handles_.at(static_cast<std::size_t>(key))->events.dispose();}
void MissionEntities::erase_registration(QuestMemberId key){registry_.erase(handles_.at(static_cast<std::size_t>(key))->id);}
EntitySubset* MissionEntities::subset(const std::vector<std::int32_t>& all){return &groups_.get(std::make_shared<ComponentGroupFilter>(all));}
EntitySubset* MissionEntities::subset(const std::vector<std::int32_t>& all,const std::vector<std::int32_t>& none){return &groups_.get(std::make_shared<ComponentGroupFilter>(all,none));}
AnimationResource* MissionEntities::resource(std::int32_t id){return resources_(id);}
bool MissionEntities::registered_id(std::int32_t value){return registry_.contains(value);}
ComponentSlots* MissionEntities::create(std::int32_t type,std::int32_t value){
    if(type!=0)throw std::invalid_argument("mission factory entity type is not bound");
    nodes_.emplace_back(value,static_cast<QuestMemberId>(handles_.size()),slots_);auto& n=nodes_.back();
    handles_.push_back(&n);node_index_.emplace(&n.components,&n);return &n.components;
}
void MissionEntities::register_entity(ComponentSlots& e,bool temporary){auto& n=*node_index_.at(&e);EntityRegistration r{n.id,&e,&n.events};registry_.add(&r,temporary);}
void MissionEntities::added(ComponentSlots& e,std::int32_t slot,Component* c){groups_.added(e,slot,c);}
void MissionEntities::removed(ComponentSlots& e,std::int32_t slot,Component* c){groups_.removed(e,slot,c);}
void MissionEntities::changed(ComponentSlots& e,std::int32_t slot,Component* a,Component* b){groups_.changed(e,slot,a,b);}
PartyCharacters MissionEntities::character_table(){
    std::fill(party_->begin(),party_->end(),nullptr); // A removed binding must not expose its previous saved state.
    for(const auto& entry:progress_.characters){const auto index=static_cast<std::size_t>(entry.first);const auto& record=catalog_.characters.at(index);
        if(!record)throw std::invalid_argument("missing mission character row");party_rows_.at(index)=bind_party_character(*record,entry.second);party_->at(index)=&*party_rows_.at(index);}
    return party_;
}
std::int32_t MissionEntities::weapon_image(std::int32_t value){const auto& row=catalog_.equipment.at(static_cast<std::size_t>(value));if(!row)throw std::invalid_argument("missing mission weapon");return row->image;}
std::int32_t MissionEntities::maximum_level(std::int32_t value){return maxima_.at(static_cast<std::size_t>(value));}
const ShopParameterUpgrade& MissionEntities::shop_upgrade(std::int32_t value){
    const auto index=static_cast<std::size_t>(value);const auto& row=catalog_.shop.at(index);if(!row)throw std::invalid_argument("missing mission shop row");
    shop_.at(index)=bind_shop_upgrade(*row,progress_.shop_purchases.at(value));return *shop_.at(index);
}
std::int32_t MissionEntities::enemy_resource_id(std::string_view name){return resource_ids_(name);}
std::int32_t MissionEntities::clip_max_frame(std::int32_t id,std::int32_t clip){
    auto& r=required(resource(id));return required(required(r.clips.get()).at(static_cast<std::size_t>(clip))).max_frame;
}
void MissionEntities::release_collision(Component* p){if(p)throw std::logic_error("mission collision recycling is not bound");}
ContactPosition& MissionEntities::acquire_position(){return acquire(positions_);} ProjectileSpeed& MissionEntities::acquire_speed(){return acquire(speeds_);}
EntitySprite& MissionEntities::acquire_sprite(){return acquire(sprites_);} EntityDepth& MissionEntities::acquire_depth(){return acquire(depths_);}
CellComponent& MissionEntities::acquire_cell(){return acquire(cells_);} EntityImage& MissionEntities::acquire_image(){return acquire(images_);}
EntityPlayback& MissionEntities::acquire_playback(){return acquire(playback_);} HumanRecordLink& MissionEntities::acquire_human(){return acquire(humans_);}
CombatParameterLinks& MissionEntities::acquire_parameters(){return acquire(parameters_);} EquipmentRecordLinks& MissionEntities::acquire_equipment(){return acquire(equipment_);}
ContactDirection& MissionEntities::acquire_direction(){return acquire(directions_);} HumanAI& MissionEntities::acquire_ai(){return acquire(ai_);}
Component& MissionEntities::acquire_input(){return acquire(inputs_);} ProjectileCollision& MissionEntities::acquire_collision(){return acquire(collisions_);}
EnemyRecordLink& MissionEntities::acquire_enemy(){return acquire(enemies_);} DefeatColor& MissionEntities::acquire_enemy_color(){return acquire(colors_);}
BulletComponent& MissionEntities::acquire_bullet(){return acquire(bullets_);} EmitterComponent& MissionEntities::acquire_emitter(){return acquire(emitters_);}
ProjectileDamage& MissionEntities::acquire_bullet_damage(){return acquire(damages_);} EffectComponent& MissionEntities::acquire_effect(){return acquire(effects_);}
GarbageLifetime& MissionEntities::acquire_garbage(){return acquire(garbage_);}
} // namespace saga::desktop
