#include "core/gameplay/enemy_impact.hpp"
#include "core/gameplay/boss_notification.hpp"
#include <array>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Host final:EnemyImpactHost,RandomSource,ComponentObserver {
    ComponentSlots effect_entity{35},a{35},b{35},c{35};ContactPosition effect_pos;
    std::array<ComponentSlots*,3> emitters{{&a,&b,&c}};std::array<ProjectileLink,3> projectiles;
    ProjectileModification modification;std::vector<std::string> calls;
    int emitted=0,acquired=0,draws=0;bool dead=false,hit=true,null_effect=false,null_emitter=false;
    ComponentSlots* hit_target=nullptr;Component* returned=nullptr;
    std::function<void()> on_emitter,on_draw,on_check,on_bullet,on_modification;
    std::function<void(ComponentSlots&)> on_boss;
    Host(){effect_entity.add(0,&effect_pos);}
    bool destroyed(ComponentSlots&)override{calls.push_back("guard");return dead;}
    void read_bullet_data(ComponentSlots&)override{calls.push_back("bulletData");}
    void create_bullet_impact(ComponentSlots&,ComponentSlots&)override{calls.push_back("bulletEffect");if(on_bullet)on_bullet();}
    bool check_hit(ComponentSlots& source,ComponentSlots& target)override{calls.push_back("checkHit");
        const auto result=EnemyImpactHost::check_hit(source,target);if(on_check)on_check();return result;}
    void bullet_hit(ComponentSlots&,ComponentSlots* target)override{calls.push_back("bulletHit");hit_target=target;}
    ComponentSlots* create_effect(std::int32_t x,std::int32_t y,std::int32_t z,ProjectilePoint p)override{
        check(x==0&&y==24&&z==0&&p.x==10&&p.y==30&&p.z==30);calls.push_back("effect");
        effect_pos.x=p.x;effect_pos.y=p.y;effect_pos.z=p.z;return null_effect?nullptr:&effect_entity;}
    ComponentSlots* create_emitter(std::int32_t x,std::int32_t y,std::int32_t z,std::int32_t u,std::int32_t v,ProjectilePoint)override{
        check(x==0&&y==1&&z==1&&u==-1&&v==0);calls.push_back("emitter");
        auto* result=emitters.at(static_cast<std::size_t>(emitted++));if(on_emitter)on_emitter();return null_emitter?nullptr:result;}
    void boss_defeated(ComponentSlots& e)override{calls.push_back("boss");if(on_boss)on_boss(e);}
    ProjectileLink& acquire_projectile()override{calls.push_back("launch");return projectiles.at(static_cast<std::size_t>(acquired++));}
    ProjectileModification& acquire_modification()override{calls.push_back("modification");if(on_modification)on_modification();return modification;}
    void add_animation(ComponentSlots&,std::int32_t rate,std::int32_t backup)override{check(rate==1&&backup==-1);calls.push_back("animation");}
    void return_component(std::int32_t slot,Component* p)override{check(slot==15);calls.push_back("pool");returned=p;}
    void removed(ComponentSlots&,std::int32_t slot,Component*)override{if(slot==15)calls.push_back("remove");}
    std::int32_t next()override{calls.push_back("draw");static constexpr std::array<int,4> values{{0,0,100,99}};
        const auto result=values[static_cast<std::size_t>(draws++%4)];if(on_draw)on_draw();return result;}
};
struct Fixture {
    Host host;ComponentSlots e{35,&host},parent{35},target{35},other{35};
    ProjectileLink p;EnemyRecordLink enemy;HumanRecordLink human;EquipmentRecordLinks equipment_link;
    ContactPosition pos,target_pos;ProjectileCollision collision,target_collision;Component cell,bullet;
    CharacterProgressionTable characters;EnemyTable enemies;EquipmentCombatTable equipment{EquipmentCombatRecord{}};
    CombatRecordTables tables{characters,enemies,equipment,equipment,equipment};
    Fixture(){pos.x=10;pos.y=20;pos.z=30;enemy.flags=1;p.parent=&parent;
        e.add(0,&pos).add(5,&cell).add(27,&p).add(24,&enemy);
        collision.radius=target_collision.radius=100;e.add(32,&collision);target.add(0,&target_pos).add(32,&target_collision);
        equipment_link.ids=std::make_shared<EventIntegers>(EventIntegers{0,0,0,0});
        parent.add(14,&human).add(8,&equipment_link);}
    void run(){enemy_projectile_impact(&e,tables,host,host);}
};
int main(){
    {struct Notifications final:BossNotificationHost,VictoryFormStack {
        VictoryForm form;std::vector<int> keys;int pushes=0;
        void key_up(std::int32_t key)override{keys.push_back(key);}
        VictoryFormStack* form_stack()override{return this;}
        VictoryForm* make_victory(std::int32_t id,bool completed)override{check(id==42&&completed);return &form;}
        void push(VictoryForm* p)override{check(p==&form);++pushes;}
        MovementControl* movement_control(std::int32_t,std::int32_t)override{return nullptr;}
        void send_touch_release(MovementControl&,std::int32_t,std::int32_t)override{check(false);}
    } notifications;
        Fixture f;f.enemies.emplace_back(EnemyRecord{});f.enemies[0]->id=42;
        QuestConditionState quest{2,false,std::nullopt};BossStageState stage{false,0,&quest};
        f.host.on_boss=[&](ComponentSlots& e){notify_boss_defeat(stage,&e,f.tables,notifications);};f.run();
        check(f.host.draws==12&&f.host.emitted==3&&stage.boss_defeated&&stage.max_z_rate==100&&notifications.pushes==1);
        check(notifications.keys==std::vector<int>{0x80000,0x20000,0x80000});}
    {Fixture f;enemy_projectile_impact(nullptr,f.tables,f.host,f.host);check(f.host.calls.empty());}
    {Fixture f;f.host.dead=true;f.run();check(f.host.calls==std::vector<std::string>{"guard"});}
    {Fixture f;f.e.remove(5);f.run();check(f.host.calls==std::vector<std::string>{"guard"});}
    {Fixture f;ProjectileModification old;f.e.add(15,&old);f.run();check(f.host.draws==12&&f.host.emitted==3&&f.host.acquired==3);
        std::vector<std::string> expected{"guard","effect"};for(int i=0;i<3;++i)
            expected.insert(expected.end(),{"emitter","draw","draw","draw","draw","launch","animation"});
        expected.insert(expected.end(),{"remove","pool","modification","boss"});check(f.host.calls==expected&&f.host.returned==&old);
        for(const auto& p:f.host.projectiles)check(p.start.x==10&&p.start.y==30&&p.end.x==60&&p.end.y==0&&p.end.z==-120&&p.speed==5&&p.parent==nullptr);
        check(f.host.modification.type==3&&f.host.modification.max_frame==10&&f.host.modification.destroy&&!f.host.modification.loop);}
    {Fixture f;f.enemy.flags=0;f.run();check(f.host.calls.back()=="modification");}
    {Fixture f;f.host.on_emitter=[&]{f.host.effect_pos.x=100;};f.host.on_draw=[&]{if(f.host.draws==1){f.host.effect_pos.x=900;f.host.effect_pos.z=300;}};
        f.run();check(f.host.projectiles[0].start.x==100&&f.host.projectiles[0].start.z==30&&f.host.projectiles[0].end.x==150&&f.host.projectiles[0].end.z==150);}
    {Fixture f;f.host.null_effect=true;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.host.draws==0&&f.host.emitted==0);}
    {Fixture f;f.host.null_emitter=true;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.host.draws==4&&f.host.acquired==0&&!f.e.contains(15));}
    {Fixture f;f.host.on_modification=[] {throw std::runtime_error("allocation");};bool failed=false;
        try{f.run();}catch(const std::runtime_error&){failed=true;}check(failed&&f.host.acquired==3&&f.host.calls.back()=="modification");}
    {Fixture f;f.e.remove(24);f.e.add(29,&f.bullet);f.human.opponent=&f.target;
        f.host.on_check=[&]{f.human.opponent=&f.other;};f.run();
        check(f.host.calls==std::vector<std::string>{"guard","bulletData","bulletEffect","checkHit","bulletHit"}&&f.host.hit_target==&f.other&&f.host.draws==0);}
    {Fixture f;f.e.remove(24);f.e.add(29,&f.bullet);f.equipment[0].reset();f.run();
        check(f.host.calls==std::vector<std::string>{"guard","bulletData","bulletEffect"});} // Unused null weapon result is permitted.
    {Fixture f;f.e.remove(24);f.e.add(29,&f.bullet);f.human.opponent=&f.target;f.target_pos.z=1000;f.run();
        check(f.host.calls.back()=="checkHit"&&f.host.hit_target==nullptr);}
    {Fixture f;f.e.add(29,&f.bullet);f.p.parent=nullptr;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.host.calls==std::vector<std::string>{"guard","bulletData"});}
}
