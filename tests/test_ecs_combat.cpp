#include "core/gameplay/ecs_combat.hpp"
#include "core/gameplay/projectile_hit.hpp"
#include <cstdlib>
#include <functional>
#include <stdexcept>
using namespace saga::core;
void check(bool ok){if(!ok)std::abort();}
EventScript rows(){auto v=std::make_shared<std::vector<EventInstruction>>();for(int i=0;i<13;++i)v->push_back(std::make_shared<EventIntegers>(EventIntegers{0,0}));return v;}
struct Draws final:RandomSource {
    std::vector<std::int32_t> values{0,5,99,0};std::size_t cursor=0;
    std::function<void()> first;
    std::int32_t next() override{auto n=values.at(cursor++);if(cursor==1&&first)first();return n;}
};
struct Presentation final:EcsCombatPresentation {
    std::int32_t damage=0,total=0;int misses=0,events=0;
    std::function<void()> listener;
    void miss(ComponentSlots&) override{++misses;}
    void effects(ComponentSlots&,const DamageNumbers& n) override{damage=n.amount;}
    void send_damage(ComponentSlots& receiver,std::int32_t n) override{++events;record_ecs_stage_damage(total,receiver,n);if(listener)listener();}
    void diagnostics(const DamageNumbers&) override{}
};
struct Fixture {
    CharacterProgressionTable characters{1};EnemyTable enemies{2};
    EquipmentCombatTable gear{2},hats{1},cloth{1};
    CombatRecordTables records{characters,enemies,gear,hats,cloth};
    EventIntegers character_levels{1},gear_levels{1,1},hat_levels{1},cloth_levels{1};
    CombatSavedLevels levels{character_levels,gear_levels,hat_levels,cloth_levels};
    ParameterMaximumLevels maxima=initial_parameter_maximum_levels(),enemy_maxima=initial_parameter_maximum_levels();
    ShopParameterTable shop;
    TableCharacterParameterHost growth{maxima,shop};
    ComponentSlots attacker{35},receiver{35};
    HumanRecordLink human;EnemyRecordLink enemy;EquipmentRecordLinks equipment;
    DamageParameter hp{200,200,0,0},energy{25,100,0,0},replacement{0,200,0,0};
    DamageParameterMap attack_map{{1,&energy}},receive_map{{0,&hp}};
    CombatParameterLinks attack_parameters,receive_parameters;
    AttributeAffinityTables affinities=initial_attribute_affinity_tables();CombatConfiguration config;
    Presentation presentation;
    EcsDamageHost host{attacker,receiver,records,levels,growth,enemy_maxima,affinities,config,presentation};
    Fixture(){
        characters[0].emplace();characters[0]->parameters=rows();
        for(auto& e:enemies){e.emplace();e->parameters=rows();e->attribute=2;}
        *enemies[0]->parameters->at(2)={40,40};
        for(auto* table:{&gear,&hats,&cloth})for(auto& e:*table){e.emplace();e->parameters=rows();}
        *gear[1]->parameters->at(8)={0,200};
        equipment.ids=std::make_shared<EventIntegers>(EventIntegers{0,0,0,0});enemy.level=1;
        attack_parameters.values=&attack_map;receive_parameters.values=&receive_map;
        attacker.add(combat_human_slot,&human).add(combat_equipment_slot,&equipment).add(combat_parameter_slot,&attack_parameters);
        receiver.add(combat_enemy_slot,&enemy).add(combat_parameter_slot,&receive_parameters);
    }
};
struct HitHost final:ProjectileHitHost {
    Fixture& fixture;Draws random;int defeats=0,impacts=0,pools=0;
    explicit HitHost(Fixture& f):fixture(f){}
    bool damage(ComponentSlots* attacker,ComponentSlots& target,std::int32_t value,std::int32_t attribute) override {
        check(attacker==&fixture.attacker && &target==&fixture.receiver);
        return apply_combat_damage(value,attribute,random,fixture.host);
    }
    void defeat(ComponentSlots&) override{++defeats;}
    void impact(ComponentSlots&,ComponentSlots*) override{++impacts;}
    void return_component(std::int32_t,Component*) override{++pools;}
};
int main(){
    {Fixture f;HitHost host(f);f.hp.value=1;ComponentSlots bullet(35);
        ProjectileLink projectile;projectile.parent=&f.attacker;ProjectileDamage damage;damage.value=100;
        ProjectileSpeed speed;speed.x=5;ProjectileCollision collision;
        bullet.add(projectile_slot,&projectile).add(projectile_damage_slot,&damage)
              .add(projectile_collision_slot,&collision).add(projectile_speed_slot,&speed);
        on_projectile_hit(&bullet,&f.receiver,host);
        check(f.hp.value==0&&f.presentation.total==44&&host.defeats==1&&host.impacts==1&&host.pools==2&&speed.x==0);
    }
    {Fixture f;Draws r;check(!apply_combat_damage(100,0,r,f.host));check(f.hp.value==156&&f.presentation.total==44&&r.cursor==4);}
    {Fixture f;set_combat_equipment(f.attacker,EquipmentSlot::weapon,1);f.gear_levels[1]=100;
        Draws r;apply_combat_damage(100,0,r,f.host);check(f.presentation.damage==137); // Live gear ID and saved level, critical.
        f.gear_levels[1]=1;Draws next;apply_combat_damage(100,0,next,f.host);check(f.presentation.damage==44);}
    {Fixture f;Draws r;r.first=[&]{f.enemy.enemy_id=1;};apply_combat_damage(100,0,r,f.host);
        check(f.presentation.damage==49); // Receiver record resolved after hit draw, no eager snapshot.
    }
    {Fixture f;Draws r;f.hp.value=1;f.presentation.listener=[&]{f.hp.value=2;f.receive_map[0]=&f.replacement;};
        check(!apply_combat_damage(100,0,r,f.host)&&f.hp.value==2); // Captured old parameter survives map replacement.
    }
    {Fixture f;f.attack_map[-7]=&f.energy;check(resolve_combat_parameter(f.attacker,-7)==&f.energy);
        f.attack_map[99]=nullptr;check(resolve_combat_parameter(f.attacker,99)==nullptr);
        bool failed=false;try{resolve_combat_parameter(f.attacker,0);}catch(const std::out_of_range&){failed=true;}check(failed);
        check(f.attack_map.size()==3); // Missing lookup must not insert a key.
    }
    {Fixture f;f.receive_map.clear();Draws r;r.values={99};check(!apply_combat_damage(100,0,r,f.host));
        check(f.presentation.misses==1&&r.cursor==1); // Miss never looks up absent receiver parameter key.
        Draws hit;bool failed=false;try{apply_combat_damage(100,0,hit,f.host);}catch(const std::out_of_range&){failed=true;}
        check(failed&&hit.cursor==1&&f.presentation.events==0);
    }
    {Fixture f;f.receiver.remove(combat_enemy_slot);check(f.host.parameter(CombatSide::receiver,9)==0);
        f.attacker.remove(combat_equipment_slot);bool failed=false;
        try{f.host.parameter(CombatSide::attacker,7);}catch(const std::invalid_argument&){failed=true;}check(failed);
    }
}
