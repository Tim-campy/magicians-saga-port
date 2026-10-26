#include "core/gameplay/combat_records.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool ok){if(!ok)std::abort();}
template<class E,class F>void fails(F fn){try{fn();check(false);}catch(const E&){} }
struct Observer final:ComponentObserver {
    int changes=0;void changed(ComponentSlots&,std::int32_t,Component*,Component*) override{++changes;}
};
int main(){
    CharacterProgressionTable characters(3);characters[2].emplace();characters[2]->id=2;
    EnemyTable enemies(4);enemies[3].emplace();enemies[3]->id=3;
    EquipmentCombatTable equipment(3),hats(3),cloth(3);
    equipment[1]=EquipmentCombatRecord{1,2,0,nullptr};equipment[2]=EquipmentCombatRecord{2,1,0,nullptr};
    hats[1]=EquipmentCombatRecord{1,0,10,nullptr};cloth[1]=EquipmentCombatRecord{1,0,20,nullptr};
    CombatRecordTables tables{characters,enemies,equipment,hats,cloth};
    Observer observer;ComponentSlots entity(35,&observer);
    HumanRecordLink human;human.character_id=2;
    EnemyRecordLink enemy;enemy.enemy_id=3;enemy.level=501;
    EquipmentRecordLinks gear;gear.ids=std::make_shared<EventIntegers>(EventIntegers{1,2,1,1});
    entity.add(combat_human_slot,&human).add(combat_enemy_slot,&enemy).add(combat_equipment_slot,&gear);
    check(resolve_combat_character(entity,tables)==&*characters[2]);
    check(resolve_combat_enemy(entity,tables)==&*enemies[3]);
    check(resolve_combat_equipment(entity,EquipmentSlot::weapon,tables)==&*equipment[1]);
    check(resolve_combat_equipment(entity,EquipmentSlot::shield,tables)==&*equipment[2]);
    check(resolve_combat_equipment(entity,EquipmentSlot::hat,tables)->flags==10);
    check(resolve_combat_equipment(entity,EquipmentSlot::cloth,tables)->flags==20);
    set_combat_equipment(entity,EquipmentSlot::shield,1);
    check(resolve_combat_equipment(entity,EquipmentSlot::shield,tables)==&*equipment[1]);
    set_combat_equipment(entity,EquipmentSlot::weapon,2);
    check(resolve_combat_equipment(entity,EquipmentSlot::weapon,tables)==&*equipment[2]&&observer.changes==0);
    equipment[2]->attribute=4;
    check(resolve_combat_equipment(entity,EquipmentSlot::weapon,tables)->attribute==4); // No copied record cache.
    set_combat_equipment(entity,EquipmentSlot::weapon,-1);
    check(gear.ids->at(0)==-1); // Setter does not validate record existence.
    fails<std::out_of_range>([&]{resolve_combat_equipment(entity,EquipmentSlot::weapon,tables);});
    set_combat_equipment(entity,EquipmentSlot::weapon,0);
    check(resolve_combat_equipment(entity,EquipmentSlot::weapon,tables)==nullptr);
    human.character_id=0;check(resolve_combat_character(entity,tables)==nullptr);
    human.character_id=-1;fails<std::out_of_range>([&]{resolve_combat_character(entity,tables);});
    enemy.enemy_id=4;fails<std::out_of_range>([&]{resolve_combat_enemy(entity,tables);});
    gear.ids->resize(1);fails<std::out_of_range>([&]{resolve_combat_equipment(entity,EquipmentSlot::shield,tables);});
    gear.ids.reset();fails<std::invalid_argument>([&]{set_combat_equipment(entity,EquipmentSlot::weapon,1);});
    entity.remove(combat_human_slot);fails<std::invalid_argument>([&]{resolve_combat_character(entity,tables);});
    entity.add(combat_human_slot,&enemy);fails<std::invalid_argument>([&]{resolve_combat_character(entity,tables);});
    // Lookup -> loaded-record projection -> existing combat math, with explicit saved level.
    entity.change(combat_human_slot,&human);human.character_id=2;
    characters[2]->parameters=std::make_shared<std::vector<EventInstruction>>();
    for(int i=0;i<13;++i)characters[2]->parameters->push_back(std::make_shared<EventIntegers>(EventIntegers{100,1000}));
    auto bound=bind_character_parameters(*resolve_combat_character(entity,tables),1100);
    const auto maxima=initial_parameter_maximum_levels();ShopParameterTable shops;
    TableCharacterParameterHost host(maxima,shops);
    check(member_base_parameter(bound,0,host)==1043);
}
