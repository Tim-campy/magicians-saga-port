#include "core/gameplay/select_form_pass.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
template<class Exception,class Action>void fails(Action action){bool failed=false;try{action();}catch(const Exception&){failed=true;}check(failed,"expected exception");}
EventInstruction row(std::initializer_list<std::int32_t> values){return std::make_shared<EventIntegers>(values);}
FormText text(const char* value){return std::make_shared<const std::string>(value);}
struct Host final:SelectTownDrawingHost {
    std::int32_t count=4,width=600,height=640,raw_height=640;
    std::int32_t share_count=0,form_state=0,occupant_reads=0,king_reads=0,share_reads=0;
    bool share_active=false,king_available=true;
    EventScript cache=std::make_shared<std::vector<EventInstruction>>();
    std::shared_ptr<std::vector<EventObject>> occupants=std::make_shared<std::vector<EventObject>>(4);
    std::vector<bool> enabled{true,true,false,false};
    std::vector<std::shared_ptr<TownBuildingView>> definitions=std::vector<std::shared_ptr<TownBuildingView>>(13);
    std::vector<std::array<std::int32_t,6>> draws; // clip,frame,image,x,y,font.
    std::vector<std::array<std::int32_t,4>> touches;
    std::vector<std::array<std::int32_t,2>> labels,share_labels;
    std::vector<std::string> operations;
    std::vector<std::int32_t> fonts{10}; // Previous floor pass leaves PushFont(10).
    std::int32_t dim=0;
    std::function<void()> after_draw,on_enabled,on_label;
    Host(){
        *cache={row({0,12,0}),row({1,4,0}),row({2,-1,0}),row({3,-1,0})};
        definitions[12]=std::make_shared<TownBuildingView>(TownBuildingView{47,8,text("Decoration")});
        definitions[4]=std::make_shared<TownBuildingView>(TownBuildingView{14,7,text("King")});
    }
    std::int32_t game_width()override{return width;}
    std::int32_t game_height()override{return height;}
    std::int32_t town_count()override{return count;}
    EventScript town_buildings()override{return cache;}
    std::shared_ptr<std::vector<EventObject>> town_occupants()override{++occupant_reads;operations.push_back("occupants");return occupants;}
    bool town_enabled(std::int32_t index)override{if(on_enabled)on_enabled();return enabled.at(static_cast<std::size_t>(index));}
    std::shared_ptr<TownBuildingView> town_building(std::int32_t id)override{return definitions.at(static_cast<std::size_t>(id));}
    std::int32_t town_view_height()override{return raw_height;}
    SelectBackgroundResource town_resource()override{return this;}
    std::int32_t town_clip(std::int32_t field)override{
        switch(field){case 0x168:return 1;case 0x190:return 11;case 0x1a8:return 17;case 0x188:return 9;}
        throw std::logic_error("unexpected clip field");
    }
    std::int32_t town_clip_frames(std::int32_t clip)override{
        if(clip==17)return 5;if(clip==9)return 7;throw std::logic_error("unexpected animated notice");
    }
    void town_draw_chip(SelectBackgroundResource resource,std::int32_t clip,std::int32_t frame,std::int32_t image,std::int32_t x,std::int32_t y)override{
        check(resource==this,"drawing resource identity");check(!fonts.empty(),"chip drawing retains font scope");
        draws.push_back({clip,frame,image,x,y,fonts.back()});operations.push_back("draw");if(after_draw)after_draw();
    }
    void town_push_font(std::int32_t value)override{fonts.push_back(value);operations.push_back("push"+std::to_string(value));}
    void town_pop_font()override{check(!fonts.empty(),"font stack underflow");operations.push_back("pop"+std::to_string(fonts.back()));fonts.pop_back();}
    void town_push_dim()override{++dim;operations.push_back("dim");}
    void town_pop_dim()override{check(dim==1,"balanced label dim scope");--dim;operations.push_back("undim");}
    void town_label(FormText name,std::int32_t x,std::int32_t y)override{check(name!=nullptr,"label uses table name");labels.push_back({x,y});if(on_label)on_label();}
    bool town_king_reward_available()override{++king_reads;return king_available;}
    std::int32_t town_share_count()override{return share_count;}
    bool town_share_active()override{++share_reads;return share_active;}
    std::int32_t town_form_state()override{return form_state;}
    void town_share_label(std::int32_t x,std::int32_t y)override{share_labels.push_back({x,y});}
    void town_touch(std::int32_t slot,bool selected,std::int32_t x,std::int32_t y)override{touches.push_back({slot,selected?1:0,x,y});}
};
struct Fixture {
    Host host;
    std::int32_t frame=0,selected=-1;
    EventScript notices=std::make_shared<std::vector<EventInstruction>>();
    void run(bool top=true,SelectBackgroundOrigin origin={0,-27}){draw_select_town_buildings({frame,selected,top,notices},origin,host);}
    void one(std::int32_t id){host.count=1;*host.cache={row({0,id,0})};}
};
void queue_contract(){
    EventScript notices=std::make_shared<std::vector<EventInstruction>>();
    queue_select_notice(notices,11,12,13,17,4,0);
    queue_select_notice(notices,9,2,3,4,5,6,-77);
    queue_select_notice(notices,11,22,33,9,6,1);
    check(*notices->at(0)==EventIntegers({11,12,13,17,4,0,0}),"default depth uses pre-append count");
    check(*notices->at(1)==EventIntegers({9,2,3,4,5,6,-77}),"explicit negative depth is retained");
    check(*notices->at(2)==EventIntegers({11,22,33,9,6,1,2}),"default depth includes explicit-depth rows");
    check(notices->at(0)!=notices->at(1),"each request owns a distinct seven-int row");
    EventScript missing;
    fails<std::invalid_argument>([&]{queue_select_notice(missing,11,0,0,0,0,0);});
    fails<std::invalid_argument>([&]{queue_select_notice(missing,11,0,0,0,0,0,7);});
}
void initial_town(){
    Fixture f;f.run();
    check(f.host.draws==std::vector<std::array<std::int32_t,6>>({
        {1,0,17,85,513,10},{1,0,18,175,513,10},{1,0,18,265,513,10},{1,0,17,355,513,10},
        {1,12,47,130,513,7},{1,4,14,220,513,7},{1,0,32,310,513,7},{1,0,32,400,513,7}}),
        "real-like initial slots use underlay cadence and per-building image overrides");
    check(f.host.fonts.empty()&&f.host.dim==0,"building prefix balances floor font10 and building font7");
    check(f.host.operations[4]=="pop10"&&f.host.operations[5]=="push7"&&f.host.operations[6]=="occupants"&&f.host.operations.back()=="pop7","font transition precedes unconditional occupant query");
    check(f.host.occupant_reads==1&&f.host.king_reads==1,"occupants captured once and only king invokes reward query");
    check(f.host.labels==std::vector<std::array<std::int32_t,2>>({{220,434}}),"decoration has no label; king label uses y-79");
    check(f.host.touches==std::vector<std::array<std::int32_t,4>>({{1,0,180,443},{2,0,270,443},{3,0,360,443}}),"decoration is not interactive; disabled vacant unselected slots are registered");
    check(f.notices->size()==2,"board and king arrow remain queued after prefix");
    check(*f.notices->at(0)==EventIntegers({11,220,433,11,1,0,0}),"board request uses frame1 and first depth");
    check(*f.notices->at(1)==EventIntegers({11,220,468,17,0,0,1}),"king request uses y-45 and next queue depth");
}
void vacancy_and_selection(){
    Fixture enabled;enabled.one(-1);enabled.frame=36;enabled.selected=0;enabled.run();
    check(enabled.host.draws.back()==std::array<std::int32_t,6>{1,36,33,130,513,7},"enabled vacancy selects texture33 and current frame");
    check(enabled.notices->size()==2&&*enabled.notices->at(0)==EventIntegers({11,130,463,17,1,0,0}),"vacancy arrow uses current frame modulo max and y-50");
    check(*enabled.notices->at(1)==EventIntegers({11,125,527,9,6,0,1}),"selected go button uses frame/6 and x-5,y+14");
    check(enabled.host.touches==std::vector<std::array<std::int32_t,4>>({{0,1,90,423}}),"selected touch uses y-90");
    Fixture disabled;disabled.one(-1);disabled.host.enabled[0]=false;disabled.selected=0;disabled.run();
    check(disabled.host.draws.back()[2]==32&&disabled.notices->empty()&&disabled.host.touches.empty(),"selected disabled vacancy has no arrow, go button or touch");
    Fixture lower;lower.one(4);lower.host.king_available=false;lower.run(false);
    check(lower.host.labels.empty()&&lower.notices->size()==1,"covered form still queues board but omits text");
    check(std::count(lower.host.operations.begin(),lower.host.operations.end(),"dim")==1,"unselected label path retains dim scope without text");
    Fixture selected;selected.one(4);selected.selected=0;selected.host.king_available=false;selected.run();
    check(std::count(selected.host.operations.begin(),selected.host.operations.end(),"dim")==0,"selected building label is not dimmed");
}
void residence_and_share(){
    Fixture empty;empty.one(4);empty.host.definitions[4]->type=6;empty.frame=8;empty.run();
    check(empty.host.king_reads==0&&empty.notices->size()==2&&empty.notices->at(1)->at(4)==3,"empty residence queues arrow with native animation remainder");
    Fixture occupied;occupied.one(4);occupied.host.definitions[4]->type=6;occupied.host.occupants->at(0)=std::make_shared<int>(9);occupied.run();
    check(occupied.notices->size()==1,"occupied residence retains board without vacancy arrow");
    Fixture inactive;inactive.one(4);inactive.host.definitions[4]->type=12;inactive.run();
    check(inactive.host.labels.empty()&&inactive.host.touches.empty()&&inactive.notices->empty(),"inactive share building omits ordinary board, indicator and touch");
    Fixture active;active.one(4);active.host.definitions[4]->type=12;active.host.share_active=true;active.run();
    check(active.host.share_labels==std::vector<std::array<std::int32_t,2>>({{130,468}})&&active.notices->size()==1&&active.host.touches.size()==1,"active first share has its label, arrow and unselected touch");
    Fixture used;used.one(4);used.host.definitions[4]->type=12;used.host.share_active=true;used.host.share_count=1;used.run();
    check(used.notices->empty()&&used.host.share_labels.empty()&&used.host.touches.size()==1,"recorded share suppresses first-share indicator only");
    Fixture changed;changed.one(4);changed.host.definitions[4]->type=12;changed.host.share_active=true;changed.host.form_state=1;changed.run();
    check(changed.notices->empty()&&changed.host.touches.size()==1,"nonzero form state suppresses share indicator");
}
void bounds_and_capture(){
    Fixture zero;zero.host.count=0;zero.host.cache.reset();zero.run();
    check(zero.host.occupant_reads==1&&zero.host.draws.empty()&&zero.host.fonts.empty(),"zero capacity still transitions fonts and captures occupants");
    Fixture outside;outside.host.cache.reset();outside.run(true,{-1000,0});
    check(outside.host.draws.empty()&&outside.notices->empty()&&outside.host.touches.empty(),"culled slots do not access cache rows");
    Fixture holes;holes.host.cache->at(1).reset();holes.run();
    check(holes.host.draws.size()==6&&holes.host.king_reads==0&&holes.notices->empty(),"null cache row skips both underlay and building passes");
    Fixture captured;const auto original=captured.host.cache;
    captured.host.after_draw=[&]{captured.host.count=0;captured.host.cache.reset();};captured.run();
    check(captured.host.draws.size()==8&&original->size()==4,"capacity and cache owner are captured before draw callbacks");
    Fixture malformed;malformed.one(4);malformed.host.cache->at(0)=row({0,4});
    fails<std::out_of_range>([&]{malformed.run();});
    check(malformed.host.draws.size()==1,"short building row fails after underlay without drawing a building");
}
void live_callbacks(){
    Fixture empty;empty.one(-1);empty.host.on_enabled=[&]{empty.frame=99;};empty.run();
    check(empty.host.draws.front()[1]==0&&empty.host.draws.back()[1]==99,"vacancy reloads frame after enable/resource callbacks");
    check(empty.host.share_reads==1,"vacancy also performs the late ShareActive read");
    Fixture existing;existing.one(4);existing.host.on_enabled=[&]{existing.frame=99;};existing.run();
    check(existing.host.draws.back()[1]==4,"existing building retains frame captured before enable callback and adds id");
    check(existing.notices->at(1)->at(4)==4,"later king indicator reads live frame99 modulo5");
    Fixture selected;selected.one(4);selected.host.on_label=[&]{selected.selected=0;};selected.run();
    check(selected.host.dim==1&&std::count(selected.host.operations.begin(),selected.host.operations.end(),"undim")==0,"label callback selection change is reread before native dim pop");
    check(selected.host.touches.back()[1]==1,"late interaction observes changed selection");
}
}
int main(){queue_contract();initial_town();vacancy_and_selection();residence_and_share();bounds_and_capture();live_callbacks();return 0;}
