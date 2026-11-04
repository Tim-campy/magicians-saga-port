#include "core/gameplay/town_residents.hpp"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
template<class Exception,class Action>void fails(Action action){bool caught=false;try{action();}catch(const Exception&){caught=true;}check(caught,"expected failure");}
EventScript script(){return std::make_shared<std::vector<EventInstruction>>();}
struct Host final:TownResidentInitializationHost {
    std::deque<CharacterProgressionRecord> definitions;
    std::deque<CharacterNewGameState> progress;
    std::shared_ptr<TownResidentCharacters> data=std::make_shared<TownResidentCharacters>();
    std::shared_ptr<TownResidentCharacters> living=std::make_shared<TownResidentCharacters>();
    EventInstruction active=std::make_shared<EventIntegers>(EventIntegers{-100,810});
    std::vector<std::string> log;
    std::deque<std::int32_t> app_values,lib_values;
    std::function<void(const std::string&)> action;
    std::int32_t challenges=0,limit=0,range_value=4;
    bool term_result=true;
    std::shared_ptr<TownResidentCharacter> add(std::int32_t id,std::int32_t flags=0x20){
        definitions.emplace_back();auto& d=definitions.back();d.id=id;d.flags=flags;d.town_id=-1;d.area_id=-1;d.appear_rate=100;
        progress.emplace_back();auto& p=progress.back();p.living_address=-1;
        auto row=std::make_shared<TownResidentCharacter>(TownResidentCharacter{&d,&p});data->push_back(row);return row;
    }
    void call(const std::string& name){log.push_back(name);if(action)action(name);}
    std::int32_t challenge_count()override{call("challenge");return challenges;}
    std::int32_t store_limit()override{call("limit");return limit;}
    std::shared_ptr<TownResidentCharacters> characters()override{call("characters");return data;}
    std::shared_ptr<TownResidentCharacters> living_submembers()override{call("living");return living;}
    EventInstruction active_range()override{call("active");return active;}
    std::array<std::int32_t,2> building_position(std::int32_t index)override{call("B"+std::to_string(index));return town_building_position(index,600);}
    std::array<std::int32_t,2> area_position(std::int32_t index)override{call("P"+std::to_string(index));return {-160-index*129,400};}
    bool conditions(const EventScript& terms)override{check(terms&&!terms->empty(),"only nonempty terms reach provider");call("terms");return term_result;}
    std::int32_t app_random(std::int32_t bound)override{
        call("A"+std::to_string(bound));if(app_values.empty())return 0;
        const auto value=app_values.front();app_values.pop_front();return value;
    }
    std::int32_t lib_random(std::int32_t bound)override{
        call("L"+std::to_string(bound));if(lib_values.empty())return bound==100?99:0;
        const auto value=lib_values.front();lib_values.pop_front();return value;
    }
    std::int32_t lib_random(std::int32_t low,std::int32_t high)override{call("R"+std::to_string(low)+":"+std::to_string(high));return range_value;}
};
void fresh_present_default(){
    Host host;TownResidentInitializationState state;initialize_town_residents(state,host);
    check(state.residents->empty()&&state.present_value==100,"fresh constructor present100 survives Init when the final gift probability misses");
    check(host.log.back()=="L100","missing gift does not consume a replacement present amount draw");
}
void discarded_shuffle_order(){
    Host h;for(int id=0;id<3;++id){h.add(id);h.definitions.back().town_id=id;}
    h.lib_values={2,0,5,6,7,99};h.app_values={10,101,20,102,30,103};TownResidentInitializationState state{script(),777};
    initialize_town_residents(state,h);
    check(state.residents->size()==3&&state.present_value==777,"ordinary residents retain present value");
    for(int i=0;i<3;++i){const auto& r=*state.residents->at(static_cast<std::size_t>(i));
        check(r==EventIntegers({i,130+90*i,0,0,0,38,2,1,0,10+10*i,101+i}),"discarded shuffle preserves original ID/order and exact row layout");}
    check(h.log==std::vector<std::string>({"challenge","limit","characters","L3","L2","living","characters","active",
        "L100","B0","A60","A9999","L100","B1","A60","A9999","L100","B2","A60","A9999","L100"}),"shuffle consumes descending Lib draws before living/candidate evaluation");
}
void living_and_visitor(){
    Host h;auto resident=h.add(1,4),visitor=h.add(2,4);resident->progress->living_address=2;visitor->progress->recruited=1;
    *h.living={nullptr,nullptr,resident};TownResidentInitializationState state;
    initialize_town_residents(state,h);
    check(state.residents->size()==2&&state.residents->at(0)->at(0)==1&&state.residents->at(1)->at(0)==2,"living address order precedes optional recruited visitor");
    check(state.residents->at(0)->at(1)==310&&state.residents->at(1)->at(1)==175,"living uses its address; visitor sub uses building0+45");
    check(std::count(h.log.begin(),h.log.end(),"A1")==1,"nonempty singleton visitor still invokes AppData.Random(1)");
    Host many;auto row=many.add(5,4);row->progress->living_address=0;many.living->assign(9,row);TownResidentInitializationState crowded;
    initialize_town_residents(crowded,many);
    check(std::count(many.log.begin(),many.log.end(),"characters")==1,"more than eight living skips the recruited scan");
    check(crowded.residents->size()==1&&std::count(many.log.begin(),many.log.end(),"L100")==10,"duplicate living candidates each consume Hits before dedup");
}
void terms_and_store_gate(){
    Host h;h.add(0);h.definitions.back().appear_rate=0;h.definitions.back().resident_terms=script();h.definitions.back().resident_terms->push_back(std::make_shared<EventIntegers>(EventIntegers{7,1,1}));
    h.add(1);h.definitions.back().resident_terms=h.definitions.front().resident_terms;h.term_result=false;
    h.lib_values={0,0,0,99};TownResidentInitializationState state;
    initialize_town_residents(state,h);
    check(state.residents->empty()&&std::count(h.log.begin(),h.log.end(),"terms")==1,"Hits precedes terms and rejected candidate skips all placement draws");
    Host dedup;dedup.add(10,0xa0);dedup.definitions.back().town_id=2;dedup.add(11);dedup.definitions.back().town_id=2;
    TownResidentInitializationState blocked;initialize_town_residents(blocked,dedup);
    check(blocked.residents->empty()&&std::count(dedup.log.begin(),dedup.log.end(),"B2")==2,"suppressed store candidate still claims its placement key");
    Host fallback;fallback.add(2,0xa0);fallback.app_values={2};TownResidentInitializationState suppressed;
    initialize_town_residents(suppressed,fallback);
    check(suppressed.residents->empty()&&std::count(fallback.log.begin(),fallback.log.end(),"A3")==1&&std::count(fallback.log.begin(),fallback.log.end(),"A810")==0,"store suppression consumes fallback lane but skips fallback X");
}
void fallback_flags_and_gift(){
    Host h;h.add(8,0xe0);h.limit=-1;h.app_values={2,44,5,7};TownResidentInitializationState state;
    initialize_town_residents(state,h);
    check(*state.residents->at(0)==EventIntegers({8,44,0,0,0,38,2,0,1,65,7}),"store forces lane0 after random lane; stop flag controls movement/frame+60");
    check(h.log==std::vector<std::string>({"challenge","limit","characters","living","characters","active","L100","A3","A810","A60","A9999","L100"}),"fallback and row RNG order");
    Host gift;gift.add(3);gift.definitions.back().town_id=0;gift.lib_values={0,0};TownResidentInitializationState present{script(),17};
    initialize_town_residents(present,gift);
    check(present.residents->at(0)->at(8)==3&&present.present_value==400&&gift.log.back()=="R2:5","gift converts first positive-X type0 and draws present amount");
    Host empty;empty.lib_values={0};empty.range_value=5;TownResidentInitializationState absent{script(),17};initialize_town_residents(absent,empty);
    check(absent.residents->empty()&&absent.present_value==500,"gift amount draw occurs even with no eligible row");
    Host negative;negative.add(4);negative.definitions.back().area_id=0;negative.lib_values={0,0};TownResidentInitializationState area;
    initialize_town_residents(area,negative);
    check(area.residents->at(0)->at(1)==-160&&area.residents->at(0)->at(8)==0&&area.present_value==400,"negative area position is not gift-eligible");
}
void aliases_and_failure_prefixes(){
    Host h;h.add(0);TownResidentInitializationState state{script(),0};const auto previous=state.residents;EventScript captured;
    h.action=[&](const auto& name){if(name=="A810"){captured=state.residents;state.residents=script();}};
    initialize_town_residents(state,h);
    check(captured&&captured!=previous&&captured->size()==1&&state.residents->empty(),"row append retains destination captured before fallback callback");
    Host failed;failed.add(0);failed.add(1);failed.lib_values={-1};TownResidentInitializationState failure{script(),9};const auto old=failure.residents;
    fails<std::out_of_range>([&]{initialize_town_residents(failure,failed);});
    check(failure.residents!=old&&failure.residents->empty()&&failure.present_value==9,"discarded shuffle retains native invalid-index failure and fresh publication");
    Host missing;missing.data.reset();TownResidentInitializationState no_data{script(),2};const auto initial=no_data.residents;
    fails<std::invalid_argument>([&]{initialize_town_residents(no_data,missing);});check(no_data.residents!=initial,"missing catalog fails after fresh resident list publication");
    Host early;early.action=[](const auto& name){if(name=="limit")throw std::runtime_error("limit");};TownResidentInitializationState unchanged{script(),4};const auto kept=unchanged.residents;
    fails<std::runtime_error>([&]{initialize_town_residents(unchanged,early);});check(unchanged.residents==kept,"pre-list captured system input failure leaves old owner");
    Host flags;flags.add(1);flags.definitions.back().town_id=0;TownResidentInitializationState late;
    flags.action=[&](const auto& name){if(name=="A60")flags.definitions.back().flags|=0x40;};initialize_town_residents(late,flags);
    check(late.residents->at(0)->at(7)==1&&late.residents->at(0)->at(9)==60,"movement flag precedes Random60 but frame flag is reread after callback");
}
struct Geometry final:TownResidentGeometryHost {
    std::int32_t x=0,y=-80,left=-729,right=960,vx=0,vy=53,height=640,capacity=4,towns=19;
    std::shared_ptr<CameraAreas> areas=std::make_shared<CameraAreas>();
    std::vector<std::string> log;std::function<void(const std::string&)> action;
    Geometry(){areas->push_back(std::make_shared<CameraAreaRecord>(CameraAreaRecord{1}));areas->push_back(std::make_shared<CameraAreaRecord>(CameraAreaRecord{0}));}
    TownResidentGeometryState state(){return {x,y,left,right,vx,vy};}
    void call(const std::string& name){log.push_back(name);if(action)action(name);}
    std::int32_t resident_view_height()override{call("height");return height;}
    std::int32_t resident_town_capacity()override{call("capacity");return capacity;}
    std::int32_t resident_town_record_count()override{call("towns");return towns;}
    std::shared_ptr<CameraAreas> resident_geometry_areas()override{call("areas");return areas;}
};
void geometry_contract(){
    Geometry h;check(town_obstacle_position(h.state(),h)==std::array<std::int32_t,2>{840,543},"native obstacle point");h.log.clear();
    check(town_resident_active_range(h.state(),h)==std::array<std::int32_t,2>{-639,810},"fresh real context active range");
    check(h.log==std::vector<std::string>({"capacity","towns","height","areas","areas"}),"active bounds preserve unused obstacle height query");
    Geometry full;full.capacity=19;full.areas->back()->state=1;
    check(town_resident_active_range(full.state(),full)==std::array<std::int32_t,2>{-669,900}&&std::count(full.log.begin(),full.log.end(),"height")==0,"full town skips obstacle and found final area keeps first left bound");
    Geometry live;live.action=[&](const auto& name){if(name=="capacity")live.x=100;if(name=="height"){live.y=100;live.vy=99;}};
    check(town_resident_active_range(live.state(),live)==std::array<std::int32_t,2>{-639,910},"active range retains earlier cameraX across obstacle callback");
    Geometry captured;captured.action=[&](const auto& name){if(name=="height"){captured.y=500;captured.vy=500;}};
    check(town_obstacle_position(captured.state(),captured)==std::array<std::int32_t,2>{840,543},"obstacle captures cameraY/viewY before height callback");
    Geometry empty;empty.areas->clear();fails<std::out_of_range>([&]{town_resident_active_range(empty.state(),empty);});
    check(town_building_position(2,640)==std::array<std::int32_t,2>{310,540},"building geometry");
    const auto min=std::numeric_limits<std::int32_t>::min(),max=std::numeric_limits<std::int32_t>::max();
    check(town_building_position(max,min)==std::array<std::int32_t,2>{40,max-99},"building multiplication and height subtraction wrap32");
}
}
int main(){fresh_present_default();discarded_shuffle_order();living_and_visitor();terms_and_store_gate();fallback_flags_and_gift();aliases_and_failure_prefixes();geometry_contract();return 0;}
