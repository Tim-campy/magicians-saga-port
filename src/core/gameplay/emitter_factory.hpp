#pragma once
#include "bullet_factory.hpp"
#include "garbage_lifetime.hpp"

namespace saga::core {
class EmitterFactoryHost:public EntityVisualHost {
public:
    virtual BulletFactoryWorld* emitter_global_world()=0;
    virtual EmitterComponent& acquire_emitter()=0;
};
struct EmitterCreation {
    std::int32_t category=0,type=0,interval=0,maximum=0,argument=0;ProjectilePoint position;
};
// World.CreateEmitter 0x113519c: global registration, Emitter/Position/Speed only.
ComponentSlots& create_emitter(EmitterCreation,EmitterFactoryHost&);
class ParticleFactoryHost:public EntityVisualHost,public GarbagePool {
public:
    // Actual AppData resource selector and current clip maximum, before entity creation.
    virtual std::int32_t particle_clip_max_frame(std::int32_t resource,std::int32_t clip)=0;
    virtual BulletFactoryWorld* particle_global_world()=0;
    virtual StageGeometry* particle_global_map()=0;
};
struct ParticleCreation {std::int32_t resource=0,clip=0,image=0;ProjectilePoint position,speed;};
// World.CreateParticle 0x1134f44. No playback/modification: the emitter adds those.
ComponentSlots& create_particle(ParticleCreation,ParticleFactoryHost&);
}
