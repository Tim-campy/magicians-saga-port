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
struct Host final:ShopFormHost {
    ShopFormState* state=nullptr;
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
    std::shared_ptr<EquipmentCombatTable> equipment=std::make_shared<EquipmentCombatTable>();
    std::vector<std::int32_t> equipment_stocks;
    std::array<std::shared_ptr<EquipmentCombatTable>,2> apparel{{std::make_shared<EquipmentCombatTable>(),std::make_shared<EquipmentCombatTable>()}};
    std::array<std::vector<std::int32_t>,2> apparel_stocks,apparel_reads;
    std::vector<std::string> kind_queries;
    std::shared_ptr<EquipmentCombatTable>& catalog_for(std::int32_t kind){
        if(kind==2)return equipment;
        if(kind==3||kind==4)return apparel.at(static_cast<std::size_t>(kind-3));
        throw std::invalid_argument("unbound test catalog kind");
    }
    std::vector<std::int32_t>& stocks_for(std::int32_t kind){
        if(kind==2)return equipment_stocks;
        return apparel_stocks.at(static_cast<std::size_t>(kind-3));
    }
    std::vector<std::int32_t>& reads_for(std::int32_t kind){
        if(kind==2)return equipment_reads;
        return apparel_reads.at(static_cast<std::size_t>(kind-3));
    }
    std::vector<std::array<std::int32_t,3>> equipment_notices;
    std::vector<std::int32_t> equipment_reads;
    std::int32_t equipment_count_override=-1,equipment_null_id=-1,equipment_null_read=-1,tier=2;
    bool null_equipment_source=false;
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
    std::int32_t shop_equipment_catalog_count(std::int32_t kind)override{
        kind_queries.push_back("count:"+std::to_string(kind));note("equipment-count");
        return equipment_count_override==-1?static_cast<std::int32_t>(catalog_for(kind)->size()):equipment_count_override;
    }
    std::shared_ptr<const EquipmentCombatTable> shop_equipment_catalog(std::int32_t kind)override{
        kind_queries.push_back("catalog:"+std::to_string(kind));note("equipment-catalog");
        return null_equipment_source?nullptr:catalog_for(kind);
    }
    bool shop_equipment_owned(std::int32_t kind,const EquipmentCombatRecord& record)override{
        kind_queries.push_back("owned:"+std::to_string(kind)+":"+std::to_string(record.id));
        note("owned:"+std::to_string(record.id));return stocks_for(kind).at(static_cast<std::size_t>(record.id))>0;
    }
    const EquipmentCombatRecord* shop_equipment_record(std::int32_t kind,std::int32_t id)override{
        kind_queries.push_back("record:"+std::to_string(kind)+":"+std::to_string(id));note("equipment-record:"+std::to_string(id));
        auto& reads=reads_for(kind).at(static_cast<std::size_t>(id));++reads;
        if(id==equipment_null_id&&reads==equipment_null_read)return nullptr;
        auto& row=catalog_for(kind)->at(static_cast<std::size_t>(id));return row?&*row:nullptr;
    }
    std::int32_t shop_unlock_value(std::int32_t kind)override{
        kind_queries.push_back("unlock:"+std::to_string(kind));note("unlock:"+std::to_string(kind));return tier;
    }
    void shop_add_equipment_stock(std::int32_t kind,std::int32_t id,std::int32_t amount)override{
        kind_queries.push_back("stock:"+std::to_string(kind)+":"+std::to_string(id));
        note("stock:"+std::to_string(id));auto& stock=stocks_for(kind).at(static_cast<std::size_t>(id));stock=std::min(stock+amount,1);
    }
    EventObject shop_make_equipment_confirmation(std::int32_t kind,std::int32_t id,std::int32_t option)override{
        kind_queries.push_back("notice:"+std::to_string(kind)+":"+std::to_string(id));
        note("make-equipment:"+std::to_string(kind)+":"+std::to_string(id)+":"+std::to_string(option));
        equipment_notices.push_back({kind,id,option});return child;
    }
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
void ready(ShopFormState& state,Host& host){
    state.state=1;state.crossfade=-1;state.selection=0;state.page_count=1;
    state.entries=std::make_shared<std::vector<std::int32_t>>(std::initializer_list<std::int32_t>{2});
    state.filters=std::make_shared<std::vector<std::uint8_t>>(3,0);state.member_system=host.member;host.state=&state;
}
void reset_and_initialize(){
    ShopFormState state;Host host;host.state=&state;
    state.page=99;state.page_count=42;state.selected_item=8;state.state=9;state.frame=87;
    state.entries=std::make_shared<std::vector<std::int32_t>>(1,44);auto old_entries=state.entries;
    state.filters=std::make_shared<std::vector<std::uint8_t>>(1,7);auto old_filters=state.filters;
    host.on_call=[&](const std::string& name){
        if(name=="labels:0")check(state.state==9&&state.frame==87,"Init labels precede stores");
        if(name=="flag:256")check(state.state==0&&state.frame==0,"FlagOn follows production stores");
    };
    initialize_shop_form(state,host);
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

    host.on_call={};host.records.clear();host.found.clear();state.page=-4;reset_shop_form(state,host);
    check(state.entries->empty()&&state.filters->empty()&&state.page_count==1&&state.page==-4,"empty catalog and negative page retained");
    host.records.resize(13,MagicShopCombatRecord{});host.found.assign(13,true);state.page=99;reset_shop_form(state,host);
    check(state.page_count==2&&state.page==2,"thirteen rows form two pages with inclusive upper clamp");
}
void reset_failure_prefixes(){
    ShopFormState state;Host host;host.state=&state;state.page=7;state.page_count=88;
    state.filters=std::make_shared<std::vector<std::uint8_t>>(1,9);auto old_filter=state.filters;
    host.fail_at="found:2";fails([&]{reset_shop_form(state,host);},"late catalog error must fail");
    check(*state.entries==std::vector<std::int32_t>({0})&&state.filters!=old_filter,"Reset retains partial publication");
    check(state.selection==-1&&state.crossfade==-1&&state.page==7&&state.page_count==88,"failure retains earlier stores only");
    host.fail_at.clear();host.records[1].reset();
    fails([&]{reset_shop_form(state,host);},"null slot is not silently skipped");
    check(*state.entries==std::vector<std::int32_t>({0}),"null slot retains preceding append");
    host.records[1]=MagicShopCombatRecord{};
    host.on_call=[&](const std::string& name){if(name=="found:0")state.entries=std::make_shared<std::vector<std::int32_t>>(1,90);};
    reset_shop_form(state,host);
    check(*state.entries==std::vector<std::int32_t>({90,0,2}),"append reloads the published list after callback");
    host.on_call=[&](const std::string& name){if(name=="buildings")state.entries.reset();};
    fails([&]{reset_shop_form(state,host);},"CheckFilter requires list after town query");
    host.on_call={};host.count_override=-2;old_filter=state.filters;
    fails([&]{reset_shop_form(state,host);},"negative allocation length fails");
    check(state.entries&&state.entries->empty()&&state.filters==old_filter,"allocation failure preserves old filter");
}
void opening_and_counters(){
    ShopFormState state;Host host;host.state=&state;initialize_shop_form(state,host);host.log.clear();
    for(int i=0;i<60;++i)check(update_shop_form(state,host),"ordinary Update true");
    check(state.witch_x==60&&state.crossfade==-1&&state.state==0&&state.loop_counter==60,"arrival waits without confirm before61");
    check(host.image_resets==1&&host.images==60&&host.blurs==60&&host.bgm==60,"opening prelude clocks and BGM");
    update_shop_form(state,host);check(state.crossfade==0&&state.loop_counter==61,"opening auto-starts crossfade at61");
    const auto before_bgm=host.bgm;
    for(int i=0;i<20;++i)update_shop_form(state,host);
    check(state.crossfade==20&&state.state==0&&host.bgm==before_bgm,"twenty crossfade passes skip BGM and main");
    update_shop_form(state,host);
    check(state.crossfade==-1&&state.state==1&&state.frame==0&&state.loop_counter==82,"crossfade21 publishes main and clears frame");
    check(host.occurrences("labels:1")==1&&host.bgm==before_bgm,"change-state pass still skips BGM");
    update_shop_form(state,host);check(host.has("find:1")&&host.bgm==before_bgm+1,"main starts on next pass");

    ShopFormState fast;Host quick;quick.state=&fast;initialize_shop_form(fast,quick);quick.pulses={0x100000U};
    for(int i=0;i<20;++i)update_shop_form(fast,quick);
    check(fast.crossfade==-1,"pulse does not bypass witch arrival");
    update_shop_form(fast,quick);check(fast.crossfade==0&&fast.loop_counter==21,"arrival pulse starts earlier crossfade");
    quick.labels={3};update_shop_form(fast,quick);check(quick.transitions==0,"crossfade ignores Back");

    ShopFormState edge;Host numeric;ready(edge,numeric);edge.loop_counter=2147483646;edge.frame=2147483647;
    update_shop_form(edge,numeric);
    check(edge.loop_counter==0&&edge.frame==std::numeric_limits<std::int32_t>::min(),"loop modulus and frame wrap");
    update_shop_form(edge,numeric);check(numeric.image_resets==1,"wrapped zero resets image clocks next pass");
    edge.state=0;edge.arrival_x=std::numeric_limits<float>::quiet_NaN();edge.witch_x=0;numeric.log.clear();
    update_shop_form(edge,numeric);check(std::isnan(edge.witch_x)&&!numeric.has("pulse:1048576"),"unordered arrival does not acknowledge");
}
void purchase_order_and_live_values(){
    ShopFormState state;Host host;ready(state,host);host.prices={100,130};host.pulses={0x02000000U,0x10000000U};host.labels={12,21,3};
    host.on_call=[&](const std::string& name){
        if(name=="check:100")state.entries=std::make_shared<std::vector<std::int32_t>>(1,0);
        if(name=="push")state.selection=1;
    };
    update_shop_form(state,host);
    check(host.money==870&&host.purchases[2]==1&&host.purchases[0]==0,"payment uses fresh second price and captured catalog slot");
    check(host.occurrences("record:2")==5&&host.price_cursor==2,"each native record lookup remains fresh");
    check(host.notices.size()==1,"purchase creates notice once");const auto notice=host.notices.front();
    check(notice.kind==1&&notice.character_index==0&&notice.parameter_key==3&&notice.add_value==7&&notice.total==810,"notice arguments follow incremented purchase state");
    check(state.selection==1&&*state.entries==std::vector<std::int32_t>({0,2}),"Reset preserves post-Push selection");
    const auto make=std::find(host.log.begin(),host.log.end(),"make"),debit=std::find(host.log.begin(),host.log.end(),"subtract:130");
    check(debit<make&&host.log.back()=="town","payment precedes child and outer Back still runs");
    check(host.has("tapjoy")&&host.has("browser")&&host.has("input")&&host.bgm==1&&host.backs==1,"same-pass optional inputs survive purchase");
    host.pulses.clear();host.transition_result=false;
    check(!update_shop_form(state,host),"Back returns the transition result");
    check(host.log[0]=="base"&&host.log[1]=="reset-images"&&state.update_pair==std::array<std::int32_t,2>{{1,0}},"Update field pair precedes prelude");
}
void purchase_failure_and_refusal(){
    ShopFormState state;Host host;ready(state,host);host.money=20;host.pulses={0x02000000U};auto entries=state.entries;
    update_shop_form(state,host);
    check(host.money==20&&host.price_cursor==1&&host.purchases[2]==0&&host.notices.empty(),"insufficient funds do not debit or push");
    check(state.entries!=entries&&state.selection==0&&host.has("pulse:268435456")&&host.bgm==1,"refused buy still Resets and runs suffix");
    host.money=1000;host.price_cursor=0;host.prices={-7};host.log.clear();state.entries=std::make_shared<std::vector<std::int32_t>>(1,2);
    update_shop_form(state,host);check(host.money==1007&&host.has("check:-7"),"negative price keeps sign extension and subtraction");

    ShopFormState partial;Host failed;ready(partial,failed);failed.pulses={0x02000000U};failed.fail_at="make";entries=partial.entries;
    fails([&]{update_shop_form(partial,failed);},"notice failure must propagate");
    check(failed.money==900&&failed.purchases[2]==1&&partial.entries==entries&&partial.selection==0,"notice failure preserves paid/incremented prefix without Reset");
    check(!failed.has("push")&&!failed.has("bgm"),"exception prevents later child and suffix");
    failed.fail_at.clear();failed.pulses={0x02000000U};partial.member_system.reset();entries=partial.entries;failed.log.clear();
    fails([&]{update_shop_form(partial,failed);},"missing MemberSystem fails after count/key and record query");
    check(failed.money==800&&failed.purchases[2]==2&&failed.has("parameter:12")&&failed.occurrences("record:2")==5&&!failed.has("total:3"),"null member failure order");
    check(partial.entries==entries,"late member failure does not Reset");
}
void returned_children_paging_and_special_input(){
    ShopFormState state;Host host;ready(state,host);host.returned=-1;host.pulses={0x10000000U};
    update_shop_form(state,host);
    check(!host.returned&&state.selection==-1&&!host.has("repeat:262144")&&!host.has("tapjoy")&&host.bgm==1,"nonzero returned child Resets then skips main suffix");
    ready(state,host);host.returned=0;host.log.clear();auto entries=state.entries;
    fails([&]{update_shop_form(state,host);},"zero result is explicit unbound unlock path");
    check(!host.returned&&state.entries==entries&&host.has("find:1"),"zero result consumption follows building query");
    host.returned.reset();host.log.clear();host.pulses.clear();host.repeats={0x40000U,0x10000U};state.page=2;state.page_count=3;
    update_shop_form(state,host);
    check(state.page==0&&host.has("repeat:262144")&&!host.has("repeat:65536"),"next page has repeat priority");
    host.repeats={0x10000U};state.page=0;update_shop_form(state,host);check(state.page==2,"previous page wraps");
    host.repeats={0x40000U};state.page=5;state.page_count=0;update_shop_form(state,host);check(state.page==6,"ARM zero-divisor remainder is defined");
    state.page=std::numeric_limits<std::int32_t>::max();state.page_count=-1;update_shop_form(state,host);check(state.page==0,"ARM INT_MIN/-1 remainder avoids C++ UB");
    host.repeats.clear();host.pulses={0x100000U,0x02000000U};state.selection=9999;host.log.clear();
    update_shop_form(state,host);check(host.executed==8&&!host.has("pulse:33554432")&&host.bgm>0,"sentinel confirm executes event8 before buy input");
    state.selection=0;host.pulses={0x100000U};host.log.clear();update_shop_form(state,host);
    check(!host.has("record:2"),"ordinary row confirmation alone never purchases");
}
void equipment_fixture(ShopFormState& state,Host& host){
    state.kind=2;host.state=&state;host.equipment->resize(7);
    host.equipment_stocks.assign(7,0);host.equipment_reads.assign(7,0);host.equipment_stocks[3]=1;
    const auto row=[&](std::int32_t id,std::int32_t unlock,std::int32_t price,std::int32_t flags){
        EquipmentCombatRecord item;item.id=id;item.unlock_level=unlock;item.buy_price=price;item.flags=flags;
        host.equipment->at(static_cast<std::size_t>(id))=item;
    };
    row(0,9,100,0x10);row(1,1,40,0x10);row(3,1,80,0x10);
    row(4,0,-10,0);row(5,2,300,0x10);row(6,0,10,0x10);
}
void equipment_reset_and_filter(){
    ShopFormState state;Host host;equipment_fixture(state,host);host.money=60;
    initialize_shop_form(state,host);
    check(*state.entries==std::vector<std::int32_t>({6,1,5,0}),"equipment Reset stable-sorts tiers, excludes owned and uses row IDs");
    check(state.filters->size()==7&&state.filters->at(5)==1&&state.filters->at(0)==0&&state.filters->at(1)==0,"equipment filter combines unlock and insufficient funds");
    check(host.has("check:100"),"locked equipment still queries CheckMoney");
    check(!host.has("count")&&!host.has("found:0")&&host.occurrences("equipment-count")==1&&host.occurrences("equipment-catalog")==1,"equipment length and source queried separately without magic discovery");
    check(!host.has("owned:4")&&!host.has("owned:2")&&host.has("owned:3"),"availability flag precedes owned predicate and nulls are removed");
    host.equipment_stocks[3]=0;reset_shop_form(state,host);
    check(*state.entries==std::vector<std::int32_t>({6,1,3,5,0}),"equal unlock tiers retain catalog order");

    ShopFormState snapshot;Host live;equipment_fixture(snapshot,live);live.money=60;
    auto original=live.equipment;
    live.on_call=[&](const std::string& name){if(name=="owned:6"){
        live.equipment=std::make_shared<EquipmentCombatTable>(*original);
        live.equipment->at(1)->flags=0;live.equipment->at(1)->buy_price=500;
    }};
    reset_shop_form(snapshot,live);
    check(*snapshot.entries==std::vector<std::int32_t>({6,1,5,0})&&snapshot.filters->at(1)==1,"Reset retains captured source while CheckFilter rereads current records");

    auto target=state.filters;
    host.on_call=[&](const std::string& name){if(name=="check:100"){
        target=state.filters;state.filters=std::make_shared<std::vector<std::uint8_t>>(7,9);
    }};
    state.state=1;state.crossfade=-1;host.log.clear();update_shop_form(state,host);
    check(target->at(0)==0&&*state.filters==std::vector<std::uint8_t>(7,9),"filter target is captured before CheckMoney");
    host.on_call={};host.money=0;host.tier=100;update_shop_form(state,host);
    check(state.filters->at(6)==1&&state.filters->at(1)==1&&state.filters->at(0)==1,"live money and unlock values refresh filters");
}
void equipment_reset_failure_prefixes(){
    ShopFormState state;Host host;equipment_fixture(state,host);
    state.entries=std::make_shared<std::vector<std::int32_t>>(1,90);auto old_entries=state.entries;
    state.filters=std::make_shared<std::vector<std::uint8_t>>(1,9);auto old_filters=state.filters;
    host.null_equipment_source=true;
    fails([&]{reset_shop_form(state,host);},"null captured equipment source fails");
    check(state.entries!=old_entries&&state.entries->empty()&&state.filters!=old_filters&&state.filters->size()==7,"null source follows fresh vector and filter publication");
    host.null_equipment_source=false;host.equipment_count_override=2;host.log.clear();
    fails([&]{reset_shop_form(state,host);},"initial equipment length can differ from captured source length");
    check(*state.entries==std::vector<std::int32_t>({6,1,5,0})&&state.filters->size()==2&&host.has("check:10"),"filter bounds failure occurs after source materialization and money query");
    host.equipment_count_override=-1;host.equipment_null_id=1;host.equipment_null_read=2;
    host.equipment_reads.assign(7,0);host.log.clear();
    fails([&]{reset_shop_form(state,host);},"fresh second equipment record is required in CheckFilter");
    check(host.has("check:10")&&!host.has("check:40")&&host.occurrences("unlock:2")==1,"unlock and price record query failure preserves earlier filter pass");
    host.equipment_null_id=-1;host.equipment_count_override=-2;old_filters=state.filters;
    fails([&]{reset_shop_form(state,host);},"negative equipment allocation length fails");
    check(state.entries->empty()&&state.filters==old_filters,"invalid first length preserves prior filter reference");
}
void equipment_purchase_order(){
    ShopFormState state;Host host;equipment_fixture(state,host);reset_shop_form(state,host);
    state.state=1;state.crossfade=-1;state.selection=1;host.log.clear();host.equipment_reads.assign(7,0);host.pulses={0x02000000U};
    bool buy=false;
    host.on_call=[&](const std::string& name){
        if(name=="pulse:33554432")buy=true;
        if(name=="check:40"&&buy)host.equipment->at(1)->buy_price=70;
        if(name=="push")state.selection=99;
    };
    update_shop_form(state,host);
    check(host.money==930&&host.equipment_stocks[1]==1&&host.notices.empty(),"equipment buy rereads cost and increments canonical stock");
    check(host.equipment_notices==std::vector<std::array<std::int32_t,3>>({{2,1,0}}),"equipment notice captures kind, ID and option0");
    check(state.selection==-1&&*state.entries==std::vector<std::int32_t>({6,5,0}),"equipment Reset clears post-Push selection and removes owned row");
    auto begin=std::find(host.log.begin(),host.log.end(),"pulse:33554432"),end=std::find(begin,host.log.end(),"equipment-count");
    check(begin!=host.log.end()&&end!=host.log.end()&&std::vector<std::string>(begin,end)==std::vector<std::string>({
        "pulse:33554432","equipment-record:1","check:40","equipment-record:1","subtract:70",
        "equipment-record:1","stock:1","make-equipment:2:1:0","push"}),"equipment purchase callback order");
    check(host.bgm==1&&host.has("pulse:268435456"),"equipment buy retains ordinary suffix");
    host.on_call={};host.returned=-1;host.log.clear();const auto money=host.money;
    update_shop_form(state,host);
    check(host.money==money&&state.selection==-1&&!host.has("pulse:33554432"),"returning equipment notice Resets before another buy pulse");

    ShopFormState locked;Host direct;equipment_fixture(locked,direct);reset_shop_form(locked,direct);
    locked.state=1;locked.crossfade=-1;locked.selection=3;direct.tier=0;direct.pulses={0x02000000U};
    update_shop_form(locked,direct);
    check(direct.equipment_stocks[0]==1&&direct.money==900,"controller does not invent a second tier gate beyond native UI selection");
}
void equipment_purchase_failure_and_refusal(){
    ShopFormState state;Host host;equipment_fixture(state,host);host.money=20;reset_shop_form(state,host);
    state.state=1;state.crossfade=-1;state.selection=1;host.pulses={0x02000000U};
    update_shop_form(state,host);
    check(host.money==20&&host.equipment_stocks[1]==0&&host.equipment_notices.empty()&&state.selection==-1,"refused equipment buy does not pay, add or notify and clears selection");

    ShopFormState signed_price;Host signed_host;equipment_fixture(signed_price,signed_host);
    signed_host.equipment->at(1)->buy_price=std::numeric_limits<std::int32_t>::min();signed_host.money=0;
    reset_shop_form(signed_price,signed_host);signed_price.state=1;signed_price.crossfade=-1;signed_price.selection=1;
    signed_host.pulses={0x02000000U};update_shop_form(signed_price,signed_host);
    check(signed_host.has("check:-2147483648")&&signed_host.money==2147483648LL&&signed_host.equipment_stocks[1]==1,
        "equipment cost keeps signed int32 through int64 check and subtraction");

    ShopFormState partial;Host failed;equipment_fixture(partial,failed);reset_shop_form(partial,failed);
    partial.state=1;partial.crossfade=-1;partial.selection=1;failed.pulses={0x02000000U};failed.fail_at="make-equipment:2:1:0";
    auto entries=partial.entries;
    fails([&]{update_shop_form(partial,failed);},"equipment notice failure propagates");
    check(failed.money==960&&failed.equipment_stocks[1]==1&&partial.entries==entries&&partial.selection==1,"equipment notice failure preserves paid/stock prefix without Reset");
    check(!failed.has("push")&&failed.bgm==0,"failed equipment notice prevents suffix");

    ShopFormState missing;Host fresh;equipment_fixture(missing,fresh);reset_shop_form(missing,fresh);
    missing.state=1;missing.crossfade=-1;missing.selection=1;fresh.pulses={0x02000000U};
    fresh.equipment_reads.assign(7,0);fresh.equipment_null_id=1;fresh.equipment_null_read=5;entries=missing.entries;
    fails([&]{update_shop_form(missing,fresh);},"equipment AddStock requires a fresh third purchase record");
    check(fresh.money==960&&fresh.equipment_stocks[1]==0&&missing.entries==entries&&!fresh.has("stock:1"),"third record failure retains payment before stock");
}

void apparel_fixture(ShopFormState& state,Host& host,std::int32_t kind){
    equipment_fixture(state,host);state.kind=kind;
    for(const auto category:{3,4}){
        host.catalog_for(category)->resize(5);host.stocks_for(category).assign(5,0);host.reads_for(category).assign(5,0);
    }
    const auto put=[&](std::int32_t category,std::int32_t id,std::int32_t unlock,std::int32_t price,std::int32_t flags){
        EquipmentCombatRecord item;item.id=id;item.unlock_level=unlock;item.buy_price=price;item.flags=flags;
        host.catalog_for(category)->at(static_cast<std::size_t>(id))=item;
    };
    put(3,0,0,5,0x10);put(3,1,2,7,0x4);put(3,2,1,20,0x4);put(3,4,2,30,0x4);
    put(4,0,0,9,0x10);put(4,1,2,12,0x4);put(4,2,1,10,0x4);put(4,4,2,3,0x4);
    host.stocks_for(3)[4]=1;host.stocks_for(4)[2]=1;
}
void apparel_reset_and_same_id_routing(){
    for(const auto kind:{3,4}){
        ShopFormState state;Host host;apparel_fixture(state,host,kind);host.money=10;host.tier=1;
        reset_shop_form(state,host);
        const auto expected=kind==3?std::vector<std::int32_t>{2,1}:std::vector<std::int32_t>{1,4};
        check(*state.entries==expected,"apparel uses mask4, signed tier order and its own owned state");
        check(!host.has("owned:0"),"apparel does not admit equipment-mask16 rows");
        check(host.has("unlock:"+std::to_string(kind)),"apparel filter asks the current shop category tier");
        check(host.has(kind==3?"check:7":"check:12"),"locked apparel still queries money");
        check((kind==3&&state.filters->at(2)==1&&state.filters->at(1)==0)
            ||(kind==4&&state.filters->at(1)==0&&state.filters->at(4)==0),"apparel filters combine unlock and affordability");
        check(!host.kind_queries.empty()&&host.kind_queries[0]=="count:"+std::to_string(kind)
            &&host.kind_queries[1]=="catalog:"+std::to_string(kind),"apparel allocation/source callbacks route the selected category");
        check(std::none_of(host.kind_queries.begin(),host.kind_queries.end(),[](const auto& item){return item=="catalog:2"||item=="count:2";}),"apparel does not borrow equipment catalog by matching IDs");
    }
    ShopFormState hat;Host host;apparel_fixture(hat,host,3);host.stocks_for(3)[4]=0;
    reset_shop_form(hat,host);check(*hat.entries==std::vector<std::int32_t>({2,1,4}),"equal apparel tiers preserve source order");
    hat.state=1;hat.crossfade=-1;hat.selection=1;host.money=37;host.pulses={0x02000000U};host.kind_queries.clear();
    bool buy=false;host.on_call=[&](const std::string& name){
        if(name=="pulse:33554432")buy=true;
        if(buy&&name=="check:7")host.catalog_for(3)->at(1)->buy_price=9;
    };
    update_shop_form(hat,host);
    check(host.money==28&&host.stocks_for(3)[1]==1&&host.stocks_for(4)[1]==0&&host.equipment_stocks[1]==0,"hat purchase rechecks Hat price and changes Hat stock only");
    check(hat.selection==-1&&host.equipment_notices==std::vector<std::array<std::int32_t,3>>({{3,1,0}}),"hat purchase makes kind3 notice and clears selection");
    check(!host.has("total:3")&&host.notices.empty(),"apparel purchase does not query or upgrade hero parameters");
}
void cloth_check_and_cross_catalog_debit(){
    ShopFormState state;Host host;apparel_fixture(state,host,4);host.money=37;host.tier=5;
    host.catalog_for(3)->at(1)->buy_price=50;reset_shop_form(state,host);
    state.state=1;state.crossfade=-1;state.selection=0;host.pulses={0x02000000U};host.log.clear();host.kind_queries.clear();
    host.on_call=[&](const std::string& name){if(name=="pulse:33554432")host.kind_queries.clear();};
    update_shop_form(state,host);
    check(host.money==0&&host.stocks_for(4)[1]==1&&host.stocks_for(3)[1]==0&&host.equipment_stocks[1]==0,"Cloth checks12 but debits Hat50, floors37 to0, and adds only Cloth stock");
    check(host.has("check:12")&&!host.has("check:50")&&host.has("subtract:50"),"Hat debit is not substituted into the earlier affordability check");
    const std::vector<std::string> prefix{"record:4:1","record:3:1","record:4:1","stock:4:1","notice:4:1","count:4"};
    check(host.kind_queries.size()>=prefix.size()&&std::equal(prefix.begin(),prefix.end(),host.kind_queries.begin()),"Cloth purchase queries preserve cross-catalog native order");
    check(state.selection==-1&&*state.entries==std::vector<std::int32_t>({4}),"Cloth Reset removes newly owned item without changing Hat ownership");
    check(host.equipment_notices==std::vector<std::array<std::int32_t,3>>({{4,1,0}})&&host.notices.empty(),"Cloth receives its own purchase notice rather than magic effects");
}
void cloth_failure_prefix_and_dispatch_capture(){
    ShopFormState missing;Host host;apparel_fixture(missing,host,4);host.money=37;host.tier=5;
    host.catalog_for(3)->at(1).reset();reset_shop_form(missing,host);
    missing.state=1;missing.crossfade=-1;missing.selection=0;host.pulses={0x02000000U};auto entries=missing.entries;
    host.on_call=[&](const std::string& name){if(name=="pulse:33554432")host.kind_queries.clear();};
    fails([&]{update_shop_form(missing,host);},"Cloth buy requires same-ID Hat record for debit");
    check(host.money==37&&host.stocks_for(4)[1]==0&&missing.entries==entries&&missing.selection==0,"missing Hat fails after Cloth check before debit/stock/Reset");
    check(host.kind_queries==std::vector<std::string>({"record:4:1","record:3:1"})&&!host.has("subtract:12")&&!host.has("stock:1"),"missing Hat failure retains exact provider prefix");

    ShopFormState refused;Host poor;apparel_fixture(refused,poor,4);poor.money=5;poor.catalog_for(3)->at(1).reset();reset_shop_form(refused,poor);
    refused.state=1;refused.crossfade=-1;refused.selection=0;poor.pulses={0x02000000U};poor.kind_queries.clear();
    update_shop_form(refused,poor);
    check(poor.money==5&&poor.stocks_for(4)[1]==0&&refused.selection==-1&&poor.equipment_notices.empty(),"refused Cloth purchase clears selection without touching Hat debit source");
    check(std::none_of(poor.kind_queries.begin(),poor.kind_queries.end(),[](const auto& value){return value=="record:3:1";}),"failed Cloth check never resolves missing Hat record");

    ShopFormState captured;Host live;apparel_fixture(captured,live,4);live.money=37;live.tier=5;
    live.catalog_for(3)->at(1)->buy_price=50;reset_shop_form(captured,live);
    captured.state=1;captured.crossfade=-1;captured.selection=0;live.pulses={0x02000000U};bool buy=false;
    live.on_call=[&](const std::string& name){
        if(name=="pulse:33554432")buy=true;
        if(name=="check:12"&&buy)captured.kind=3;
    };
    update_shop_form(captured,live);
    check(live.money==0&&live.stocks_for(4)[1]==1&&live.stocks_for(3)[1]==0,"purchase dispatch kind survives affordability callback changes");
    check(live.equipment_notices==std::vector<std::array<std::int32_t,3>>({{3,1,0}}),"purchase factory rereads the live kind after stock");
    check(captured.kind==3&&captured.selection==-1&&*captured.entries==std::vector<std::int32_t>({2,1}),"plain Reset observes the changed live category");
}

void unsupported_and_widths(){
    ShopFormState state;Host host;state.kind=6;
    fails([&]{initialize_shop_form(state,host);},"other shop kind cannot invent a catalog");check(host.log.empty(),"unbound kind fails before callbacks");
    ready(state,host);state.kind=1;state.thread_state=1;
    fails([&]{update_shop_form(state,host);},"billing thread branch is explicit");check(!host.has("find:1"),"unbound thread fails before ordinary query");
    state.thread_state=0;state.selection=-1;host.pulses={0x02000000U};
    fails([&]{update_shop_form(state,host);},"invalid buy selection faults instead of silently ignoring input");
    state=ShopFormState{};host=Host{};host.height=std::numeric_limits<std::int32_t>::min();initialize_shop_form(state,host);
    check(state.witch_y==static_cast<float>(2147483492),"Init height subtraction wraps before binary32 conversion");
}
}
int main(){
    reset_and_initialize();reset_failure_prefixes();opening_and_counters();purchase_order_and_live_values();
    purchase_failure_and_refusal();returned_children_paging_and_special_input();unsupported_and_widths();
    equipment_reset_and_filter();equipment_reset_failure_prefixes();equipment_purchase_order();equipment_purchase_failure_and_refusal();
    apparel_reset_and_same_id_routing();cloth_check_and_cross_catalog_debit();cloth_failure_prefix_and_dispatch_capture();
}
