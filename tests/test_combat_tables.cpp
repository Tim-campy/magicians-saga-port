#include "core/gameplay/combat_tables.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace saga::core;
void check(bool ok){if(!ok)std::abort();}
std::string read(const char* path){
    std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("cannot open table");
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
std::string endpoints(){std::string s="13";for(int i=0;i<13;++i)s+="\t2\t100\t1000";return s;}
int main(int argc,char** argv){
    if(argc==6){
        const auto characters=load_character_progression_table(read(argv[1]),false,parse_event_decimal);
        const auto shop=load_magic_shop_combat_table(read(argv[2]),false,parse_event_decimal);
        auto maxima=initial_parameter_maximum_levels();ShopParameterTable upgrades(shop.size());
        for(std::size_t i=0;i<shop.size();++i)if(shop[i])upgrades[i]=bind_shop_upgrade(*shop[i],0);
        TableCharacterParameterHost host(maxima,upgrades);
        std::size_t character_count=0,equipment_count=0,queries=0,shop_count=0;
        std::uint64_t hash=14695981039346656037ULL;
        const auto record=[&](std::int32_t value){hash^=static_cast<std::uint32_t>(value);hash*=1099511628211ULL;++queries;};
        for(const auto& row:characters)if(row){++character_count;
            for(const auto level:{1,50,100,200,501,1000,1100,1200}){
                auto c=bind_character_parameters(*row,level);
                for(int index=0;index<13;++index)record(member_base_parameter(c,index,host));
            }
        }
        for(const auto& row:shop)if(row){++shop_count;record(shop_parameter_addition(bind_shop_upgrade(*row,3)));}
        for(int arg=3;arg<6;++arg){
            auto table=load_equipment_combat_table(read(argv[arg]),arg==3?EquipmentTableKind::weapon_or_shield:EquipmentTableKind::cloth_or_hat,false,parse_event_decimal);
            for(const auto& row:table)if(row){++equipment_count;
                for(const auto level:{1,50,100}){
                    auto e=bind_equipment_parameters(*row,level);
                    for(int index=0;index<13;++index)record(equipment_parameter(e.parameters,index));
                }
            }
        }
        std::cout<<"characters="<<character_count<<" shop="<<shop_count<<" equipment="<<equipment_count
                 <<" queries="<<queries<<" hash="<<hash<<'\n';return 0;
    }
    const std::string character="2\tName\t7\tres\t8\tres2\t9\t2\t3\t4\t5\t150\t"+endpoints()+"\t6\t7\t8\t-1\t9\t258";
    auto chars=load_character_progression_table(character,false,parse_event_decimal);
    check(chars[2]->weapon_id==9&&chars[2]->initial_level==4&&chars[2]->flags==258);
    auto c=bind_character_parameters(*chars[2],1000);check(c.level==1000&&c.flags==2&&c.values==chars[2]->parameters);
    auto shop=load_magic_shop_combat_table("0\tUpgrade\t5\t123\t7\tDescription\t2\n2\tGap\t6\t1\t9\tD\t0",false,parse_event_decimal);
    check(shop.size()==3&&!shop[1]&&shop[0]->category==5&&shop[0]->value_per_purchase==7);
    auto maxima=initial_parameter_maximum_levels();ShopParameterTable upgrades(3);upgrades[0]=bind_shop_upgrade(*shop[0],3);
    TableCharacterParameterHost host(maxima,upgrades);check(member_base_parameter(c,0,host)==1021);
    const std::string gear="3\tBlade\t1\t2\t3\t4\tRes\t5\t6\t2\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0\t"+endpoints()+"\tD\t4";
    auto weapons=load_equipment_combat_table(gear,EquipmentTableKind::weapon_or_shield,false,parse_event_decimal);
    auto e=bind_equipment_parameters(*weapons[3],100);check(e.attribute==2&&equipment_parameter(e.parameters,0)==1000);
    const std::string clothing="1\tHat\t7\tRes\t8\t9\t10\t11\t12\t"+endpoints()+"\tD\t6";
    auto hats=load_equipment_combat_table(clothing,EquipmentTableKind::cloth_or_hat,false,parse_event_decimal);
    check(hats[1]->flags==6&&hats[1]->parameters->at(12)->at(0)==100);
    auto duplicate=load_magic_shop_combat_table("0\tA\t1\t2\t3\tD\t0\n0\tB\t4\t5\t6\tD\t1\textra",false,parse_event_decimal);
    check(duplicate[0]->value_per_purchase==6);
    auto nulls=load_equipment_combat_table("0\tHat\t0\tRes\t0\t0\t0\t0\t0\t-1\tD\t0",EquipmentTableKind::cloth_or_hat,false,parse_event_decimal);
    check(!nulls[0]->parameters);
    try{load_magic_shop_combat_table("1\tName\t2",false,parse_event_decimal);check(false);}catch(const std::out_of_range&){}
    try{load_equipment_combat_table(clothing.substr(0,clothing.size()-2),EquipmentTableKind::cloth_or_hat,false,parse_event_decimal);check(false);}catch(const std::out_of_range&){}
}
