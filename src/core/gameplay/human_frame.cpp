#include "human_frame.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing human frame input");return *p;}
template<class T>T* pointer(ComponentSlots& e,std::int32_t slot){return dynamic_cast<T*>(e.get(slot));}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(pointer<T>(e,slot));}
HumanRecordLink& human(ComponentSlots& e){return component<HumanRecordLink>(e,14);}
EnemyRecordLink& enemy(ComponentSlots& e){return component<EnemyRecordLink>(e,24);}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,0);}
ProjectileSpeed& speed(ComponentSlots& e){return component<ProjectileSpeed>(e,1);}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t negate(std::int32_t a){return word(0U-static_cast<std::uint32_t>(a));}
std::int32_t divide(std::int32_t a,std::int32_t b){if(!b)return 0;if(b==-1&&a==std::numeric_limits<std::int32_t>::min())return a;return a/b;}
std::int32_t remainder(std::int32_t a,std::int32_t b){if(!b)return a;if(b==-1&&a==std::numeric_limits<std::int32_t>::min())return 0;return a%b;}
std::int32_t total(const DamageParameter& p){return add(p.value,p.extra);}
std::int32_t maximum(const DamageParameter& p){return add(p.maximum,p.extra_maximum);}
std::int32_t sign(float value){if(std::isnan(value))throw std::domain_error("Math.Sign NaN");return value>0?1:value<0?-1:0;}
const CharacterProgressionRecord* data(ComponentSlots& e,HumanFrameHost& host){return resolve_combat_character(e,host.human_records());}
const EquipmentCombatRecord& weapon(ComponentSlots& e,HumanFrameHost& host){return required(resolve_combat_equipment(e,EquipmentSlot::weapon,host.human_records()));}
std::int32_t parameter(HumanFrameSystem& s,ComponentSlots& e,std::int32_t key,HumanFrameHost& host){required(s.members);return host.human_parameter(e,key);}
std::int32_t attribute(HumanFrameSystem& s,ComponentSlots& e,std::int32_t key,HumanFrameHost& host){required(s.members);return static_cast<std::uint32_t>(key)<=2U?host.human_parameter(e,key+10):0;}
float squared_distance(ContactPosition& a,ContactPosition& b){
    const float dx=a.x-b.x,dy=a.y-b.y,dz=a.z-b.z,xx=dx*dx,yy=dy*dy,zz=dz*dz,xy=xx+yy;return xy+zz;
}
}
void accelerate_humans(HumanFrameSystem& s){
    auto cursor=required(s.subset).cursor();while(const auto entry=cursor.next()){
        auto& h=human(required(*entry));if((static_cast<std::uint32_t>(h.state)&0xfffffffeU)==4U)continue;
        const float candidate=h.dash_speed+0.1F;h.dash_speed=candidate>2.0F?2.0F:candidate;
    }
}
void reset_human_speeds(HumanFrameSystem& s){
    auto cursor=required(s.subset).cursor();while(const auto entry=cursor.next())human(required(*entry)).dash_speed=0;
}
bool stage_attack_active(const StageStartState& s){
    const auto buttons=s.attack_buttons;for(auto* b:required(buttons.get()))if(required(b).state==1)return true;return false;
}
ComponentSlots* nearest_attack_enemy(EnemyCreationSystem& s,ContactPosition* origin,std::int32_t distance){
    auto& group=required(s.subset);auto cursor=group.cursor();
    const float radius=static_cast<float>(multiply(distance,28)),limit=radius*radius;
    float best=std::numeric_limits<float>::max();ComponentSlots* selected=nullptr;
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);if(enemy_is_dead(enemy(e)))continue;
        if(total(required(resolve_combat_parameter(e,0)))<1)continue;
        auto* p=pointer<ContactPosition>(e,0);const auto d=squared_distance(required(origin),required(p));
        if(d<=limit&&d<best){selected=&e;best=d;}
    }return selected;
}
std::int32_t human_attack_attribute(HumanFrameSystem& s,const CharacterProgressionRecord* input,HumanFrameHost& host){
    const auto& c=required(input);if((c.flags&2)!=0)return required(s.stage).attack_attribute;
    const auto& row=host.human_records().equipment.at(static_cast<std::size_t>(c.weapon_id));return required(row?&*row:nullptr).attribute;
}
std::int32_t human_mp_cost(HumanFrameSystem& s,ComponentSlots* input,std::int32_t key,HumanFrameHost& host){
    auto& e=required(input);const auto base=weapon(e,host).use_mp;
    return add(base,attribute(s,e,key,host)/4);
}
bool human_has_mp(HumanFrameSystem& s,ComponentSlots* input,std::int32_t key,HumanFrameHost& host){
    auto& e=required(input);auto& mp=required(resolve_combat_parameter(e,1));const auto cap=maximum(mp),value=total(mp);
    const auto cost=human_mp_cost(s,&e,key,host);return (cost>cap&&value>=cap)||value>=cost;
}
void use_human_mp(ComponentSlots* input,HumanFrameHost& host){
    auto& e=required(input);auto* mp=resolve_combat_parameter(e,1);const auto amount=weapon(e,host).use_mp;
    auto& value=required(mp);value.value=subtract_parameter(value.value,value.maximum,amount).value;
}
std::int32_t human_attack_distance(HumanFrameSystem& s,ComponentSlots* input,HumanFrameHost& host){
    auto& e=required(input);const auto base=weapon(e,host).distance;
    const auto key=human_attack_attribute(s,data(e,host),host);
    return std::min(add(base,attribute(s,e,key,host)/30),12);
}
std::int32_t human_burst_count(HumanFrameSystem& s,ComponentSlots* input,HumanFrameHost& host){
    auto& e=required(input);const auto base=weapon(e,host).burst_count;
    const auto key=human_attack_attribute(s,data(e,host),host);
    return std::min(add(base,attribute(s,e,key,host)/20),10);
}
std::int32_t human_attack_speed(HumanFrameSystem& s,ComponentSlots* input,HumanFrameHost& host){return add(parameter(s,required(input),6,host),100)/2;}
std::int32_t human_attack_interval(HumanFrameSystem& s,ComponentSlots* e,HumanFrameHost& host){
    const auto fps=host.human_frames_per_second();return std::max(divide(multiply(fps,100),human_attack_speed(s,e,host)),2);
}
std::int32_t human_burst_wait(HumanFrameSystem& s,ComponentSlots* e,HumanFrameHost& host){
    const auto fps=host.human_frames_per_second();return std::max(divide(multiply(fps,10),human_attack_speed(s,e,host)),2);
}
float human_move_speed(HumanFrameSystem& s,ComponentSlots* e,HumanFrameHost& host){
    const auto p=add(parameter(s,required(e),5,host),100)/2;const float scaled=static_cast<float>(p)*2.23F;return scaled/100.0F;
}
bool search_human_attack(HumanFrameSystem& s,ComponentSlots* input,HumanFrameHost& host){
    if(!stage_attack_active(required(s.stage)))return false;
    auto& e=required(input);const auto key=human_attack_attribute(s,data(e,host),host);
    if(!human_has_mp(s,&e,key,host))return false;
    auto* system=s.enemies;auto* p=pointer<ContactPosition>(e,0);const auto distance=human_attack_distance(s,&e,host);
    auto* target=nearest_attack_enemy(required(system),p,distance);
    if(!target||host.human_target_destroyed(*target)||enemy_is_dead(enemy(*target))||enemy(*target).state==5)return false;
    if((required(resolve_combat_enemy(*target,host.human_records())).flags&1)!=0)return false;
    human(e).opponent=target;auto& v=speed(e);v.x=0;v.y=0;v.z=0;change_human_state(e,2,host);return true;
}
bool move_human_base(HumanFrameSystem& s,ComponentSlots* input,float x,float z,float amount,HumanFrameHost& host){
    if(amount>=0.0F&&amount<0.1F)amount=human_move_speed(s,input,host);
    auto& e=required(input);auto* p=pointer<ContactPosition>(e,0);
    if(amount<0){required(p).x=x;position(e).z=z;return true;}
    const auto px=required(p).x,pz=position(e).z;
    const float dx=x-px,dz=z-pz,xx=dx*dx,zz=dz*dz,d=xx+zz,limit=amount*amount;
    if(d<=limit){position(e).x=x;position(e).z=z;return true;}
    const auto length=std::sqrt(d);if(length<amount)amount=length;
    speed(e).x=(dx*amount)/length;speed(e).z=(dz*amount)/length;return false;
}
bool move_human(HumanFrameSystem& s,ComponentSlots* e,float x,float z,float amount,HumanFrameHost& host){
    if(amount<=0.0F)amount=human_move_speed(s,e,host);return move_human_base(s,e,x,z,amount,host);
}
void update_human_waiting(HumanFrameSystem& s,ComponentSlots* input,HumanFrameHost& host){
    if(search_human_attack(s,input,host))return;auto& e=required(input);
    if((required(data(e,host)).flags&4)==0)return;
    required(s.members);const auto x=host.human_formation_x(e);required(s.members);const auto z=host.human_formation_z(e);
    const auto px=position(e).x,pz=position(e).z;const float dx=x-px,dz=z-pz,xx=dx*dx,zz=dz*dz,d=xx+zz;
    if(d>0.0F)change_human_state(e,1,host);
}
std::int32_t add_human_parameter(DamageParameter& p,std::int32_t amount){
    const auto result=subtract_parameter(p.value,p.maximum,negate(amount));p.value=result.value;return result.remainder;
}
void update_human_walking(HumanFrameSystem& s,ComponentSlots* input,HumanFrameHost& host){
    required(s.members);auto& hero=required(member_hero(host.human_world()));const auto hero_vz=speed(hero).z;
    if(hero_vz==0.0F&&search_human_attack(s,input,host))return;
    auto& e=required(input);const auto dash=human(e).dash_speed;
    const auto clip=component<EntitySprite>(e,2).animation.clip;const auto group=word(static_cast<std::uint32_t>(clip)&0xfffffffcU);
    const auto animation=dash>0.0F?8:40;if(group!=animation)host.change_animation(e,animation,false);
    const auto* c=&required(data(e,host));
    if((c->flags&2)!=0){const auto amount=human_move_speed(s,&e,host);const auto direction=sign(speed(e).z);speed(e).z=amount*static_cast<float>(direction);}
    else if((c->flags&4)!=0){
        const auto amount=human_move_speed(s,&e,host);
        if(hero_vz!=0.0F){const auto direction=sign(speed(hero).z);speed(e).z=amount*static_cast<float>(direction);
            if(direction>=1&&position(e).z>position(hero).z)speed(e).z=0;
        }else{
            const auto px=position(e).x,pz=position(e).z;
            required(s.members);const auto x=host.human_formation_x(e);required(s.members);const auto z=host.human_formation_z(e);
            const float dx=x-px,dz=z-pz,xx=dx*dx,zz=dz*dz,d=xx+zz,limit=amount*amount;
            if(d<=limit){position(e).x=x;position(e).z=z;change_human_state(e,0,host);}
            else{const auto length=std::sqrt(d);const auto step=length<amount?length:amount;speed(e).z=(dz*step)/length;}
        }
    }
    auto* output=pointer<ProjectileSpeed>(e,1);const auto vz=speed(e).z,current_dash=human(e).dash_speed;
    const auto direction=sign(speed(e).z);const float extra=current_dash*static_cast<float>(direction);required(output).z=vz+extra;
    if(speed(e).z>0.0F){auto& v=speed(e);v.z=v.z*0.7F;}
    auto interval=host.human_frames_per_second();if(weapon(e,host).attribute==3)interval=std::max(interval/2,1);
    if(remainder(s.loop_counter,interval)!=0)return;
    const auto recovery=parameter(s,e,4,host);const auto roll=random_inclusive(host.human_random(),3,5);
    const auto rate=std::clamp(add(roll,recovery/3),1,20);
    auto* hp=resolve_combat_parameter(e,0);auto* mp=resolve_combat_parameter(e,1);
    const auto floor=add(recovery/2,1);
    const auto hp_amount=std::max(multiply(maximum(required(hp)),rate)/100,floor);
    const auto mp_amount=std::max(multiply(maximum(required(mp)),rate)/100,floor);
    add_human_parameter(*hp,hp_amount);add_human_parameter(*mp,mp_amount);
}
void update_human_attacking(HumanFrameSystem& s,ComponentSlots* input,HumanFrameHost& host){
    auto& e=required(input);auto* captured=&human(e);auto* target=captured->opponent;
    if(target&&!host.human_target_destroyed(*target)&&!enemy_is_dead(enemy(*target))&&enemy(*target).state!=5&&stage_attack_active(required(s.stage))){
        const auto interval=human_attack_interval(s,&e,host),wait=human_burst_wait(s,&e,host);
        const auto elapsed=add(captured->attack_frame,negate(interval));
        // SUBS/B.LT compares the mathematical signed difference, including overflow.
        if(captured->attack_frame<interval||remainder(elapsed,wait)!=0)return;
        if(host.shoot_human(e,*target))component<EntitySprite>(e,2).animation.frame=0;
        const auto mp=total(required(resolve_combat_parameter(e,1)));
        const auto count=mp<1?0:add(human_burst_count(s,&e,host),-1);
        const auto end=add(interval,multiply(count,wait));if(captured->attack_frame<end)return;
        captured->attack_frame=0;
    }
    captured->opponent=nullptr;change_human_state(e,0,host);
}
void update_human_bounding(HumanFrameSystem&,ComponentSlots* input,HumanFrameHost& host){
    auto& e=required(input);auto& v=speed(e);v.z=v.z*0.8F;human(e).dash_speed=0;
    if(!(std::fabs(speed(e).z)<0.1F))return;
    component<ContactDirection>(e,12).locked=false;speed(e).z=0;change_human_state(e,0,host);
}
void update_human_celebration(ComponentSlots* input,HumanFrameHost& host){
    auto& e=required(input);speed(e).z=0;const auto period=host.celebration_frame_period();
    if(remainder(human(e).frame,period)==0){auto* output=pointer<ContactDirection>(e,12);const auto direction=random_bounded(host.human_random(),4);required(output).direction=direction;}
    host.change_animation(e,0,false);
}
void limit_human_position(HumanFrameSystem& s,ComponentSlots* input,HumanFrameHost& host){
    auto* stage=s.stage;const auto first=stage_z(required(stage),required(stage).min_z_rate,host.human_coordinates());
    stage=s.stage;const auto last=stage_z(required(stage),required(stage).max_z_rate,host.human_coordinates());
    auto& e=required(input);const auto z=position(e).z;auto* output=pointer<ContactPosition>(e,0);
    const auto upper=static_cast<float>(add(first,108));if(z>upper)required(output).z=upper;
    else if(required(output).z<static_cast<float>(last))position(e).z=static_cast<float>(last);
}
void update_humans(HumanFrameSystem& s,HumanFrameHost& host){
    auto cursor=required(s.subset).cursor();
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);auto* captured=&human(e);const auto state=captured->state;
        if(state==0)update_human_waiting(s,&e,host);
        else if(state==1)update_human_walking(s,&e,host);
        else if(state==2)update_human_attacking(s,&e,host);
        else if(state==3)update_human_bounding(s,&e,host);
        else if(state==6)update_human_celebration(&e,host);
        const auto next=add(captured->attack_frame,1);captured->attack_frame=next;
        if(state==2){if(captured->state!=2&&next>human_attack_interval(s,&e,host))captured->attack_frame=0;}
        else if(next>human_attack_interval(s,&e,host))captured->attack_frame=human_attack_interval(s,&e,host);
        limit_human_position(s,&e,host);
        if(captured->invincible_time>0){--captured->invincible_time;if(captured->invincible_time==0&&e.contains(15)){
            if(component<ProjectileModification>(e,15).type==3)host.remove_human_modification(e);
        }}
        captured->frame=add(captured->frame,1)%std::numeric_limits<std::int32_t>::max();
    }
    s.loop_counter=add(s.loop_counter,1)%std::numeric_limits<std::int32_t>::max();
}
}
