#pragma once
#include "projectile_launch.hpp"
#include <array>

namespace saga::core {
struct ImpactCell {std::int32_t x=0,z=0,offset_x=0,offset_z=0;};
struct ImpactCells {std::array<ImpactCell,25> cells{};std::size_t size=0;};
// CellUtil.GetRange/Visit4Cells for the impact caller's capped range (<=4).
ImpactCells impact_cells(std::int32_t x,std::int32_t z,std::int32_t range);
struct BulletImpactData {std::int32_t category=0,attribute=0,effect_image=0,flags=0;};
class ImpactEffectHost:public ProjectileLaunchHost {
public:
    // Captured record must remain alive; subsequent reads observe its live fields.
    virtual const BulletImpactData* bullet_data(ComponentSlots&)=0;
    virtual std::int32_t attribute_value(ComponentSlots& owner,std::int32_t attribute)=0;
    virtual ComponentSlots* create_effect(std::int32_t,std::int32_t,std::int32_t,ProjectilePoint)=0;
    virtual ComponentSlots* create_entity(std::int32_t)=0;
    virtual ContactPosition& acquire_position()=0;
    // Borrowed storage; impact creation initializes the collision payload before insertion.
    virtual ProjectileCollision& acquire_collision()=0;
    virtual ProjectileDamage& acquire_damage()=0;
    // Garbage lifetime scheduling and color rendering remain world services.
    virtual void add_garbage(ComponentSlots&,std::int32_t frames)=0;
    virtual void add_color(ComponentSlots&,std::int32_t,std::int32_t,std::int32_t,std::int32_t)=0;
};
// EnemySystem.CreateImpactEffect 0x114ec84.
void create_impact_effect(ComponentSlots* bullet,ComponentSlots* owner,
    const CombatRecordTables&,ImpactEffectHost&);
}
