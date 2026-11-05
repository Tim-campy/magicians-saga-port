#include "core/gameplay/combat_tables.hpp"
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool ok){if(!ok)std::abort();}
std::string read(const char* path){
    std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("cannot open table");
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
std::string endpoints(){std::string s="13";for(int i=0;i<13;++i)s+="\t2\t100\t1000";return s;}
template<class E,class F>void expect(F action){try{action();}catch(const E&){return;}check(false);}
struct ShopPriceHost:MagicShopPriceHost {
    std::vector<std::optional<std::int32_t>> purchases{1,2,3,4,5,6,7,8};
    LevelTable records=LevelTable(30);
    std::vector<std::string> calls;
    std::function<void(std::int32_t)> on_purchases,on_price;
    ShopPriceHost(){for(std::size_t i=0;i<records.size();++i){records[i].emplace();const auto n=static_cast<std::int32_t>(i);records[i]->shop_parameters={100+n,200+n,300+n};}}
    const std::int32_t* saved_purchases(std::int32_t slot)override{
        calls.push_back("P"+std::to_string(slot));if(on_purchases)on_purchases(slot);
        const auto& value=purchases.at(static_cast<std::size_t>(slot));return value?&*value:nullptr;
    }
    const LevelTableRecord* price_record(std::int32_t index)override{
        calls.push_back("T"+std::to_string(index));if(on_price)on_price(index);
        const auto& row=records.at(static_cast<std::size_t>(index));return row?&*row:nullptr;
    }
};
void magic_shop_economy_contract(){
    const MagicShopCombatRecord compatible{7,3,9,2};
    check(compatible.id==7&&compatible.category==3&&compatible.value_per_purchase==9&&compatible.flags==2&&
          compatible.base_price==0&&compatible.name.empty()&&compatible.explanation.empty());
    const std::array<std::int32_t,8> keys{0,1,2,3,4,10,11,12};
    for(std::int32_t category=0;category<8;++category){
        MagicShopCombatRecord row;row.category=category;row.base_price=37;
        ShopPriceHost h;const auto group=category<2?0:category<5?1:2;
        const std::vector<std::string> reads=group==0?std::vector<std::string>{"P0","P1"}:
            group==1?std::vector<std::string>{"P2","P3","P4"}:std::vector<std::string>{"P5","P6","P7"};
        const auto count=group==0?3:group==1?12:21;
        check(magic_shop_grouped_count(row,h)==count&&h.calls==reads);
        h.calls.clear();auto price_reads=reads;price_reads.push_back("T"+std::to_string(count));
        const auto price=group==0?38:group==1?78:118;
        check(magic_shop_price(row,h)==price&&h.calls==price_reads);
        check(magic_shop_parameter_key(category)==keys[static_cast<std::size_t>(category)]&&
              shop_id_for_parameter(magic_shop_parameter_key(category))==category);
    }
    for(const auto category:{std::numeric_limits<std::int32_t>::min(),-1,8,std::numeric_limits<std::int32_t>::max()}){
        MagicShopCombatRecord row;row.category=category;row.base_price=0;ShopPriceHost h;h.purchases.clear();h.records.clear();
        check(magic_shop_grouped_count(row,h)==std::numeric_limits<std::int32_t>::max()&&
              magic_shop_price(row,h)==std::numeric_limits<std::int32_t>::max()&&magic_shop_parameter_key(category)==-1&&h.calls.empty());
    }
    {MagicShopCombatRecord row;ShopPriceHost h;
        h.on_purchases=[&](std::int32_t slot){if(slot==1){h.purchases[0]=100;h.purchases[1]=4;}};
        check(magic_shop_grouped_count(row,h)==5&&h.calls==std::vector<std::string>({"P0","P1"}));}
    {MagicShopCombatRecord row;row.base_price=10;ShopPriceHost h;
        h.on_purchases=[&](std::int32_t slot){if(slot==0)row.category=5;};
        h.on_price=[&](std::int32_t index){check(index==3);row.base_price=37;row.category=0;};
        check(magic_shop_price(row,h)==112&&h.calls==std::vector<std::string>({"P0","P1","T3"}));
        check(row.base_price==37&&row.category==0);}
    {MagicShopCombatRecord row;ShopPriceHost h;
        h.on_purchases=[&](std::int32_t slot){if(slot==1)row.category=-1;};
        check(magic_shop_price(row,h)==std::numeric_limits<std::int32_t>::max()&&h.calls==std::vector<std::string>({"P0","P1"}));}
    {MagicShopCombatRecord row;row.base_price=std::numeric_limits<std::int32_t>::max();ShopPriceHost h;h.records[3]->shop_parameters[0]=200;
        check(magic_shop_price(row,h)==-2);row.base_price=std::numeric_limits<std::int32_t>::min();check(magic_shop_price(row,h)==0);
        row.base_price=-3;h.records[3]->shop_parameters[0]=150;check(magic_shop_price(row,h)==-4);
        row.base_price=-1;h.records[3]->shop_parameters[0]=99;check(magic_shop_price(row,h)==0);}
    for(const auto slot:{0,1,2,3,4,5,6,7}){
        MagicShopCombatRecord row;row.category=slot<2?0:slot<5?2:5;ShopPriceHost h;h.purchases[static_cast<std::size_t>(slot)].reset();
        expect<std::invalid_argument>([&]{magic_shop_price(row,h);});
        const auto first=slot<2?0:slot<5?2:5;std::vector<std::string> prefix;
        for(auto i=first;i<=slot;++i)prefix.push_back("P"+std::to_string(i));
        check(h.calls==prefix);
    }
    {MagicShopCombatRecord row;ShopPriceHost h;h.purchases.resize(1);
        expect<std::out_of_range>([&]{magic_shop_grouped_count(row,h);});check(h.calls==std::vector<std::string>({"P0","P1"}));}
    {MagicShopCombatRecord row;row.base_price=0;ShopPriceHost h;h.records[3].reset();
        expect<std::invalid_argument>([&]{magic_shop_price(row,h);});check(h.calls==std::vector<std::string>({"P0","P1","T3"}));}
    {MagicShopCombatRecord row;ShopPriceHost h;h.records.resize(3);
        expect<std::out_of_range>([&]{magic_shop_price(row,h);});check(h.calls==std::vector<std::string>({"P0","P1","T3"}));}
    {MagicShopCombatRecord row;ShopPriceHost h;h.purchases[0]=std::numeric_limits<std::int32_t>::max();h.purchases[1]=1;
        check(magic_shop_grouped_count(row,h)==std::numeric_limits<std::int32_t>::min());h.calls.clear();
        expect<std::out_of_range>([&]{magic_shop_price(row,h);});
        check(h.calls==std::vector<std::string>({"P0","P1","T-2147483648"}));}
    {MagicShopCombatRecord row;row.category=2;ShopPriceHost h;h.purchases[2]=std::numeric_limits<std::int32_t>::max();h.purchases[3]=1;h.purchases[4]=-1;
        check(magic_shop_grouped_count(row,h)==std::numeric_limits<std::int32_t>::max()&&h.calls==std::vector<std::string>({"P2","P3","P4"}));}
    {MagicShopCombatRecord row;ShopPriceHost h;
        h.on_purchases=[](std::int32_t slot){if(slot==1)throw std::runtime_error("synthetic purchase provider failure");};
        expect<std::runtime_error>([&]{magic_shop_price(row,h);});check(h.calls==std::vector<std::string>({"P0","P1"}));}
    {MagicShopCombatRecord row;ShopPriceHost h;
        h.on_price=[](std::int32_t){throw std::runtime_error("synthetic price provider failure");};
        expect<std::runtime_error>([&]{magic_shop_price(row,h);});check(h.calls==std::vector<std::string>({"P0","P1","T3"}));}
    std::int32_t purchases=std::numeric_limits<std::int32_t>::max();add_magic_shop_purchases(purchases,1);
    check(purchases==std::numeric_limits<std::int32_t>::min());add_magic_shop_purchases(purchases,-1);
    check(purchases==std::numeric_limits<std::int32_t>::max());purchases=-1;add_magic_shop_purchases(purchases,1);check(purchases==0);
    add_magic_shop_purchases(purchases,-7);check(purchases==-7);add_magic_shop_purchases(purchases,0);check(purchases==-7);
}
void equipment_shop_entries_contract(){
    const EquipmentCombatRecord compatible{7,3,2,{}};
    check(compatible.id==7&&compatible.attribute==3&&compatible.flags==2&&compatible.unlock_level==0&&
          compatible.buy_price==0&&compatible.explanation.empty());
    const auto row=[](std::int32_t id,std::int32_t level,std::int32_t flags){
        EquipmentCombatRecord value;value.id=id;value.unlock_level=level;value.flags=flags;return value;
    };
    {EquipmentCombatTable records{row(31,2,2),std::nullopt,row(19,-1,3),row(32,2,2),row(7,0,1),
         row(9,std::numeric_limits<std::int32_t>::min(),2),row(2,std::numeric_limits<std::int32_t>::max(),2)};
     std::vector<std::int32_t> output{77},calls;
     append_equipment_shop_entries(output,records,2,[&](const auto& value){calls.push_back(value.id);return value.id==19;});
     check(calls==std::vector<std::int32_t>({9,19,31,32,2}));
     check(output==std::vector<std::int32_t>({77,9,31,32,2}));}
    {EquipmentCombatTable records{row(5,0,2),row(5,0,2),row(5,0,2)};
     std::vector<std::int32_t> output,calls;std::size_t index=0;
     append_equipment_shop_entries(output,records,2,[&](const auto& value){
         check(&value==&*records.at(index));calls.push_back(value.id);return index++==1;
     });
     check(index==3&&calls==std::vector<std::int32_t>({5,5,5})&&output==std::vector<std::int32_t>({5,5}));}
    {EquipmentCombatTable records{row(3,30,2),row(1,10,2),row(2,20,0),row(4,40,2)};
     std::vector<std::int32_t> output,calls;
     append_equipment_shop_entries(output,records,2,[&](const auto& value){
         calls.push_back(value.id);
         if(value.id==1){records[0]->unlock_level=-100;records[2]->flags=2;records[3]->flags=0;}
         if(value.id==3)records[0]->id=44;
         return false;
     });
     // All keys/order were resolved before have; later flags and the appended id stay live.
     check(calls==std::vector<std::int32_t>({1,2,3})&&output==std::vector<std::int32_t>({1,2,44}));}
    {EquipmentCombatTable records{row(3,30,2),row(2,20,2),row(1,10,2)};
     std::vector<std::int32_t> output{90},calls;
     expect<std::runtime_error>([&]{append_equipment_shop_entries(output,records,2,[&](const auto& value){
         calls.push_back(value.id);if(value.id==2)throw std::runtime_error("synthetic have failure");return false;
     });});
     check(calls==std::vector<std::int32_t>({1,2})&&output==std::vector<std::int32_t>({90,1}));}
    {EquipmentCombatTable records{std::nullopt,row(1,0,0),row(2,0,std::numeric_limits<std::int32_t>::min())};
     std::vector<std::int32_t> output{77};
     append_equipment_shop_entries(output,records,0,{});check(output==std::vector<std::int32_t>({77}));
     append_equipment_shop_entries(output,records,0x80000000U,[](const auto&){return false;});
     check(output==std::vector<std::int32_t>({77,2}));
     expect<std::bad_function_call>([&]{append_equipment_shop_entries(output,records,0x80000000U,{});});
     check(output==std::vector<std::int32_t>({77,2}));}
    {std::vector<std::int32_t> output{77};append_equipment_shop_entries(output,{},2,{});
     check(output==std::vector<std::int32_t>({77}));}
}
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
    magic_shop_economy_contract();
    equipment_shop_entries_contract();
    const std::string character="2\tName\t7\tres\t8\tres2\t9\t2\t3\t4\t5\t150\t"+endpoints()+"\t6\t7\t8\t-1\t9\t258";
    auto chars=load_character_progression_table(character,false,parse_event_decimal);
    check(chars[2]->weapon_id==9&&chars[2]->initial_level==4&&chars[2]->flags==258);
    auto c=bind_character_parameters(*chars[2],1000);check(c.level==1000&&c.flags==2&&c.values==chars[2]->parameters);
    auto shop=load_magic_shop_combat_table("0\tUpgrade\t5\t123\t7\tDescription\t2\n2\tGap\t6\t1\t9\tD\t0",false,parse_event_decimal);
    check(shop.size()==3&&!shop[1]&&shop[0]->category==5&&shop[0]->value_per_purchase==7);
    check(shop[0]->name=="Upgrade"&&shop[0]->base_price==123&&shop[0]->explanation=="Description");
    {ShopNewGameState state{7,55,false};MagicShopCombatRecord record;record.flags=2;
     reset_shop_for_new_game(state,record,1);check(state.discovery_state==7&&!state.is_new&&state.purchases==0);
     state.purchases=55;reset_shop_for_new_game(state,record,2);check(state.discovery_state==1&&state.is_new&&state.purchases==0);
     state.purchases=55;int calls=0;reset_shop_for_new_game(state,record,2,[&](bool fresh){check(fresh&&state.purchases==55);++calls;state.purchases=8;});
     check(calls==1&&state.purchases==0);state.purchases=55;
     try{reset_shop_for_new_game(state,record,2,[&](bool){state.discovery_state=9;throw std::runtime_error("shop find");});check(false);}catch(const std::runtime_error&){}
     check(state.purchases==55&&state.discovery_state==9);}
    auto maxima=initial_parameter_maximum_levels();ShopParameterTable upgrades(3);upgrades[0]=bind_shop_upgrade(*shop[0],3);
    TableCharacterParameterHost host(maxima,upgrades);check(member_base_parameter(c,0,host)==1021);
    const std::string gear="3\tBlade\t1\t2\t3\t4\tRes\t5\t6\t2\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0\t"+endpoints()+"\tD\t4";
    auto weapons=load_equipment_combat_table(gear,EquipmentTableKind::weapon_or_shield,false,parse_event_decimal);
    check(weapons[3]->name=="Blade"&&weapons[3]->unlock_level==3&&weapons[3]->buy_price==0&&weapons[3]->explanation=="D");
    std::string priced=gear;
    for(const auto token:{12,4}){std::size_t where=0;for(int i=0;i<token;++i)where=priced.find('\t',where)+1;
        priced.replace(where,priced.find('\t',where)-where,token==12?"-2147483648":"2147483647");}
    const auto priced_weapons=load_equipment_combat_table(priced,EquipmentTableKind::weapon_or_shield,false,parse_event_decimal);
    check(priced_weapons[3]->unlock_level==std::numeric_limits<std::int32_t>::max()&&
          priced_weapons[3]->buy_price==std::numeric_limits<std::int32_t>::min()&&priced_weapons[3]->explanation=="D");
    auto e=bind_equipment_parameters(*weapons[3],100);check(e.attribute==2&&equipment_parameter(e.parameters,0)==1000);
    std::string ranged=gear;std::size_t at=0;
    for(int token=0;token<17;++token)at=ranged.find('\t',at)+1;
    ranged.replace(at,ranged.find('\t',at)-at,"-7");
    const auto ranged_weapons=load_equipment_combat_table(ranged,EquipmentTableKind::weapon_or_shield,false,parse_event_decimal);
    check(ranged_weapons[3]->range==-7&&ranged_weapons[3]->parameters->at(12)->at(1)==1000);
    std::string firing=gear;
    for(int token=19;token>=15;--token){std::size_t where=0;for(int i=0;i<token;++i)where=firing.find('\t',where)+1;
        firing.replace(where,firing.find('\t',where)-where,std::to_string(token));}
    const auto armed=load_equipment_combat_table(firing,EquipmentTableKind::weapon_or_shield,false,parse_event_decimal);
    check(armed[3]->distance==15&&armed[3]->burst_count==16&&armed[3]->range==17&&armed[3]->use_mp==18);
    check(armed[3]->initial_level==19);
    check(weapons[3]->type==2&&weapons[3]->bullet_id==6&&weapons[3]->max_height==0&&weapons[3]->bullet_speed==0);
    check(weapons[3]->image==5&&weapons[3]->resource=="Res");
    const std::string clothing="1\tHat\t7\tRes\t8\t9\t10\t11\t12\t"+endpoints()+"\tD\t6";
    auto hats=load_equipment_combat_table(clothing,EquipmentTableKind::cloth_or_hat,false,parse_event_decimal);
    check(hats[1]->flags==6&&hats[1]->parameters->at(12)->at(0)==100);
    check(hats[1]->image==8&&hats[1]->resource=="Res");
    check(hats[1]->initial_level==11);
    check(hats[1]->unlock_level==0&&hats[1]->buy_price==0&&hats[1]->explanation=="D");
    for(const auto flags:{0,1,2,3,-1}){
        EquipmentCombatRecord r;r.flags=flags;r.initial_level=-7;
        EquipmentNewGameState state{99,{4,true,88}};reset_equipment_for_new_game(state,r);
        check(state.level==-7&&state.inventory.stock==static_cast<std::int32_t>((static_cast<std::uint32_t>(flags)>>1)&1U));
        check(state.inventory.discovery_state==((flags&1)?1:4)&&state.inventory.is_new==((flags&1)?false:true));
    }
    {EquipmentCombatRecord r;r.flags=1;r.initial_level=5;EquipmentNewGameState state{99,{4,true,88}};int calls=0;
     reset_equipment_for_new_game(state,r,[&](bool fresh){check(!fresh);++calls;find_item_stock(state.inventory,fresh);r.flags=2;r.initial_level=17;});
     check(calls==1&&state.level==17&&state.inventory.stock==1&&state.inventory.discovery_state==1&&!state.inventory.is_new);
     r.flags=1;state.level=99;state.inventory.stock=88;
     try{reset_equipment_for_new_game(state,r,[&](bool){find_item_stock(state.inventory,true);throw std::runtime_error("find");});check(false);}catch(const std::runtime_error&){}
     check(state.level==99&&state.inventory.stock==88&&state.inventory.discovery_state==1&&state.inventory.is_new);}
    auto duplicate=load_magic_shop_combat_table("0\tA\t1\t2\t3\tD\t0\n0\tB\t4\t5\t6\tD\t1\textra",false,parse_event_decimal);
    check(duplicate[0]->value_per_purchase==6);
    check(duplicate[0]->name=="B"&&duplicate[0]->base_price==5&&duplicate[0]->explanation=="D");
    const auto signed_shop=load_magic_shop_combat_table("0\t name \t-1\t-2147483648\t-7\t explain \t-1\n",false,parse_event_decimal);
    check(signed_shop[0]->name==" name "&&signed_shop[0]->base_price==std::numeric_limits<std::int32_t>::min()&&
          signed_shop[0]->value_per_purchase==-7&&signed_shop[0]->explanation==" explain "&&signed_shop[0]->flags==-1);
    const auto empty_shop=load_magic_shop_combat_table("0\t\t0\t-3\t1\t\t1",false,parse_event_decimal);
    check(empty_shop[0]->name.empty()&&empty_shop[0]->explanation.empty()&&empty_shop[0]->base_price==-3);
    expect<std::invalid_argument>([]{load_magic_shop_combat_table("0\tN\t0\tbad\t1\tD\t1",false,parse_event_decimal);});
    auto nulls=load_equipment_combat_table("0\tHat\t0\tRes\t0\t0\t0\t0\t0\t-1\tD\t0",EquipmentTableKind::cloth_or_hat,false,parse_event_decimal);
    check(!nulls[0]->parameters);
    try{load_magic_shop_combat_table("1\tName\t2",false,parse_event_decimal);check(false);}catch(const std::out_of_range&){}
    try{load_equipment_combat_table(clothing.substr(0,clothing.size()-2),EquipmentTableKind::cloth_or_hat,false,parse_event_decimal);check(false);}catch(const std::out_of_range&){}
}
