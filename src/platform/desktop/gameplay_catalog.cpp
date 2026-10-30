#include "gameplay_catalog.hpp"
#include <stdexcept>

namespace saga::desktop {
GameplayCatalog load_gameplay_catalog(ResourceArchive& archive,bool normalize,const core::EventIntegerParser& parser,bool include_rewards,bool include_results,bool include_quests,bool include_areas,bool include_towns){
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
    if(include_results)result.levels=core::load_level_table(read("Table.txt"),normalize,parser);
    if(include_quests)result.quests=core::load_quest_table(read("Quest.txt"),normalize,parser);
    if(include_areas)result.areas=std::make_shared<core::AreaTable>(core::load_area_table(read("Area.txt"),normalize,parser));
    if(include_towns)result.towns=std::make_shared<core::TownTable>(core::load_town_table(read("Town.txt"),normalize,parser));
    return result;
}
} // namespace saga::desktop
