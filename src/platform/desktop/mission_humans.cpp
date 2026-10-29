#include "mission_humans.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mission human input");return *p;}
template<class T>T& field(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
EventIntegers levels(const EquipmentCombatTable& table,const std::map<std::int32_t,std::int32_t>& saved){
    EventIntegers result(table.size(),0);for(std::size_t i=0;i<table.size();++i)if(table[i])result[i]=saved.at(static_cast<std::int32_t>(i));return result;
}
[[noreturn]]void unbound(const char* service){throw std::logic_error(std::string("mission service unbound: ")+service);}
}
MissionHumans::MissionHumans(MissionEntities& world,const GameplayCatalog& data,MissionProgress& saved,StageGeometry& geometry,MissionRuntimeConfig runtime,MissionHumanScenario scenario)
    :catalog_(data),world_(world),saved_(saved),geometry_(geometry),runtime_(std::move(runtime)),random_(runtime_.seed),
     records_{data.characters,required(data.enemies.get()),data.equipment,data.hats,data.cloth},characters_(data.characters.size(),0),
     equipment_(levels(data.equipment,saved.equipment_levels)),hats_(levels(data.hats,saved.hat_levels)),cloth_(levels(data.cloth,saved.cloth_levels)){
    if(runtime_.backend!="system"||runtime_.fps<1)throw std::invalid_argument("mission human runtime requires positive FPS and system RNG backend");
    for(const auto& entry:saved.characters)characters_.at(static_cast<std::size_t>(entry.first))=entry.second.level;
    table_quest_=static_cast<bool>(scenario.quest);
    stage_.quest=scenario.quest?std::move(scenario.quest):std::make_shared<StageQuest>();
    if(!table_quest_)stage_.quest->distance=scenario.distance;
    stage_.min_z_rate=scenario.min_z_rate;stage_.max_z_rate=scenario.max_z_rate;stage_.attack_attribute=scenario.attribute;
    button_.state=scenario.button;stage_.attack_buttons=std::make_shared<std::vector<AttackButtonState*>>(1,&button_);
    enemies_.subset=world_.subset({24});stage_.members=&members_;stage_.humans=&system_;system_.transitions=static_cast<HumanFrameHost*>(this);
    auto cursor=required(world_.subset({14})).cursor();while(const auto actor=cursor.next()){
        static_cast<void>(saved.characters.at(field<HumanRecordLink>(**actor,14).character_id)); // Require saved state for every live actor.
        add_member(members_,*actor);
    }
    world_.events().subscribe(30,[this](ComponentSlots* e){stage_human_dead(required(e),records_,*this);});
    initialize_humans(system_,this,services_);
    // Native receiver order: Human, Enemy, Projectile (event23); Human, Enemy (event29).
    initialize_enemies(enemies_,this,enemy_services_);
    initialize_projectiles(projectiles_,this,*this);
}
void MissionHumans::update(std::int32_t pass,bool enabled){if(gameplay_needs_update(4,enabled,pass)){
    // Refresh preallocated projections after native result gear upgrades; saved maps remain canonical.
    const auto sync=[](auto& output,const auto& records,const auto& saved){for(std::size_t i=0;i<records.size();++i)if(records[i])output.at(i)=saved.at(static_cast<std::int32_t>(i));};
    sync(equipment_,catalog_.equipment,saved_.equipment_levels);sync(hats_,catalog_.hats,saved_.hat_levels);sync(cloth_,catalog_.cloth,saved_.cloth_levels);
    update_humans(system_,*this);
}}
void MissionHumans::set_state(ComponentSlots& e,std::int32_t state,float speed_z){
    change_human_state(e,state,static_cast<HumanFrameHost&>(*this));field<ProjectileSpeed>(e,1).z=speed_z;
}
std::int32_t MissionHumans::human_parameter(ComponentSlots& e,std::int32_t key){
    const auto id=field<HumanRecordLink>(e,14).character_id;
    characters_.at(static_cast<std::size_t>(id))=saved_.characters.at(id).level;
    return ecs_member_total_parameter(e,key,records_,levels_,world_);
}
bool MissionHumans::playing_cut_in(ComponentSlots& e){return field<EntityPlayback>(e,10).state.return_clip!=-1;}
void MissionHumans::change_animation(ComponentSlots& e,std::int32_t clip,bool reset){core::change_animation(live_animation(e),clip,reset);}
std::int32_t MissionHumans::celebration_frame_period(){return world_.clip_max_frame(1,0);}
void MissionHumans::remove_human_modification(ComponentSlots& e){e.remove(15);} // Detached objects remain owned; pool reuse is unbound.
void MissionHumans::subscribe_human_impact(EntityEvents::Single receiver,std::optional<EventHashes> hash){world_.events().subscribe(23,std::move(receiver),hash);}
void MissionHumans::subscribe_human_contact(EntityEvents::Pair receiver,std::optional<EventHashes> hash){world_.events().subscribe(29,std::move(receiver),hash);}
ComponentSlots& MissionHumans::hero(){return required(member_hero(human_world()));}
void MissionHumans::cut_in_animation(ComponentSlots& e,std::int32_t clip,std::int32_t backup){core::cut_in_animation(live_animation(e),clip,backup);}
bool MissionHumans::shoot_human(ComponentSlots& actor,ComponentSlots& target){
    const bool made=core::shoot_human(system_,&actor,&target,*this,*this);if(made)++shots_;return made;
}
void MissionHumans::contact_effect(std::int32_t clip,std::int32_t image,std::int32_t loop,float x,float y,float z){world_.effect({x,y,z},clip,image,loop!=0);}
void MissionHumans::impact_effect(std::int32_t clip,std::int32_t image,std::int32_t loop,float x,float y,float z){world_.effect({x,y,z},clip,image,loop!=0);}
void MissionHumans::send_human_dead(ComponentSlots& e){
    world_.events().send(30,&e);
}
DropPlacementWorld* MissionHumans::drop_world(){unbound("stage drops");}
} // namespace saga::desktop
