#include "modification_frame.hpp"
#include "combat_rules.hpp"
#include "projectile_motion.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing modification input");return *p;}
ProjectileModification* modification(ComponentSlots& e){
    auto* raw=e.get(15);auto* value=dynamic_cast<ProjectileModification*>(raw);
    if(raw&&!value)throw std::invalid_argument("wrong modification component type");return value;
}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t increment(std::int32_t a){return word(static_cast<std::uint32_t>(a)+1U);}
std::int32_t subtract(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t divide(std::int32_t a,std::int32_t b){if(!b)return 0;if(b==-1&&a==std::numeric_limits<std::int32_t>::min())return a;return a/b;}
std::int32_t integer(float value){
    if(std::isnan(value))return 0;
    if(value==std::numeric_limits<float>::infinity()||value<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(value>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();return static_cast<std::int32_t>(value);
}
void apply_type(ProjectileModification& m){
    const auto frame=m.frame;
    switch(m.type){
    case 0:{
        const auto ratio=std::min(divide(multiply(frame,100),m.max_frame/2),100);
        m.y=sine_degrees(static_cast<float>(multiply(ratio,180)/100))*-13.0F;
        m.scale_y=easing_float(0.25F,1.0F,7.0F,static_cast<float>(m.frame),-80.0F);
        const auto half=m.max_frame/2;
        const auto phase=divide(multiply(subtract(m.frame,half),100),half);
        const auto angle=multiply(phase,360)/100;
        m.angle=integer(sine_degrees(static_cast<float>(angle))*4.0F);break;
    }
    case 1:{const auto ratio=std::min(divide(multiply(frame,100),m.max_frame),100);
        const auto angle=multiply(ratio,1800)/100;m.angle=integer(sine_degrees(static_cast<float>(angle))*3.0F);break;}
    case 2:
        m.scale_x=easing_float(1.0F,0.0F,static_cast<float>(m.max_frame),static_cast<float>(frame),100.0F);
        m.scale_y=easing_float(1.0F,1.5F,static_cast<float>(m.max_frame),static_cast<float>(m.frame),100.0F);
        m.alpha=easing(255,0,m.max_frame,m.frame,100);break;
    case 3:m.scale_x=(frame&1)==0?0.0F:1.0F;break;
    case 4:{const float offset=static_cast<float>(subtract(m.max_frame,frame)/2);m.x=(frame&1)!=0?offset:-offset;break;}
    case 5:m.alpha=easing(255,0,m.max_frame,frame,-100);break;
    case 6:m.alpha=easing(0,255,m.max_frame,frame,100);break;
    case 8:m.angle=multiply(frame,45)%360;break;
    default:break; // Type 7 and all values outside 0..8 retain payload, but still age/reset/expire.
    }
}
}
void initialize_modifications(ModificationSystemState& s,ModificationInitializationWorld* input){
    auto& world=required(input);s.subset=world.modification_subset({15});s.expired=std::make_shared<EmitterExpiryQueue>();
}
ComponentSlots& remove_animation_modification(ComponentSlots& e,ModificationFrameHost& host){
    auto* previous=modification(e);e.remove(15);host.return_modification(previous);return e;
}
void update_modifications(ModificationSystemState& s,ModificationFrameHost& host){
    {
        auto cursor=required(s.subset).cursor();
        while(const auto entry=cursor.next()){
            auto& e=required(*entry);auto& m=required(modification(e));
            if(!m.loop&&m.max_frame==0){required(s.expired.get()).add(&e);continue;}
            if(m.frame<0){m.frame=increment(m.frame);continue;}
            apply_type(m);const auto previous=m.frame;m.frame=increment(previous);
            if(previous<m.max_frame)continue;
            if(m.loop)m.frame=0;else required(s.expired.get()).add(&e);
        }
    }
    const auto queue=s.expired;auto& pending=required(queue.get());const auto version=pending.version();
    for(std::size_t i=0;;++i){
        if(pending.version()!=version)throw std::runtime_error("modification expiry list changed during enumeration");
        if(i==pending.size())break;
        auto& e=required(pending.at(i));if(required(modification(e)).destroy)host.destroy_modified_entity(e);
        else remove_animation_modification(e,host);
    }
    required(s.expired.get()).clear();
}
}
