#include "core/gameplay/select_guidance_save.hpp"
#include <cstdlib>
#include <iostream>
#include <functional>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool value) {if(!value){std::cerr<<"guidance/save contract failure\n";std::exit(1);}}
struct Host final:SelectGuidanceSaveHost {
    std::shared_ptr<BuildingSlots> slots=std::make_shared<BuildingSlots>();
    std::vector<std::string> calls;
    std::string fail;
    bool top=true,pressed=false,print=true,enabled=false,native_failure=false;
    std::function<void()> enable_hook;
    void call(const std::string& name) {
        calls.push_back(name);
        if(fail==name) {
            if(native_failure)throw std::runtime_error("host failure");
            throw SelectSaveError{};
        }
    }
    std::shared_ptr<BuildingSlots> building_slots() override {call("slots");return slots;}
    bool can_build(std::int32_t index) override {check(index==0);call("enable");if(enable_hook)enable_hook();return enabled;}
    bool is_top_form() override {call("top");return top;}
    void save_game(bool automatic,bool option) override {check(option);call(automatic?"auto":"save");}
    void report_autosave_error(const SelectSaveError&) override {call("auto_error");}
    bool soft_label(std::int32_t index) override {check(index==19);call("label");return pressed;}
    void save_system() override {call("system");}
    void show_offerwall() override {call("offer");}
    bool print_enabled() override {call("print");return print;}
    void report_offerwall_error(const SelectSaveError&) override {call("offer_error");}
};
struct Fixture {
    std::int32_t camera=0;float drag=0;
    SelectBuildingState state{-1,camera,0,drag,-1,-1};
    SelectFormPassState form{};Host host;
    void run(){process_select_guidance_save(state,form,host);}
};
int main() {
    {Fixture f;f.state.selected_chip=999;f.run();check(f.state.unlock_info_number==999 && f.state.unlock_info_frames==3 && f.state.selected_chip==999);check(f.host.calls==std::vector<std::string>{"label"});}
    {Fixture f;f.state.selected_chip=0;f.host.slots->push_back(std::make_shared<EventIntegers>(EventIntegers{0,-1}));f.host.enable_hook=[&]{f.state.selected_chip=7;};f.run();check(f.state.selected_chip==-1 && f.state.unlock_info_number==7 && f.state.unlock_info_frames==60);}
    {Fixture f;f.state.selected_chip=0;f.host.slots->push_back(nullptr);f.run();check(f.state.selected_chip==0);check((f.host.calls==std::vector<std::string>{"slots","label"}));}
    {Fixture f;f.state.selected_chip=-2;try{f.run();check(false);}catch(const std::out_of_range&){}check(f.host.calls.size()==1);}
    {Fixture f;f.form.auto_save=true;f.host.top=false;f.run();check(f.form.auto_save);check((f.host.calls==std::vector<std::string>{"top","label"}));}
    {Fixture f;f.form.auto_save=true;f.host.fail="auto";f.run();check(!f.form.auto_save);check((f.host.calls==std::vector<std::string>{"top","auto","auto_error","label"}));}
    {Fixture f;f.form.auto_save=true;f.host.fail="top";try{f.run();check(false);}catch(const SelectSaveError&){}check(f.form.auto_save);check(f.host.calls.size()==1);}
    {Fixture f;f.form.auto_save=true;f.host.fail="auto";f.host.native_failure=true;try{f.run();check(false);}catch(const std::runtime_error&){}check(f.form.auto_save);check(f.host.calls.size()==2);}
    {Fixture f;f.form.auto_save=true;f.host.pressed=true;f.run();check(!f.form.auto_save);check((f.host.calls==std::vector<std::string>{"top","auto","label","save","system","offer"}));}
    for(const auto* failure:{"save","system","offer"}) {
        Fixture f;f.host.pressed=true;f.host.fail=failure;f.run();
        check(f.host.calls[f.host.calls.size()-2]=="print" && f.host.calls.back()=="offer_error");
        const auto count=std::string(failure)=="save"?4U:std::string(failure)=="system"?5U:6U;
        check(f.host.calls.size()==count);
    }
    {Fixture f;f.host.pressed=true;f.host.print=false;f.host.fail="save";f.run();check(f.host.calls.back()=="print");}
    {Fixture f;f.host.fail="label";try{f.run();check(false);}catch(const SelectSaveError&){}check(f.host.calls.size()==1);}
}
