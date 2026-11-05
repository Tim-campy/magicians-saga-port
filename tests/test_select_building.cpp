#include "core/gameplay/select_building.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"building contract failure\n";std::exit(1);}}
template<class F> void fails(F f) {try{f();check(false);}catch(const std::exception&) {}}
struct Host:SelectBuildingHost,FormStackHost {
    std::int32_t camera=12;float drag=4.9F;
    SelectBuildingState state{0,camera,13,drag,9,60};
    SelectFormPassState form{};
    bool button=true,enabled=true,push_failure=false;
    std::vector<int> log;
    int catalog_reads=0,change_kind=0,share_value=0;
    EventObject child=std::make_shared<int>(3);
    std::shared_ptr<BuildingSlots> slots=std::make_shared<BuildingSlots>();
    std::shared_ptr<EventIntegers> save=std::make_shared<EventIntegers>(13,0);
    std::shared_ptr<BuildingMenuCatalog> data=std::make_shared<BuildingMenuCatalog>();
    std::shared_ptr<std::vector<EventObject>> members=std::make_shared<std::vector<EventObject>>(1);
    Host() {form.form.manager=this;slots->push_back(std::make_shared<EventIntegers>(EventIntegers{0,0}));data->push_back(std::make_shared<BuildingMenuRecord>(BuildingMenuRecord{1}));}
    bool run() {return process_select_building_button(state,form,*this);}
    bool pressed(std::uint32_t mask) override {check(mask==0x08000000U);log.push_back(1);return button;}
    void play_button_sound() override {log.push_back(2);}
    std::shared_ptr<BuildingSlots> building_slots() override {log.push_back(3);return slots;}
    std::shared_ptr<EventIntegers> save_integers() override {log.push_back(4);return save;}
    std::shared_ptr<BuildingMenuCatalog> buildings() override {++catalog_reads;if(change_kind && catalog_reads==2)data->at(0)->kind=change_kind;return data;}
    bool can_build(std::int32_t i) override {check(i==0);log.push_back(5);return enabled;}
    std::shared_ptr<std::vector<EventObject>> living_members() override {return members;}
    std::int32_t system_share_value() override {return share_value;}
    EventObject make_shop(std::int32_t type) override {log.push_back(100+type);return child;}
    void set_shop_selection(EventObject shop,std::int32_t i) override {check(shop==child && i==0);log.push_back(6);}
    EventObject make_build_list(std::int32_t i) override {check(i==0);log.push_back(7);return child;}
    EventObject make_human_house(EventObject m,std::int32_t i,bool option) override {check(m==members->at(0) && i==0 && !option);log.push_back(8);return child;}
    EventObject make_subform(std::int32_t type) override {check(type==15);log.push_back(9);return child;}
    EventObject make_share_window(bool first) override {check(first==(share_value==0));log.push_back(10);return child;}
    void push(EventObject,EventObject value,bool option) override {check(value==child && !option && form.auto_save && form.events.event_check_requested);log.push_back(11);if(push_failure)throw std::runtime_error("manager");}
};
void catalog_contract(){
    BuildingDefinition record{9,"old-name","old-resource",3,4,5,6,7,"old-explanation",8};
    int parses=0;const EventIntegerParser parser=[&](std::string_view text){++parses;return parse_event_decimal(text);};
    check(&load_building_definition(record,std::nullopt,parser)==&record);
    check(&load_building_definition(record,std::string_view{},parser)==&record&&parses==0);
    check(record.id==9&&record.name=="old-name"&&record.explanation=="old-explanation"&&record.flags==8);
    const auto line=std::string_view("2\tHouse\tresident\t77\t-1\t8\t2147483647\t-2147483648\tDescription\t2\tignored");
    check(&load_building_definition(record,line,parser)==&record&&parses==7);
    check(record.id==2&&record.name=="House"&&record.resource=="resident"&&record.image==77&&record.owner_id==-1);
    check(record.type==8&&record.cost==std::numeric_limits<std::int32_t>::max()&&record.base_have==std::numeric_limits<std::int32_t>::min());
    check(record.explanation=="Description"&&record.flags==2);
    fails([&]{load_building_definition(record,"6\tChanged\tother\t44\t-7\t9\tbad",parser);});
    check(record.id==6&&record.name=="Changed"&&record.resource=="other"&&record.image==44&&record.owner_id==-7&&record.type==9);
    check(record.cost==std::numeric_limits<std::int32_t>::max()&&record.base_have==std::numeric_limits<std::int32_t>::min()&&record.explanation=="Description"&&record.flags==2);
    fails([&]{load_building_definition(record,"7\tN\tR\t1\t2\t3\t4\t5\tnew-explanation",parser);});
    check(record.id==7&&record.cost==4&&record.base_have==5&&record.explanation=="new-explanation"&&record.flags==2);
    const auto table=load_building_table("\xef\xbb\xbf\n2\tFirst\ta\t3\t4\t8\t6\t7\ttext\t1\r\n0\tZero\tb\t8\t9\t12\t10\t11\tother\t2\r\n2\tLast\tc\t13\t14\t7\t15\t16\tlast\t3\tignored\n",true,parse_event_decimal);
    check(table.size()==3&&table[0]&&!table[1]&&table[2]);
    check(table[0]->id==0&&table[0]->type==12&&table[2]->name=="Last"&&table[2]->image==13&&table[2]->base_have==16&&table[2]->flags==3);
    check(load_building_table("",false,parse_event_decimal).size()==1&&!load_building_table("",false,parse_event_decimal)[0]);
    fails([&]{load_building_table("-1\tN\tR\t0\t0\t0\t0\t0\tE\t0",false,parse_event_decimal);});
    fails([&]{load_building_table("2147483647\tN\tR\t0\t0\t0\t0\t0\tE\t0",false,parse_event_decimal);});
    for(auto type:{8,12})check(building_is_decoration(type));
    for(auto type:{std::numeric_limits<std::int32_t>::min(),-1,0,7,9,11,13,std::numeric_limits<std::int32_t>::max()})check(!building_is_decoration(type));
}
void king_catalog_contract(){
    KingRewardDefinition record;record.id=9;record.name="old";record.score_value=7;record.crystal_level=8;record.timing=9;record.received=true;
    record.terms=std::make_shared<std::vector<EventInstruction>>(1,std::make_shared<EventIntegers>(EventIntegers{1,2}));record.execs=record.terms;
    const auto old=record.terms;
    int calls=0;const EventIntegerParser parser=[&](std::string_view token){++calls;return parse_event_decimal(token);};
    check(&load_king_reward_definition(record,std::nullopt,parser)==&record);
    check(&load_king_reward_definition(record,std::string_view{},parser)==&record&&calls==0&&record.received&&record.terms==old);
    load_king_reward_definition(record,"3\tGift\t42\t2\t9\t1\t2\t10\t20\t-1\tignored",parser);
    check(record.id==3&&record.name=="Gift"&&record.score_value==42&&record.crystal_level==2&&record.timing==9&&record.received);
    check(record.terms&&record.terms->size()==1&&*record.terms->at(0)==EventIntegers({10,20})&&!record.execs);
    record.terms=old;record.execs=old;
    fails([&]{load_king_reward_definition(record,"4\tChanged\t55\t3\t10\t1\t2\t12",parser);});
    check(record.id==4&&record.name=="Changed"&&record.score_value==55&&record.timing==10&&record.terms==old&&record.execs==old&&record.received);
    fails([&]{load_king_reward_definition(record,"5\tOther\t66\t4\t11\t0\t1\t2\t3",parser);});
    check(record.id==5&&record.score_value==66&&record.terms&&record.terms->empty()&&record.execs==old&&record.received);
    const auto empty=record.terms;reset_king_reward_for_new_game(record);
    check(!record.received&&record.id==5&&record.name=="Other"&&record.score_value==66&&record.crystal_level==4&&record.timing==11&&record.terms==empty&&record.execs==old);
    const auto table=load_king_reward_table("\xef\xbb\xbf\n2\tFirst\t10\t0\t0\t0\t0\r\n0\tZero\t20\t1\t1\t-1\t0\r\n2\tLast\t30\t2\t2\t0\t-1\tignored\n",true,parse_event_decimal);
    check(table.size()==3&&table[0]&&!table[1]&&table[2]);
    check(table[0]->id==0&&!table[0]->terms&&table[0]->execs&&table[0]->execs->empty());
    check(table[2]->name=="Last"&&table[2]->score_value==30&&table[2]->terms&&table[2]->terms->empty()&&!table[2]->execs&&!table[2]->received);
    fails([&]{load_king_reward_definition(record,"1\tBad\t0\t0\t0\t1\t-1\t0",parser);});
    fails([&]{load_king_reward_table("-1\tBad\t0\t0\t0\t0\t0",false,parser);});
}
void building_new_game_contract(){
    BuildingNewGameState state;check(state.discovery_state==0&&!state.is_new&&state.have==0);
    BuildingDefinition record;record.flags=4;record.base_have=-17;
    state={7,true,9};reset_building_for_new_game(state,record,2);
    check(state.discovery_state==7&&state.is_new&&state.have==-17);
    state={2,false,9};reset_building_for_new_game(state,record,4);
    check(state.discovery_state==1&&state.is_new&&state.have==-17);
    state={0,false,9};record.flags=1;reset_building_for_new_game(state,record,1);
    check(state.discovery_state==1&&state.is_new&&state.have==-17);
    record.flags=std::numeric_limits<std::int32_t>::min();record.base_have=1;int finds=0;
    reset_building_for_new_game(state,record,std::numeric_limits<std::int32_t>::min(),[&](bool fresh){
        check(fresh&&state.have==-17);++finds;record.base_have=std::numeric_limits<std::int32_t>::min();record.flags=0;
    });
    check(finds==1&&state.have==std::numeric_limits<std::int32_t>::min());
    record.flags=4;record.base_have=23;state={7,false,9};
    fails([&]{reset_building_for_new_game(state,record,4,[&](bool fresh){
        check(fresh);state.discovery_state=3;state.is_new=true;record.base_have=42;throw std::runtime_error("Find failure");
    });});
    check(state.discovery_state==3&&state.is_new&&state.have==9);
    finds=0;reset_building_for_new_game(state,record,0,[&](bool){++finds;});
    check(finds==0&&state.discovery_state==3&&state.is_new&&state.have==42);
}
struct RewardHost:KingRewardAvailabilityHost {
    std::int64_t score=0;int score_reads=0,reads=0;
    std::shared_ptr<KingRewardRows> data=std::make_shared<KingRewardRows>();
    std::vector<char> log;std::function<void(int)> on_read;
    std::int64_t high_score()override{++score_reads;log.push_back('S');return score;}
    std::shared_ptr<KingRewardRows> rewards()override{log.push_back('R');++reads;if(on_read)on_read(reads);return data;}
    std::shared_ptr<KingRewardDefinition> add(std::int32_t threshold,bool received=false){
        auto row=std::make_shared<KingRewardDefinition>();row->score_value=threshold;row->received=received;data->push_back(row);return row;
    }
};
void king_availability_contract(){
    {RewardHost h;check(!king_reward_available(h)&&h.log==std::vector<char>({'S','R'})&&h.score_reads==1);}
    {RewardHost h;h.data.reset();fails([&]{king_reward_available(h);});check(h.log==std::vector<char>({'S','R'}));}
    {RewardHost h;h.data->push_back(nullptr);fails([&]{king_reward_available(h);});check(h.reads==2&&h.score_reads==1);}
    {RewardHost h;h.score=10;const auto taken=h.add(-100,true),ready=h.add(10);h.data->push_back(nullptr);
        check(king_reward_available(h)&&h.reads==5&&h.score_reads==1&&taken->received&&!ready->received);}
    {RewardHost h;h.score=10;h.add(0,true);h.add(11);check(!king_reward_available(h)&&h.reads==6);}
    {RewardHost h;h.score=0x100000005LL;h.add(5);check(king_reward_available(h)&&h.reads==3);}
    {RewardHost h;h.score=0x80000000LL;h.add(0);check(!king_reward_available(h));
        h.data->at(0)->score_value=std::numeric_limits<std::int32_t>::min();check(king_reward_available(h));}
    {RewardHost h;h.score=std::numeric_limits<std::int64_t>::max();h.add(0);check(!king_reward_available(h));
        h.data->at(0)->score_value=-1;check(king_reward_available(h));}
    {RewardHost h;h.score=std::numeric_limits<std::int64_t>::min();h.add(0);check(king_reward_available(h));}
    {RewardHost h;h.add(1);h.on_read=[&](int n){if(n==1)h.score=1000;};check(!king_reward_available(h)&&h.score_reads==1);}
    {RewardHost h;const auto previous=h.add(100);h.on_read=[&](int n){if(n==3){h.data=std::make_shared<KingRewardRows>();h.add(-10);}};
        check(king_reward_available(h)&&h.reads==3&&previous->score_value==100&&!previous->received);}
    {RewardHost h;h.add(-10);h.on_read=[&](int n){if(n==2)h.data->at(0)->received=true;};
        check(!king_reward_available(h)&&h.reads==3);}
    {RewardHost h;h.add(-10);h.on_read=[&](int n){if(n==3)h.data->at(0).reset();};
        fails([&]{king_reward_available(h);});check(h.reads==3&&h.score_reads==1);}
}
struct ExecutionHost:KingRewardExecutionHost {
    RewardHost source;
    char failure=0;
    EventObject child=std::make_shared<int>(1);
    std::vector<std::array<std::int32_t,2>> formats;
    std::vector<EventInstruction> instructions;
    std::vector<EventContext> contexts;
    std::vector<EventObject> pushed;
    std::vector<std::int32_t> credits;
    std::vector<KingRewardDefinition*> marked;
    std::function<void()> on_format,on_execute,on_credit;
    std::int64_t high_score()override{return source.high_score();}
    std::shared_ptr<KingRewardRows> rewards()override{return source.rewards();}
    void call(char value){source.log.push_back(value);if(value==failure)throw std::runtime_error("synthetic king service failure");}
    EventObject format_arguments(std::int32_t crystal,std::int32_t score)override{
        call('F');formats.push_back({crystal,score});
        const auto args=std::make_shared<EventIntegers>(EventIntegers{crystal,score});if(on_format)on_format();return args;
    }
    EventObject execute_instruction(const EventInstruction& row,const EventContext& context)override{
        call('X');instructions.push_back(row);contexts.push_back(context);if(on_execute)on_execute();return row?child:EventObject{};
    }
    void push_form(EventObject value)override{call('P');pushed.push_back(std::move(value));}
    void add_crystals(std::int32_t value)override{call('C');credits.push_back(value);if(on_credit)on_credit();}
    void mark_received(KingRewardDefinition& row)override{call('M');marked.push_back(&row);row.received=true;}
    std::shared_ptr<KingRewardDefinition> add(std::int32_t threshold,std::int32_t crystals,EventScript script){
        const auto row=source.add(threshold);row->crystal_level=crystals;row->execs=std::move(script);return row;
    }
    std::string log()const{return {source.log.begin(),source.log.end()};}
};
EventScript king_script(std::initializer_list<EventInstruction> rows){return std::make_shared<std::vector<EventInstruction>>(rows);}
void king_execution_contract(){
    const auto first=std::make_shared<EventIntegers>(EventIntegers{8,0});
    const auto second=std::make_shared<EventIntegers>(EventIntegers{12,1});
    const std::string complete="SRRRRRRRFXPRRCRMR";
    {ExecutionHost h;const auto row=h.add(0,3,nullptr);h.failure='F';
        check(check_and_execute_king_rewards(false,h)&&h.log()=="SRRR"&&!row->received&&h.credits.empty());}
    {ExecutionHost h;const auto row=h.add(0,3,king_script({first}));
        check(check_and_execute_king_rewards(true,h)&&h.log()==complete&&row->received&&h.credits==std::vector<std::int32_t>({3}));
        check(h.formats==std::vector<std::array<std::int32_t,2>>({{3,0}})&&h.instructions==std::vector<EventInstruction>({first}));
        check(h.pushed==std::vector<EventObject>({h.child})&&h.contexts.size()==1&&!h.contexts[0].argi&&
              *std::static_pointer_cast<EventIntegers>(h.contexts[0].args)==EventIntegers({3,0}));
        check(h.source.score_reads==1&&h.source.reads==11);}
    {ExecutionHost h;h.source.score=0x10000000aLL;
        const auto taken=h.add(-100,99,nullptr);taken->received=true;
        const auto a=h.add(0,-3,king_script({first})),b=h.add(10,7,king_script({second}));
        const auto later=h.add(11,100,nullptr);a->terms=nullptr;a->timing=std::numeric_limits<std::int32_t>::min();
        h.on_execute=[&]{h.source.score=9999;};
        check(check_and_execute_king_rewards(true,h)&&a->received&&b->received&&!later->received&&taken->received);
        check(h.credits==std::vector<std::int32_t>({-3,7})&&h.instructions==std::vector<EventInstruction>({first,second})&&
              h.pushed==std::vector<EventObject>({h.child,h.child})&&h.source.score_reads==1);}
    {ExecutionHost h;h.source.score=0x80000000LL;const auto row=h.add(0,3,nullptr);
        check(!check_and_execute_king_rewards(true,h)&&!row->received&&h.credits.empty());}
    {ExecutionHost h;const auto row=h.add(0,0,king_script({}));
        check(check_and_execute_king_rewards(true,h)&&row->received&&h.credits==std::vector<std::int32_t>({0})&&h.formats.empty());}
    {ExecutionHost h;const auto row=h.add(0,2,king_script({nullptr}));
        check(check_and_execute_king_rewards(true,h)&&row->received&&h.instructions.size()==1&&!h.instructions[0]&&h.pushed.empty());}
    for(const auto failure:{'F','X','P','C','M'}){
        ExecutionHost h;const auto row=h.add(0,3,king_script({first}));h.failure=failure;
        fails([&]{check_and_execute_king_rewards(true,h);});
        check(h.log()==complete.substr(0,complete.find(failure)+1)&&!row->received&&h.marked.empty());
        check(h.credits.size()==(failure=='M'?1U:0U));
    }
    {ExecutionHost h;h.source.data.reset();fails([&]{check_and_execute_king_rewards(true,h);});check(h.log()=="SR");}
    {ExecutionHost h;h.source.data->push_back(nullptr);fails([&]{check_and_execute_king_rewards(true,h);});check(h.log()=="SRR");}
    {ExecutionHost h;const auto row=h.add(0,3,nullptr);fails([&]{check_and_execute_king_rewards(true,h);});check(h.log()=="SRRRR"&&!row->received);}
    {ExecutionHost h;h.add(0,3,king_script({}));h.source.on_read=[&](int n){if(n==5)h.source.data->at(0).reset();};
        fails([&]{check_and_execute_king_rewards(true,h);});check(h.log()=="SRRRRR"&&h.credits.empty());}
    {ExecutionHost h;const auto row=h.add(0,3,king_script({first}));
        h.source.on_read=[&](int n){if(n==5)row->execs=king_script({});};
        fails([&]{check_and_execute_king_rewards(true,h);});check(h.log()=="SRRRRR"&&h.formats.empty()&&!row->received);}
    {ExecutionHost h;const auto original=h.add(0,3,king_script({first}));
        auto replacement=std::make_shared<KingRewardDefinition>();replacement->crystal_level=11;replacement->execs=king_script({});
        h.on_format=[&]{h.source.data=std::make_shared<KingRewardRows>(KingRewardRows{replacement});};
        check(check_and_execute_king_rewards(true,h)&&h.instructions==std::vector<EventInstruction>({first})&&!original->received&&replacement->received);
        check(h.formats==std::vector<std::array<std::int32_t,2>>({{3,0}})&&h.credits==std::vector<std::int32_t>({11})&&h.marked[0]==replacement.get());}
    {ExecutionHost h;const auto original=h.add(0,3,king_script({first}));
        auto crystal=std::make_shared<KingRewardDefinition>();crystal->crystal_level=5;
        auto score=std::make_shared<KingRewardDefinition>();score->crystal_level=9;score->score_value=7;score->execs=king_script({});
        h.source.on_read=[&](int n){if(n==6)h.source.data=std::make_shared<KingRewardRows>(KingRewardRows{crystal});
                                   if(n==7)h.source.data=std::make_shared<KingRewardRows>(KingRewardRows{score});};
        check(check_and_execute_king_rewards(true,h)&&h.instructions[0]==first&&!original->received&&score->received);
        check(h.formats==std::vector<std::array<std::int32_t,2>>({{5,7}})&&h.credits==std::vector<std::int32_t>({9}));}
    {ExecutionHost h;const auto a=h.add(0,3,king_script({first}));
        auto b=std::make_shared<KingRewardDefinition>();b->crystal_level=4;b->execs=king_script({});
        h.on_execute=[&]{if(h.instructions.size()==1){a->execs=king_script({first,second});h.source.data=std::make_shared<KingRewardRows>(KingRewardRows{a,b});}};
        check(check_and_execute_king_rewards(true,h)&&a->received&&b->received&&h.instructions==std::vector<EventInstruction>({first,second})&&
              h.credits==std::vector<std::int32_t>({3,4}));}
    {ExecutionHost h;const auto original=h.add(0,3,king_script({}));
        auto replacement=std::make_shared<KingRewardDefinition>();replacement->crystal_level=9;replacement->execs=king_script({});
        h.on_credit=[&]{h.source.data=std::make_shared<KingRewardRows>(KingRewardRows{replacement});};
        check(check_and_execute_king_rewards(true,h)&&h.credits==std::vector<std::int32_t>({3})&&!original->received&&replacement->received);}
    {ExecutionHost h;const auto row=h.add(0,3,king_script({first}));
        h.source.on_read=[&](int n){if(n==10)h.source.data->at(0).reset();};
        fails([&]{check_and_execute_king_rewards(true,h);});check(h.credits==std::vector<std::int32_t>({3})&&!row->received&&h.marked.empty());}
    {ExecutionHost h;const auto a=h.add(0,3,king_script({})),b=h.add(0,4,nullptr);
        fails([&]{check_and_execute_king_rewards(true,h);});check(a->received&&!b->received&&h.credits==std::vector<std::int32_t>({3}));}
}
int main() {
    catalog_contract();
    building_new_game_contract();king_catalog_contract();king_availability_contract();king_execution_contract();
    for(auto kind:{1,2,3,4,5,9,10})check(building_is_shop(kind));
    for(auto kind:{-1,0,6,7,8,11,12,std::numeric_limits<int>::max()})check(!building_is_shop(kind));
    {Host h;h.button=false;check(!h.run() && h.log==std::vector<int>({1}));}
    {Host h;h.state.selected_chip=-1;h.slots.reset();check(!h.run() && h.log==std::vector<int>({1,2,3}));}
    {Host h;h.state.selected_chip=1;check(!h.run() && h.state.selected_chip==1 && h.save->at(11)==0);}
    {Host h;h.change_kind=10;check(h.run());check(h.state.selected_chip==-1 && h.catalog_reads==2 && h.log==std::vector<int>({1,2,3,4,4,4,110,11}));check(h.save->at(11)==12 && h.save->at(12)==13 && h.save->at(10)==4);}
    {Host h;h.slots->at(0).reset();check(h.run() && h.state.selected_chip==-1 && !h.form.auto_save);}
    {Host h;h.slots->at(0)->at(1)=-1;h.enabled=false;check(h.run() && h.state.unlock_info_frames==60 && !h.form.auto_save);}
    {Host h;h.slots->at(0)->at(1)=-1;h.push_failure=true;fails([&]{h.run();});check(h.state.selected_chip==0 && h.state.unlock_info_number==-1 && h.state.unlock_info_frames==-1 && h.form.auto_save);}
    {Host h;h.data->at(0)->kind=6;check(h.run() && h.log.at(h.log.size()-3)==106 && h.log.at(h.log.size()-2)==6);}
    {Host h;h.data->at(0)->kind=6;h.members->at(0)=std::make_shared<int>(1);check(h.run() && h.log.at(h.log.size()-2)==8);}
    {Host h;h.data->at(0)->kind=6;h.child.reset();fails([&]{h.run();});check(h.state.selected_chip==0 && !h.form.auto_save);}
    for(auto kind:{7,12,99}) {Host h;h.data->at(0)->kind=kind;check(h.run() && h.state.selected_chip==-1);check(h.form.auto_save==(kind!=99));check(h.catalog_reads==(kind==7 ? 3:4));}
    {Host h;h.save->resize(12);fails([&]{h.run();});check(h.save->at(11)==12 && h.save->at(10)==0 && h.state.selected_chip==0);}
    {Host h;h.drag=std::numeric_limits<float>::infinity();save_select_position(h.state,h);check(h.save->at(10)==std::numeric_limits<int>::min());h.drag=2147483648.0F;save_select_position(h.state,h);check(h.save->at(10)==std::numeric_limits<int>::max());h.drag=std::numeric_limits<float>::quiet_NaN();save_select_position(h.state,h);check(h.save->at(10)==0);}
}
