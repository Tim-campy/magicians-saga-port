#include "core/gameplay/select_touch_move.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cmath>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"touch move contract failure\n";std::exit(1);}}
struct Host:SelectTouchMoveHost {
    std::shared_ptr<TownHumanRows> rows=std::make_shared<TownHumanRows>();
    int queries=0,bubbles=0;bool fail=false;
    std::shared_ptr<TownHumanRows> town_humans() override {++queries;return rows;}
    void set_talk_bubble(std::int32_t index,bool option) override {check(index==0 && !option);++bubbles;if(fail)throw std::runtime_error("bubble");}
};
int main() {
    Host h;SelectTouchMoveState s{false,0,10,8,9};update_select_touch_move(s,h);check(h.queries==0 && s.collision_id==9);
    s.enabled=true;update_select_touch_move(s,h);check(s.witch_x==6 && s.direction==1 && s.collision_id==-1);
    update_select_touch_move(s,h);check(s.witch_x==10 && s.direction==1);
    s={true,10,0,0,-1};update_select_touch_move(s,h);check(s.witch_x==6 && s.direction==2);
    s={true,-5,-4,0,-1};update_select_touch_move(s,h);check(s.witch_x==0 && s.direction==1); // abs target allows overshoot.
    s={true,0,0,8,-1};update_select_touch_move(s,h);check(s.direction==8);
    auto row=std::make_shared<EventIntegers>(9,0);h.rows->push_back(row);
    s={true,0,0,0,-1};update_select_touch_move(s,h);check(h.bubbles==1 && s.collision_id==0);
    update_select_touch_move(s,h);check(h.bubbles==1);
    row->at(1)=5;update_select_touch_move(s,h);check(s.collision_id==-1); // Strict distance, not <=5.
    row->at(1)=4;row->at(8)=2;update_select_touch_move(s,h);check(s.collision_id==-1);
    row->at(8)=0;h.fail=true;
    try{update_select_touch_move(s,h);check(false);}catch(const std::runtime_error&){}
    check(s.collision_id==-1);h.fail=false;
    const auto nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    s={true,nan,0,7,4};update_select_touch_move(s,h);check(std::isnan(s.witch_x) && s.direction==7 && s.collision_id==-1);
    s={true,inf,0,0,-1};update_select_touch_move(s,h);check(s.witch_x==-2147483648.0F && s.direction==2);
    s={true,-inf,0,0,-1};update_select_touch_move(s,h);check(s.witch_x==-2147483648.0F);
    s={true,2147483648.0F,0,0,-1};update_select_touch_move(s,h);check(s.witch_x==2147483648.0F);
    h.rows->at(0)=nullptr;
    try{update_select_touch_move(s,h);check(false);}catch(const std::invalid_argument&){}
}
