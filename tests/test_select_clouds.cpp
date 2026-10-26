#include "core/gameplay/select_clouds.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"cloud contract failure\n";std::exit(1);}}
struct Host:SelectCloudHost {
    std::shared_ptr<SelectClouds> list=std::make_shared<SelectClouds>();
    std::vector<int> bounds;
    bool low=false,fail=false;
    std::shared_ptr<SelectClouds> clouds() override{return list;}
    std::int32_t right_edge() override{return 100;}
    std::int32_t cloud_resource() override{return 77;}
    std::int32_t game_height() override{return 500;}
    bool low_aspect() override{return low;}
    std::int32_t random(std::int32_t n) override{bounds.push_back(n);if(fail)throw std::runtime_error("RNG");return n==30 ? 9:2;}
};
auto cloud(int x,int tick=19) {return std::make_shared<EventIntegers>(EventIntegers{x,0,0,0,0,0,tick,20});}
int main() {
    Host h;std::int32_t counter=3;
    auto a=cloud(100),b=cloud(101);h.list->assign({a,b});
    update_select_clouds(counter,h);
    check(h.bounds==std::vector<int>({30,4,30,4}));
    check(a->at(0)==101 && b->at(0)==102 && a->at(6)==0);
    check(h.list->size()==2 && *h.list->at(0)==EventIntegers({-30,211,77,0,1,2,0,20}));
    check(h.list->at(1)->at(6)==0); // Newly appended clouds are not updated again.
    h.bounds.clear();a=cloud(100);h.list->assign({a});counter=2;
    update_select_clouds(counter,h);check(a->at(0)==100 && a->at(6)==0 && h.bounds.empty());
    a->at(6)=std::numeric_limits<int>::max();update_select_clouds(counter,h);
    check(a->at(6)==std::numeric_limits<int>::min());
    h.low=true;h.list->assign({cloud(101)});counter=-3;update_select_clouds(counter,h);
    check(h.list->at(0)->at(1)==206);
    h.fail=true;h.list->assign({cloud(101)});
    try{update_select_clouds(counter,h);check(false);}catch(const std::runtime_error&){}
    check(h.list->empty()); // Removal precedes RNG failure, no rollback.
    h.list->assign({std::make_shared<EventIntegers>(7,0)});
    try{update_select_clouds(counter,h);check(false);}catch(const std::out_of_range&){}
    check(h.list->at(0)->at(6)==1); // Tick mutation precedes missing threshold failure.
}
