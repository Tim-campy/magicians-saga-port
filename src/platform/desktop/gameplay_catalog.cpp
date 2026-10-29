#include "gameplay_catalog.hpp"
#include <stdexcept>

namespace saga::desktop {
GameplayCatalog load_gameplay_catalog(ResourceArchive& archive,bool normalize,const core::EventIntegerParser& parser,bool include_rewards){
    GameplayCatalog result;
    auto read=[&](const char* name){
        auto data=archive.data(name);if(!data)throw std::invalid_argument(std::string("missing gameplay table: ")+name);
        result.members.emplace(name,data->member);return std::string(data->bytes.begin(),data->bytes.end());
    };
    result.characters=core::load_character_progression_table(read("Chara.txt"),normalize,parser);
    result.equipment=core::load_equipment_combat_table(read("Equipment.txt"),core::EquipmentTableKind::weapon_or_shield,normalize,parser);
    result.cloth=core::load_equipment_combat_table(read("Cloth.txt"),core::EquipmentTableKind::cloth_or_hat,normalize,parser);
    result.hats=core::load_equipment_combat_table(read("Hat.txt"),core::EquipmentTableKind::cloth_or_hat,normalize,parser);
    result.shop=core::load_magic_shop_combat_table(read("MagicShop.txt"),normalize,parser);
    result.enemies=std::make_shared<core::EnemyTable>(core::load_enemy_table(read("Enemy.txt"),normalize,parser));
    result.bullets=std::make_shared<core::BulletTable>(core::load_bullet_table(read("Bullet.txt"),normalize,parser));
    if(include_rewards)result.treasures=core::load_treasure_table(read("Treasure.txt"),normalize,parser);
    return result;
}
} // namespace saga::desktop
