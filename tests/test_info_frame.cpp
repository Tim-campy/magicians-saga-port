#include "core/gameplay/info_frame.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
using namespace saga::core;
void check_at(bool v,int line){if(!v){std::cerr<<"info contract failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Popup final:PopupBar {
    int cat=0,kind=0,rank=0,h=20,life=3,disposed=0,height_reads=0,duration_reads=0;bool only=false;
    std::function<void()> on_height,on_duration,on_dispose,on_category,on_wait;
    std::function<bool(PopupBar*)> on_same;
    explicit Popup(int id=1):PopupBar(id){}
    int category()override{if(on_category)on_category();return cat;}int type()override{return kind;}
    int priority()override{return rank;}bool single()override{return only;}
    int height()override{++height_reads;if(on_height)on_height();return h;}
    int duration()override{++duration_reads;if(on_duration)on_duration();return life;}
    void wait()override{if(on_wait)on_wait();else PopupBar::wait();}
    void dispose()override{++disposed;if(on_dispose)on_dispose();}
    bool same_popup(PopupBar* other)override{return on_same?on_same(other):this==other;}
};
struct Slide final:SlideBar {
    int w=80,h=12,life=3,height_reads=0,duration_reads=0;
    std::function<void()> on_height,on_width,on_duration;
    explicit Slide(int id=1):SlideBar(id){}
    int width()override{if(on_width)on_width();return w;}
    int height()override{++height_reads;if(on_height)on_height();return h;}
    int duration()override{++duration_reads;if(on_duration)on_duration();return life;}
};
struct Host final:InfoHost,InfoInitializationWorld {
    InfoState state;std::vector<std::string> calls;std::vector<Click> clicks;std::vector<Button> buttons;std::vector<Town> towns;
    EntitySubset monsters{[](auto*){return true;},[](const auto&){return 1;}};
    bool paused=false;int height=100;std::function<void()> on_view,on_subscribe;
    std::shared_ptr<PopupBar> popup=std::make_shared<Popup>();std::shared_ptr<SlideBar> slide=std::make_shared<Slide>();
    ComponentSlots* target=nullptr;EventObject town;
    Host(){initialize_info(state,this,*this);calls.clear();}
    int info_view_height()override{calls.push_back("height");if(on_view)on_view();return height;}
    bool info_world_paused()override{calls.push_back("paused");return paused;}
    EntitySubset* info_monsters()override{calls.push_back("monsters16");return &monsters;}
    std::shared_ptr<PopupBar> make_monster_info(ComponentSlots* e)override{calls.push_back("make-monster");target=e;return popup;}
    std::shared_ptr<SlideBar> make_attack_notice(EventObject value)override{calls.push_back("make-attack");town=std::move(value);return slide;}
    void subscribe_info_click(Click f)override{calls.push_back("4");if(on_subscribe)on_subscribe();clicks.push_back(std::move(f));}
    void subscribe_info_button(Button f)override{calls.push_back("11");if(on_subscribe)on_subscribe();buttons.push_back(std::move(f));}
    void subscribe_info_town(Town f)override{calls.push_back("28");if(on_subscribe)on_subscribe();towns.push_back(std::move(f));}
};
struct Node {ComponentSlots entity{35};ContactPosition p;Node(float x,float y,float z){p.x=x;p.y=y;p.z=z;entity.add(0,&p);}};
int main()try{
    {Host h;auto list=h.state.popups;auto p=std::make_shared<Popup>();list->append(p);auto d=std::make_shared<StageDialog>();h.state.dialog=d;
     h.state.emphasized=p;h.state.emphasize_frame=4;h.state.offset_bottom=9;h.calls.clear();initialize_info(h.state,&h,h);
     check(h.calls==std::vector<std::string>{"4","11","28"});check(h.state.popups==list&&h.state.dialog==d&&list->size()==1);
     check(h.state.emphasize_frame==-1&&!h.state.emphasized&&h.state.offset_bottom==0);
     h.state.offset_bottom=5;fails([&]{initialize_info(h.state,nullptr,h);});check(h.state.offset_bottom==0&&h.state.dialog==d);}
    {Host h;h.on_subscribe=[&]{if(h.clicks.size()>1)throw std::runtime_error("second subscription");};
     fails([&]{initialize_info(h.state,&h,h);});check(h.clicks.size()==2&&h.buttons.size()==1&&h.towns.size()==1);}
    {Popup b;b.bottom=100;for(int i=1;i<=8;++i){check(!update_popup(b));check(b.state==0&&b.frame==i&&b.state_frame==i);}
     check(b.y==80);check(!update_popup(b)&&b.state==1&&b.state_frame==0);
     check(!update_popup(b)&&!update_popup(b)&&b.state==1);check(!update_popup(b)&&b.state==2&&b.frame==12&&b.state_frame==0);
     for(int i=1;i<8;++i)check(!update_popup(b));const auto y=b.y,reads=b.height_reads;
     check(update_popup(b)&&b.frame==20&&b.state_frame==8&&b.y==y&&b.height_reads==reads&&y<100);}
    {Slide b;for(int i=0;i<9;++i)check(!update_slide(b));check(b.x==0&&b.state==1&&b.state_frame==0);
     for(int i=0;i<3;++i)check(!update_slide(b));check(b.state==2);for(int i=0;i<7;++i)check(!update_slide(b));
     const auto x=b.x;check(update_slide(b)&&b.x==x&&x> -80);}
    {Popup b;b.life=-1;b.bottom=100;for(int i=0;i<100;++i)check(!update_popup(b));check(b.state==1&&b.y==80);
     drop_popup(&b);check(b.frame==7&&b.state_frame==0&&b.state==2);b.state_frame=3;drop_popup(&b);check(b.state_frame==3);
     drop_popup(nullptr);change_popup_state(b,9);check(b.state_frame==0&&b.frame==7);check(!update_popup(b)&&b.frame==8);}
    {Popup b;b.state=1;b.frame=100;b.on_duration=[&]{b.life=b.duration_reads==1?2:200;};
     check(!update_popup(b)&&b.duration_reads==2&&b.state==1);
     b.on_duration={};b.life=std::numeric_limits<int>::max();check(popup_start_frame(b)==std::numeric_limits<int>::min()+7);
     check(popup_max_frame(b)==std::numeric_limits<int>::min()+15);}
    {Popup b;b.bottom=100;b.on_height=[&]{b.bottom=200;};b.wait();check(b.y==80);
     b.bottom=100;b.state_frame=8;popup_in(b);check(b.y==80);b.bottom=100;popup_out(b);check(b.y==200);}
    {Host h;update_info(h.state,h);check(h.calls.empty());h.state.emphasize_frame=5;h.state.emphasized=h.popup;
     h.state.popups.reset();fails([&]{update_popup_bars(h.state,h);});check(h.state.emphasize_frame==-1&&!h.state.emphasized);}
    {Host h;auto a=std::make_shared<Popup>(1),b=std::make_shared<Popup>(2),c=std::make_shared<Popup>(3);
     a->rank=1;b->rank=3;c->rank=1;add_popup(h.state,a,true);add_popup(h.state,b,true);add_popup(h.state,c,true);
     check(h.state.popups->at(0)==b&&h.state.popups->at(1)==a&&h.state.popups->at(2)==c);
     add_popup(h.state,a,false);check(h.state.popups->size()==3&&h.state.emphasized==a&&h.state.emphasize_frame==0);
     for(int i=0;i<6;++i)update_popup_bars(h.state,h);check(h.state.emphasize_frame==-1&&!h.state.emphasized);
     check(info_emphasis_scale(h.state)==1);h.state.emphasize_frame=3;check(info_emphasis_scale(h.state)==1.100000023841858F);}
    {Host h;auto a=std::make_shared<Popup>(1),b=std::make_shared<Popup>(2);a->life=-1;b->only=true;
     add_popup(h.state,a,true);add_popup(h.state,b,true);check(a->state==2&&a->frame==7&&h.state.popups->size()==2);
     auto replacement=std::make_shared<InfoBars<PopupBar>>();a->on_duration=[&]{h.state.popups=replacement;};a->state=0;
     pause_info(h.state);check(h.state.popups==replacement);}
    {Host h;auto a=std::make_shared<Popup>();add_popup(h.state,a,true);a->on_same=[&](auto*){h.state.popups->append(std::make_shared<Popup>());return false;};
     fails([&]{search_same_popup(h.state,nullptr);});check(h.state.popups->size()==2);
     a->on_same=[&](auto*){h.state.popups->append(std::make_shared<Popup>());return true;};check(search_same_popup(h.state,nullptr)==a);}
    {Host h;auto a=std::make_shared<Popup>(1),b=std::make_shared<Popup>(2);a->life=b->life=-1;
     add_popup(h.state,a,true);add_popup(h.state,b,true);for(int i=0;i<9;++i)update_popup_bars(h.state,h);
     check(a->bottom==100&&a->y==80&&b->bottom==80&&b->y==60);b->only=true;
     update_popup_bars(h.state,h);check(b->bottom==100&&b->y==80);}
    {Host h;auto a=std::make_shared<Popup>();a->state=2;a->state_frame=7;add_popup(h.state,a,true);
     auto replacement=std::make_shared<InfoBars<PopupBar>>();replacement->append(std::make_shared<Popup>());
     a->on_dispose=[&]{h.state.popups=replacement;};update_popup_bars(h.state,h);check(a->disposed==1&&replacement->size()==0);}
    {Host h;auto a=std::make_shared<Popup>();a->state=2;a->state_frame=7;a->on_dispose=[](){throw std::runtime_error("dispose");};add_popup(h.state,a,true);
     fails([&]{update_info(h.state,h);});check(a->disposed==1&&h.state.popups->size()==1&&a->frame==1);}
    {Host h;h.state.popups->append(nullptr);int heights=0;h.on_view=[&]{++heights;};fails([&]{update_popup_bars(h.state,h);});check(heights==1);}
    {Host h;auto a=std::make_shared<Slide>(1),b=std::make_shared<Slide>(2);add_slide(h.state,a);add_slide(h.state,b);
     update_slide_bars(h.state);check(a->y==65&&b->y==77&&a->frame==1&&b->frame==1);
     a->state=2;a->state_frame=7;update_slide_bars(h.state);check(h.state.slides->size()==1&&b->y==65&&b->frame==2);}
    {Host h;auto a=std::make_shared<Slide>();add_slide(h.state,a);auto replacement=std::make_shared<InfoBars<SlideBar>>();
     auto b=std::make_shared<Slide>();replacement->append(b);a->on_height=[&]{h.state.slides=replacement;};
     update_slide_bars(h.state);check(a->y==65&&a->frame==1&&b->y==0&&b->frame==0);}
    {Host h;auto a=std::make_shared<Popup>();a->kind=5;a->life=10;add_popup(h.state,a,true);h.state.dialog=std::make_shared<StageDialog>();
     update_info(h.state,h);check(!h.state.dialog&&a->state==2&&a->frame==19&&a->state_frame==1);
     a->state=1;a->on_duration=[](){throw std::runtime_error("duration");};h.state.dialog=std::make_shared<StageDialog>();
     fails([&]{update_info(h.state,h);});check(h.state.dialog!=nullptr);}
    {InputTouch event{2,nullptr};info_button_touch(&event,nullptr);event.type=5;fails([&]{info_button_touch(&event,nullptr);});fails([&]{info_button_touch(nullptr,nullptr);});}
    {Host h;h.paused=true;info_camera_click(h.state,nullptr,h);check(h.calls==std::vector<std::string>{"paused"});
     h.paused=false;h.calls.clear();info_camera_click(h.state,nullptr,h);check(h.calls==std::vector<std::string>{"paused","monsters16"});
     Node first(10,0,0),closer(0,0,0);h.monsters.add(&first.entity);h.monsters.add(&closer.entity);
     auto point=std::make_shared<CursorPoint>(CursorPoint{0,-15});info_camera_click(h.state,point,h);
     check(h.target==&first.entity&&h.state.popups->size()==1);fails([&]{info_camera_click(h.state,nullptr,h);});
     point->x=10000;info_camera_click(h.state,point,h);check(h.state.popups->size()==1);}
    {Host h;auto town=std::make_shared<int>(123);h.towns[0](town);check(h.town==town&&h.state.slides->at(0)==h.slide);
     add_slide(h.state,nullptr);fails([&]{update_slide_bars(h.state);});check(h.state.slides->size()==2);}
    check(gameplay_needs_update(25,true,0)&&gameplay_needs_update(25,true,7)&&!gameplay_needs_update(25,false,0));
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
