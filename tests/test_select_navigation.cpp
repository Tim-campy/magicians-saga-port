#include "core/gameplay/select_navigation.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"navigation contract failure\n";std::exit(1);}}
struct Host:SelectNavigationHost,FormStackHost {
    std::uint32_t buttons=0;
    std::vector<std::uint32_t> log;
    std::function<void()> pending;
    EventObject room=std::make_shared<int>(1),shop=std::make_shared<int>(2),pushed;
    bool room_failure=false,push_failure=false;
    bool pressed(std::uint32_t mask) override {log.push_back(mask);return (buttons&mask)!=0;}
    void play_button_sound() override {log.push_back(1);}
    void make_transition(std::int32_t type,bool option,std::uint32_t argb,std::function<void()> action) override {
        check(type==3 && option && argb==0xff303030U);log.push_back(2);pending=std::move(action);
    }
    EventObject room_form() override {log.push_back(3);if(room_failure)throw std::runtime_error("AppData");return room;}
    EventObject make_shop(std::int32_t type) override {check(type==11);log.push_back(4);return shop;}
    void push(EventObject,EventObject value,bool option) override {check(!option);log.push_back(5);pushed=value;if(push_failure)throw std::runtime_error("push");}
    void finish() {auto action=std::move(pending);pending={};action();}
};
int main() {
    auto host=std::make_shared<Host>();
    auto state=std::make_shared<SelectFormPassState>();state->form.manager=host.get();state->markers.weapon_new=true;
    host->buttons=0x05000000U;
    check(process_select_navigation(state,host)==SelectNavigationResult::ReturnTrue);
    check(host->log==std::vector<std::uint32_t>({0x04000000U,1,2}));
    check(state->markers.weapon_new && !state->auto_save && !host->pushed); // No eager callback or shop query.
    host->finish();check(!state->markers.weapon_new && state->auto_save && state->events.event_check_requested && host->pushed==host->room);
    host->log.clear();host->buttons=0x01000000U;state->markers.weapon_new=true;
    check(process_select_navigation(state,host)==SelectNavigationResult::ReturnTrue);
    check(host->log==std::vector<std::uint32_t>({0x04000000U,0x01000000U,4,5}));
    check(state->markers.weapon_new && host->pushed==host->shop);
    host->buttons=0;host->log.clear();check(process_select_navigation(state,host)==SelectNavigationResult::ContinueBuildingButtons);
    check(host->log.size()==2);
    host->buttons=0x04000000U;state->auto_save=false;state->events.event_check_requested=false;
    process_select_navigation(state,host);host->room_failure=true;
    try{host->finish();check(false);}catch(const std::runtime_error&){}
    check(!state->markers.weapon_new && !state->auto_save && !state->events.event_check_requested);
    host->room_failure=false;host->room.reset();process_select_navigation(state,host);host->finish();
    check(!host->pushed && state->auto_save); // Null child still goes through Push.
    host->buttons=0x01000000U;host->push_failure=true;state->auto_save=false;state->events.event_check_requested=false;
    try{process_select_navigation(state,host);check(false);}catch(const std::runtime_error&){}
    check(state->auto_save && state->events.event_check_requested);
    host->push_failure=false;host->buttons=0x04000000U;
    std::weak_ptr<SelectFormPassState> retained=state;process_select_navigation(state,host);state.reset();
    check(!retained.expired());host->finish();check(retained.expired());
}
