#include "core/gameplay/boss_notification.hpp"
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:BossNotificationHost,VictoryFormStack {
    std::vector<std::string> calls;VictoryForm form;MovementControl a,b,other;
    MovementControl *first=&a,*second=&b;VictoryFormStack* stack=this;VictoryForm* result=&form;
    std::function<void()> on_key,on_make,on_send;bool complete=false;int enemy_id=-1;
    void key_up(std::int32_t mask)override{calls.push_back(mask==0x80000?"key8":"key2");if(on_key)on_key();}
    VictoryFormStack* form_stack()override{calls.push_back("stack");return stack;}
    VictoryForm* make_victory(std::int32_t id,bool completed)override{calls.push_back("make");enemy_id=id;complete=completed;if(on_make)on_make();return result;}
    void push(VictoryForm* p)override{check(p==result);calls.push_back("push");}
    MovementControl* movement_control(std::int32_t group,std::int32_t id)override{check(group==35);calls.push_back(id==0x10011?"get1":"get2");return id==0x10011?first:second;}
    void send_touch_release(MovementControl& c,std::int32_t type,std::int32_t event)override{check(type==1&&event==9);calls.push_back(&c==&a?"send1":&c==&b?"send2":"other");if(on_send)on_send();}
};
struct Fixture {
    Host host;QuestConditionState quest{0,false,std::nullopt};BossStageState stage{false,7,&quest};
    ComponentSlots enemy{35};EnemyRecordLink link;
    CharacterProgressionTable characters;EnemyTable enemies{EnemyRecord{}};EquipmentCombatTable equipment;
    CombatRecordTables tables{characters,enemies,equipment,equipment,equipment};
    Fixture(){enemies[0]->id=17;enemy.add(24,&link);}
    void run(){notify_boss_defeat(stage,&enemy,tables,host);}
};
int main(){
    {Fixture f;f.run();check(f.stage.boss_defeated&&f.stage.max_z_rate==100&&!f.host.complete&&f.host.enemy_id==17);
        check(f.host.calls==std::vector<std::string>{"key8","stack","make","push","key2","key8","get1","get2","send1","send2"});}
    for(int flags=0;flags<4;++flags)for(bool completed:{false,true}){Fixture f;f.quest.flags=static_cast<std::uint8_t>(flags);f.quest.completed=completed;f.run();check(f.host.complete==((flags&2)!=0||completed));}
    {Fixture f;f.stage.quest=nullptr;notify_boss_defeat(f.stage,nullptr,f.tables,f.host);check(f.host.calls==std::vector<std::string>{"key8","key2","key8","get1","get2","send1","send2"});}
    {Fixture f;f.host.on_key=[&]{check(f.stage.boss_defeated&&f.stage.max_z_rate==100);f.quest.completed=true;};f.run();check(f.host.complete);}
    {Fixture f;f.host.on_make=[&]{f.host.stack=nullptr;};f.run();check(f.host.calls[3]=="push");} // Captured stack survives replacement.
    {Fixture f;f.host.result=nullptr;f.run();check(f.host.calls[3]=="push");}
    {Fixture f;f.host.on_send=[&]{f.host.second=&f.host.other;};f.run();check(f.host.calls.back()=="send2");}
    for(int mask=0;mask<4;++mask){Fixture f;if(!(mask&1))f.host.first=nullptr;if(!(mask&2))f.host.second=nullptr;
        release_stage_movement(f.host);check(f.host.calls.size()==static_cast<std::size_t>(4+((mask&1)!=0)+((mask&2)!=0)));}
    {Fixture f;f.host.stack=nullptr;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.stage.boss_defeated&&f.stage.max_z_rate==100&&f.host.calls==std::vector<std::string>{"key8","stack","make"});}
    {Fixture f;f.stage.quest=nullptr;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}check(failed&&f.host.calls.back()=="stack");}
    {Fixture f;f.enemies[0].reset();bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}check(failed&&f.host.calls.back()=="stack");}
    {Fixture f;f.host.on_make=[] {throw std::runtime_error("factory");};bool failed=false;try{f.run();}catch(const std::runtime_error&){failed=true;}
        check(failed&&f.stage.boss_defeated&&f.host.calls.back()=="make");}
    {Fixture f;f.host.on_send=[] {throw std::runtime_error("subscriber");};bool failed=false;try{f.run();}catch(const std::runtime_error&){failed=true;}
        check(failed&&f.host.calls.back()=="send1");}
    {Fixture f;f.run();f.run();check(f.host.calls.size()==20);} // No already-defeated guard.
}
