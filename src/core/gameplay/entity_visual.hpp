#pragma once
#include <array>
#include "projectile_launch.hpp"
#include "depth_component.hpp"
#include "cell_component.hpp"
#include "../ecs/entity_creation.hpp"
#include "../animation/animation_transitions.hpp"
namespace saga::core {
struct EntitySprite final:Component {SpriteAnimation animation;std::vector<std::int32_t> layers;};
using EffectSprite=EntitySprite;
using EffectDepth=EntityDepth;
class EntityVisualHost:public EntityCreationHost,public CellPool {
public:
    virtual ContactPosition& acquire_position()=0;
    virtual ProjectileSpeed& acquire_speed()=0;
    virtual EntitySprite& acquire_sprite()=0;
    virtual EntityDepth& acquire_depth()=0;
    virtual void add_cell(ComponentSlots& e,std::int32_t x,std::int32_t z){add_cell_component(e,x,z,nullptr,*this);}
    virtual void add_image(ComponentSlots&,const std::array<std::int32_t,6>&)=0;
};
// Shared factory conversion at 0x1134bc8 / 0x11353fc: FCVTZS, positive-infinity override, signed /24.
std::int32_t factory_cell_coordinate(float);
}
