#include "core/ecs/system_initialization.hpp"
#include "core/gameplay/stage_start.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/gameplay/input_frame.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"native initializer failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct StageWorld final:StageInitializationWorld {
    struct Enemy final:StageEnemyStart {void start_enemies(std::shared_ptr<StageQuest>)override{std::abort();}} enemy;
    StageHumanStart human;MemberRoster member;std::vector<int> calls;
    int failure=0;bool null_results=false;StageStartState* state=nullptr;
    std::vector<std::function<void(ComponentSlots*)>> death;
    std::vector<std::function<void(const InputTouch*)>> touch;
    std::vector<std::function<void(ComponentSlots*,std::int32_t)>> damage;
    void call(int id){calls.push_back(id);if(id==failure)throw std::runtime_error("prefix");}
    StageHumanStart* initialization_humans()override{call(1);return null_results?nullptr:&human;}
    MemberRoster* initialization_members()override{call(2);return null_results?nullptr:&member;}
    StageEnemyStart* initialization_enemies()override{call(3);return null_results?nullptr:&enemy;}
    void subscribe_stage_death(std::function<void(ComponentSlots*)> f)override{
        check(state->attack_buttons->size()==3&&state->collection.collecting->empty());call(30);death.push_back(std::move(f));
    }
    void subscribe_stage_touch(std::function<void(const InputTouch*)> f)override{call(9);touch.push_back(std::move(f));}
    void subscribe_stage_damage(std::function<void(ComponentSlots*,std::int32_t)> f)override{call(15);damage.push_back(std::move(f));}
};
struct AuxiliaryWorld final:AuxiliaryInitializationWorld,TownInitializationWorld {
    std::vector<int> calls;int failure=0;bool null_global=false;PlayerWorldState world;
    std::vector<Button> buttons;std::vector<std::function<void()>> orientations;
    std::function<void(const TownReturnEvent&)> returned;
    void subscribe_auxiliary_button(std::int32_t factory,Button f)override{calls.push_back(factory);if(failure==factory)throw std::runtime_error("button");buttons.push_back(std::move(f));}
    void subscribe_menu_orientation(std::function<void()> f)override{calls.push_back(27);if(failure==99)throw std::runtime_error("orientation");orientations.push_back(std::move(f));}
    void subscribe_town_return(std::function<void(const TownReturnEvent&)> f)override{calls.push_back(31);returned=std::move(f);}
    PlayerWorldState* current_town_world()override{calls.push_back(100);return null_global?nullptr:&world;}
    void initialize_fresh_town()override{calls.push_back(101);if(failure)throw std::runtime_error("fresh town required domain service");}
};
int main()try{
    {StageStartState s;StageWorld w;w.state=&s;s.state=5;s.frame=19;s.attack_attribute=2;s.last_active_button=-1;
     s.attack_buttons=std::make_shared<std::vector<AttackButtonState*>>(1,nullptr);
     auto old_buttons=s.attack_buttons;auto old_pending=std::make_shared<std::vector<ComponentSlots*>>(1,nullptr);s.collection.collecting=old_pending;
     std::vector<int> received;StageInitializationReceivers receivers{[&](auto*){received.push_back(30);},[&](auto*){received.push_back(9);},[&](auto*,auto amount){received.push_back(amount);}};
     initialize_stage(s,&w,receivers);check(w.calls==std::vector<int>{1,2,3,30,9,15});
     check(s.humans==&w.human&&s.members==&w.member&&s.enemies==&w.enemy&&s.attack_buttons!=old_buttons&&s.collection.collecting!=old_pending);
     check(s.state==5&&s.frame==19&&s.attack_attribute==2&&s.last_active_button==-1&&old_pending->size()==1);
     auto first=s.attack_buttons;for(auto* b:*first)check(b&&b->state==0&&b->frame==4);
     check(first->at(0)!=first->at(1)&&first->at(1)!=first->at(2));
     w.death[0](nullptr);w.touch[0](nullptr);w.damage[0](nullptr,15);check(received==std::vector<int>{30,9,15});
     initialize_stage(s,&w,receivers);check(s.attack_buttons!=first&&s.attack_buttons->at(0)!=first->at(0));
     s.attack_buttons->at(0)->frame=99;check(first->at(0)->frame==4&&w.death.size()==2);}
    {StageStartState s;StageWorld w;w.state=&s;s.attack_buttons=std::make_shared<std::vector<AttackButtonState*>>(1,nullptr);auto old=s.attack_buttons;
     w.failure=2;fails([&]{initialize_stage(s,&w,{});});check(s.humans==&w.human&&s.members==nullptr&&s.attack_buttons==old);
     w.failure=15;fails([&]{initialize_stage(s,&w,{});});check(w.death.size()==1&&w.touch.size()==1&&w.damage.empty()&&s.attack_buttons!=old);
     w.failure=0;w.null_results=true;initialize_stage(s,&w,{});check(!s.humans&&!s.members&&!s.enemies);
     fails([&]{initialize_stage(s,nullptr,{});});}
    {AuxiliaryWorld w;auto shapes=initial_menu_row_shapes();check(shapes->size()==7);for(const auto row:*shapes)check(row&&*row==4);
     initialize_submenu(&w,{});initialize_menu(shapes,&w,{},{});check(w.calls==std::vector<int>{27,28,27}&&w.buttons.size()==2&&w.orientations.size()==1);
     shapes->at(0)=2;fails([&]{initialize_menu(shapes,nullptr,{},{});});check(w.buttons.size()==2);
     shapes->at(0)=std::nullopt;fails([&]{initialize_menu(shapes,&w,{},{});});fails([&]{initialize_menu(nullptr,&w,{},{});});
     shapes->clear();w.failure=99;fails([&]{initialize_menu(shapes,&w,{},{});});check(w.buttons.size()==3&&w.orientations.size()==1);
     fails([&]{initialize_submenu(nullptr,{});});initialize_status_render();}
    {InputControl c{35,123};InputTouch event{5,&c};int hidden=0;
     submenu_button_touch(&event);fails([&]{submenu_button_touch(nullptr);});event.component=nullptr;fails([&]{submenu_button_touch(&event);});
     menu_button_touch(false,nullptr,[&]{++hidden;});fails([&]{menu_button_touch(true,nullptr,{});});
     event.component=&c;menu_button_touch(true,&event,[&]{++hidden;});check(hidden==1);
     event.type=2;menu_button_touch(true,&event,[&]{++hidden;});c.group=99;event.type=5;menu_button_touch(true,&event,[&]{++hidden;});check(hidden==1);}
    {AuxiliaryWorld w;w.world.loaded=128;initialize_town_entry(&w,{});check(w.calls==std::vector<int>{31,100});
     w.calls.clear();w.null_global=true;fails([&]{initialize_town_entry(&w,{});});check(w.calls==std::vector<int>{31,100});
     w.calls.clear();w.null_global=false;w.world.loaded=0;w.failure=1;fails([&]{initialize_town_entry(&w,{});});check(w.calls==std::vector<int>{31,100,101});}
    {InitializationWorld world;InitList list;InitializationSchedule schedule{&list,true};std::array<std::unique_ptr<CallbackInitializer>,32> owners;
     std::array<InitSystem*,32> factory{};std::vector<int> order;
     for(std::size_t i=0;i<32;++i){owners[i]=std::make_unique<CallbackInitializer>([&,i](auto* w){check(w==&world&&schedule.paused);order.push_back(static_cast<int>(i));});factory[i]=owners[i].get();}
     auto bad=factory;bad[31]=nullptr;fails([&]{append_gameplay_initializers(list,bad);});
     append_gameplay_initializers(list,factory);initialize_systems(schedule,&world);check(order.size()==32&&!schedule.paused);
     for(int i=0;i<32;++i)check(order[static_cast<std::size_t>(i)]==i);}
    std::cout<<"native initializers contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
