#include "core/gameplay/select_form_pass.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool v,const char* message) {if(!v){std::cerr<<message<<'\n';std::exit(1);}}
struct Host:SelectFormPassHost,SelectCloudHost,SelectMarkerHost,EventExecutionHost,FormStackHost {
    SelectFormPassState state{0,false,false,{nullptr,this,nullptr},{false,false,nullptr},{0,false}};
    EventDelayQueue queue;
    DecodedEventConditionState decoded;
    DecodedEventConditionHost conditions{decoded};
    std::vector<std::string> log;
    bool input=false,label=false,balance=false,fail_push=false;
    int town_count=0,effects=0;
    EventObject child=std::make_shared<int>(7);
    std::shared_ptr<SelectClouds> cloud_list=std::make_shared<SelectClouds>();
    std::shared_ptr<EventCatalog> catalog=std::make_shared<EventCatalog>(2);
    Host() {decoded.context_present=true;decoded.context_character_count=1;decoded.money=0;}
    SelectPassResult run() {return process_select_form_pass(state,*this,*this,*this,queue,*this,conditions);}
    bool capture_input(std::uint32_t mask) override {check(mask==0x100000U,"capture mask");log.push_back("input");return input;}
    void change_state(std::int32_t n) override {log.push_back("change");state.state=n;}
    bool soft_label(std::int32_t n) override {check(n==3,"soft label");log.push_back("label");return label;}
    void finish_capture() override {log.push_back("finish");}
    void capture() override {log.push_back("capture");}
    void play_bgm() override {log.push_back("bgm");}
    void update_notifications() override {log.push_back("notify");}
    bool acquire_balance() override {log.push_back("balance");return balance;}
    void set_active_x() override {log.push_back("active");}
    void update_decorations() override {log.push_back("decor");}
    void update_town_humans() override {log.push_back("humans");}
    std::int32_t town_enable_count() override {log.push_back("count");return town_count;}
    bool update_town_enable(std::int32_t n) override {log.push_back("enable"+std::to_string(n));return true;}
    void check_guide_arrow(bool force) override {check(force,"forced guide");log.push_back("guide");}
    std::shared_ptr<SelectClouds> clouds() override {log.push_back("clouds");return cloud_list;}
    std::int32_t right_edge() override {return 100;}
    std::int32_t cloud_resource() override {return 1;}
    std::int32_t game_height() override {return 500;}
    bool low_aspect() override {return false;}
    std::int32_t random(std::int32_t) override {return 0;}
    bool is_top_form() override {log.push_back("top");return true;}
    bool debug_enabled() override {return false;}
    std::shared_ptr<ConditionCatalog<AreaMarkerRecord>> areas() override {return std::make_shared<ConditionCatalog<AreaMarkerRecord>>();}
    std::shared_ptr<AreaQuestState> quest(std::int32_t) override {throw std::logic_error("unexpected quest");}
    std::shared_ptr<ConditionCatalog<EquipmentMarkerState>> equipment(EquipmentMarkerKind) override {return std::make_shared<ConditionCatalog<EquipmentMarkerState>>();}
    std::shared_ptr<EventCatalog> events() override {return catalog;}
    EventObject default_event_args() override {return nullptr;}
    void empty_event_diagnostic(std::int32_t) override {}
    EventObject dispatch(const EventInstruction& row,const EventContext&) override {
        ++effects;log.push_back("effect");
        if(row->at(0)==1) {decoded.money=decoded.money.value()+1;return nullptr;}
        return child;
    }
    std::int32_t identify_script(const EventScript& script) override {return identify_event_script(*catalog,script,[]{return -1;});}
    EventScript resolve_script(std::int32_t id) override {return resolve_event_script(*catalog,id);}
    std::int32_t read_talk_character() override {return 0;}
    void restore_talk_character(std::size_t slot,std::int32_t) override {check(slot==7,"restore slot");log.push_back("restore");}
    void push(EventObject,EventObject value,bool option) override {
        check(value==child && !option && state.auto_save && state.events.event_check_requested,"flags precede push");
        log.push_back("push");if(fail_push)throw std::runtime_error("manager failed");
    }
    void add_event(int opcode) {
        auto record=std::make_shared<EventConditionRecord>();
        record->terms=std::make_shared<std::vector<EventInstruction>>();
        record->terms->push_back(std::make_shared<EventIntegers>(EventIntegers{1,0,4}));
        record->effects=std::make_shared<std::vector<EventInstruction>>();
        record->effects->push_back(std::make_shared<EventIntegers>(EventIntegers{opcode}));
        catalog->at(1)=record;
    }
};
int main() {
    {
        Host h;h.state.form.dialog=h.child;
        check(h.run()==SelectPassResult::ReturnTrue && !h.state.form.dialog && h.log.empty(),"dialog clears before state dispatch");
        h.state.state=3;h.state.captured=true;
        check(h.run()==SelectPassResult::ReturnTrue && h.log.empty(),"captured gate");
        h.state.captured=false;h.input=true;h.run();
        check(h.state.state==4 && h.log==std::vector<std::string>({"input","change"}),"capture transition returns without bgm");
        h.log.clear();h.run();check(h.log==std::vector<std::string>({"capture"}),"state4");
        h.log.clear();h.state.state=3;h.input=false;h.label=true;h.run();
        check(h.log==std::vector<std::string>({"input","label","finish","bgm"}),"capture finish ordering");
        h.log.clear();h.state.state=-1;h.run();check(h.log==std::vector<std::string>({"bgm"}),"other states skip events");
    }
    {
        Host h;h.balance=true;h.state.markers.scene_resume=true;
        h.queue.push_back(std::make_shared<DelayedEvent>(DelayedEvent{1,0,2,-1,0,{}}));
        check(h.run()==SelectPassResult::ReturnTrue && h.state.events.loop_counter==1,"counter precedes balance gate");
        check(h.log==std::vector<std::string>({"clouds","top","notify","balance"}),"clouds and markers precede notifications and balance");
        check(h.queue.at(0)->wait==2,"balance gate must not consume delay");
    }
    {
        Host h;h.add_event(8);h.town_count=2;
        check(h.run()==SelectPassResult::ContinueInput,"initial direct event result discarded");
        check(h.effects==2 && h.catalog->at(1)->execution_count==2 && !h.state.auto_save,"initial events bypass eligibility and do not select-push");
        check(h.log==std::vector<std::string>({"clouds","notify","balance","active","decor","humans","count","enable0","effect","count","enable1","effect","count","guide"}),"early counter initialization ordering");
    }
    {
        Host h;h.add_event(8);h.state.events={299,false};h.fail_push=true;
        try{h.run();check(false,"push should fail");}catch(const std::runtime_error&){}
        check(h.effects==1 && h.state.auto_save && h.state.events.event_check_requested,"automatic event push retains mutations on failure");
        check(h.log.back()=="push","event result reaches manager");
    }
    {
        Host h;h.add_event(1);h.state.events={2,false};
        h.queue.push_back(std::make_shared<DelayedEvent>(DelayedEvent{1,0,1,-1,0,{}}));
        check(h.run()==SelectPassResult::ContinueInput && h.queue.empty() && h.decoded.money==1,"queue resumes after counter2; not gated by initialization");
        check(h.log==std::vector<std::string>({"clouds","notify","balance","active","decor","humans","restore","effect"}),"queue order");
    }
}
