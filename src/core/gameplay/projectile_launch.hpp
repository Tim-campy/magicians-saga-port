#pragma once
#include "contact_collision.hpp"
#include "game_random.hpp"

namespace saga::core {
inline constexpr std::int32_t modify_animation_slot=15;
struct ProjectileModification final:Component {
    std::int32_t type=0;
    float x=0,y=0,z=0,scale_x=0,scale_y=0;
    std::int32_t angle=0,frame=0,max_frame=0,anchor=0;
    bool destroy=false,loop=false;
    std::int32_t alpha=0;
};
class AnimationModificationPool {
public:
    virtual ~AnimationModificationPool()=default;
    virtual ProjectileModification& acquire_modification()=0;
};
struct AnimationModificationRequest {
    std::int32_t type=0,angle=0,frame=0,max_frame=0,anchor=0;
    float x=0,y=0,z=0,scale_x=1,scale_y=1;
    bool destroy=false,loop=false;
    std::int32_t alpha=255;
};
// Entity.AddModifyAnimation 0x113027c: all payload fields before slot 15.
ComponentSlots& add_animation_modification(ComponentSlots&,AnimationModificationRequest,AnimationModificationPool&);
class ProjectileLaunchHost:public AnimationModificationPool {
public:
    virtual ~ProjectileLaunchHost()=default;
    // Host owns returned objects and must keep them alive through slot callbacks.
    virtual ProjectileLink& acquire_projectile()=0;
    virtual void return_component(std::int32_t,Component*)=0;
    virtual void add_animation(ComponentSlots&,std::int32_t rate,std::int32_t backup)=0;
};
// Shared generated ModifyAnimation payload initialization; add preserves slot semantics.
void add_projectile_modification(ComponentSlots&,std::int32_t type,std::int32_t max_frame,
    bool destroy,bool loop,ProjectileLaunchHost&);
// ProjectileSystem.Fire 0x1157230. Configures components; no per-frame motion here.
std::int32_t launch_projectile(ComponentSlots&,ProjectilePoint start,ProjectilePoint end,
    std::int32_t speed,ComponentSlots* parent,bool animate,bool rotate,std::int32_t height_percent,
    ProjectileLaunchHost&);
// EnemySystem.FlyAway 0x1150fa8. Uses the existing game's inclusive random wrapper.
void fly_away(ComponentSlots&,std::int32_t min_x,std::int32_t max_x,
    std::int32_t min_z,std::int32_t max_z,bool modify,RandomSource&,ProjectileLaunchHost&);
class DamagePresentationHost:public ProjectileLaunchHost {
public:
    virtual void damage_labels(ComponentSlots&,const DamageNumbers&)=0;
    virtual void positional_damage_effect(ProjectilePoint)=0;
};
// EnemySystem.Damage post-subtraction suffix 0x115069c..0x115078c, before event15.
// Text-label selection/output is a service; enemy shake and positional effect remain real mutations.
void present_combat_damage(ComponentSlots&,const DamageNumbers&,DamagePresentationHost&);
}
