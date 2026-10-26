#include "core/gameplay/combat_parameters.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace saga::core;
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
EventScript rows(std::int32_t first,std::int32_t last) {
    auto result=std::make_shared<std::vector<EventInstruction>>();
    for(int i=0;i<13;++i)result->push_back(std::make_shared<EventIntegers>(EventIntegers{first,last}));
    return result;
}
struct Member final:MemberParameterHost {
    std::array<EquipmentParameters,4> gear;
    std::vector<int> calls;
    std::int32_t base=20;
    Member(){for(auto& g:gear)g={1,rows(4,40)};}
    std::int32_t base_parameter(std::int32_t index) override {calls.push_back(100+index);return base;}
    const EquipmentParameters& equipment(EquipmentSlot slot) override {
        auto index=static_cast<std::size_t>(slot);calls.push_back(static_cast<int>(index));
        return gear.at(index);
    }
};
struct Integration final:DamageHost {
    Member member;
    DamageParameter hp{200,200,0,0},energy{25,100,0,0};
    AttributeAffinityTables tables=initial_attribute_affinity_tables();
    DamageNumbers result;
    Integration(){
        for(auto& g:member.gear){g.values=rows(0,0);g.values->at(2)=std::make_shared<EventIntegers>(EventIntegers{4,40});
            g.values->at(12)=std::make_shared<EventIntegers>(EventIntegers{5,50});}
    }
    CombatKind kind(CombatSide) override{return CombatKind::human;}
    std::int32_t parameter(CombatSide,std::int32_t index) override {
        member.base=index==2?20:index==12?10:0;return member_total_parameter(member,index);
    }
    DamageParameter* receiver_parameter_zero() override{return &hp;}
    ReceiverDefence receiver_defence() override {
        const auto defence=parameter(CombatSide::receiver,2);
        member.base=10;const auto bonus=member_attribute_bonus(member,2);
        return {defence,2,bonus};
    }
    const AttributeAffinityTables& affinity_tables() override{return tables;}
    std::int32_t attacker_parameter_one_rate() override{return damage_parameter_rate(energy);}
    bool debug_enabled() override{return false;}
    void show_miss() override{throw std::logic_error("unexpected miss");}
    void after_damage(const DamageNumbers& n) override{result=n;}
};
struct Draws final:RandomSource {
    std::array<std::int32_t,4> values{{0,5,99,2}};std::size_t cursor=0;
    std::int32_t next() override{return values.at(cursor++);}
};
struct Growth final:CharacterParameterHost {
    std::int32_t maximum=100;
    std::vector<int> calls;
    ShopParameterUpgrade upgrade{7,3};
    std::int32_t maximum_level(std::int32_t index) override{calls.push_back(index);return maximum;}
    const ShopParameterUpgrade& shop_upgrade(std::int32_t id) override{calls.push_back(100+id);return upgrade;}
};
int main(){
    EquipmentParameters g{50,rows(0,198)};
    check(equipment_parameter(g,0)==98,"level 50 uses progress 49 of 99");
    g.level=1;check(equipment_parameter(g,12)==0,"first endpoint");
    g.level=100;check(equipment_parameter(g,2)==198,"last endpoint");
    g.level=1000;check(equipment_parameter(g,2)==198,"upper clamp");
    g.level=0;check(equipment_parameter(g,2)==0,"lower clamp");
    g.level=std::numeric_limits<std::int32_t>::min();check(equipment_parameter(g,2)==198,"level minus one wraps");
    g.values->at(0)=std::make_shared<EventIntegers>(EventIntegers{3});
    bool failed=false;try{equipment_parameter(g,0);}catch(const std::out_of_range&){failed=true;}
    check(failed,"short row faults before progress clamp");
    failed=false;try{equipment_parameter(g,-1);}catch(const std::out_of_range&){failed=true;}
    check(failed,"negative index faults");
    g.values->at(0)=nullptr;failed=false;try{equipment_parameter(g,0);}catch(const std::invalid_argument&){failed=true;}
    check(failed,"null row is not zero contribution");
    Member m;check(member_total_parameter(m,2)==36,"all four equipment contributions");
    check(m.calls==std::vector<int>{102,0,1,2,3},"base then weapon shield hat cloth order");
    m.calls.clear();check(member_attribute_bonus(m,-1)==0&&member_attribute_bonus(m,3)==0&&m.calls.empty(),"attribute unsigned guard skips providers");
    check(member_attribute_bonus(m,2)==36&&m.calls[0]==112,"attribute two selects parameter twelve");
    m.base=std::numeric_limits<std::int32_t>::max();check(member_total_parameter(m,2)==-2147483633,"total wraps");
    m.calls.clear();m.gear[1].values=nullptr;failed=false;
    try{member_total_parameter(m,2);}catch(const std::invalid_argument&){failed=true;}
    check(failed&&m.calls==std::vector<int>{102,0,1},"failure stops before hat and cloth");
    check(damage_parameter_rate({25,100,0,0})==25,"ordinary rate");
    check(damage_parameter_rate({25,0,25,100})==0,"zero base maximum guard ignores extra maximum");
    check(damage_parameter_rate({25,100,25,100})==25,"both extras participate");
    check(damage_parameter_rate({25,100,0,-100})==0,"zero total denominator SDIV");
    check(damage_parameter_rate({-1,100,0,0})==0&&damage_parameter_rate({101,100,0,0})==100,"rate clamps");
    check(damage_parameter_rate({std::numeric_limits<std::int32_t>::max(),100,0,0})==0,"rate multiply wraps before division");
    check(damage_parameter_rate({536870912,-1,0,0})==0,"SDIV INT_MIN divided by minus one");
    Integration h;Draws r;check(!apply_combat_damage(100,0,r,h),"equipment-bound damage survives");
    check(h.result.amount==58&&h.hp.value==142&&r.cursor==4,"defence 36 bonus 30 rate 25 compose to damage 58");
    Growth growth;CharacterParameters character{rows(10,1000),100,2};
    check(member_base_parameter(character,0,growth)==1021,"main character gets shop contribution after growth");
    check(growth.calls==std::vector<int>{0,100},"growth precedes shop lookup");
    check(member_base_parameter_at_level(character,0,1,growth)==31,"explicit level overrides stored level before shop bonus");
    character.level=1200;character.flags=0;growth.calls.clear();
    check(member_base_parameter(character,0,growth)==1086,"parameter zero extends by 86 at level 1200");
    check(growth.calls==std::vector<int>{0},"nonmain does not read shop");
    check(character_parameter(character,1,650,growth)==1043,"parameter one extension halfway");
    check(character_parameter(character,2,1200,growth)==1000,"other parameters have no extension");
    check(character_parameter(character,0,1,growth)==10&&character_parameter(character,0,100,growth)==1000,"growth endpoints");
    character.flags=2;growth.calls.clear();
    check(member_base_parameter(character,7,growth)==1000&&growth.calls==std::vector<int>{7},"unmapped parameter skips shop");
    for(int key=0;key<13;++key)check(shop_id_for_parameter(key)==(key<=4?key:key>=10?key-5:-1),"shop key mapping");
    check(shop_id_for_parameter(-1)==-1&&shop_id_for_parameter(13)==-1,"shop mapping rejects other indices");
    check(shop_parameter_addition({std::numeric_limits<std::int32_t>::max(),2})==-2,"shop product wraps");
    check(shop_parameter_addition({7,-3})==-21,"negative purchases preserved");
    character.values->at(0)=std::make_shared<EventIntegers>(EventIntegers{100,999});
    check(character_parameter(character,0,1200,growth)==1084,"extension truncates binary64 85.914 before easing");
    character.values->at(0)=std::make_shared<EventIntegers>(EventIntegers{100,-1000});
    check(character_parameter(character,0,1200,growth)==-1086,"descending negative extension");
    character.values->at(0)=nullptr;growth.calls.clear();failed=false;
    try{member_base_parameter(character,0,growth);}catch(const std::invalid_argument&){failed=true;}
    check(failed&&growth.calls.empty(),"invalid character row faults before maximum and shop providers");
}
