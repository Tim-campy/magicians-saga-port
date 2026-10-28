#include "core/gameplay/animation_resources.hpp"
#include <cstdlib>
#include <limits>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class E,class F>void fails(F f){bool caught=false;try{f();}catch(const E&){caught=true;}check(caught);}
int main(){
    {const auto c=index_resource_catalog({2,-1,2,-1,8,-1});
     check(c.ids==std::vector<std::int32_t>{2,0,2,1,8,3}&&c.rows.size()==9);
     check(c.rows[0]==1&&c.rows[1]==3&&c.rows[2]==2&&c.rows[3]==5&&c.rows[8]==4);
     check(!c.rows[4]&&!c.rows[7]);}
    {const auto c=index_resource_catalog({0,0,0});check(c.rows.size()==3&&c.rows[0]==2&&!c.rows[1]&&!c.rows[2]);}
    check(index_resource_catalog({}).rows.empty());
    check(index_resource_catalog({-1,-1,-1}).ids==std::vector<std::int32_t>{0,1,2});
    fails<std::out_of_range>([]{index_resource_catalog({-2});});
    fails<std::out_of_range>([]{index_resource_catalog({std::numeric_limits<std::int32_t>::max()});});
    fails<std::length_error>([]{index_resource_catalog({8},8);});
    AnimationResource r[13];ApplicationAnimationResources app;
    app.characters=&r[0];app.common=&r[1];app.enemies=&r[2];app.equipment=&r[3];
    app.quest=&r[4];app.residents=&r[5];app.selection=&r[6];app.chips=&r[7];app.bullets=&r[8];
    app.effects=&r[9];app.hats=&r[10];app.weapons=&r[11];app.gauges=&r[12];
    const std::vector<int> supported{1,2,3,4,7,8,11,18,19,20,21,22,23};
    std::size_t found=0;
    for(int id=0;id<24;++id){auto* result=get_animation_resource(app,id);
        if(found<supported.size()&&id==supported[found])check(result==&r[found++]);else check(!result);}
    check(found==13&&!get_animation_resource(app,-1)&&!get_animation_resource(app,24));
    check(!get_animation_resource(app,std::numeric_limits<std::int32_t>::min()));
    check(!get_animation_resource(app,std::numeric_limits<std::int32_t>::max()));
    saga::assets::Seb one{0,13,{},{}},two{1,0,{},{}};
    {const auto indexed=index_resource_catalog({2,0,2,-1});
     PreparedAnimationResource catalog(indexed,{&one,&two,&two,&one});
     const auto& slots=*catalog.resource().clips;
     check(slots.size()==4&&slots[0]->max_frame==0&&slots[1]->max_frame==13&&slots[2]->max_frame==0&&!slots[3]);
     fails<std::out_of_range>([&]{PreparedAnimationResource bad(indexed,{});});}
    PreparedAnimationResource prepared({&one,nullptr,&two,&one});app.effects=&prepared.resource();
    check(animation_clip_max_frame(app,20,0)==13&&animation_clip_max_frame(app,20,2)==0);
    check(prepared.resource().clips->at(0)->frame==0&&!prepared.resource().clips->at(1));
    check(prepared.resource().clips->at(0)!=prepared.resource().clips->at(3)); // Same file, separate loaded instances.
    advance_shared_animation(*prepared.resource().clips->at(0),{false,true});
    check(prepared.resource().clips->at(0)->frame==1&&prepared.resource().clips->at(3)->frame==0);
    one.header_value=9;check(animation_clip_max_frame(app,20,0)==13); // Storage owns scalar clocks.
    fails<std::invalid_argument>([&]{animation_clip_max_frame(app,0,0);});
    fails<std::invalid_argument>([&]{animation_clip_max_frame(app,20,1);});
    fails<std::out_of_range>([&]{animation_clip_max_frame(app,20,-1);});
    fails<std::out_of_range>([&]{animation_clip_max_frame(app,20,4);});
}
