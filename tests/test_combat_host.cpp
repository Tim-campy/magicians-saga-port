#include "core/gameplay/combat_host.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool ok,const char* why){if(!ok){std::cerr<<why<<'\n';std::exit(1);}}
EventScript zero_rows(){
    auto rows=std::make_shared<std::vector<EventInstruction>>();
    for(int i=0;i<13;++i)rows->push_back(std::make_shared<EventIntegers>(EventIntegers{0,0}));
    return rows;
}
struct Draws final:RandomSource {
    std::vector<std::int32_t> values{0,5,99,0};std::size_t cursor=0;
    std::int32_t next() override{return values.at(cursor++);}
};
struct Presentation final:CombatPresentation {
    std::vector<std::string> calls;
    std::int32_t total=0,amount=0;
    std::function<void()> listener;
    void miss(CombatEntity&) override{calls.push_back("miss");}
    void effects(CombatEntity&,const DamageNumbers& n) override{calls.push_back("effects");amount=n.amount;}
    void send_damage(CombatEntity& receiver,std::int32_t value) override{
        calls.push_back("event15");record_stage_damage(total,&receiver,value);if(listener)listener();
    }
    void diagnostics(const DamageNumbers&) override{calls.push_back("diagnostics");}
};
struct Fixture {
    ParameterMaximumLevels character_maxima=initial_parameter_maximum_levels(),enemy_maxima=initial_parameter_maximum_levels();
    ShopParameterTable shop{8};
    TableCharacterParameterHost characters{character_maxima,shop};
    CharacterParameters character{zero_rows(),1,0};
    CombatEquipment gear{{1,zero_rows()},0};
    CombatHuman human{&character,{{&gear,&gear,&gear,&gear}}};
    EnemyRecord enemy;
    CombatEnemy monster{&enemy,1};
    DamageParameter hp{10,100,0,0},energy{25,100,0,0};
    DamageParameterMap attack_parameters{{1,&energy}},receive_parameters{{0,&hp}};
    CombatEntity attacker{&human,nullptr,&attack_parameters},receiver{nullptr,&monster,&receive_parameters};
    AttributeAffinityTables affinities=initial_attribute_affinity_tables();
    CombatConfiguration config;
    Presentation presentation;
    ResolvedDamageHost host{attacker,receiver,characters,enemy_maxima,affinities,config,presentation};
    Fixture(){enemy.parameters=zero_rows();enemy.attribute=2;*enemy.parameters->at(2)={40,40};}
};
int main(){
    {
        Fixture f;Draws r;
        check(apply_combat_damage(100,0,r,f.host),"integrated hit is lethal");
        check(f.hp.value==0&&f.presentation.amount==44&&f.presentation.total==44,"overkill records offered damage not lost HP");
        check(f.presentation.calls==std::vector<std::string>{"effects","event15"},"presentation then event order");
    }
    {
        Fixture f;Draws r;f.presentation.listener=[&]{f.hp.value=2;f.config={true,true};};
        check(!apply_combat_damage(100,0,r,f.host),"listener resurrection precedes death decision");
        check(f.presentation.calls==std::vector<std::string>{"effects","event15","diagnostics"},"live diagnostic flags read after event");
    }
    {
        Fixture f;Draws r;r.values={99};
        f.receiver.parameters=nullptr; // Miss must not resolve this.
        check(!apply_combat_damage(100,0,r,f.host)&&r.cursor==1,"miss consumes one draw");
        check(f.presentation.calls==std::vector<std::string>{"miss"}&&f.presentation.total==0,"miss does not send event");
    }
    {
        Fixture f;Draws r;f.attacker.human=nullptr;f.attacker.enemy=&f.monster;f.config.debug=true;
        check(!apply_combat_damage(100,0,r,f.host)&&f.hp.value==10,"nonhuman DEBUG protection");
        check(f.presentation.total==81&&r.cursor==4,"DEBUG-skipped damage still counted");
    }
    {
        Fixture f;f.receiver.human=&f.human;
        check(f.host.kind(CombatSide::receiver)==CombatKind::human&&f.host.parameter(CombatSide::receiver,2)==0,"human stat precedence");
        record_stage_damage(f.presentation.total,&f.receiver,7);
        check(f.presentation.total==7,"event counts enemy presence even with human component");
        f.receiver.enemy=nullptr;record_stage_damage(f.presentation.total,&f.receiver,5);
        check(f.presentation.total==7,"pure human receiver is not counted");
        f.receiver.human=nullptr;
        check(f.host.parameter(CombatSide::receiver,2)==0&&f.host.receiver_defence().attribute==4,"other entity defaults");
    }
    {
        Fixture f;f.receiver.human=&f.human;f.gear.attribute=2;
        *f.gear.parameters.values->at(2)={4,4};*f.gear.parameters.values->at(12)={5,5};
        const auto defence=f.host.receiver_defence();
        check(defence.parameter2==16&&defence.attribute==2&&defence.bonus==20,"equipment defence and weapon attribute bonus are bound");
        f.human.equipment[1]=nullptr;bool failed=false;
        try{f.host.parameter(CombatSide::receiver,2);}catch(const std::invalid_argument&){failed=true;}
        check(failed,"missing equipment faults instead of falling back to enemy");
    }
    {
        Fixture f;*f.enemy.parameters->at(2)={100,1000};f.monster.level=501;
        check(f.host.parameter(CombatSide::receiver,2)==407,"enemy level reaches curved growth");
        f.enemy_maxima[2]=200;check(f.host.parameter(CombatSide::receiver,2)==1000,"live enemy maximum binding");
        f.presentation.total=std::numeric_limits<std::int32_t>::max();
        record_stage_damage(f.presentation.total,&f.receiver,1);
        check(f.presentation.total==std::numeric_limits<std::int32_t>::min(),"stage damage wraps");
        bool failed=false;try{record_stage_damage(f.presentation.total,nullptr,1);}catch(const std::invalid_argument&){failed=true;}
        check(failed,"null event entity faults");
    }
}
