#include "enemy_frame.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing enemy frame input");return *p;}
template<class T>T* pointer(ComponentSlots& e,std::int32_t slot){return dynamic_cast<T*>(e.get(slot));}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(pointer<T>(e,slot));}
EnemyRecordLink& enemy(ComponentSlots& e){return component<EnemyRecordLink>(e,24);}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,0);}
ProjectileSpeed& speed(ComponentSlots& e){return component<ProjectileSpeed>(e,1);}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t remainder(std::int32_t a,std::int32_t b){if(b==0)return a;if(b==-1&&a==std::numeric_limits<std::int32_t>::min())return 0;return a%b;}
const EnemyRecord* record(EnemyRecordLink& link,EnemyFrameHost& host){
    const auto records=host.population_records();const auto& value=required(records.get()).at(static_cast<std::size_t>(link.enemy_id));return value?&*value:nullptr;
}
void enter_run_away(EnemyCreationSystem& s,ComponentSlots& e,EnemyFrameHost& host){
    if(e.contains(22)){auto* old=e.get(22);e.remove(22);host.release_enemy_garbage(old);}
    const auto frames=multiply(host.enemy_frames_per_second(),3);add_garbage_lifetime(e,frames,host);
    s.remaining_enemies=std::max(add(s.remaining_enemies,-1),1);change_enemy_state(e,5);
}
}
bool enemy_is_normal(const EnemyRecordLink& e){return (e.flags&3)==0;}
bool enemy_waits_for_players(const EnemyRecordLink& e){return (e.flags&3)!=0;}
bool enemy_is_alive(const EnemyRecordLink& e){return e.state!=4;}
bool enemy_is_dead(const EnemyRecordLink& e){return e.state==4;}
bool within_enemy_activation_circle(ProjectilePoint first,ProjectilePoint second,float radius){
    const float rr=radius*radius,dx=second.x-first.x,xx=dx*dx,dz=second.z-first.z,zz=dz*dz;
    const float distance=xx+zz;return distance<=rr;
}
void update_enemy_walking(EnemyCreationSystem& s,ComponentSlots* input,EnemyFrameHost& host){
    auto& e=required(input);auto* data=record(enemy(e),host);const auto level=enemy(e).level;
    const auto amount=enemy_move_speed(required(data),level,host.frame_enemy_maximum_levels());speed(e).z=amount;
    auto* target=enemy(e).opponent;
    if(target&&!host.target_destroyed(*target)){
        auto& live_target=required(enemy(e).opponent);const auto target_z=position(live_target).z,enemy_z=position(e).z;
        if(std::fabs(target_z-enemy_z)<150.0F){
            auto* output=pointer<ProjectileSpeed>(e,1);
            const auto target_x=position(required(enemy(e).opponent)).x,enemy_x=position(e).x;
            required(output).x=(target_x-enemy_x)/30.0F;
        }else speed(e).x=0;
    }else speed(e).x=0;
    if((enemy(e).flags&1)!=0){
        required(s.members);auto* hero=member_hero(host.enemy_world());const auto z=position(e).z;
        const float limit=position(required(hero)).z+10.0F;
        if(z>limit){auto* output=pointer<ContactPosition>(e,0);const auto hero_z=position(*hero).z;required(output).z=hero_z;}
    }
}
void update_enemy_bounding(EnemyCreationSystem& s,ComponentSlots* input,EnemyFrameHost& host){
    auto& e=required(input);auto& velocity=speed(e);velocity.z=velocity.z*0.8F;
    if(!(std::fabs(speed(e).z)<0.1F))return;
    if(e.contains(12))component<ContactDirection>(e,12).locked=false;
    speed(e).z=0;
    if((enemy(e).flags&1)!=0){change_enemy_state(e,1);return;}
    if(required(s.stage).collection.boss_defeated){enter_run_away(s,e,host);return;}
    const auto& data=required(record(enemy(e),host));
    if((data.flags&2)!=0&&random_hits(host.population_random(),20)){enter_run_away(s,e,host);return;}
    change_enemy_state(e,1);
}
void update_enemy_run_away(ComponentSlots* input){speed(required(input)).z=-10.0F;}
void update_enemies(EnemyCreationSystem& s,EnemyFrameHost& host){
    auto& group=update_enemy_population(s,host);auto cursor=group.cursor();
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);auto* captured=pointer<EnemyRecordLink>(e,24);
        auto& hp=required(resolve_combat_parameter(e,0));
        if(add(hp.value,hp.extra)>=1&&damage_parameter_rate(hp)<=99){
            const auto frame=required(captured).frame;const auto divisor=add(damage_parameter_rate(hp),2);
            const auto secondary=remainder(frame,divisor)==0?0:255;
            component<DefeatColor>(e,6).values={{255,secondary,secondary,255}};
        }
        const auto state=required(captured).state;
        if(state==1)update_enemy_walking(s,&e,host);
        else if(state==3)update_enemy_bounding(s,&e,host);
        else if(state==5)update_enemy_run_away(&e);
        // Native waiting/battling/flying-away handlers are empty; dispatch does no work.
        captured->frame=add(captured->frame,1)%std::numeric_limits<std::int32_t>::max();
        if(enemy_is_normal(*captured)){
            const auto height=required(host.enemy_map()).height;
            const auto step=required(host.enemy_map()).step_z;
            const auto z=position(e).z;const float edge=static_cast<float>(multiply(add(height,-2),step));
            if(z>edge&&add(hp.value,hp.extra)>=1&&!e.contains(15)){
                AnimationModificationRequest request;request.type=5;request.destroy=true;
                add_animation_modification(e,request,host.enemy_modifications());
            }
        }else if(enemy_waits_for_players(*captured)&&(captured->flags&4)==0){
            auto& hero=required(world_player_entity(required(host.enemy_world())));
            auto* hero_position=pointer<ContactPosition>(hero,0);auto* entity_position=pointer<ContactPosition>(e,0);
            const auto& h=required(hero_position);const ProjectilePoint hero_point{h.x,h.y,h.z};
            const auto& p=required(entity_position);const ProjectilePoint entity_point{p.x,p.y,p.z};
            if(!within_enemy_activation_circle(hero_point,entity_point,250.0F))continue;
            auto* output=pointer<ProjectileSpeed>(e,1);auto* data=record(enemy(e),host);const auto level=enemy(e).level;
            const auto amount=enemy_move_speed(required(data),level,host.frame_enemy_maximum_levels());
            required(output).z=amount;
            auto& live=enemy(e);live.flags=word(static_cast<std::uint32_t>(live.flags)|4U);change_enemy_state(e,1);
        }
    }
}
} // namespace saga::core
