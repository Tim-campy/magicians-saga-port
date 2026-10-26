#include "core/gameplay/subset_cursors.hpp"
#include "core/ecs/update_scheduler.hpp"
#include "core/gameplay/human_contact.hpp"
#include "core/ecs/entity_events.hpp"
#include "core/gameplay/combat_state.hpp"
#include "core/gameplay/projectile_launch.hpp"
#include <cstdlib>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Host final:ContactCollisionHost,HumanContactHost,ProjectileLaunchHost,RandomSource,CollisionPassHost {
    ProjectileLink flying;ProjectileModification modification;
    EntitySubset* contact_subset=nullptr;EntityEvents* events=nullptr;
    std::unique_ptr<CollisionCursor> open_collision_cursor()override{return open_collision_subset(*contact_subset);}
    void send_collision(std::int32_t id,ComponentSlots& a,ComponentSlots& b)override{check(id==29);events->send(id,&a,&b);}
    std::int32_t next()override{return 0;}
    ProjectileLink& acquire_projectile()override{return flying;}
    ProjectileModification& acquire_modification()override{return modification;}
    void add_animation(ComponentSlots&,std::int32_t,std::int32_t)override{check(false);}
    std::vector<std::string> calls;
    bool lethal=false,blocked=false;int progress=0;
    ComponentSlots* hero_entity=nullptr;ComponentSlots* owner=nullptr;
    std::int32_t amount=0,attribute=0,animation=0,frame=0;
    std::function<void()> after_defeat;
    bool damage(ComponentSlots* p,ComponentSlots&,std::int32_t a,std::int32_t b)override{
        calls.push_back("damage");owner=p;amount=a;attribute=b;return lethal;
    }
    void defeat(ComponentSlots&)override{calls.push_back("defeat");if(after_defeat)after_defeat();}
    void impact(ComponentSlots&,ComponentSlots*)override{calls.push_back("impact");}
    void return_component(std::int32_t slot,Component*)override{calls.push_back("pool"+std::to_string(slot));}
    bool destroyed(ComponentSlots&)override{return blocked;}
    void contact_effect(std::int32_t a,std::int32_t id,std::int32_t b,float x,float y,float z)override{
        check(a==0&&id==2&&b==0&&x==1&&y==12&&z==11);calls.push_back("humanEffect");
    }
    bool collision_blocked(ComponentSlots&)override{calls.push_back("blocked");return blocked;}
    void cut_in_animation(ComponentSlots&,std::int32_t a,std::int32_t f)override{calls.push_back("cut");animation=a;frame=f;}
    void change_animation(ComponentSlots&,std::int32_t a,std::int32_t f)override{check(a==16&&f==1);calls.push_back("animate");}
    void effect(std::int32_t a,std::int32_t id,std::int32_t b,float x,float y,float z)override{
        check(a==0&&b==0&&x==1&&y==12&&(id==2?z==112:z==20));calls.push_back("effect"+std::to_string(id));
    }
    void fly_away(ComponentSlots& e,std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d,bool enemy)override{
        check(a==-100&&b==100&&c==(enemy?-100:50)&&d==(enemy?-50:100));calls.push_back(enemy?"flyEnemy":"flyHuman");
        saga::core::fly_away(e,a,b,c,d,enemy,*this,*this);
    }
    bool playing_cut_in(ComponentSlots&)override{return false;}
    void change_animation(ComponentSlots&,std::int32_t clip,bool reset)override{check(clip==36&&reset);}
    std::int32_t attack_interval(ComponentSlots&)override{return 10;}
    void human_state(ComponentSlots& e,std::int32_t state)override{check(state==4);calls.push_back("human4");change_human_state(e,state,*this);}
    void enemy_state(ComponentSlots& e,std::int32_t state)override{calls.push_back("enemy"+std::to_string(state));change_enemy_state(e,state);}
    std::int32_t progress_rate(float depth)override{check(depth==100);calls.push_back("progress");return progress;}
    ComponentSlots& hero()override{calls.push_back("hero");return *hero_entity;}
    void contact_sound()override{calls.push_back("sound");}
};
struct Fixture {
    Host host;ComponentSlots enemy{35},human{35},hero{35},source{35};
    EnemyRecordLink enemy_link;HumanRecordLink human_link;ContactPosition ep,hp,rp;
    ContactDirection direction;ProjectileSpeed speed;
    CharacterProgressionTable characters{CharacterProgressionRecord{}};
    EnemyTable enemies{EnemyRecord{}};EquipmentCombatTable equipment;
    CombatRecordTables tables{characters,enemies,equipment,equipment,equipment};
    ParameterMaximumLevels maxima=initial_parameter_maximum_levels();
    Fixture(){
        enemies[0]->enemy_seb=7;enemies[0]->attribute=2;
        enemies[0]->parameters=std::make_shared<std::vector<EventInstruction>>(13,std::make_shared<EventIntegers>(EventIntegers{123,456}));
        enemy_link.level=1;ep.x=hp.x=1;ep.y=hp.y=2;ep.z=100;hp.z=20;rp.z=10;
        speed.x=4;speed.y=5;speed.z=6;
        enemy.add(combat_enemy_slot,&enemy_link).add(contact_position_slot,&ep).add(projectile_speed_slot,&speed).add(contact_direction_slot,&direction);
        human.add(combat_human_slot,&human_link).add(contact_position_slot,&hp);
        hero.add(contact_position_slot,&rp);host.hero_entity=&hero;
    }
    void run(){on_contact_collision(&enemy,&human,tables,maxima,host);}
};
int main(){
    {Fixture f;ProjectileSpeed human_speed;ContactDirection human_direction;
        f.human.add(1,&human_speed).add(12,&human_direction);
        EntityEvents events;
        // Both real receiver bodies in the factory-established event-29 order.
        events.subscribe(29,EntityEvents::Pair([&](ComponentSlots* a,ComponentSlots* b){on_human_contact(a,b,f.tables,f.host);}));
        events.subscribe(29,EntityEvents::Pair([&](ComponentSlots* a,ComponentSlots* b){
            check(f.human_link.state==3&&f.hp.z==23);
            on_contact_collision(a,b,f.tables,f.maxima,f.host);
        }));
        // Real live group -> collision pair selection -> event delivery -> both real receivers.
        std::vector<std::int32_t> required{32,0};SubsetFilter filter{&required};
        EntitySubset contacts(filter,[&](const ComponentSlots& e){return &e==&f.enemy?1:2;});
        ProjectileCollision enemy_collision,human_collision;enemy_collision.radius=100;human_collision.radius=100;
        f.enemy.add(32,&enemy_collision);f.human.add(32,&human_collision);
        contacts.add(&f.enemy);contacts.add(&f.human);f.host.contact_subset=&contacts;f.host.events=&events;
        CollisionPass pass;bool enabled=false;
        GameplayUpdate collision_system(12,enabled,[&](std::int32_t){pass.update(f.host);});
        std::vector<UpdateSystem*> systems{&collision_system};UpdateSchedule schedule{&systems,nullptr};
        update_systems(schedule,1);check(f.host.calls.empty()&&f.hp.z==20);
        enabled=true;update_systems(schedule,1);
        check(f.host.calls==std::vector<std::string>{"cut","humanEffect","cut","effect2","damage","progress","enemy3","sound"});
        check(f.hp.z==23&&human_speed.z==3&&f.ep.z==90&&f.speed.z==-10);
    }

    {Fixture f;f.run();check(f.host.calls==std::vector<std::string>{"cut","effect2","damage","progress","enemy3","sound"});
        check(f.host.owner==&f.enemy&&f.host.amount==123&&f.host.attribute==2);
        check(f.host.animation==52&&f.host.frame==30&&f.direction.locked&&f.ep.z==90&&f.speed.z==-10&&f.speed.x==4&&f.speed.y==5);}
    {Fixture f;f.host.lethal=true;f.human_link.state=2;f.human_link.attack_frame=11;f.human_link.frame=99;f.enemy_link.frame=7;
        on_contact_collision(&f.human,&f.enemy,f.tables,f.maxima,f.host);
        check(f.host.calls==std::vector<std::string>{"cut","effect2","damage","flyHuman","animate","effect24","human4","progress","enemy3","sound"});
        check(f.human_link.state==4&&f.human_link.frame==0&&f.human_link.attack_frame==0&&f.enemy_link.state==3&&f.enemy_link.frame==0);}
    for(int state:{4,5}){Fixture f;f.human_link.state=state;f.run();check(f.host.calls==std::vector<std::string>{"cut","effect2"}&&f.ep.z==100);}
    {Fixture f;f.enemies[0]->flags=1;f.run();check(f.host.calls==std::vector<std::string>{"cut","effect2","flyEnemy","enemy4","sound"}&&f.speed.z==6);}
    {Fixture f;f.human_link.invincible_time=1;f.run();check(f.host.calls==std::vector<std::string>{"cut","effect2","progress","enemy3","sound"}&&f.ep.z==90);}
    for(int variant=0;variant<6;++variant){Fixture f;f.characters[0]->flags=4;
        if(variant==0)f.host.progress=80;
        if(variant==1)f.enemy_link.flags=1;
        if(variant==2)f.host.progress=79;
        if(variant==3)f.rp.z=20;
        if(variant==4)f.rp.z=30;
        if(variant==5)f.rp.z=std::numeric_limits<float>::quiet_NaN();
        f.run();const float distance=variant<2?3:variant==2?16:10;check(f.ep.z==100-distance&&f.speed.z==-distance);
        check(f.host.calls.size()==(variant<2?6:7));}
    {Fixture f;f.human.remove(combat_human_slot);f.run();check(f.host.calls==std::vector<std::string>{"cut","effect2"});}
    {Fixture f;ProjectileDamage d;d.value=77;d.attribute=8;d.parent=&f.hero;f.source.add(projectile_damage_slot,&d);
        f.host.lethal=true;f.host.after_defeat=[&]{f.enemies[0]->enemy_seb=8;};
        on_contact_collision(&f.source,&f.enemy,f.tables,f.maxima,f.host);
        check(f.host.calls==std::vector<std::string>{"blocked","damage","defeat","cut","effect2"});
        check(f.host.owner==&f.hero&&f.host.amount==77&&f.host.attribute==8&&f.host.animation==56);}
    {Fixture f;ProjectileDamage d;f.human.add(projectile_damage_slot,&d);f.host.blocked=true;f.run();check(f.host.calls==std::vector<std::string>{"blocked"});}
    {Fixture f;ProjectileDamage d;f.source.add(projectile_damage_slot,&d);on_contact_collision(&f.source,&f.human,f.tables,f.maxima,f.host);check(f.host.calls.empty());}
    for(bool reverse:{false,true}){Fixture f;ProjectileLink p;ProjectileCollision c;ProjectileDamage d;ProjectileSpeed s;Component bullet;
        p.parent=&f.hero;d.parent=&f.human;d.value=9;
        f.source.add(contact_bullet_slot,&bullet).add(projectile_slot,&p).add(projectile_collision_slot,&c).add(projectile_damage_slot,&d).add(projectile_speed_slot,&s);
        on_contact_collision(reverse?&f.enemy:&f.source,reverse?&f.source:&f.enemy,f.tables,f.maxima,f.host);
        check(f.host.calls==std::vector<std::string>{"damage","impact","pool27","pool32"}&&f.host.owner==&f.hero);}
    {Fixture f;f.enemy.remove(projectile_speed_slot);bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.ep.z==90&&f.host.calls.back()=="progress");}
    {Fixture f;f.enemy.remove(combat_enemy_slot);bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}check(failed&&f.host.calls.empty());}
    {Fixture f;bool failed=false;try{on_contact_collision(&f.enemy,nullptr,f.tables,f.maxima,f.host);}catch(const std::invalid_argument&){failed=true;}check(failed&&f.host.calls.empty());}
}
