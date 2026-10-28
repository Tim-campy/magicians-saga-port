#include "projectile_motion.hpp"
#include "combat_rules.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& component(ComponentSlots& e,std::int32_t slot){
    auto* p=dynamic_cast<T*>(e.get(slot));
    if(!p)throw std::invalid_argument("missing projectile motion component");
    return *p;
}
ProjectileLink& projectile(ComponentSlots& e){return component<ProjectileLink>(e,projectile_slot);}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,contact_position_slot);}
ProjectileSpeed& speed(ComponentSlots& e){return component<ProjectileSpeed>(e,projectile_speed_slot);}
std::int32_t bits(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
}
float easing_float(float start,float end,float duration,float progress,float amplitude){
    if(progress<0)return start;
    if(progress>=duration)return end;
    const float ratio=progress/duration;
    const float pi=ratio*0x1.921fb6p+1F;
    const float degrees=pi*180.0F;
    const float angle=degrees/0x1.921fb6p+1F;
    const float sine=sine_degrees(angle);
    const float difference=end-start;
    const float weight=amplitude/314.159271240234375F;
    const float correction=weight*sine;
    const float curved=ratio+correction;
    const float scaled=difference*curved;
    float result=scaled+start;
    if(start<end){if(result<start)result=start;if(result>end)result=end;}
    else{if(result>start)result=start;if(result<end)result=end;}
    return result;
}
namespace {
std::int32_t angle(float x,float y){
    const double radians=std::atan2(static_cast<double>(y),static_cast<double>(x));
    const double product=radians*180.0;
    float degrees=static_cast<float>(product/0x1.921fb54442d18p+1);
    if(y<0){degrees=degrees+180.0F;degrees=degrees+180.0F;} // Helper and caller both add 180.
    std::int32_t value=0;
    if(degrees==std::numeric_limits<float>::infinity()||degrees<=-2147483648.0F)value=std::numeric_limits<std::int32_t>::min();
    else if(degrees>=2147483648.0F)value=std::numeric_limits<std::int32_t>::max();
    else if(!std::isnan(degrees))value=static_cast<std::int32_t>(degrees);
    return bits(static_cast<std::uint32_t>(value)+90U);
}
}
bool advance_projectile(ComponentSlots& e,ProjectileMotionHost& host){
    const auto end=projectile(e).end,start=projectile(e).start;
    const float dx=end.x-start.x,dy=end.y-start.y,dz=end.z-start.z;
    const float xx=dx*dx,yy=dy*dy,zz=dz*dz;
    const float xy=xx+yy,total=xy+zz,length=std::sqrt(total);
    const float nx=length==0?0.0F:dx/length,nz=length==0?0.0F:dz/length;
    const auto velocity=projectile(e).speed,frame=projectile(e).frame;
    const auto half=projectile(e).max_frame/2;
    const bool descending=frame>half;
    const auto progress=descending?bits(static_cast<std::uint32_t>(frame)-static_cast<std::uint32_t>(half)):frame;
    const auto next=bits(static_cast<std::uint32_t>(progress)+1U);
    const float peak=projectile(e).start.y+static_cast<float>(projectile(e).max_height);
    const float from=descending?peak:projectile(e).start.y;
    const float to=descending?projectile(e).end.y:peak;
    position(e).y=easing_float(from,to,static_cast<float>(half),static_cast<float>(progress),descending?-80.0F:80.0F);
    const float following=easing_float(from,to,static_cast<float>(half),static_cast<float>(next),descending?-80.0F:80.0F);
    const float current=position(e).y;
    const float vertical=following-current;
    speed(e).x=nx*static_cast<float>(velocity);
    speed(e).y=vertical;
    speed(e).z=nz*static_cast<float>(velocity);
    if(e.contains(modify_animation_slot)){
        const float px=speed(e).x-speed(e).z;
        const float sum=speed(e).x+speed(e).z;
        const float py=sum*0.5F-vertical;
        component<ProjectileModification>(e,modify_animation_slot).angle=angle(px,py);
    }
    auto& p=projectile(e);const auto previous=p.frame;p.frame=bits(static_cast<std::uint32_t>(previous)+1U);
    if(previous<1||!(position(e).y<=projectile(e).end.y))return false;
    position(e).x=projectile(e).end.x;position(e).y=projectile(e).end.y;position(e).z=projectile(e).end.z;
    speed(e).x=0;speed(e).y=0;speed(e).z=0;
    if(e.contains(modify_animation_slot)){
        auto* old=&component<ProjectileModification>(e,modify_animation_slot);
        e.remove(modify_animation_slot);host.return_component(modify_animation_slot,old);
    }
    return true;
}
void finish_projectile(ComponentSlots& e,ProjectileMotionHost& host){
    host.send_impact(e);
    // Entity.RemoveProjectile 0x1130f64 returns if its first getter is null.
    // The impact event can destroy an emitter and remove all its components.
    if(!e.get(projectile_slot))return;
    auto* old=&projectile(e);e.remove(projectile_slot);host.return_component(projectile_slot,old);
}
}
