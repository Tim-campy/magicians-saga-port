#include "core/gameplay/select_form_pass.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
template<class Exception,class Action>void fails(Action action){bool caught=false;try{action();}catch(const Exception&){caught=true;}check(caught,"expected exception");}
EventInstruction resident(std::int32_t id,std::int32_t x,std::int32_t lane,std::int32_t kind=0,std::int32_t clip=38,std::int32_t frame=7){
    return std::make_shared<EventIntegers>(EventIntegers{id,x,lane,0,0,clip,2,1,kind,frame,0});
}
struct Host final:SelectResidentDrawingHost {
    EventScript data=std::make_shared<std::vector<EventInstruction>>();
    std::int32_t width=600,height=640,raw_height=640;
    bool top=true;
    int top_reads=0,list_reads=0,height_reads=0,flag_reads=0;
    std::map<std::int32_t,std::int32_t> flags;
    std::vector<std::array<std::int32_t,5>> draws;
    std::vector<std::array<std::int32_t,3>> touches,levels,attributes;
    std::vector<std::array<std::int32_t,2>> special;
    std::vector<std::int32_t> fonts{10};
    std::vector<std::string> log;
    std::function<void(const std::string&)> action;
    void call(const std::string& name){log.push_back(name);if(action)action(name);}
    std::int32_t game_width()override{call("width");return width;}
    std::int32_t game_height()override{call("height");return height;}
    bool resident_top_form()override{++top_reads;const auto snapshot=top;call("top");return snapshot;}
    EventScript resident_rows()override{++list_reads;call("rows");return data;}
    std::int32_t resident_view_height()override{++height_reads;call("raw");return raw_height;}
    void resident_draw_human(std::int32_t character,std::int32_t clip,std::int32_t frame,std::int32_t x,std::int32_t y)override{
        draws.push_back({character,clip,frame,x,y});call("human");
    }
    std::int32_t resident_character_flags(std::int32_t character)override{++flag_reads;call("flags");return flags[character];}
    std::int32_t resident_arrow_clip()override{call("arrow");return 17;}
    std::int32_t resident_arrow_frames(std::int32_t clip)override{check(clip==17,"arrow selector is forwarded");call("frames");return 5;}
    void resident_push_font(std::int32_t font)override{fonts.push_back(font);call("push"+std::to_string(font));}
    void resident_pop_font()override{check(!fonts.empty(),"font stack underflow");call("pop"+std::to_string(fonts.back()));fonts.pop_back();}
    void resident_touch(std::int32_t kind,std::int32_t x,std::int32_t y)override{touches.push_back({kind,x,y});call("touch"+std::to_string(kind));}
    void resident_label_color()override{call("color");}
    void resident_special_label(std::int32_t x,std::int32_t y)override{special.push_back({x,y});call("special");}
    void resident_level_label(std::int32_t character,std::int32_t x,std::int32_t y)override{levels.push_back({character,x,y});call("level");}
    void resident_attribute_label(std::int32_t character,std::int32_t x,std::int32_t y)override{attributes.push_back({character,x,y});call("attribute");}
};
struct Fixture {
    Host host;
    std::int32_t camera_x=-10,camera_y=-80,view_x=3,view_y=53,frame=36;
    EventScript notices=std::make_shared<std::vector<EventInstruction>>();
    void run(std::int32_t lane=0){draw_select_resident_lane({camera_x,camera_y,view_x,view_y,frame,notices},lane,host);}
    EventInstruction one(std::int32_t kind=0){const auto row=resident(7,20,0,kind);host.data->push_back(row);return row;}
};
void lane_materialization(){
    Fixture f;*f.host.data={resident(1,20,0,0,38,7),resident(2,40,1,0,42,9),resident(3,60,0,0,39,11)};
    f.run();
    check(f.host.draws==std::vector<std::array<std::int32_t,5>>({{1,38,7,13,521},{3,39,11,53,521}}),"lane filter preserves source order and original character/clip/frame cells");
    check(f.host.top_reads==1&&f.host.list_reads==1&&f.host.height_reads==2&&f.host.fonts==EventIntegers({10}),"top and source owners are captured once before per-row height queries");
    Fixture aliases;const auto first=resident(1,20,0),second=resident(2,40,0);
    *aliases.host.data={first,second};aliases.host.action=[&](const auto& name){if(name=="human"&&aliases.host.draws.size()==1){
        aliases.host.data=std::make_shared<std::vector<EventInstruction>>();
        aliases.host.data->push_back(resident(99,500,0));second->at(0)=9;second->at(1)=70;second->at(5)=40;second->at(9)=33;}};
    aliases.run();
    check(aliases.host.draws==std::vector<std::array<std::int32_t,5>>({{1,38,7,13,521},{9,40,33,63,521}}),"materialized array retains row aliases and excludes later source replacements");
    check(aliases.host.list_reads==1,"source replacement does not restart enumeration");
}
void snapshots_and_height_order(){
    Fixture origin;origin.one();origin.host.action=[&](const auto& name){if(name=="top"){
        origin.camera_x=500;origin.camera_y=500;origin.view_x=500;origin.view_y=500;}};
    origin.run();check(origin.host.draws[0]==std::array<std::int32_t,5>{7,38,7,13,521},"camera and view origin values are captured before top-form callback");
    Fixture height;const auto row=resident(7,20,1);height.host.data->push_back(row);
    height.host.action=[&](const auto& name){if(name=="raw"){row->at(1)=999;row->at(2)=2;height.host.raw_height=650;}};
    height.run(1);check(height.host.draws[0]==std::array<std::int32_t,5>{7,38,7,13,549},"row X precedes height callback; row lane follows it, without refiltering");
    Fixture top;top.one(1);top.host.action=[&](const auto& name){if(name=="human")top.host.top=false;};top.run();
    check(top.host.special.size()==1&&top.host.top_reads==1,"label uses the earlier top-form snapshot");
    Fixture frame;frame.one(1);frame.host.action=[&](const auto& name){if(name=="frames")frame.frame=13;};frame.run();
    check(frame.notices->at(0)->at(4)==3,"arrow counter is read after its max-frame callback");
}
void special_rows(){
    for(auto kind:{1,3}){
        Fixture f;f.one(kind);f.run();
        check(f.notices->size()==2&&*f.notices->at(0)==EventIntegers({11,13,497,17,1,0,0}),"special row queues its live animated arrow first");
        const auto offset=kind==1?14:22,clip=kind==1?64:73;
        check(*f.notices->at(1)==EventIntegers({2,13-offset,493,clip,0,0,1}),"store/gift icons retain resource2, offset and insertion depth");
        check(f.host.touches==std::vector<std::array<std::int32_t,3>>({{kind,-3,489}}),"special touch keeps kind and native rectangle origin");
        check(f.host.special==std::vector<std::array<std::int32_t,2>>({{29,497}})&&f.host.fonts==EventIntegers({10}),"special label uses font8, x+16/y-24 and balanced font scope");
        check(f.host.flag_reads==0&&f.host.levels.empty()&&f.host.attributes.empty(),"special rows do not query ordinary character labels");
    }
    Fixture covered;covered.one(1);covered.host.top=false;covered.run();
    check(covered.notices->size()==2&&covered.host.touches.size()==1&&covered.host.special.empty()&&covered.host.fonts==EventIntegers({10}),"covered form suppresses only the special text, retaining indicators and touch");
    Fixture live_kind;const auto row=live_kind.one(1);live_kind.host.action=[&](const auto& name){if(name=="touch1")row->at(8)=3;};live_kind.run();
    check(live_kind.notices->size()==3&&live_kind.notices->at(2)->at(3)==73&&live_kind.host.touches.size()==2,"kind is reread after store touch before the separate gift branch");
    Fixture changed;const auto initially_normal=changed.one();changed.host.action=[&](const auto& name){if(name=="human")initially_normal->at(8)=1;};changed.run();
    check(changed.notices->size()==2,"special-kind decision occurs after human drawing");
    Fixture retained;retained.one(3);retained.notices->push_back(std::make_shared<EventIntegers>(EventIntegers{11,0,0,11,1,0,0}));retained.run();
    check(retained.notices->size()==3&&retained.notices->at(1)->at(6)==1&&retained.notices->at(2)->at(6)==2,"resident notices append after earlier town requests without flushing them");
}
void ordinary_labels(){
    Fixture f;f.one();f.host.flags[7]=4;f.run();
    check(f.host.levels==std::vector<std::array<std::int32_t,3>>({{7,1,483}})&&f.host.attributes==std::vector<std::array<std::int32_t,3>>({{7,1,493}}),"ordinary labels keep captured character, shared X and distinct level/attribute Y");
    const std::vector<std::string> suffix{"flags","push8","color","level","color","attribute","pop8"};
    check(f.host.log.size()>=suffix.size()&&std::equal(suffix.begin(),suffix.end(),f.host.log.end()-static_cast<std::ptrdiff_t>(suffix.size())),"each ordinary text gets its native preceding color call");
    check(f.notices->empty()&&f.host.touches.empty()&&f.host.fonts==EventIntegers({10}),"normal label path adds no indicators or controls");
    Fixture covered;covered.one();covered.host.flags[7]=4;covered.host.top=false;covered.run();
    check(covered.host.levels.empty()&&covered.host.attributes.empty()&&std::count(covered.host.log.begin(),covered.host.log.end(),"color")==2,"covered ordinary row still sets color twice before popping font");
    Fixture alias;const auto row=alias.one();alias.host.flags[7]=4;alias.host.action=[&](const auto& name){if(name=="human")row->at(0)=99;};alias.run();
    check(alias.host.levels.size()==1&&alias.host.levels[0][0]==7,"post-draw character flag/name consumers use captured character ID");
    Fixture unmarked;unmarked.one();unmarked.host.flags[7]=2;unmarked.run();
    check(unmarked.host.levels.empty()&&unmarked.host.attributes.empty()&&unmarked.host.fonts==EventIntegers({10}),"ordinary rows without flag4 have no label scope");
}
void limits_and_failures(){
    Fixture empty;empty.host.data->clear();empty.run();check(empty.host.height_reads==0&&empty.host.top_reads==1&&empty.host.list_reads==1,"empty lane still obtains top-form/source snapshot");
    Fixture null_list;null_list.host.data.reset();fails<std::invalid_argument>([&]{null_list.run();});
    Fixture malformed;malformed.one();malformed.host.data->push_back(nullptr);fails<std::invalid_argument>([&]{malformed.run();});
    check(malformed.host.draws.empty()&&malformed.host.height_reads==0,"whole filter materialization precedes every human draw");
    Fixture short_filter;short_filter.host.data->push_back(std::make_shared<EventIntegers>(EventIntegers{7,0}));
    fails<std::out_of_range>([&]{short_filter.run();});check(short_filter.host.height_reads==0,"short predicate row fails before height/provider work");
    Fixture culled;culled.host.data->push_back(std::make_shared<EventIntegers>(EventIntegers{7,-1000,0}));culled.run();
    check(culled.host.draws.empty()&&culled.host.height_reads==1&&culled.host.flag_reads==0,"culling avoids absent clip/frame/type cells after the raw-height read");
    Fixture visible;visible.host.data->push_back(std::make_shared<EventIntegers>(EventIntegers{7,20,0}));
    fails<std::out_of_range>([&]{visible.run();});check(visible.host.draws.empty()&&visible.host.height_reads==1,"visible short row fails at later sprite inputs");
    Fixture no_notices;no_notices.one(1);no_notices.notices.reset();fails<std::invalid_argument>([&]{no_notices.run();});
    check(no_notices.host.draws.size()==1&&no_notices.host.fonts==EventIntegers({10,8})&&no_notices.host.touches.empty(),"missing notice queue retains human and pushed font before first append failure");
    Fixture label_failure;label_failure.one(3);label_failure.host.action=[&](const auto& name){if(name=="special")throw std::runtime_error("label");};
    fails<std::runtime_error>([&]{label_failure.run();});check(label_failure.notices->size()==2&&label_failure.host.touches.size()==1&&label_failure.host.fonts==EventIntegers({10,8}),"label failure does not pop native font or erase prior indicators");
}
}
int main(){lane_materialization();snapshots_and_height_order();special_rows();ordinary_labels();limits_and_failures();return 0;}
