#include "emitter_factory.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing emitter factory input");return *p;}
std::int32_t decrement(std::int32_t a){const auto v=static_cast<std::uint32_t>(a)-1U;
    return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
void add_position(ComponentSlots& e,ProjectilePoint at,EntityVisualHost& host){
    auto& p=host.acquire_position();p.x=at.x;p.y=at.y;p.z=at.z;p.offset_x=0;p.offset_y=0;p.offset_z=0;p.parent=nullptr;e.add(0,&p);
}
void add_speed(ComponentSlots& e,ProjectilePoint v,EntityVisualHost& host){
    auto& speed=host.acquire_speed();speed.x=v.x;speed.y=v.y;speed.z=v.z;e.add(1,&speed);
}
}
ComponentSlots& create_emitter(EmitterCreation r,EmitterFactoryHost& host){
    auto& world=required(host.emitter_global_world());auto& e=create_entity(world.ids,0,false,world.entities);
    auto& em=host.acquire_emitter();em.category=r.category;em.type=r.type;em.interval=r.interval;em.frame=0;
    em.maximum=r.maximum;em.argument=r.argument;e.add(31,&em);
    add_position(e,r.position,host);add_speed(e,{},host);return e;
}
ComponentSlots& create_particle(ParticleCreation r,ParticleFactoryHost& host){
    const auto maximum=host.particle_clip_max_frame(r.resource,r.clip);
    auto& world=required(host.particle_global_world());auto& e=create_entity(world.ids,0,false,world.entities);
    add_position(e,r.position,host);add_speed(e,r.speed,host);
    const auto step_x=required(host.particle_global_map()).step_x;
    const auto step_z=required(host.particle_global_map()).step_z;
    host.add_cell(e,factory_cell_coordinate(r.position.x,step_x),factory_cell_coordinate(r.position.z,step_z));
    auto& sprite=host.acquire_sprite();sprite.animation={r.resource,r.clip,0};sprite.layers=std::make_shared<std::vector<std::int32_t>>(1,-1);e.add(2,&sprite);
    host.add_image(e,{{r.image,r.resource,-1,-1,-1,-1}});
    auto& depth=host.acquire_depth();depth.value=0;depth.extra=0;e.add(4,&depth);
    add_garbage_lifetime(e,decrement(maximum),host);return e;
}
}
