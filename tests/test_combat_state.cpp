#include "core/gameplay/combat_state.hpp"
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Host final:HumanStateHost {
    std::vector<std::string> calls;bool cut=false;std::int32_t interval=10;
    std::function<void()> animate,query;
    bool playing_cut_in(ComponentSlots&)override{calls.push_back("cut");return cut;}
    void change_animation(ComponentSlots&,std::int32_t clip,bool reset)override{
        check(clip==36&&reset);calls.push_back("animate");if(animate)animate();
    }
    std::int32_t attack_interval(ComponentSlots&)override{calls.push_back("interval");if(query)query();return interval;}
};
struct Fixture {
    ComponentSlots entity{35};HumanRecordLink human;EnemyRecordLink enemy;
    ContactDirection direction;ProjectileSpeed speed;Host host;
    Fixture(){human.frame=99;human.attack_frame=11;direction.direction=7;direction.locked=true;
        speed.x=3;speed.y=4;speed.z=5;
        entity.add(combat_human_slot,&human).add(combat_enemy_slot,&enemy)
            .add(contact_direction_slot,&direction).add(projectile_speed_slot,&speed);}
    void run(int from,int to){human.state=from;change_human_state(entity,to,host);}
};
int main(){
    {Fixture f;f.enemy.frame=99;change_enemy_state(f.entity,4);check(f.enemy.state==4&&f.enemy.frame==0);
        f.enemy.frame=8;change_enemy_state(f.entity,4);check(f.enemy.frame==0);}
    {Fixture f;f.host.animate=[&]{check(f.human.state==1&&f.human.frame==99&&f.direction.direction==0);};
        f.run(1,2);check(f.human.state==2&&f.human.frame==0&&f.human.attack_frame==11&&f.direction.locked);
        check(f.host.calls==std::vector<std::string>{"animate"});}
    for(bool cut:{false,true}){Fixture f;f.host.cut=cut;
        f.host.animate=[&]{check(f.human.state==0&&f.human.frame==0&&f.speed.z==0);};f.run(1,0);
        check(f.speed.x==3&&f.speed.y==4&&f.speed.z==0&&f.direction.direction==0);
        check(f.host.calls==(cut?std::vector<std::string>{"cut"}:std::vector<std::string>{"cut","animate"}));}
    for(int frame:{9,10,11}){Fixture f;f.human.attack_frame=frame;f.run(2,4);
        check(f.human.attack_frame==(frame>10?0:frame)&&f.host.calls==std::vector<std::string>{"interval"});}
    {Fixture f;f.run(2,2);check(f.human.frame==0&&f.human.attack_frame==11&&f.host.calls.empty());}
    {Fixture f;f.run(2,0);check(f.speed.z==5&&f.human.attack_frame==0);
        check(f.host.calls==std::vector<std::string>{"cut","animate","interval"});}
    {Fixture f;HumanRecordLink replacement;replacement.attack_frame=88;
        f.host.animate=[&]{f.entity.change(combat_human_slot,&replacement);};f.run(1,2);
        check(f.human.state==1&&replacement.state==2&&replacement.frame==0);}
    {Fixture f;HumanRecordLink replacement;replacement.attack_frame=88;
        f.host.query=[&]{f.human.attack_frame=1;f.entity.change(combat_human_slot,&replacement);};f.run(2,4);
        check(f.human.attack_frame==1&&replacement.attack_frame==0); // Captured comparison, live destination.
    }
    {Fixture f;f.host.animate=[] {throw std::runtime_error("animation");};bool failed=false;
        try{f.run(1,2);}catch(const std::runtime_error&){failed=true;}
        check(failed&&f.direction.direction==0&&f.human.state==1&&f.human.frame==99);}
    {Fixture f;f.entity.remove(contact_direction_slot);bool failed=false;
        try{f.run(1,0);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.human.state==0&&f.human.frame==0&&f.speed.z==0);}
}
