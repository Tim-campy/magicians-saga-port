#include "core/gameplay/state_data_fields.hpp"
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"data state fields contract failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
struct Fields {
    std::string name;std::size_t calls=0;std::vector<const void*> addresses;
    template<class... T>void operator()(std::string_view type,const T&... values){name=std::string(type);++calls;addresses={static_cast<const void*>(std::addressof(values))...};}
};
template<class T>void expect(const T& value,std::string_view name,std::initializer_list<const void*> addresses){
    Fields fields;visit_state_fields(value,fields);check(fields.calls==1&&fields.name==name&&fields.addresses==std::vector<const void*>(addresses));
}
void catalogs(){
    CharacterProgressionRecord character;expect(character,"CharacterProgressionRecord",{&character.id,&character.required_experience_rate,&character.weapon_id,&character.initial_level,&character.flags,&character.parameters,&character.image,&character.base_name});
    EquipmentCombatRecord equipment;expect(equipment,"EquipmentCombatRecord",{&equipment.id,&equipment.attribute,&equipment.flags,&equipment.parameters,&equipment.range,&equipment.distance,&equipment.burst_count,&equipment.use_mp,
        &equipment.type,&equipment.bullet_id,&equipment.max_height,&equipment.bullet_speed,&equipment.image,&equipment.resource,&equipment.initial_level,&equipment.name});
    MagicShopCombatRecord shop;expect(shop,"MagicShopCombatRecord",{&shop.id,&shop.category,&shop.value_per_purchase,&shop.flags});
    EnemyRecord enemy;expect(enemy,"EnemyRecord",{&enemy.id,&enemy.name,&enemy.enemy_seb,&enemy.experience,&enemy.treasures,&enemy.voice,&enemy.attribute,&enemy.resource,&enemy.image,&enemy.seb,&enemy.frame,&enemy.rank,&enemy.parameters,&enemy.flags});
    BulletRecord bullet;expect(bullet,"BulletRecord",{static_cast<const BulletImpactData*>(&bullet),&bullet.id,&bullet.image,&bullet.trail_image,&bullet.resource,&bullet.impact_resource});
    const auto& impact=static_cast<const BulletImpactData&>(bullet);expect(impact,"BulletImpactData",{&impact.category,&impact.attribute,&impact.effect_image,&impact.flags});
    TreasureRecord treasure;expect(treasure,"TreasureRecord",{&treasure.id,&treasure.name,&treasure.resource,&treasure.image,&treasure.experience,&treasure.money,&treasure.crystals,&treasure.weapon_id,&treasure.character_id,&treasure.cloth_id,&treasure.hat_id,&treasure.flags});
    LevelTableRecord level;expect(level,"LevelTableRecord",{&level.id,&level.level,&level.experience,&level.shop_parameters,&level.unlock_money,&level.unlock_crystal,&level.unlock_web_page,&level.unlock_rescue});
    QuestDefinition quest;expect(quest,"QuestDefinition",{&quest.id,&quest.image,&quest.need_stamina,&quest.name,&quest.resource,&quest.stage,&quest.discovery_state,&quest.is_new,&quest.record});
    AreaDefinition area;expect(area,"AreaDefinition",{&area.id,&area.image,&area.flags,&area.name,&area.stages});
    TownDefinition town;expect(town,"TownDefinition",{&town.id,&town.first_build,&town.flags,&town.open_term,&town.rock_term});
}
void caches_and_configurations(){
    PartyCharacter party;expect(party,"PartyCharacter",{&party.id,&party.flags,&party.saved,&party.image,&party.weapon_id,&party.parameters});
    HumanHat hat;expect(hat,"HumanHat",{&hat.flags,&hat.image});
    ShopParameterUpgrade upgrade;expect(upgrade,"ShopParameterUpgrade",{&upgrade.value_per_purchase,&upgrade.purchases});
    CharacterParameters character;expect(character,"CharacterParameters",{&character.values,&character.level,&character.flags});
    EquipmentParameters equipment;expect(equipment,"EquipmentParameters",{&equipment.level,&equipment.values});
    EntityIdState ids;expect(ids,"EntityIdState",{&ids.counter});
    ImageDrawingPosition drawing;expect(drawing,"ImageDrawingPosition",{&drawing.x,&drawing.y,&drawing.scale_x,&drawing.scale_y,&drawing.angle});
    SpriteMatrix matrix;expect(matrix,"SpriteMatrix",{&matrix.values,&matrix.concat_work,&matrix.rotation_work});
    CombatEquipment combat_equipment;expect(combat_equipment,"CombatEquipment",{&combat_equipment.parameters,&combat_equipment.attribute});
    CombatHuman human;expect(human,"CombatHuman",{&human.character,&human.equipment});
    CombatEnemy enemy;expect(enemy,"CombatEnemy",{&enemy.data,&enemy.level});
    CombatEntity entity;expect(entity,"CombatEntity",{&entity.human,&entity.enemy,&entity.parameters});
    CombatConfiguration config;expect(config,"CombatConfiguration",{&config.debug,&config.damage_diagnostics});
    AttributeAffinityTables affinities{};expect(affinities,"AttributeAffinityTables",{&affinities.pairs,&affinities.rates});
    CharacterProgressionTable characters;EnemyTable enemies;EquipmentCombatTable gear,hats,cloth;
    CombatRecordTables records{characters,enemies,gear,hats,cloth};expect(records,"CombatRecordTables",{&characters,&enemies,&gear,&hats,&cloth});
    EventIntegers character_levels,gear_levels,hat_levels,cloth_levels;CombatSavedLevels levels{character_levels,gear_levels,hat_levels,cloth_levels};
    expect(levels,"CombatSavedLevels",{&character_levels,&gear_levels,&hat_levels,&cloth_levels});
    MissionSelectionRecords selection;expect(selection,"MissionSelectionRecords",{&selection.area,&selection.quest});
    ResourceCatalogIndex index;expect(index,"ResourceCatalogIndex",{&index.ids,&index.rows});
    ApplicationAnimationResources resources;expect(resources,"ApplicationResourceFields",{&resources.characters,&resources.common,&resources.enemies,&resources.equipment,&resources.quest,&resources.residents,&resources.selection,
        &resources.chips,&resources.bullets,&resources.effects,&resources.hats,&resources.weapons,&resources.gauges});
}
void live_fields_and_aliases(){
    CharacterProgressionRecord character;character.required_experience_rate=17;character.base_name=std::make_shared<const std::string>("synthetic");
    auto row=std::make_shared<EventIntegers>(EventIntegers{3,5});character.parameters=std::make_shared<std::vector<EventInstruction>>(2,row);
    visit_state_fields(character,[&](std::string_view name,const auto&... values){const auto fields=std::tie(values...);
        check(name=="CharacterProgressionRecord"&&std::get<1>(fields)==17&&std::get<7>(fields)==character.base_name);
        check(std::get<5>(fields)->at(0)==std::get<5>(fields)->at(1)&&std::get<5>(fields)->at(0)==row);
    });
    EquipmentCombatRecord equipment;equipment.name="gear";equipment.resource="archive";equipment.initial_level=19;equipment.parameters=character.parameters;
    visit_state_fields(equipment,[&](std::string_view,const auto&... values){const auto fields=std::tie(values...);
        check(std::get<3>(fields)==character.parameters&&std::get<13>(fields)=="archive"&&std::get<14>(fields)==19&&std::get<15>(fields)=="gear");
    });
    EnemyRecord enemy;enemy.voice=23;enemy.enemy_seb=29;enemy.frame=31;enemy.treasures=EnemyTreasureGroups{{EventIntegers{37},EventIntegers{41}}};
    visit_state_fields(enemy,[&](std::string_view,const auto&... values){const auto fields=std::tie(values...);
        check(std::get<2>(fields)==29&&!std::get<3>(fields)&&std::get<4>(fields)->at(0).at(1).at(0)==41&&std::get<5>(fields)==23&&std::get<10>(fields)==31);
    });
    enemy.experience=EventIntegers{};visit_state_fields(enemy,[](std::string_view,const auto&... values){const auto fields=std::tie(values...);check(std::get<3>(fields)&&std::get<3>(fields)->empty());});
    BulletRecord bullet;bullet.category=43;bullet.attribute=47;bullet.effect_image=53;bullet.flags=59;bullet.impact_resource="impact";
    visit_state_fields(bullet,[&](std::string_view name,const auto&... values){const auto fields=std::tie(values...);
        check(name=="BulletRecord"&& &std::get<0>(fields)==static_cast<const BulletImpactData*>(&bullet)&&std::get<5>(fields)=="impact");
        visit_state_fields(std::get<0>(fields),[](std::string_view base,const auto& category,const auto& attribute,const auto& image,const auto& flags){check(base=="BulletImpactData"&&category==43&&attribute==47&&image==53&&flags==59);});
    });
    QuestDefinition first,second;first.record=std::make_shared<QuestRecord>();second.record=first.record;first.stage.completed=1;first.discovery_state=61;first.is_new=true;
    visit_state_fields(first,[&](std::string_view,const auto&... values){const auto fields=std::tie(values...);
        check(&std::get<5>(fields)==&first.stage&&std::get<5>(fields).completed==1&&std::get<6>(fields)==61&&std::get<7>(fields)&&std::get<8>(fields)==second.record);
    });
    TownDefinition town;town.open_term=character.parameters;town.rock_term=town.open_term;
    visit_state_fields(town,[](std::string_view,const auto&... values){const auto fields=std::tie(values...);check(std::get<3>(fields)==std::get<4>(fields));});
    PartyCharacter party;party.saved.experience=67;party.parameters.values=character.parameters;party.parameters.flags=7;
    visit_state_fields(party,[&](std::string_view,const auto&... values){const auto fields=std::tie(values...);
        check(&std::get<2>(fields)==&party.saved&&std::get<2>(fields).experience==67);
        visit_state_fields(std::get<5>(fields),[&](std::string_view,const auto& script,const auto&,const auto& flags){check(script==character.parameters&&flags==7);});
    });
    SpriteMatrix matrix;matrix.concat_work=matrix.values;matrix.rotation_work=std::make_shared<std::vector<float>>(9,-0.0F);
    visit_state_fields(matrix,[](std::string_view,const auto& values,const auto& concat,const auto& rotation){check(values==concat&&values!=rotation&&rotation->size()==9);});
    ImageDrawingPosition drawing;drawing.scale_y=-71;drawing.angle=73;visit_state_fields(drawing,[](std::string_view,const auto&,const auto&,const auto&,const auto& sy,const auto& angle){check(sy==-71&&angle==73);});
    AttributeAffinityTables affinities{};affinities.pairs[4][1]=79;affinities.rates[3]=83;
    visit_state_fields(affinities,[](std::string_view,const auto& pairs,const auto& rates){check(pairs[4][1]==79&&rates[3]==83);});
    AnimationResource shared;ApplicationAnimationResources resources;resources.characters=&shared;resources.common=&shared;
    visit_state_fields(resources,[&](std::string_view,const auto&... values){const auto fields=std::tie(values...);check(std::get<0>(fields)==std::get<1>(fields)&&std::get<0>(fields)==&shared&&!std::get<12>(fields));});
}
int main(){catalogs();caches_and_configurations();live_fields_and_aliases();}
