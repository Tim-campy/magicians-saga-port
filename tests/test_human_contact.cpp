#include "core/gameplay/human_contact.hpp"
#include <cstdlib>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}check(caught);}
struct Host final:HumanContactHost {
    bool blocked=false;ComponentSlots* leader=nullptr;std::vector<std::string> calls;
    std::function<void()> on_cut,on_effect,on_hero;
    float effect_x=0,effect_y=0,effect_z=0;
    bool destroyed(ComponentSlots&)override{return blocked;}
    ComponentSlots& hero()override{calls.push_back("hero");if(on_hero)on_hero();if(!leader)throw std::invalid_argument("hero");return *leader;}
    void cut_in_animation(ComponentSlots&,std::int32_t a,std::int32_t b)override{check(a==4&&b==36);calls.push_back("cut");if(on_cut)on_cut();}
    void contact_effect(std::int32_t a,std::int32_t b,std::int32_t c,float x,float y,float z)override{
        check(a==0&&b==2&&c==0);calls.push_back("effect");effect_x=x;effect_y=y;effect_z=z;if(on_effect)on_effect();}
    bool playing_cut_in(ComponentSlots&)override{check(false);return false;}
    void change_animation(ComponentSlots&,std::int32_t,bool)override{check(false);}
    std::int32_t attack_interval(ComponentSlots&)override{calls.push_back("interval");return 7;}
};
struct Fixture {
    ComponentSlots human{35},enemy{35},hero{35};HumanRecordLink h;EnemyRecordLink e;
    ContactPosition p,hp;ProjectileSpeed speed;ContactDirection direction;Host host;
    CharacterProgressionTable chars{CharacterProgressionRecord{}};EnemyTable enemies{EnemyRecord{}};
    EquipmentCombatTable equipment;CombatRecordTables tables{chars,enemies,equipment,equipment,equipment};
    Fixture(){human.add(14,&h).add(0,&p).add(1,&speed).add(12,&direction);enemy.add(24,&e);hero.add(0,&hp);
        host.leader=&hero;p.x=1;p.y=2;p.z=20;hp.z=10;speed.x=8;speed.y=9;h.frame=55;}
    void run(){on_human_contact(&human,&enemy,tables,host);}
};
int main(){
    for(int mode=0;mode<6;++mode){Fixture f;float delta=3;
        if(mode==1){f.e.flags=1;delta=10;f.chars.clear();} // No character lookup in this route.
        if(mode>=2)f.chars[0]->flags=4;
        if(mode==2)delta=0.5F;
        if(mode==3)f.hp.z=20;
        if(mode==4)f.hp.z=std::numeric_limits<float>::quiet_NaN();
        if(mode==5){f.host.on_hero=[&]{f.p.z=100;};delta=0.5F;}
        on_human_contact(&f.enemy,&f.human,f.tables,f.host);
        check(f.p.z==(mode==5?100:20)+delta&&f.speed.z==delta&&f.speed.x==8&&f.speed.y==9);
        check(f.h.state==3&&f.h.frame==0&&f.direction.locked);
        check(f.host.effect_x==1&&f.host.effect_y==12&&f.host.effect_z==f.p.z-12);}
    for(int guard=0;guard<6;++guard){Fixture f;Component damage;
        if(guard==0)f.h.state=4;if(guard==1)f.h.state=5;if(guard==2)f.host.blocked=true;
        if(guard==3)f.enemies[0]->flags=1;if(guard==4)f.enemy.add(33,&damage);if(guard==5)f.human.add(33,&damage);
        f.run();check(f.host.calls.empty()&&f.p.z==20);}
    {Fixture f;f.h.state=2;f.h.attack_frame=8;f.run();
        check(f.host.calls==std::vector<std::string>{"cut","effect","interval"}&&f.h.attack_frame==0);}
    {Fixture f;ContactPosition replacement;replacement.x=7;replacement.y=8;replacement.z=9;
        f.host.on_cut=[&]{f.human.change(0,&replacement);};f.host.on_effect=[&]{f.h.state=2;f.h.attack_frame=8;};f.run();
        check(f.p.z==23&&f.host.effect_x==7&&f.host.effect_y==18&&f.host.effect_z==-3&&f.h.attack_frame==0);}
    {Fixture f;f.human.remove(12);fails([&]{f.run();});
        check(f.p.z==23&&f.speed.z==3&&f.host.calls==std::vector<std::string>{"cut"}&&f.h.state==0);}
    {Fixture f;f.host.on_effect=[]{throw std::invalid_argument("effect");};fails([&]{f.run();});
        check(f.direction.locked&&f.p.z==23&&f.h.state==0&&f.h.frame==55);}
    {Fixture f;Component damage;f.human.add(33,&damage);on_human_contact(&f.human,nullptr,f.tables,f.host);
        f.human.remove(33);fails([&]{on_human_contact(&f.human,nullptr,f.tables,f.host);});
        fails([&]{on_human_contact(nullptr,&f.enemy,f.tables,f.host);});}
    {Fixture f;f.enemy.remove(24);f.run();check(f.host.calls.empty());}
    {Fixture f;f.human.remove(14);f.run();check(f.host.calls.empty());}
    {Fixture f;f.chars[0]->flags=4;f.host.leader=nullptr;fails([&]{f.run();});check(f.p.z==20);}
}
