#include "core/gameplay/projectile_motion.hpp"
#include "core/gameplay/movement.hpp"
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void checked(bool value,int line){if(!value){std::cerr<<"check failed at "<<line<<'\n';std::abort();}}
#define check(...) checked((__VA_ARGS__),__LINE__)
struct Host final:ProjectileMotionHost,ProjectileLaunchHost,ComponentObserver {
    ProjectileLink p;ProjectileModification m;std::vector<std::string> calls;
    std::function<void(ComponentSlots&)> impact;
    ProjectileLink& acquire_projectile()override{return p;}
    ProjectileModification& acquire_modification()override{return m;}
    void add_animation(ComponentSlots&,std::int32_t,std::int32_t)override{check(false);}
    void return_component(std::int32_t slot,Component*)override{calls.push_back("pool"+std::to_string(slot));}
    void removed(ComponentSlots&,std::int32_t slot,Component*)override{calls.push_back("remove"+std::to_string(slot));}
    void send_impact(ComponentSlots& e)override{calls.push_back("impact");check(e.contains(projectile_slot));if(impact)impact(e);}
};
struct Fixture {
    Host host;ComponentSlots e{35,&host};ContactPosition pos;ProjectileSpeed speed;
    Fixture(int max_frame=10){
        e.add(contact_position_slot,&pos).add(projectile_speed_slot,&speed);
        launch_projectile(e,{},{0,0,40},4,nullptr,false,true,200,host);host.p.max_frame=max_frame;
    }
};
int main(){
    {Fixture f;for(int frame=0;frame<=10;++frame){
            const bool complete=advance_projectile(f.e,f.host);
            check(complete==(frame==10)&&f.host.p.frame==frame+1);
            if(frame==0)check(f.pos.y==0&&std::abs(f.speed.y-9.091634750366211F)<0.000001F); // Independent binary32 model.
            if(frame==5)check(f.pos.y==26&&f.speed.y==0); // Peak uses ascent for equality.
            if(!complete){check(f.speed.x==0&&f.speed.z==4);move_entity(f.e);}
        }
        check(f.pos.x==0&&f.pos.y==0&&f.pos.z==40&&f.speed.x==0&&f.speed.y==0&&f.speed.z==0);
        check(f.e.contains(27)&&!f.e.contains(15)&&f.host.calls==std::vector<std::string>{"remove15","pool15"});
        finish_projectile(f.e,f.host);check(!f.e.contains(27));
        check(f.host.calls==std::vector<std::string>{"remove15","pool15","impact","remove27","pool27"});}
    {Fixture f(9);int ticks=0;while(!advance_projectile(f.e,f.host)){++ticks;check(ticks<20);}check(ticks==8);}
    {Fixture f(0);f.host.p.max_height=0;check(!advance_projectile(f.e,f.host));check(advance_projectile(f.e,f.host));}
    {Fixture f;f.host.p.start=f.host.p.end={};f.host.p.max_height=0;
        check(!advance_projectile(f.e,f.host)&&f.speed.x==0&&f.speed.z==0);check(advance_projectile(f.e,f.host));}
    {Fixture f;f.host.p.max_height=0;check(!advance_projectile(f.e,f.host));check(f.host.m.angle==243);}
    {Fixture f;f.host.p.start.y=30;f.host.p.end.y=40;f.host.p.frame=1;f.host.p.max_height=0;
        check(advance_projectile(f.e,f.host));check(f.pos.y==40); // Height test can end before duration.
    }
    {Fixture f;f.host.p.frame=2;f.host.p.end.y=std::numeric_limits<float>::quiet_NaN();
        check(!advance_projectile(f.e,f.host)&&f.e.contains(27));}
    {Fixture f;f.host.p.frame=std::numeric_limits<std::int32_t>::max();f.host.p.max_height=0;
        check(advance_projectile(f.e,f.host)&&f.host.p.frame==std::numeric_limits<std::int32_t>::min());}
    {Fixture f;f.host.impact=[](ComponentSlots& e){e.remove(27);};
        finish_projectile(f.e,f.host);
        check(!f.e.contains(27)&&f.host.calls==std::vector<std::string>{"impact","remove27"});}
    {Fixture f;f.e.remove(projectile_speed_slot);bool failed=false;
        f.host.p.frame=2;try{advance_projectile(f.e,f.host);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.pos.y>0&&f.host.p.frame==2);}
    {Fixture first(0),second(0);first.host.p.max_height=second.host.p.max_height=0;
        first.host.p.frame=second.host.p.frame=1;
        check(advance_projectile(first.e,first.host));check(advance_projectile(second.e,second.host));
        first.host.impact=[&](ComponentSlots&){check(second.pos.z==40&&second.e.contains(27));};
        finish_projectile(first.e,first.host);finish_projectile(second.e,second.host);}
    {Fixture f;for(int i=0;i<11;++i){
            if(advance_projectile(f.e,f.host))finish_projectile(f.e,f.host);
            const float old_y=f.pos.y;move_entity(f.e);
            check(f.pos.y==old_y+f.speed.y); // Move integrates y too, after the projectile pass.
        }
        check(!f.e.contains(27)&&f.pos.z==40&&f.pos.y==0);}
}
