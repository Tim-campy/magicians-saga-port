#pragma once
#include "entity_visual.hpp"
#include "impact_effect.hpp"
#include "enemy_defeat.hpp"
#include "stage_placement.hpp"

namespace saga::core {
// BulletData projection; inheritance lets the impact consumer borrow the same live record.
struct BulletRecord:BulletImpactData {
    std::int32_t id=-1,image=0,trail_image=0;
    std::string resource,impact_resource;
};
using BulletTable=std::vector<std::optional<BulletRecord>>;
using BulletRecords=std::shared_ptr<BulletTable>;
// Fixed managed-array lengths; captured record addresses survive catalog replacement/callbacks.
BulletRecord& load_bullet_record(BulletRecord&,std::string_view,const EventIntegerParser&); // BulletData.Load 0x116d414.
BulletTable load_bullet_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
struct BulletComponent final:Component {std::int32_t data_id=0;}; // Slot 29, +0x10.
struct EmitterComponent final:Component { // Slot 31, +0x10..+0x24.
    std::int32_t category=0,type=0,interval=0,frame=0,maximum=0,argument=0;
};
struct BulletFactoryWorld {EntityIdState& ids;EntityCreationHost& entities;};
class BulletFactoryHost:public EntityVisualHost {
public:
    virtual BulletRecords bullet_records()=0; // Data.instance_.bulletData_, fresh per query.
    virtual BulletFactoryWorld* bullet_global_world()=0; // World.world_, independent of factory receiver.
    virtual StageGeometry* bullet_global_map()=0; // Fresh World.world_.map lookup, including its lazy cache contract.
    virtual BulletComponent& acquire_bullet()=0;
    virtual EmitterComponent& acquire_emitter()=0;
    virtual ProjectileCollision& acquire_bullet_collision()=0;
    virtual ProjectileDamage& acquire_bullet_damage()=0;
    virtual DefeatColor& acquire_bullet_color()=0;
};
struct BulletCreation {std::int32_t id=0,damage=0,attribute=0;ComponentSlots* parent=nullptr;ProjectilePoint position;};
// World.CreateBullet 0x1134cbc. Invalid IDs fault; a null row faults after Seb insertion.
ComponentSlots& create_bullet(BulletCreation,BulletFactoryHost&);
// EquipmentData.get_bullet 0x116efd8; -1 avoids even querying the catalog.
const BulletRecord* equipment_bullet(const EquipmentCombatRecord*,BulletFactoryHost&);
}
