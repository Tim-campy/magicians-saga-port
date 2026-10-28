#include "quest_camera_fixture.hpp"
#include "cell_fixture.hpp"
#include <algorithm>
#include <limits>

int main()try{
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();auto& e=*c.state.camera;
     check(field<QuestCameraComponent>(e,3).angle==45&&field<QuestCameraComponent>(e,3).scale==100);
     check(field<ContactPosition>(e,0).x==0&&!e.contains(1)&&c.state.history->empty()&&c.state.pinch_scale==120);
     check(c.calls==std::vector<std::string>{"create-camera","create","register","position","new-camera"});
     auto* old=c.state.camera;auto history=c.state.history;auto range=c.state.scale_range;c.state.dirty=true;c.state.dragging=true;c.state.cursor_revision=true;
     c.init();check(c.state.camera!=old&&c.state.history!=history&&c.state.scale_range==range);
     check(!c.state.dirty&&!c.state.dragging&&!c.state.cursor_revision&&t.f.registry.contains(t.f.entity_id(*old)));
     fails([&]{initialize_quest_camera(c.state,nullptr);});
     struct Null final:QuestCameraInitializationWorld {ComponentSlots* create_camera()override{return nullptr;}} null;
     c.state.dirty=true;fails([&]{initialize_quest_camera(c.state,&null);});check(!c.state.camera&&c.state.dirty);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.hook=[](const std::string& s){if(s=="new-camera")throw std::runtime_error(s);};
     fails([&]{c.init();});auto& e=t.f.nodes.back().entity;check(e.contains(0)&&!e.contains(3)&&t.f.registry.contains(t.f.entity_id(e)));}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();t.f.form.scale=75;c.pixels_width=481;c.pixels_height=641;
     check(quest_game_width(&c.form,c)==641&&quest_game_height(&c.form,c)==854);c.view_x=1;check(quest_game_width(&c.form,c)==640);
     c.hook=[&](const std::string& s){if(s=="view-x")t.f.form.scale=150;};check(quest_game_width(&c.form,c)==320);
     c.hook={};t.f.form.scale=0;check(quest_game_width(&c.form,c)==0);c.form.view=nullptr;fails([&]{quest_game_width(&c.form,c);});}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();t.f.form.scale=100;c.calls.clear();
     set_quest_cursor(&c.form,1000,-200,c);check(c.save->at(17)==1000&&c.save->at(18)==-200&&c.form.x==760&&c.form.y==-520);
     check(std::count(c.calls.begin(),c.calls.end(),"map")==5&&std::count(c.calls.begin(),c.calls.end(),"save")==4&&!c.form.update_static_render);
     set_quest_camera_position(&c.form,-10,20,c);check(c.form.x==-10&&c.form.y==20&&c.form.update_static_render&&c.save->at(17)==230&&c.save->at(18)==340);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();c.save=std::make_shared<EventIntegers>(18,0);c.form.x=7;c.form.y=8;
     fails([&]{set_quest_cursor(&c.form,10,20,c);});check(c.save->at(17)==10&&c.form.x==7&&c.form.y==8);
     fails([&]{set_quest_camera_position(&c.form,11,22,c);});check(c.form.x==11&&c.form.y==22&&c.save->at(17)==251&&!c.form.update_static_render);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();int saves=0;
     c.hook=[&](const std::string& s){if(s=="save"&&++saves==2)c.form.y=90;};
     set_quest_camera_position(&c.form,10,20,c);check(c.save->at(17)==250&&c.save->at(18)==410&&c.form.y==90);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();int heights=0;c.form.y=99;
     c.hook=[&](const std::string& s){if(s=="height"&&++heights==2)throw std::runtime_error(s);};
     fails([&]{set_quest_cursor(&c.form,1000,2000,c);});check(c.form.x==760&&c.form.y==99&&c.save->at(18)==2000);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();c.mapped=nullptr;int calls=0;
     c.hook=[&](const std::string& s){if(s=="map"&&++calls==2)c.mapped=&map;};
     set_quest_cursor(&c.form,0,0,c);check(calls==5);c.mapped=nullptr;c.hook={};
     fails([&]{set_quest_camera_position(&c.form,1,2,c);});check(c.form.x==-240&&c.form.y==-320);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();auto& hero=t.hero();
     auto& p=field<ContactPosition>(hero,0);p.x=1000;p.z=2000;auto& human=field<HumanRecordLink>(hero,14);human.formation_x=10;human.formation_z=20;
     check(indexed_member_formation_x(5,t.f)==1026&&indexed_member_formation_z(5,t.f)==2016);
     check(indexed_member_formation_x(-5,t.f)==954&&indexed_member_formation_z(-5,t.f)==1944);
     update_quest_camera(c.state,c);check(c.save->at(17)==-970&&c.save->at(18)==1497&&c.form.x==-1210&&c.form.y==1177);
     c.state.camera->remove(3);update_quest_camera(c.state,c); // Update discards a nullable Camera getter.
     c.members=nullptr;fails([&]{update_quest_camera(c.state,c);});c.current=nullptr;fails([&]{set_camera_cursor(1,2,c);});}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();c.pixels_width=240;check(quest_camera_scale_min(c)==10);
     c.pixels_width=241;check(quest_camera_scale_min(c)==75&&quest_camera_scale_max()==200);
     auto range=quest_camera_scale_range(c.state,c);check(*range==EventIntegers({75,200}));
     auto old=range;c.pixels_width=240;c.hook=[&](const std::string& s){if(s=="camera-view-width")c.state.scale_range=std::make_shared<EventIntegers>(EventIntegers{111,222});};
     auto next=quest_camera_scale_range(c.state,c);check(old->at(0)==10&&next->at(0)==111&&next->at(1)==200);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();set_quest_camera_scale(c.state,10,c);
     check(field<QuestCameraComponent>(*c.state.camera,3).scale==75&&t.f.form.scale==100&&c.form.update_static_render);
     c.form.update_static_render=false;c.calls.clear();set_quest_camera_scale(c.state,75,c);check(!c.form.update_static_render);
     check(std::find(c.calls.begin(),c.calls.end(),"form")==c.calls.end());c.state.camera->remove(0);
     fails([&]{set_quest_camera_scale(c.state,200,c);});check(field<QuestCameraComponent>(*c.state.camera,3).scale==200);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();QuestCameraTouch down{0,0,0},drag{8,15,-8},click{5,0,0},up{1,0,0};
     on_quest_camera_touch(c.state,&down,c);check(c.state.dragging);
     on_quest_camera_touch(c.state,&drag,c);check(c.state.dirty&&*c.state.history->at(0)==EventIntegers({15,-8}));
     on_quest_camera_touch(c.state,&click,c);check(c.state.cursor_revision);on_quest_camera_touch(c.state,&drag,c);check(!c.state.cursor_revision);
     on_quest_camera_touch(c.state,&up,c);check(!c.state.dragging&&c.state.history->size()==2);
     on_quest_camera_touch(c.state,&down,c);for(int i=0;i<5;++i){drag.dx=i;on_quest_camera_touch(c.state,&drag,c);}
     auto oldest=c.state.history->back();drag.dx=5;on_quest_camera_touch(c.state,&drag,c);
     check(c.state.history->size()==5&&c.state.history->at(0)==oldest&&oldest->at(0)==5);
     c.touches=2;c.state.dirty=false;drag.dx=999;on_quest_camera_touch(c.state,&drag,c);check(!c.state.dirty&&oldest->at(0)==5);
     c.state.history->back()=std::make_shared<EventIntegers>(1,0);c.touches=1;
     fails([&]{on_quest_camera_touch(c.state,&drag,c);});check(c.state.history->size()==5&&c.state.history->back()->at(0)==999);}
    {HumanFixture t;MapComponent map;CameraServices c(t,map);c.init();QuestCameraTouch event{12,0,0};c.state.history.reset();
     on_quest_camera_touch(c.state,&event,c);check(c.state.dirty);fails([&]{on_quest_camera_touch(c.state,nullptr,c);});}
    // Concrete camera -> QuestForm/save offsets -> inverse projection -> visible-cell pass.
    {HumanFixture t;CellServices cells([&](const auto& all){return &t.f.groups.get(std::make_shared<ComponentGroupFilter>(all));});
     cells.map.step_x=312;cells.map.step_z=312;cells.map.size_width=624;cells.init();CameraServices c(t,cells.map);c.init();cells.view=&c.cell_view;
     set_quest_camera_position(&c.form,936,0,c);check(*c.cell_view.cell_view_origin(312,156)==EventIntegers({1,-1}));
     check(c.cell_view.cell_game_width()==480&&c.cell_view.cell_game_height()==640);
     update_cell_visibility(cells.index,cells);check(cells.visible_count>=0);
     bool enabled=true;GameplayUpdate camera(8,enabled,[&](std::int32_t){update_quest_camera(c.state,c);});
     GameplayUpdate culling(20,enabled,[&](std::int32_t){update_cell_visibility(cells.index,cells);});
     std::vector<UpdateSystem*> systems{&camera,&culling};UpdateSchedule schedule{&systems,&c.clock};c.calls.clear();
     update_systems(schedule,1);check(c.calls.empty());update_systems(schedule,0);check(!c.calls.empty()&&c.clock.stops==2);}
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
