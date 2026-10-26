#include "core/gameplay/projectile_launch.hpp"
#include <cstdlib>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Host final:ProjectileLaunchHost,ComponentObserver {
    ProjectileLink projectile;ProjectileModification modification;
    std::vector<std::string> calls;Component* returned=nullptr;
    std::function<void()> on_added;
    ProjectileLink& acquire_projectile()override{calls.push_back("projectile");return projectile;}
    ProjectileModification& acquire_modification()override{calls.push_back("modify");return modification;}
    void return_component(std::int32_t slot,Component* p)override{check(slot==15);calls.push_back("pool");returned=p;}
    void add_animation(ComponentSlots&,std::int32_t rate,std::int32_t backup)override{check(rate==1&&backup==-1);calls.push_back("animation");}
    void added(ComponentSlots&,std::int32_t slot,Component*)override{calls.push_back("add"+std::to_string(slot));if(on_added)on_added();}
    void removed(ComponentSlots&,std::int32_t slot,Component*)override{calls.push_back("remove"+std::to_string(slot));}
};
struct Random final:RandomSource {
    int count=0;std::function<void()> callback;
    std::int32_t next()override{++count;if(callback)callback();return 0;}
};
int main(){
    {Host h;ComponentSlots e{35,&h},parent{35};
        const auto duration=launch_projectile(e,{1,2,3},{1,32,43},4,&parent,true,true,200,h);
        check(duration==12&&h.projectile.max_height==32&&h.projectile.frame==0&&h.projectile.parent==&parent);
        check(h.projectile.start.y==2&&h.projectile.end.z==43&&h.projectile.speed==4);
        check(h.calls==std::vector<std::string>{"projectile","add27","modify","add15","animation"});
        check(h.modification.type==7&&h.modification.loop&&!h.modification.destroy&&h.modification.max_frame==0);
        check(h.modification.scale_x==1&&h.modification.scale_y==1&&h.modification.alpha==255);}
    {Host h;ComponentSlots e{35};launch_projectile(e,{},{0,0,2},4,nullptr,false,false,200,h);
        check(h.projectile.max_frame==0&&h.projectile.max_height==0&&h.calls==std::vector<std::string>{"projectile"});}
    for(int speed:{0,-4}){Host h;ComponentSlots e{35};auto duration=launch_projectile(e,{},{0,0,50},speed,nullptr,false,false,200,h);
        check(duration==(speed==0?std::numeric_limits<std::int32_t>::min():-12));}
    {Host h;ComponentSlots e{35};check(launch_projectile(e,{},{},0,nullptr,false,false,0,h)==0);}
    {Host h;ComponentSlots e{35};launch_projectile(e,{},{0,0,9},1,nullptr,false,false,std::numeric_limits<std::int32_t>::max(),h);
        check(h.projectile.max_height==21474836);}
    {Host h;ComponentSlots e{35,&h};ContactPosition p;p.x=10;p.y=20;p.z=30;e.add(0,&p);h.calls.clear();Random r;
        r.callback=[&]{if(r.count==1){p.x=100;p.z=300;}};
        fly_away(e,-5,-5,-7,-7,false,r,h);
        check(r.count==2&&h.projectile.start.x==10&&h.projectile.start.z==30);
        check(h.projectile.end.x==5&&h.projectile.end.y==0&&h.projectile.end.z==293);
        check(h.calls==std::vector<std::string>{"projectile","add27"});}
    {Host h;ComponentSlots e{35,&h};ContactPosition p;ProjectileModification old;
        e.add(0,&p).add(15,&old);h.calls.clear();Random r;
        fly_away(e,0,0,0,0,true,r,h);
        check(r.count==2&&h.calls==std::vector<std::string>{"projectile","add27","remove15","pool","modify","add15"});
        check(h.returned==&old&&h.modification.type==8&&h.modification.max_frame==99&&!h.modification.loop);}
    {Host h;ComponentSlots e{35,&h};ContactPosition p;ProjectileModification added;e.add(0,&p);h.calls.clear();Random r;
        h.on_added=[&]{if(e.contains(27)&&!e.contains(15)){e.add(15,&added);}};
        fly_away(e,0,0,0,0,true,r,h);check(h.returned==&added);}
    {Host h;ComponentSlots e{35,&h};h.on_added=[] {throw std::runtime_error("observer");};bool failed=false;
        try{launch_projectile(e,{},{0,0,50},4,nullptr,true,true,200,h);}catch(const std::runtime_error&){failed=true;}
        check(failed&&e.contains(27)&&!e.contains(15)&&h.projectile.max_frame==12);}
    {Host h;ComponentSlots e{35};Random r;bool failed=false;
        try{fly_away(e,0,0,0,0,true,r,h);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&r.count==0&&h.calls.empty());}
}
