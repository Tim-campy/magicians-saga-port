#pragma once
#include "resource_archive.hpp"
#include "core/gameplay/bullet_factory.hpp"
#include "core/gameplay/combat_tables.hpp"
#include <map>

namespace saga::desktop {
// Selected synchronous table set, not the complete native Data.Load/bootstrap.
// Loaded records/arrays outlive their entity factories; saved progression is supplied separately.
struct GameplayCatalog {
    core::CharacterProgressionTable characters;
    core::EquipmentCombatTable equipment,cloth,hats;
    core::MagicShopCombatTable shop;
    std::shared_ptr<core::EnemyTable> enemies;
    core::BulletRecords bullets;
    std::map<std::string,std::string> members; // Logical table name -> actual selected archive member.
};
GameplayCatalog load_gameplay_catalog(ResourceArchive&,bool normalize_crlf,const core::EventIntegerParser&);
} // namespace saga::desktop
