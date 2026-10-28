#pragma once
#include <array>
#include "projectile_launch.hpp"
#include "depth_component.hpp"
#include "cell_component.hpp"
#include "visual_components.hpp"
#include "../ecs/entity_creation.hpp"
#include "../animation/animation_transitions.hpp"
namespace saga::core {
struct EntitySprite final:Component {SpriteAnimation animation;std::vector<std::int32_t> layers;};
using EffectSprite=EntitySprite;
using EffectDepth=EntityDepth;
class EntityVisualHost:public EntityCreationHost,public CellPool,public ImagePool {
public:
    virtual ContactPosition& acquire_position()=0;
    virtual ProjectileSpeed& acquire_speed()=0;
    virtual EntitySprite& acquire_sprite()=0;
    virtual EntityDepth& acquire_depth()=0;
    virtual void add_cell(ComponentSlots& e,std::int32_t x,std::int32_t z){add_cell_component(e,x,z,nullptr,*this);}
    virtual void add_image(ComponentSlots& e,const std::array<std::int32_t,6>& values){add_image_component(e,values,*this);}
};
// Shared factory conversion at 0x1134bc8 / 0x11353fc: FCVTZS, positive-infinity override, signed /24.
std::int32_t factory_cell_coordinate(float);
// Same conversion with an explicit live map divisor, used by Bullet/Particle factories.
std::int32_t factory_cell_coordinate(float,std::int32_t step);
// World.CreateZObject 0x1135530. Registered world object with depth 200;
// Position/Speed/Cell/Depth/Seb only, without Image or playback.
ComponentSlots& create_z_object(ProjectilePoint,std::int32_t resource,std::int32_t clip,EntityIdState&,EntityVisualHost&);
}
