#include "gameplay_catalog.hpp"
#include "assets/jar_archive.hpp"
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>

namespace saga::desktop {
std::vector<std::uint8_t> read_game_archive(const std::filesystem::path& path){
    std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("cannot open decoded archive");
    const auto size=in.tellg();if(size<8||size>static_cast<std::streamoff>(saga::assets::JarLimits{}.max_bytes))throw std::length_error("decoded archive size outside limits");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));in.seekg(0);in.read(reinterpret_cast<char*>(bytes.data()),size);
    if(!in)throw std::runtime_error("incomplete archive read");return bytes;
}
GameResourceIndex read_game_resource_index(const std::filesystem::path& root,bool town){
    const auto v2=root/"mission-resources-v2.tsv";
    const auto path=town?root/"town-resources.tsv":std::filesystem::exists(v2)?v2:root/"mission-resources.tsv";
    if(std::filesystem::file_size(path)>16384)throw std::length_error("mission resource index exceeds16KiB");
    std::ifstream list(path);std::string signature;GameResourceIndex result{};
    if(!(list>>signature>>result.version)||signature!=(town?"town_resources":"mission_resources")||(result.version!=1&&(town||result.version!=2)))throw std::invalid_argument(town?"invalid town resource index header":"invalid mission resource index header");
    for(std::string line;std::getline(list,line);){
        std::istringstream entry(line);std::string relative;GameArchiveEntry row{};
        entry>>std::ws;if(entry.eof())continue;
        if(!(entry>>row.kind>>row.id>>std::quoted(relative)>>std::quoted(row.name)))throw std::invalid_argument("invalid mission resource index row");
        entry>>std::ws;if(!entry.eof())throw std::invalid_argument("unexpected mission resource index fields");
        const auto child=std::filesystem::u8path(relative);
        if(child.is_absolute()||child.parent_path()!=std::filesystem::path("textassets")||child.filename().empty())throw std::invalid_argument("mission resource path must be a direct textassets child");
        row.path=root/child;result.entries.push_back(std::move(row));
    }
    if(!list.eof())throw std::invalid_argument("incomplete mission resource index");return result;
}
NewGameSaveDefaults read_new_game_save_defaults(const std::filesystem::path& root){
    const auto path=root/"new-game-save.tsv";
    if(std::filesystem::file_size(path)>1024)throw std::length_error("new-game defaults exceed1KiB");
    std::ifstream seeds(path);seeds.imbue(std::locale::classic());std::string signature,selection,town,wallet;
    std::int32_t version=0;NewGameSaveDefaults result{};
    if(!(seeds>>signature>>version>>selection>>result.area8>>result.quest9>>town>>result.town_capacity>>wallet>>result.money>>result.crystals)||
       signature!="new_game_save"||version!=1||selection!="selection"||town!="town_capacity"||wallet!="wallet")throw std::invalid_argument("invalid new-game save defaults");
    seeds>>std::ws;if(!seeds.eof())throw std::invalid_argument("unexpected new-game default fields");return result;
}
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
