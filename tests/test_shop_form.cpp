#include "core/gameplay/shop_form.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace saga::core;
namespace {
void check(bool value,const char* message){
    if(!value){std::cerr<<message<<'\n';std::exit(1);}
}
template<class F>void fails(F action,const char* message){
    try{action();}catch(const std::exception&){return;}check(false,message);
}
struct Host final:MagicShopFormHost {
    MagicShopFormState* state=nullptr;
    std::vector<std::string> log;
    std::function<void(const std::string&)> on_call;
    std::string fail_at;
    std::vector<std::optional<MagicShopCombatRecord>> records{
        MagicShopCombatRecord{10,0,3,0},MagicShopCombatRecord{11,1,5,0},MagicShopCombatRecord{12,3,7,0}};
    std::vector<bool> found{true,false,true};
    std::vector<std::int32_t> purchases{0,0,0};
    std::vector<std::int32_t> prices{100};std::size_t price_cursor=0;
    std::vector<std::uint32_t> pulses,repeats;std::vector<std::int32_t> labels;
    EventObject member=std::make_shared<int>(1),touch=std::make_shared<int>(2),child=std::make_shared<int>(3);
    std::optional<std::int32_t> returned;
    std::int32_t height=640,count_override=-1;
    std::int64_t money=1000;
    std::vector<MagicShopBuyRequest> notices;
    int image_resets=0,images=0,blurs=0,bgm=0,backs=0,transitions=0,executed=-1;bool transition_result=true;
    void note(std::string value){log.push_back(value);if(on_call)on_call(value);if(value==fail_at)throw std::runtime_error(value);}
    bool has(std::string value)const{return std::find(log.begin(),log.end(),value)!=log.end();}
    std::size_t occurrences(std::string value)const{return static_cast<std::size_t>(std::count(log.begin(),log.end(),value));}
    void shop_common_initialize()override{note("init");}
    EventObject shop_member_system()override{note("member");return member;}
    EventObject shop_touch_panel()override{note("touch");return touch;}
    std::int32_t shop_game_height()override{note("height");return height;}
    void shop_change_soft_labels(std::int32_t value)override{note("labels:"+std::to_string(value));}
    void shop_flag_system(std::int32_t value)override{note("flag:"+std::to_string(value));}
    std::int32_t shop_catalog_count()override{note("count");return count_override==-1?static_cast<std::int32_t>(records.size()):count_override;}
    bool shop_catalog_is_found(std::int32_t value)override{note("found:"+std::to_string(value));(void)records.at(static_cast<std::size_t>(value)).value();return found.at(static_cast<std::size_t>(value));}
    void shop_town_buildings()override{note("buildings");}
    void shop_base_update()override{note("base");}
    void shop_reset_image_animations()override{note("reset-images");++image_resets;}
    void shop_update_image_animations()override{note("images");++images;}
    void shop_update_main_blur()override{note("blur");++blurs;}
    bool shop_key_repeat(std::uint32_t value)override{note("repeat:"+std::to_string(value));return std::find(repeats.begin(),repeats.end(),value)!=repeats.end();}
    bool shop_key_pulse(std::uint32_t value)override{note("pulse:"+std::to_string(value));return std::find(pulses.begin(),pulses.end(),value)!=pulses.end();}
    bool shop_soft_label(std::int32_t value)override{note("soft:"+std::to_string(value));return std::find(labels.begin(),labels.end(),value)!=labels.end();}
    float shop_clamp(float value,float lower,float upper)override{note("clamp");if(value>upper)value=upper;if(value<lower)value=lower;return value;}
    float shop_clamp_max(float value,float maximum)override{note("clamp-max");return value>maximum?maximum:value;}
    void shop_find_building(std::int32_t value)override{note("find:"+std::to_string(value));}
    std::optional<std::int32_t> shop_take_returned_child_result()override{note("child-result");return std::exchange(returned,std::nullopt);}
    const MagicShopCombatRecord* shop_record(std::int32_t index)override{
        note("record:"+std::to_string(index));auto& value=records.at(static_cast<std::size_t>(index));return value?&*value:nullptr;
    }
    std::int32_t shop_price(const MagicShopCombatRecord& value)override{
        note("price:"+std::to_string(value.id));const auto position=std::min(price_cursor,prices.size()-1);++price_cursor;return prices.at(position);
    }
    bool shop_check_money(std::int64_t value)override{note("check:"+std::to_string(value));return value<1||money>=value;}
    void shop_subtract_money(std::int32_t value)override{note("subtract:"+std::to_string(value));money=std::max<std::int64_t>(money-value,0);}
    void shop_add_purchases(const MagicShopCombatRecord& value,std::int32_t amount)override{
        note("add:"+std::to_string(value.id));purchases.at(static_cast<std::size_t>(value.id-10))+=amount;
    }
    std::int32_t shop_parameter_key(const MagicShopCombatRecord& value)override{note("parameter:"+std::to_string(value.id));return value.category;}
    std::int32_t shop_hero_total(const EventObject& value,std::int32_t key)override{
        note("total:"+std::to_string(key));check(value==member,"captured member identity");return 800+purchases[2]*10;
    }
    EventObject shop_make_buy_confirmation(const MagicShopBuyRequest& value)override{note("make");notices.push_back(value);return child;}
    void shop_push_form(EventObject value)override{note("push");check(value==child,"confirmation identity");}
    void shop_execute_event(std::int32_t value)override{note("event:"+std::to_string(value));executed=value;}
    void shop_open_tapjoy()override{note("tapjoy");}
    void shop_open_browser()override{note("browser");}
    void shop_open_text_input()override{note("input");}
    void shop_play_bgm()override{note("bgm");++bgm;}
    void shop_back_sound()override{note("back-sound");++backs;}
    bool shop_change_to_town()override{note("town");++transitions;return transition_result;}
};
void ready(MagicShopFormState& state,Host& host){
    state.state=1;state.crossfade=-1;state.selection=0;state.page_count=1;
    state.entries=std::make_shared<std::vector<std::int32_t>>(std::initializer_list<std::int32_t>{2});
    state.filters=std::make_shared<std::vector<std::uint8_t>>(3,0);state.member_system=host.member;host.state=&state;
}
void reset_and_initialize(){
    MagicShopFormState state;Host host;host.state=&state;
    state.page=99;state.page_count=42;state.selected_item=8;state.state=9;state.frame=87;
    state.entries=std::make_shared<std::vector<std::int32_t>>(1,44);auto old_entries=state.entries;
    state.filters=std::make_shared<std::vector<std::uint8_t>>(1,7);auto old_filters=state.filters;
    host.on_call=[&](const std::string& name){
        if(name=="labels:0")check(state.state==9&&state.frame==87,"Init labels precede stores");
        if(name=="flag:256")check(state.state==0&&state.frame==0,"FlagOn follows production stores");
    };
    initialize_magic_shop(state,host);
    check(state.member_system==host.member&&state.touch_panel==host.touch,"Init owner references");
    check(state.arrival_x==60&&state.witch_x==-20&&state.witch_y==484,"Init coordinates");
    check(state.state==0&&state.frame==0&&state.crossfade==-1&&state.selection==-1,"Init and Reset fields");
    check(*state.entries==std::vector<std::int32_t>({0,2}),"Reset keeps found catalog indices rather than IDs");
    check(*state.filters==std::vector<std::uint8_t>(3,0),"kind1 does not fill affordability filters");
    check(state.page_count==1&&state.page==1&&state.selected_item==8,"inclusive ClampMax and retained selected item");
    check(*old_entries==std::vector<std::int32_t>({44})&&*old_filters==std::vector<std::uint8_t>({7}),"Reset replaces publications");
    check(host.log.size()>7&&std::vector<std::string>(host.log.begin(),host.log.begin()+7)==
        std::vector<std::string>({"init","member","touch","height","labels:0","flag:256","count"}),"Init service order");
    check(host.occurrences("count")==5&&host.log.back()=="buildings","fresh catalog bounds and final town query");

    host.on_call={};host.records.clear();host.found.clear();state.page=-4;reset_magic_shop(state,host);
    check(state.entries->empty()&&state.filters->empty()&&state.page_count==1&&state.page==-4,"empty catalog and negative page retained");
    host.records.resize(13,MagicShopCombatRecord{});host.found.assign(13,true);state.page=99;reset_magic_shop(state,host);
    check(state.page_count==2&&state.page==2,"thirteen rows form two pages with inclusive upper clamp");
}
void reset_failure_prefixes(){
    MagicShopFormState state;Host host;host.state=&state;state.page=7;state.page_count=88;
    state.filters=std::make_shared<std::vector<std::uint8_t>>(1,9);auto old_filter=state.filters;
    host.fail_at="found:2";fails([&]{reset_magic_shop(state,host);},"late catalog error must fail");
    check(*state.entries==std::vector<std::int32_t>({0})&&state.filters!=old_filter,"Reset retains partial publication");
    check(state.selection==-1&&state.crossfade==-1&&state.page==7&&state.page_count==88,"failure retains earlier stores only");
    host.fail_at.clear();host.records[1].reset();
    fails([&]{reset_magic_shop(state,host);},"null slot is not silently skipped");
    check(*state.entries==std::vector<std::int32_t>({0}),"null slot retains preceding append");
    host.records[1]=MagicShopCombatRecord{};
    host.on_call=[&](const std::string& name){if(name=="found:0")state.entries=std::make_shared<std::vector<std::int32_t>>(1,90);};
    reset_magic_shop(state,host);
    check(*state.entries==std::vector<std::int32_t>({90,0,2}),"append reloads the published list after callback");
    host.on_call=[&](const std::string& name){if(name=="buildings")state.entries.reset();};
    fails([&]{reset_magic_shop(state,host);},"CheckFilter requires list after town query");
    host.on_call={};host.count_override=-2;old_filter=state.filters;
    fails([&]{reset_magic_shop(state,host);},"negative allocation length fails");
    check(state.entries&&state.entries->empty()&&state.filters==old_filter,"allocation failure preserves old filter");
}
void opening_and_counters(){
    MagicShopFormState state;Host host;host.state=&state;initialize_magic_shop(state,host);host.log.clear();
    for(int i=0;i<60;++i)check(update_magic_shop(state,host),"ordinary Update true");
    check(state.witch_x==60&&state.crossfade==-1&&state.state==0&&state.loop_counter==60,"arrival waits without confirm before61");
    check(host.image_resets==1&&host.images==60&&host.blurs==60&&host.bgm==60,"opening prelude clocks and BGM");
    update_magic_shop(state,host);check(state.crossfade==0&&state.loop_counter==61,"opening auto-starts crossfade at61");
    const auto before_bgm=host.bgm;
    for(int i=0;i<20;++i)update_magic_shop(state,host);
    check(state.crossfade==20&&state.state==0&&host.bgm==before_bgm,"twenty crossfade passes skip BGM and main");
    update_magic_shop(state,host);
    check(state.crossfade==-1&&state.state==1&&state.frame==0&&state.loop_counter==82,"crossfade21 publishes main and clears frame");
    check(host.occurrences("labels:1")==1&&host.bgm==before_bgm,"change-state pass still skips BGM");
    update_magic_shop(state,host);check(host.has("find:1")&&host.bgm==before_bgm+1,"main starts on next pass");

    MagicShopFormState fast;Host quick;quick.state=&fast;initialize_magic_shop(fast,quick);quick.pulses={0x100000U};
    for(int i=0;i<20;++i)update_magic_shop(fast,quick);
    check(fast.crossfade==-1,"pulse does not bypass witch arrival");
    update_magic_shop(fast,quick);check(fast.crossfade==0&&fast.loop_counter==21,"arrival pulse starts earlier crossfade");
    quick.labels={3};update_magic_shop(fast,quick);check(quick.transitions==0,"crossfade ignores Back");

    MagicShopFormState edge;Host numeric;ready(edge,numeric);edge.loop_counter=2147483646;edge.frame=2147483647;
    update_magic_shop(edge,numeric);
    check(edge.loop_counter==0&&edge.frame==std::numeric_limits<std::int32_t>::min(),"loop modulus and frame wrap");
    update_magic_shop(edge,numeric);check(numeric.image_resets==1,"wrapped zero resets image clocks next pass");
    edge.state=0;edge.arrival_x=std::numeric_limits<float>::quiet_NaN();edge.witch_x=0;numeric.log.clear();
    update_magic_shop(edge,numeric);check(std::isnan(edge.witch_x)&&!numeric.has("pulse:1048576"),"unordered arrival does not acknowledge");
}
void purchase_order_and_live_values(){
    MagicShopFormState state;Host host;ready(state,host);host.prices={100,130};host.pulses={0x02000000U,0x10000000U};host.labels={12,21,3};
    host.on_call=[&](const std::string& name){
        if(name=="check:100")state.entries=std::make_shared<std::vector<std::int32_t>>(1,0);
        if(name=="push")state.selection=1;
    };
    update_magic_shop(state,host);
    check(host.money==870&&host.purchases[2]==1&&host.purchases[0]==0,"payment uses fresh second price and captured catalog slot");
    check(host.occurrences("record:2")==5&&host.price_cursor==2,"each native record lookup remains fresh");
    check(host.notices.size()==1,"purchase creates notice once");const auto notice=host.notices.front();
    check(notice.kind==1&&notice.character_index==0&&notice.parameter_key==3&&notice.add_value==7&&notice.total==810,"notice arguments follow incremented purchase state");
    check(state.selection==1&&*state.entries==std::vector<std::int32_t>({0,2}),"Reset preserves post-Push selection");
    const auto make=std::find(host.log.begin(),host.log.end(),"make"),debit=std::find(host.log.begin(),host.log.end(),"subtract:130");
    check(debit<make&&host.log.back()=="town","payment precedes child and outer Back still runs");
    check(host.has("tapjoy")&&host.has("browser")&&host.has("input")&&host.bgm==1&&host.backs==1,"same-pass optional inputs survive purchase");
    host.pulses.clear();host.transition_result=false;
    check(!update_magic_shop(state,host),"Back returns the transition result");
    check(host.log[0]=="base"&&host.log[1]=="reset-images"&&state.update_pair==std::array<std::int32_t,2>{{1,0}},"Update field pair precedes prelude");
}
void purchase_failure_and_refusal(){
    MagicShopFormState state;Host host;ready(state,host);host.money=20;host.pulses={0x02000000U};auto entries=state.entries;
    update_magic_shop(state,host);
    check(host.money==20&&host.price_cursor==1&&host.purchases[2]==0&&host.notices.empty(),"insufficient funds do not debit or push");
    check(state.entries!=entries&&state.selection==0&&host.has("pulse:268435456")&&host.bgm==1,"refused buy still Resets and runs suffix");
    host.money=1000;host.price_cursor=0;host.prices={-7};host.log.clear();state.entries=std::make_shared<std::vector<std::int32_t>>(1,2);
    update_magic_shop(state,host);check(host.money==1007&&host.has("check:-7"),"negative price keeps sign extension and subtraction");

    MagicShopFormState partial;Host failed;ready(partial,failed);failed.pulses={0x02000000U};failed.fail_at="make";entries=partial.entries;
    fails([&]{update_magic_shop(partial,failed);},"notice failure must propagate");
    check(failed.money==900&&failed.purchases[2]==1&&partial.entries==entries&&partial.selection==0,"notice failure preserves paid/incremented prefix without Reset");
    check(!failed.has("push")&&!failed.has("bgm"),"exception prevents later child and suffix");
    failed.fail_at.clear();failed.pulses={0x02000000U};partial.member_system.reset();entries=partial.entries;failed.log.clear();
    fails([&]{update_magic_shop(partial,failed);},"missing MemberSystem fails after count/key and record query");
    check(failed.money==800&&failed.purchases[2]==2&&failed.has("parameter:12")&&failed.occurrences("record:2")==5&&!failed.has("total:3"),"null member failure order");
    check(partial.entries==entries,"late member failure does not Reset");
}
void returned_children_paging_and_special_input(){
    MagicShopFormState state;Host host;ready(state,host);host.returned=-1;host.pulses={0x10000000U};
    update_magic_shop(state,host);
    check(!host.returned&&state.selection==-1&&!host.has("repeat:262144")&&!host.has("tapjoy")&&host.bgm==1,"nonzero returned child Resets then skips main suffix");
    ready(state,host);host.returned=0;host.log.clear();auto entries=state.entries;
    fails([&]{update_magic_shop(state,host);},"zero result is explicit unbound unlock path");
    check(!host.returned&&state.entries==entries&&host.has("find:1"),"zero result consumption follows building query");
    host.returned.reset();host.log.clear();host.pulses.clear();host.repeats={0x40000U,0x10000U};state.page=2;state.page_count=3;
    update_magic_shop(state,host);
    check(state.page==0&&host.has("repeat:262144")&&!host.has("repeat:65536"),"next page has repeat priority");
    host.repeats={0x10000U};state.page=0;update_magic_shop(state,host);check(state.page==2,"previous page wraps");
    host.repeats={0x40000U};state.page=5;state.page_count=0;update_magic_shop(state,host);check(state.page==6,"ARM zero-divisor remainder is defined");
    state.page=std::numeric_limits<std::int32_t>::max();state.page_count=-1;update_magic_shop(state,host);check(state.page==0,"ARM INT_MIN/-1 remainder avoids C++ UB");
    host.repeats.clear();host.pulses={0x100000U,0x02000000U};state.selection=9999;host.log.clear();
    update_magic_shop(state,host);check(host.executed==8&&!host.has("pulse:33554432")&&host.bgm>0,"sentinel confirm executes event8 before buy input");
    state.selection=0;host.pulses={0x100000U};host.log.clear();update_magic_shop(state,host);
    check(!host.has("record:2"),"ordinary row confirmation alone never purchases");
}
void unsupported_and_widths(){
    MagicShopFormState state;Host host;state.kind=6;
    fails([&]{initialize_magic_shop(state,host);},"other shop kind cannot invent a catalog");check(host.log.empty(),"unbound kind fails before callbacks");
    ready(state,host);state.kind=1;state.thread_state=1;
    fails([&]{update_magic_shop(state,host);},"billing thread branch is explicit");check(!host.has("find:1"),"unbound thread fails before ordinary query");
    state.thread_state=0;state.selection=-1;host.pulses={0x02000000U};
    fails([&]{update_magic_shop(state,host);},"invalid buy selection faults instead of silently ignoring input");
    state=MagicShopFormState{};host=Host{};host.height=std::numeric_limits<std::int32_t>::min();initialize_magic_shop(state,host);
    check(state.witch_y==static_cast<float>(2147483492),"Init height subtraction wraps before binary32 conversion");
}
}
int main(){
    reset_and_initialize();reset_failure_prefixes();opening_and_counters();purchase_order_and_live_values();
    purchase_failure_and_refusal();returned_children_paging_and_special_input();unsupported_and_widths();
}
