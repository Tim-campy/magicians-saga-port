#include "core/gameplay/select_form_pass.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
template<class Exception,class Action>void fails(Action action){bool failed=false;try{action();}catch(const Exception&){failed=true;}check(failed,"expected exception");}
FormText text(const char* value){return std::make_shared<const std::string>(value);}
std::shared_ptr<SelectAreaDrawingRecord> record(std::int32_t image,std::int32_t discovery,const char* name){
    return std::make_shared<SelectAreaDrawingRecord>(SelectAreaDrawingRecord{image,discovery,text(name)});
}
struct Draw {
    std::int32_t resource,clip;
    std::optional<std::int32_t> frame;
    std::int32_t x,y,font;
};
struct Host final:SelectAreaDrawingHost {
    std::shared_ptr<SelectAreaDrawingRows> rows=std::make_shared<SelectAreaDrawingRows>();
    std::vector<Draw> draws;
    std::vector<std::array<std::int32_t,5>> touches; // index, selected, locked, x, y.
    std::vector<std::array<std::int32_t,2>> labels;
    std::vector<FormText> names;
    std::vector<std::string> log;
    std::vector<std::int32_t> fonts;
    std::deque<std::int32_t> numbers;
    std::function<void(const std::string&)> action;
    std::int32_t width=600,height=640,raw_height=640,number=0;
    std::int32_t catalog_reads=0,height_reads=0,number_reads=0,colors=0,clip_reads=0;
    Host(){rows->push_back(record(7,1,"First"));rows->push_back(record(8,0,"Second"));}
    void call(const std::string& name){log.push_back(name);if(action)action(name);}
    std::shared_ptr<SelectAreaDrawingRows> drawing_areas()override{++catalog_reads;call("areas");return rows;}
    std::int32_t game_width()override{call("width");return width;}
    std::int32_t game_height()override{call("height");return height;}
    std::int32_t area_view_height()override{++height_reads;call("raw");return raw_height;}
    void area_push_font(std::int32_t font)override{fonts.push_back(font);call("font");}
    void area_draw(std::int32_t resource,std::int32_t clip,std::optional<std::int32_t> frame,std::int32_t x,std::int32_t y)override{
        check(!fonts.empty(),"area drawing keeps its font scope");draws.push_back({resource,clip,frame,x,y,fonts.back()});call("draw");
    }
    std::int32_t area_clip(std::int32_t field)override{
        ++clip_reads;call("clip");
        switch(field){case 0x18c:return 10;case 0x188:return 9;case 0x194:return 12;case 0x1a8:return 17;}
        throw std::logic_error("unexpected area clip selector");
    }
    std::int32_t area_clip_frames(std::int32_t resource,std::int32_t clip)override{
        call("frames");if(resource==2&&clip==15)return 8;
        if(resource==11&&clip==9)return 7;if(resource==11&&clip==17)return 5;
        throw std::logic_error("unexpected animated area notice");
    }
    std::int32_t area_number()override{
        ++number_reads;call("number");if(numbers.empty())return number;
        const auto result=numbers.front();numbers.pop_front();return result;
    }
    void area_label_color()override{++colors;call("color");}
    void area_label(FormText value,std::int32_t x,std::int32_t y)override{names.push_back(std::move(value));labels.push_back({x,y});call("label");}
    void area_touch(std::int32_t index,bool selected,bool locked,std::int32_t x,std::int32_t y)override{
        touches.push_back({index,selected?1:0,locked?1:0,x,y});call("touch");
    }
};
struct Fixture {
    Host host;
    std::int32_t frame=0,selected=-1,state=0;
    EventInstruction marks=std::make_shared<EventIntegers>(2,0);
    EventScript notices=std::make_shared<std::vector<EventInstruction>>();
    void run(bool top=true,SelectBackgroundOrigin origin={449,-27}){
        draw_select_town_areas({frame,selected,state,top,marks,notices},origin,host);
    }
};
void positions(){
    check(select_area_position(0,640)==std::array<std::int32_t,2>{-160,483},"first area position");
    check(select_area_position(1,320)==std::array<std::int32_t,2>{-289,163},"area spacing and raw height");
    check(select_area_position(-1,640)==std::array<std::int32_t,2>{-31,483},"negative index uses wrapped arithmetic");
    const auto min=std::numeric_limits<std::int32_t>::min(),max=std::numeric_limits<std::int32_t>::max();
    check(select_area_position(max,min)==std::array<std::int32_t,2>{max-30,max-156},"area multiplication and height subtraction wrap32");
    check(select_area_position(min,max)==std::array<std::int32_t,2>{max-159,max-157},"negative product and X subtraction wrap32");
}
void first_frame(){
    Fixture f;const auto name=f.host.rows->at(0)->name;f.run();
    check(f.host.log.size()>2&&f.host.log[0]=="raw"&&f.host.log[1]=="font","entry raw height precedes font10 and enumeration");
    check(f.host.fonts==EventIntegers({10}),"area prefix leaves font10 active");
    check(f.host.draws.size()==2,"one discovered area and next fence draw");
    const auto& map=f.host.draws[0];
    check(map.resource==0&&map.clip==3&&map.frame==7&&map.x==307&&map.y==433&&map.font==10,"area image selects bg0 clip3 explicit frame and native offset");
    const auto& fence=f.host.draws[1];
    check(fence.resource==11&&fence.clip==12&&!fence.frame&&fence.x==160&&fence.y==456,"next fence uses select current clip frame without counter substitution");
    check(f.notices->size()==1&&*f.notices->at(0)==EventIntegers({11,353,451,10,0,0,0}),"unselected area name board stays queued with depth0");
    check(f.host.colors==1&&f.host.names==std::vector<FormText>({name})&&f.host.labels==std::vector<std::array<std::int32_t,2>>({{353,457}}),"name aliases captured table row and uses y+1");
    check(f.host.touches==std::vector<std::array<std::int32_t,5>>({{0,0,0,289,456},{1,0,1,160,456}}),"discovered and locked area touches retain their distinct profiles");
    check(f.host.number_reads==2,"unfound branch queries area number twice");
}
void selection_and_markers(){
    Fixture f;f.selected=0;f.frame=36;f.marks->at(0)=1;f.run();
    check(f.notices->size()==4,"selected board, go button and two marker requests");
    check(*f.notices->at(0)==EventIntegers({11,353,451,10,1,0,0}),"selected board frame1");
    check(*f.notices->at(1)==EventIntegers({11,353,528,9,6,0,1}),"go button uses counter/6 before clip remainder");
    check(*f.notices->at(2)==EventIntegers({2,339,432,15,1,0,2}),"common marker uses counter/4 and follows go request");
    check(*f.notices->at(3)==EventIntegers({11,353,428,17,1,0,3}),"arrow uses live counter modulo max and final depth");
    check(f.host.touches[0]==std::array<std::int32_t,5>{0,1,0,339,501},"selected area touch uses x+50,y+45");
    Fixture covered;covered.frame=-36;covered.marks->at(0)=1;covered.run(false);
    check(covered.host.labels.empty()&&covered.host.colors==1&&covered.notices->size()==3,"covered form suppresses only label, retaining color and markers");
    check(covered.notices->at(1)->at(4)==-1&&covered.notices->at(2)->at(4)==-1,"negative animation frames retain signed remainder");
    for(auto mark:{-1,0,2,3}){Fixture other;other.marks->at(0)=mark;other.run();check(other.notices->size()==1,"only mark1 queues indicators");}
    Fixture retained;queue_select_notice(retained.notices,11,1,2,11,1,0);retained.run();
    check(retained.notices->size()==2&&retained.notices->at(0)->at(1)==1&&retained.notices->at(1)->at(6)==1,"earlier building notices are retained and precede area notices");
}
void unfound_gates(){
    Fixture changed;changed.host.numbers={1,0};changed.run();
    check(changed.host.number_reads==2&&changed.host.draws.size()==2,"second area-number query controls next locked area");
    Fixture escaped;escaped.host.number=2;escaped.run();
    check(escaped.host.number_reads==1&&escaped.host.draws.size()==1,"next index beyond catalog length skips second query and fence");
    Fixture later;later.host.number=1;later.run();
    check(later.host.number_reads==2&&later.host.draws.size()==1,"area that is not next has no fence");
    Fixture modal;modal.state=3;modal.run();
    check(modal.host.draws.size()==2&&modal.host.touches.size()==1,"nonzero state hides locked touch but preserves fence and discovered touch");
    Fixture none;none.host.rows->resize(1);none.host.rows->at(0)->discovery=2;none.host.number=-1;none.run();
    check(none.host.draws.size()==1&&none.host.draws[0].resource==11&&none.notices->empty(),"discovery2 is not found; no found areas makes index0 the fence");
}
void culling_and_failures(){
    Fixture zero;zero.host.rows->clear();zero.marks.reset();zero.run();
    check(zero.host.height_reads==1&&zero.host.fonts==EventIntegers({10})&&zero.host.draws.empty(),"empty catalog retains entry height/font side effects");
    Fixture culled;culled.host.rows->assign(2,nullptr);culled.marks.reset();culled.run(true,{-1000,0});
    check(culled.host.draws.empty()&&culled.host.number_reads==0&&culled.host.clip_reads==0&&culled.host.catalog_reads==3,"culled rows use captured bound/row owner and avoid found/resource reads");
    Fixture missing;missing.host.rows.reset();fails<std::invalid_argument>([&]{missing.run();});
    check(missing.host.height_reads==1&&missing.host.fonts==EventIntegers({10}),"missing catalog fails after entry scope");
    Fixture null_row;null_row.host.rows->at(0).reset();fails<std::invalid_argument>([&]{null_row.run();});
    check(null_row.host.draws.empty(),"visible null discovery row fails before image draw");
    Fixture no_marks;no_marks.marks.reset();fails<std::invalid_argument>([&]{no_marks.run();});
    check(no_marks.host.draws.size()==1&&no_marks.notices->size()==1&&no_marks.host.labels.size()==1,"missing markers fail after map, board, touch and label effects");
    Fixture short_marks;short_marks.marks->clear();fails<std::out_of_range>([&]{short_marks.run();});
    check(short_marks.host.draws.size()==1&&short_marks.notices->size()==1,"short marks retain drawing prefix");
}
void captured_and_live_values(){
    Fixture changed;const auto original=changed.host.rows->at(0);original->discovery=0;
    changed.host.action=[&](const auto& name){if(name=="raw"&&changed.host.height_reads==2){
        changed.host.rows=std::make_shared<SelectAreaDrawingRows>();changed.host.rows->push_back(record(99,1,"Replacement"));}};
    changed.run();
    check(changed.host.draws.size()==1&&changed.host.draws[0].frame==7&&changed.host.names[0]==original->name,"position callback can replace catalog; fresh discovery uses replacement but image/name retain captured row");
    Fixture shortened;shortened.host.action=[&](const auto& name){if(name=="draw")shortened.host.rows=std::make_shared<SelectAreaDrawingRows>();};shortened.run();
    check(shortened.host.draws.size()==1&&shortened.host.labels.size()==1,"catalog length is refreshed only at next loop after captured row drawing");
    Fixture selection;selection.host.action=[&](const auto& name){if(name=="clip"&&selection.host.clip_reads==1)selection.selected=0;};selection.run();
    check(selection.notices->at(0)->at(4)==0&&selection.host.touches[0][1]==0,"area selection is captured before name-clip callback");
    Fixture live_frame;live_frame.selected=0;live_frame.host.action=[&](const auto& name){if(name=="draw")live_frame.frame=36;};live_frame.run();
    check(live_frame.notices->at(1)->at(4)==6,"selected animation reads counter after map drawing");
    Fixture selector;selector.selected=0;selector.host.action=[&](const auto& name){if(name=="clip"&&selector.host.clip_reads==2)selector.frame=36;};selector.run();
    check(selector.notices->at(1)->at(4)==6,"GO selector callback precedes the counter read");
    Fixture clock;clock.selected=0;clock.host.action=[&](const auto& name){if(name=="frames")clock.frame=36;};clock.run();
    check(clock.notices->at(1)->at(4)==6,"max-frame provider precedes the counter read");
}
}
int main(){positions();first_frame();selection_and_markers();unfound_gates();culling_and_failures();captured_and_live_values();return 0;}
