#include "projectile_launch.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t bits(std::uint32_t value){return static_cast<std::int32_t>(value<=0x7fffffffU?static_cast<std::int64_t>(value):static_cast<std::int64_t>(value)-0x100000000LL);}
// Fire explicitly maps positive infinity to INT_MIN after ARM FCVTZS.
std::int32_t integer(float value){
    if(std::isnan(value))return 0;
    if(value==std::numeric_limits<float>::infinity()||value<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(value>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(value);
}
ContactPosition& position(ComponentSlots& e){
    auto* p=dynamic_cast<ContactPosition*>(e.get(contact_position_slot));
    if(!p)throw std::invalid_argument("missing projectile position");
    return *p;
}
}
ComponentSlots& add_animation_modification(ComponentSlots& e,AnimationModificationRequest r,AnimationModificationPool& host){
    auto& p=host.acquire_modification();
    p.type=r.type;p.x=r.x;p.y=r.y;p.z=r.z;p.scale_x=r.scale_x;p.scale_y=r.scale_y;
    p.angle=r.angle;p.frame=r.frame;p.max_frame=r.max_frame;p.anchor=r.anchor;p.destroy=r.destroy;p.loop=r.loop;p.alpha=r.alpha;
    return e.add(modify_animation_slot,&p);
}
void add_projectile_modification(ComponentSlots& e,std::int32_t type,std::int32_t max_frame,bool destroy,bool loop,ProjectileLaunchHost& host){
    AnimationModificationRequest r;r.type=type;r.max_frame=max_frame;r.destroy=destroy;r.loop=loop;
    add_animation_modification(e,r,host);
}
std::int32_t launch_projectile(ComponentSlots& e,ProjectilePoint start,ProjectilePoint end,
    std::int32_t speed,ComponentSlots* parent,bool animate,bool rotate,std::int32_t height_percent,
    ProjectileLaunchHost& host){
    const float x=end.x-start.x,y=end.y-start.y,z=end.z-start.z;
    const float xx=x*x,yy=y*y,zz=z*z;
    const float xy=xx+yy,sum=xy+zz;
    const float distance=std::sqrt(sum);
    const auto duration=integer(distance/static_cast<float>(speed));
    const auto height=bits(static_cast<std::uint32_t>(integer(distance/3.0F))*static_cast<std::uint32_t>(height_percent))/100;
    auto& p=host.acquire_projectile();p.start=start;p.end=end;p.speed=speed;
    p.max_height=height;p.max_frame=duration;p.frame=0;p.parent=parent;
    e.add(projectile_slot,&p);
    if(rotate)add_projectile_modification(e,7,0,false,true,host);
    if(animate)host.add_animation(e,1,-1);
    return duration;
}
void fly_away(ComponentSlots& e,std::int32_t min_x,std::int32_t max_x,
    std::int32_t min_z,std::int32_t max_z,bool modification,RandomSource& random,ProjectileLaunchHost& host){
    const ProjectilePoint start{position(e).x,position(e).y,position(e).z};
    const float x=position(e).x;
    const auto dx=random_inclusive(random,min_x,max_x);
    const float z=position(e).z;
    const auto dz=random_inclusive(random,min_z,max_z);
    const ProjectilePoint end{x+static_cast<float>(dx),0.0F,z+static_cast<float>(dz)};
    launch_projectile(e,start,end,4,nullptr,false,false,200,host);
    if(modification){
        if(e.contains(modify_animation_slot)){
            auto* old=e.get(modify_animation_slot);e.remove(modify_animation_slot);
            host.return_component(modify_animation_slot,old);
        }
        add_projectile_modification(e,8,99,false,false,host);
    }
}
void present_combat_damage(ComponentSlots& e,const DamageNumbers& numbers,DamagePresentationHost& host){
    host.damage_labels(e,numbers);
    if(e.contains(combat_enemy_slot)){
        if(e.contains(modify_animation_slot)){
            auto* previous=e.get(modify_animation_slot);e.remove(modify_animation_slot);host.return_component(modify_animation_slot,previous);
        }
        AnimationModificationRequest request;request.type=4;request.max_frame=8;
        add_animation_modification(e,request,host);
    }
    host.positional_damage_effect({position(e).x,position(e).y+10.0F,position(e).z});
}
}
