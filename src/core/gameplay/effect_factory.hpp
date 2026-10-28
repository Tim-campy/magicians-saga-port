#pragma once
#include "entity_visual.hpp"
#include "garbage_lifetime.hpp"
#include "../ecs/entity_creation.hpp"
#include "../animation/animation_transitions.hpp"
#include <array>

namespace saga::core {
struct EffectComponent final:Component {
    std::int32_t type=0,value=0,frame=0,max_frame=0;bool depth=false;ComponentSlots* parent=nullptr;
};
struct EffectRequest {
    ProjectilePoint position;
    std::int32_t type,value,resource,clip;
    bool depth,loop;
    std::int32_t max_frame,frame;
    ComponentSlots* parent;
    std::int32_t image;
    bool animate;
};
class EffectFactoryHost:public EntityVisualHost,public GarbagePool {
public:
    virtual std::int32_t clip_max_frame(std::int32_t resource,std::int32_t clip)=0;
    virtual EffectComponent& acquire_effect()=0;
    virtual void add_playback(ComponentSlots&,std::int32_t rate,std::int32_t backup)=0;
};
// World.CreateEffect 0x1134a70; resource, Cell/Image and playback services stay explicit.
ComponentSlots& create_effect(EffectRequest,EntityIdState&,EffectFactoryHost&);
// Short overload 0x1134a28: resource 20, depth/animation true, type/value/frame 0.
ComponentSlots& create_visual_effect(ProjectilePoint,std::int32_t clip,std::int32_t image,bool loop,
                                    EntityIdState&,EffectFactoryHost&);
}
