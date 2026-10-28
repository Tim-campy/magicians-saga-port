#include "emitter_frame.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing emitter frame input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,0);}
ProjectileSpeed& speed(ComponentSlots& e){return component<ProjectileSpeed>(e,1);}
ProjectilePoint point(ComponentSlots& e){return {position(e).x,position(e).y,position(e).z};}
std::int32_t increment(std::int32_t a){const auto v=static_cast<std::uint32_t>(a)+1U;
    return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t divide(std::int32_t a,std::int32_t b){if(!b)return 0;if(b==-1&&a==std::numeric_limits<std::int32_t>::min())return a;return a/b;}
std::int32_t remainder(std::int32_t a,std::int32_t b){if(!b)return a;if(b==-1&&a==std::numeric_limits<std::int32_t>::min())return 0;return a%b;}
void copy_modification_field(ComponentSlots& particle,ComponentSlots& source,float ProjectileModification::* field){
    auto* output=dynamic_cast<ProjectileModification*>(particle.get(15));
    auto* input=dynamic_cast<ProjectileModification*>(source.get(15));
    const auto value=required(input).*field;required(output).*field=value;
}
void emit_trail(ComponentSlots& source,EmitterFrameHost& host){
    if(!source.contains(29))return;
    const auto angle=source.contains(15)?component<ProjectileModification>(source,15).angle:0;
    const auto* record=resolve_bullet_record(component<BulletComponent>(source,29),host);
    auto* world=host.emitter_particle_world();const auto at=point(source);
    const float vx=speed(source).x,vy=speed(source).y,vz=speed(source).z;
    const auto image=required(record).trail_image;
    auto& particle=create_particle({19,1,image,at,{vx/10.0F,vy/10.0F,vz/10.0F}},required(world));
    add_animation_component(particle,1,-1,host);
    AnimationModificationRequest request;request.type=7;request.angle=angle;request.loop=true;
    add_animation_modification(particle,request,host);
    copy_modification_field(particle,source,&ProjectileModification::scale_x);
    copy_modification_field(particle,source,&ProjectileModification::scale_y);
    copy_modification_field(particle,source,&ProjectileModification::x);
    copy_modification_field(particle,source,&ProjectileModification::y);
    if((record->flags&1)!=0){auto& color=host.acquire_emitter_color();color.values={{255,255,255,3192}};particle.add(6,&color);}
}
}
void initialize_emitters(EmitterSystemState& s,EmitterInitializationWorld* input){
    auto& world=required(input);s.subset=world.emitter_subset({31});s.expired=std::make_shared<EmitterExpiryQueue>();
}
void update_emitters(EmitterSystemState& s,EmitterFrameHost& host){
    s.loop_counter=increment(s.loop_counter)%std::numeric_limits<std::int32_t>::max();
    {
        auto cursor=required(s.subset).cursor();
        while(const auto entry=cursor.next()){
            auto& e=required(*entry);auto& emitter=component<EmitterComponent>(e,31);
            const auto next=increment(emitter.frame),interval=emitter.interval,quotient=divide(next,interval);
            emitter.frame=next;if(remainder(next,interval)!=0)continue;
            if(emitter.maximum!=-1&&quotient>emitter.maximum){required(s.expired.get()).add(&e);continue;}
            if(emitter.category!=0)continue;
            if(emitter.type==0)emit_trail(e,host);
            else if(emitter.type==1){auto* world=host.emitter_effect_world();const auto at=point(e);auto& w=required(world);
                create_visual_effect(at,0,27,false,w.ids,w.factory);}
            else if(emitter.type==2){position(e);position(e);} // Native branch requires both lookups, then returns to traversal.
        }
    }
    const auto queue=s.expired;auto& pending=required(queue.get());const auto expected=pending.version();
    for(std::size_t i=0;;++i){
        if(pending.version()!=expected)throw std::runtime_error("emitter expiry list changed during enumeration");
        if(i==pending.size())break;
        host.destroy_emitter_entity(required(pending.at(i)));
    }
    required(s.expired.get()).clear(); // Re-read current list; callbacks may have replaced it.
}
}
