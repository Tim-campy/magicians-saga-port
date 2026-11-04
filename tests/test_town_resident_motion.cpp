#include "core/gameplay/town_residents.hpp"
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
EventInstruction row(std::int32_t x=50,std::int32_t vx=0,std::int32_t move=0,std::int32_t type=0,std::int32_t frame=0){
    return std::make_shared<EventIntegers>(EventIntegers{7,x,2,vx,19,123,2,move,type,frame,456});
}
EventScript rows(std::initializer_list<EventInstruction> values){return std::make_shared<std::vector<EventInstruction>>(values);}
struct Host final:TownResidentUpdateHost {
    EventInstruction bounds=std::make_shared<EventIntegers>(EventIntegers{0,100});
    EventScript source=rows({});float hero=40;int range_queries=0,hero_queries=0;
    std::deque<std::pair<std::int32_t,std::int32_t>> random;
    std::function<void(std::int32_t)> on_random;
    EventInstruction active_range()override{++range_queries;return bounds;}
    EventScript resident_rows()override{return source;}
    float resident_hero_x()override{++hero_queries;return hero;}
    std::int32_t app_random(std::int32_t bound)override{
        check(!random.empty()&&random.front().first==bound,"resident RNG bound/order");
        const auto value=random.front().second;random.pop_front();if(on_random)on_random(bound);return value;
    }
};
void facing_and_wrap(){
    Host h;auto person=row(50,2,0,0,std::numeric_limits<std::int32_t>::max());h.source=rows({person});
    check(update_town_residents(h),"native resident update succeeds with true");
    check(*person==EventIntegers({7,52,2,2,19,38,2,0,0,std::numeric_limits<std::int32_t>::min(),456}),"static resident advances frame/X, faces left and preserves unrelated fields");
    check(h.range_queries==1&&h.hero_queries==1&&h.random.empty(),"range captured once; static facing consumes no RNG");
    h.hero=std::numeric_limits<float>::quiet_NaN();update_town_residents(h);
    check(person->at(5)==37&&person->at(6)==1,"unordered hero comparison selects direction1");
    auto unknown=row(20,-1,7);h.source=rows({unknown});update_town_residents(h);
    check(unknown->at(5)==123&&unknown->at(6)==2&&h.hero_queries==2,"unknown MOVE preserves direction/clip without querying hero");
}
void walking_and_clamps(){
    Host h;auto walking=row(50,-1,1,0,60);h.source=rows({walking});h.random={{100,7}};
    update_town_residents(h);check(walking->at(1)==49&&walking->at(5)==42&&walking->at(6)==2&&walking->at(9)==61,"moving direction/OR40 and nonzero random retain the growing frame");
    auto stopped=row(50,0,1,0,60);h.source=rows({stopped});h.random={{100,0},{5,0}};
    update_town_residents(h);check(stopped->at(3)==-1&&stopped->at(9)==0&&stopped->at(5)==38,"velocity change/reset follows animation selection without recomputing clip");
    auto right=row(99,8,1);auto left=row(1,-8,1);left->at(6)=1;h.source=rows({right,left});
    update_town_residents(h);check(right->at(1)==100&&right->at(3)==0&&right->at(5)==38,"right clamp forces idle despite captured positive velocity");
    check(left->at(1)==0&&left->at(3)==0&&left->at(5)==37,"left clamp resets local velocity before choosing idle");
}
void type_and_owner_aliases(){
    Host h;auto special=row(1000,-3,2,2);h.bounds.reset();h.source=rows({special});update_town_residents(h);
    check(special->at(1)==997&&special->at(3)==8&&special->at(5)==9&&special->at(6)==1,"captured type2 bypasses bounds and final override forces velocity/clip/direction");
    h.bounds=std::make_shared<EventIntegers>(EventIntegers{0,100});auto first=row(99,8,1,0,60);h.source=rows({first});h.random={{100,1}};
    auto untouched=row(20,0,7,0,10);auto later=row(30,0,7,0,20);
    h.on_random=[&](auto){first->at(8)=2;h.source=rows({untouched,later});};
    update_town_residents(h);
    check(first->at(1)==100&&first->at(3)==8&&first->at(5)==9,"initial type controls clamp but final type is reread after RNG");
    check(untouched->at(9)==10&&later->at(9)==21,"next index resolves current list; earlier row alias remains owned");
}
void failure_prefixes(){
    Host h;auto short_row=std::make_shared<EventIntegers>(9,0);h.source=rows({short_row});
    bool caught=false;try{update_town_residents(h);}catch(const std::out_of_range&){caught=true;}
    check(caught&&h.range_queries==1&&short_row->at(1)==0,"ten-cell gate precedes frame/X writes");
    auto person=row(50,2,1,0,60);h.source=rows({person});h.random={{100,0},{5,0}};
    h.on_random=[&](auto bound){if(bound==5)person->resize(4);};caught=false;
    try{update_town_residents(h);}catch(const std::out_of_range&){caught=true;}
    check(caught&&person->at(1)==52&&person->at(3)==-1,"late short row retains position and velocity write before frame-reset failure");
    Host absent;absent.source.reset();caught=false;try{update_town_residents(absent);}catch(const std::invalid_argument&){caught=true;}
    check(caught&&absent.range_queries==1,"active range query precedes missing-list failure");
}
}
int main(){facing_and_wrap();walking_and_clamps();type_and_owner_aliases();failure_prefixes();}
