#include "core/gameplay/town_bubble.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"town bubble contract failure\n";std::exit(1);}}
template<class F> void fails(F f) {try{f();check(false);}catch(const std::exception&) {}}
FormText text(const char* s) {return std::make_shared<const std::string>(s);}
void test_voice_loading(){
    const auto prior=std::make_shared<std::vector<FormText>>(1,text("retained"));
    TownVoice voice{8,prior,9};
    load_town_voice_definition(voice,std::nullopt,parse_event_decimal);
    load_town_voice_definition(voice,"",parse_event_decimal);
    check(voice.id==9&&voice.type==8&&voice.texts==prior);
    load_town_voice_definition(voice,"2\t4\t2\tfirst\t\tignored",parse_event_decimal);
    check(voice.id==2&&voice.type==4&&voice.texts&&voice.texts->size()==2);
    check(*voice.texts->at(0)=="first"&&voice.texts->at(1)&&voice.texts->at(1)->empty());
    const auto retained=voice.texts;
    fails([&]{load_town_voice_definition(voice,"5\tbad",parse_event_decimal);});
    check(voice.id==5&&voice.type==4&&voice.texts==retained);
    fails([&]{load_town_voice_definition(voice,"6\t3\t2\tonly-one",parse_event_decimal);});
    check(voice.id==6&&voice.type==3&&voice.texts==retained);
    load_town_voice_definition(voice,"7\t2\t-1",parse_event_decimal);
    check(voice.id==7&&voice.type==2&&!voice.texts);
    load_town_voice_definition(voice,"8\t1\t0",parse_event_decimal);
    check(voice.id==8&&voice.type==1&&voice.texts&&voice.texts->empty());
    auto rows=load_town_voice_table("\xef\xbb\xbf" "3\t0\t1\told\n1\t4\t2\ta\tb\n3\t2\t1\tlast",false,parse_event_decimal);
    check(rows.size()==4&&!rows[0]&&rows[1]&&!rows[2]&&rows[3]);
    check(rows[1]->id==1&&rows[1]->type==4&&*rows[1]->texts->at(1)=="b");
    check(rows[3]->id==3&&rows[3]->type==2&&*rows[3]->texts->at(0)=="last");
    check(*load_town_voice_table("0\t0\t1\ta\r\n",false,parse_event_decimal)[0]->texts->at(0)=="a\r");
    check(*load_town_voice_table("0\t0\t1\ta\r\n",true,parse_event_decimal)[0]->texts->at(0)=="a");
    check(*load_town_voice_table("0\t0\t1\ta\r",true,parse_event_decimal)[0]->texts->at(0)=="a\r");
    fails([&]{load_town_voice_table("-1\t0\t0",false,parse_event_decimal);});
}
struct Host:TownBubbleHost {
    std::vector<int> calls;
    int width=100,height=100;
    std::shared_ptr<TownHumanRows> rows=std::make_shared<TownHumanRows>();
    std::shared_ptr<TownVoices> v=std::make_shared<TownVoices>();
    std::shared_ptr<TownSpeakers> c=std::make_shared<TownSpeakers>();
    std::shared_ptr<TownHumanRows> town_humans() override {calls.push_back(1);return rows;}
    std::shared_ptr<TownVoices> voices() override {calls.push_back(2);return v;}
    std::shared_ptr<TownSpeakers> speakers() override {calls.push_back(3);return c;}
    std::int32_t game_width() override {calls.push_back(4);return width;}
    std::int32_t game_height() override {calls.push_back(5);return height;}
    Host() {
        rows->push_back(std::make_shared<EventIntegers>(11,0));
        c->push_back(std::make_shared<TownSpeaker>(TownSpeaker{0,-1,0}));
        v->push_back(std::make_shared<TownVoice>(TownVoice{0,std::make_shared<std::vector<FormText>>(std::initializer_list<FormText>{text("a"),text("b")})}));
        v->push_back(std::make_shared<TownVoice>(TownVoice{4,std::make_shared<std::vector<FormText>>(1,text("recruit"))}));
    }
};
int main() {
    test_voice_loading();
    Host h;TownBubbleState state{-1,text("old"),8};
    h.rows->at(0)->at(10)=3;set_town_bubble(state,0,true,h);
    check(*state.text=="b" && state.frames==120 && state.index==0);
    check(h.calls==std::vector<int>({1,2,3,3}));
    h.calls.clear();h.c->at(0)->recruit_value=-1;set_town_bubble(state,0,false,h);
    check(*state.text=="recruit" && h.calls==std::vector<int>({1,2,3,3,3,2}));
    h.c->at(0)->living_id=0;set_town_bubble(state,0,false,h);check(*state.text=="b");
    h.c->at(0)->living_id=-1;
    auto normal=h.v->at(0)->texts;h.v->at(0)->texts.reset();const auto old=state.text;state.frames=7;
    fails([&]{set_town_bubble(state,0,false,h);});check(state.text==old && state.frames==7); // Normal lookup precedes fallback.
    h.v->at(0)->texts=normal;h.v->at(1)->type=3;
    fails([&]{set_town_bubble(state,0,false,h);});check(state.frames==7);
    h.v->at(1)->type=4;h.v->at(1)->texts->at(0)=nullptr;set_town_bubble(state,0,false,h);
    check(!state.text && state.frames==120); // Null text element is legal.
    h.c->at(0)->recruit_value=0;h.rows->at(0)->at(10)=-1;
    fails([&]{set_town_bubble(state,0,false,h);});
    h.rows->at(0)->at(10)=-2;set_town_bubble(state,0,false,h);check(*state.text=="a");
    fails([&]{set_town_bubble(state,9,false,h);});check(state.index==9 && *state.text=="a");
    state.index=0;state.frames=1;h.calls.clear();update_town_bubble_lifetime(state,0,0,h);
    check(state.frames==0 && state.index==0 && h.calls==std::vector<int>({1,5,4,5}));
    update_town_bubble_lifetime(state,0,0,h);check(state.frames==-1 && state.index==-1 && *state.text=="a");
    h.calls.clear();update_town_bubble_lifetime(state,0,0,h);check(h.calls.empty());
    state={0,text("visible"),120};h.calls.clear();update_town_bubble_lifetime(state,117,0,h);
    check(state.index==-1 && state.frames==-1 && state.text && state.text->empty());
    check(h.calls==std::vector<int>({1,5,4})); // Right edge fails without second height query.
    h.calls.clear();check(!check_select_view_area(-17,0,16,0,h) && h.calls.empty());
    check(check_select_view_area(-16,100,16,0,h)); // Inclusive boundaries.
    TownBubbleTouchHost adapter(state,h);SelectTouchMoveState movement{true,0,0,0,-1};
    update_select_touch_move(movement,adapter);check(movement.collision_id==0 && state.index==0 && state.frames==120 && *state.text=="a");
    // Feed the real loader shape into collision -> selection -> late lifetime.
    h.v=std::make_shared<TownVoices>(load_town_voice_table("0\t0\t2\tnormal0\tnormal1\n1\t4\t2\trecruit0\trecruit1",false,parse_event_decimal));
    h.rows->at(0)->at(10)=3;h.c->at(0)->recruit_value=0;movement.collision_id=-1;
    update_select_touch_move(movement,adapter);
    check(*state.text=="normal1"&&state.frames==120);
    update_town_bubble_lifetime(state,0,0,h);check(state.frames==119);
    update_select_touch_move(movement,adapter);check(state.frames==119); // Same contact does not restart it.
    h.c->at(0)->recruit_value=1;h.c->at(0)->living_id=-1;movement.collision_id=-1;
    update_select_touch_move(movement,adapter);check(*state.text=="recruit1"&&state.frames==120);
}
