#pragma once
#include "entity_visual.hpp"
#include "quest_rewards.hpp"
namespace saga::core {
struct DropComponent final:Component {std::int32_t type=0,value=0;float x0=0,z0=0;std::int32_t frame=-1;};
class DropFactoryHost:public EntityVisualHost {
public:
    virtual DropComponent& acquire_drop()=0;
    virtual ProjectileModification& acquire_modification()=0;
    virtual Component* sync_animation()=0; // Shared marker from the component-pool static +0x100.
};
// World.CreateDrop 0x1135298. Invalid type returns null before any side effects; no RNG calls.
ComponentSlots* create_drop(std::int32_t type,std::int32_t value,ProjectilePoint position,std::int32_t scale,
                            const TreasureTable&,EntityIdState&,DropFactoryHost&);
}
