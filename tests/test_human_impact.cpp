#include "core/gameplay/human_impact.hpp"
#include "core/gameplay/projectile_motion.hpp"
#include "core/gameplay/movement.hpp"
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:HumanImpactHost,HumanDeathHost,ProjectileLaunchHost,ProjectileMotionHost,RandomSource {
    bool dead=false;std::vector<std::string> calls;std::function<void()> animate,effect;
    ProjectileLink projectile;ProjectileModification modification;
    const CombatRecordTables* tables=nullptr;
    bool destroyed(ComponentSlots&)override{calls.push_back("guard");return dead;}
    bool playing_cut_in(ComponentSlots&)override{check(false);return false;}
    std::int32_t attack_interval(ComponentSlots&)override{calls.push_back("interval");return 10;}
    void change_animation(ComponentSlots&,std::int32_t clip,bool reset)override{
        check(clip==12&&reset);calls.push_back("animation");if(animate)animate();}
    void impact_effect(std::int32_t a,std::int32_t b,std::int32_t c,float x,float y,float z)override{
        check(a==100&&b==100&&c==1&&x==1&&y==12&&z==3);calls.push_back("effect");if(effect)effect();}
    void send_human_dead(ComponentSlots& e)override{calls.push_back("dead30");stage_human_dead(e,*tables,*this);}
    void game_over(std::int32_t reason)override{check(reason==1);calls.push_back("gameover1");}
    ProjectileLink& acquire_projectile()override{return projectile;}
    ProjectileModification& acquire_modification()override{return modification;}
    void add_animation(ComponentSlots&,std::int32_t,std::int32_t)override{check(false);}
    void return_component(std::int32_t slot,Component*)override{calls.push_back("pool"+std::to_string(slot));}
    void send_impact(ComponentSlots& e)override{check(e.contains(27));calls.push_back("impact23");human_projectile_impact(&e,*this);}
    std::int32_t next()override{return 0;}
};
struct Fixture {
    Host host;ComponentSlots e{35};HumanRecordLink human;ContactPosition p;ProjectileSpeed speed;Component cell;
    CharacterProgressionTable characters{CharacterProgressionRecord{}};EnemyTable enemies;EquipmentCombatTable equipment;
    CombatRecordTables tables{characters,enemies,equipment,equipment,equipment};
    Fixture(){host.tables=&tables;human.state=4;human.frame=9;p.x=1;p.y=2;p.z=3;characters[0]->flags=2;
        e.add(14,&human).add(0,&p).add(1,&speed).add(5,&cell);}
};
int main(){
    {Fixture f;human_projectile_impact(nullptr,f.host);check(f.host.calls.empty());}
    {Fixture f;f.host.dead=true;human_projectile_impact(&f.e,f.host);check(f.host.calls==std::vector<std::string>{"guard"});}
    for(int slot:{5,14}){Fixture f;f.e.remove(slot);human_projectile_impact(&f.e,f.host);check(f.host.calls==std::vector<std::string>{"guard"});}
    for(int state:{0,1,2,3,5,6}){Fixture f;f.human.state=state;human_projectile_impact(&f.e,f.host);check(f.host.calls==std::vector<std::string>{"guard"});}
    {Fixture f;f.host.animate=[&]{check(f.human.state==4);};f.host.effect=[&]{check(f.human.state==4&&f.human.frame==9);};
        human_projectile_impact(&f.e,f.host);check(f.human.state==5&&f.human.frame==0);
        check(f.host.calls==std::vector<std::string>{"guard","animation","effect","dead30","gameover1"});
        f.host.calls.clear();human_projectile_impact(&f.e,f.host);check(f.host.calls==std::vector<std::string>{"guard"});}
    {Fixture f;f.characters[0]->flags=4;human_projectile_impact(&f.e,f.host);check(f.host.calls.back()=="dead30"&&f.human.state==5);}
    {Fixture f;f.host.effect=[&]{f.human.state=2;f.human.attack_frame=11;};human_projectile_impact(&f.e,f.host);
        check(f.human.attack_frame==0&&f.host.calls==std::vector<std::string>{"guard","animation","effect","interval","dead30","gameover1"});}
    {Fixture f;f.host.effect=[] {throw std::runtime_error("effect failure");};bool failed=false;
        try{human_projectile_impact(&f.e,f.host);}catch(const std::runtime_error&){failed=true;}check(failed&&f.human.state==4&&f.human.frame==9);}
    {Fixture f;f.characters[0].reset();bool failed=false;
        try{human_projectile_impact(&f.e,f.host);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.human.state==5&&f.human.frame==0&&f.host.calls.back()=="dead30");}
    {Fixture f;f.e.remove(14);bool failed=false;try{stage_human_dead(f.e,f.tables,f.host);}catch(const std::invalid_argument&){failed=true;}check(failed);}
    {Fixture f;launch_projectile(f.e,{1,2,-37},{1,2,3},4,nullptr,false,false,200,f.host);
        bool complete=false;for(int i=0;i<20&&!complete;++i){complete=advance_projectile(f.e,f.host);
            if(complete)finish_projectile(f.e,f.host);
            move_entity(f.e);}
        check(complete&&!f.e.contains(27)&&f.human.state==5&&f.p.z==3);
        check(f.host.calls==std::vector<std::string>{"impact23","guard","animation","effect","dead30","gameover1","pool27"});}
}
