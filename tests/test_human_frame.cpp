#include "human_frame_fixture.hpp"
#include <cmath>

namespace {
struct HumanDrawRecorder final:HumanDrawingHost {
    struct Icon {std::int32_t resource,clip,frame,x,y,scale;};
    std::vector<std::string> calls;std::vector<Icon> icons;std::vector<ParameterGaugeRequest> gauges;
    GaugePoint point{40,50};const ContactPosition* projected=nullptr;
    std::function<void(const std::string&)> on_call;std::string fail;bool mode=false;
    void step(const std::string& name){calls.push_back(name);if(on_call)on_call(name);if(fail==name)throw std::runtime_error(name);}
    GaugePoint gauge_screen_position(const ContactPosition* p)override{projected=p;step("project");return point;}
    void set_render_mode(std::int32_t kind,std::int32_t src,std::int32_t dst)override{
        check(kind==1&&src==255&&dst==255);step("mode");mode=true;
    }
    void reset_render_mode()override{step("reset");mode=false;}
    void draw_scaled_seb(std::int32_t resource,std::int32_t clip,std::int32_t frame,
                         std::int32_t x,std::int32_t y,std::int32_t scale)override{
        check(mode);step("icon");icons.push_back({resource,clip,frame,x,y,scale});
    }
    void parameter_gauge(const ParameterGaugeRequest& request)override{
        check(!mode);step(request.parameter==0?"hp":"mp");gauges.push_back(request);
    }
};
struct HumanDrawFixture:HumanFixture {
    EntitySubset selected{[](ComponentSlots*){return true;},[](const ComponentSlots&){return 0;}};
    HumanDrawRecorder drawing;
    HumanDrawFixture(){selected.add(&hero());host.system.subset=&selected;flat_parameters(100);f.fps=10;
        f.state.attack_attribute=0;resolve_combat_parameter(hero(),0)->value=10;resolve_combat_parameter(hero(),0)->extra=0;}
    void draw(){draw_human_gauges(host.system,host,drawing);}
};
void drawing_contracts(){
    // Projection precedes all actor gates; the two speed queries precede the icon and both gauges.
    {HumanDrawFixture t;auto& e=t.hero();auto& h=field<HumanRecordLink>(e,14);h.attack_frame=0;h.frame=8;
     t.host.on_fps=[&]{t.drawing.calls.push_back("fps");};
     t.host.parameters=[&](ComponentSlots& actor,std::int32_t key){check(&actor==&e&&key==6);t.drawing.calls.push_back("speed");return 100;};
     t.draw();check(t.drawing.calls==std::vector<std::string>{"project","fps","speed","fps","speed","mode","icon","reset","hp","mp"});
     check(t.drawing.projected==&field<ContactPosition>(e,0)&&t.host.fps_queries==2);
     const auto& icon=t.drawing.icons.at(0);check(icon.resource==20&&icon.clip==103&&icon.frame==0&&icon.x==48&&icon.y==33&&icon.scale==100);
     const auto& hp=t.drawing.gauges.at(0);check(hp.entity==&e&&hp.x==40&&hp.y==50&&hp.image==7&&hp.parameter==0&&hp.anchor==2&&hp.force);
     const auto& mp=t.drawing.gauges.at(1);check(mp.entity==&e&&mp.x==40&&mp.y==53&&mp.image==3&&mp.parameter==1&&mp.anchor==2&&mp.force);
     check(!t.drawing.mode&&h.attack_frame==0&&h.frame==8&&t.host.system.loop_counter==0);}
    for(auto state:{0,1,2,3,4,5,77}){HumanDrawFixture t;auto& e=t.hero();field<HumanRecordLink>(e,14).state=state;
     t.f.node(e).lifetime.destroyed=true;t.draw();check(t.drawing.icons.size()==1&&t.drawing.gauges.size()==2);}
    for(auto value:{0,-1,std::numeric_limits<std::int32_t>::min()}){HumanDrawFixture t;auto& e=t.hero();
     resolve_combat_parameter(e,0)->value=value;e.remove(14);t.draw();check(t.drawing.calls==std::vector<std::string>{"project"}&&t.host.fps_queries==0);}
    {HumanDrawFixture t;auto& hp=*resolve_combat_parameter(t.hero(),0);hp.value=std::numeric_limits<std::int32_t>::max();hp.extra=1;
     t.draw();check(t.drawing.calls==std::vector<std::string>{"project"});}
    {HumanDrawFixture t;auto& hp=*resolve_combat_parameter(t.hero(),0);hp.value=0;hp.extra=1;t.draw();check(t.drawing.icons.size()==1);}
    {HumanDrawFixture t;field<HumanRecordLink>(t.hero(),14).state=6;t.draw();check(t.drawing.calls==std::vector<std::string>{"project"}&&t.host.fps_queries==0);}
    {HumanDrawFixture t;t.hero().remove(0);t.draw();check(!t.drawing.projected&&t.drawing.icons.size()==1);}
    {HumanDrawFixture t;t.hero().remove(23);fails([&]{t.draw();});check(t.drawing.calls==std::vector<std::string>{"project"});}
    {HumanDrawFixture t;t.selected.clear_entities();t.host.system.members=nullptr;t.draw();check(t.drawing.calls.empty());
     t.host.system.subset=nullptr;fails([&]{t.draw();});check(t.drawing.calls.empty());}
    // Both charge boundaries and signed wrapping loop angles retain native int32/binary32 behavior.
    for(const auto pair:std::array<std::array<std::int32_t,2>,4>{{{{-1,100}},{{0,100}},{{5,38}},{{9,25}}}}){
     HumanDrawFixture t;field<HumanRecordLink>(t.hero(),14).attack_frame=pair[0];t.draw();
     check(t.drawing.icons.at(0).clip==103&&t.drawing.icons[0].scale==pair[1]);}
    const std::array<std::int32_t,10> ready_scale{{15,18,19,19,16,14,11,11,12,15}};
    for(std::int32_t loop=0;loop<10;++loop){HumanDrawFixture t;field<HumanRecordLink>(t.hero(),14).attack_frame=10;t.host.system.loop_counter=loop;t.draw();
     check(t.drawing.icons.at(0).clip==104&&t.drawing.icons[0].scale==ready_scale.at(static_cast<std::size_t>(loop))&&t.drawing.icons[0].frame==loop);}
    for(const auto pair:std::array<std::array<std::int32_t,2>,3>{{{{-1,12}},{{std::numeric_limits<std::int32_t>::min(),15}},{{std::numeric_limits<std::int32_t>::max(),12}}}}){
     HumanDrawFixture t;field<HumanRecordLink>(t.hero(),14).attack_frame=std::numeric_limits<std::int32_t>::max();t.host.system.loop_counter=pair[0];t.draw();
     check(t.drawing.icons.at(0).clip==104&&t.drawing.icons[0].scale==pair[1]);}
    for(auto direction:{-1,0,1,2,3,4}){HumanDrawFixture t;field<ContactDirection>(t.hero(),12).direction=direction;
     t.draw();check(t.drawing.icons.at(0).x==(direction==2?32:48));}
    {HumanDrawFixture t;t.drawing.point={std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::min()};t.draw();
     check(t.drawing.icons.at(0).x==std::numeric_limits<std::int32_t>::min()+7&&t.drawing.icons[0].y==std::numeric_limits<std::int32_t>::max()-16);
     check(t.drawing.gauges.at(1).y==std::numeric_limits<std::int32_t>::min()+3);}
    // Providers may replace components or mutate clocks; later native reads remain fresh.
    {HumanDrawFixture t;auto& e=t.hero();HumanRecordLink replacement;replacement.character_id=0;replacement.attack_frame=10;
     t.host.on_fps=[&]{if(t.host.fps_queries==2){t.f.fps=20;e.change(14,&replacement);}};
     t.drawing.on_call=[&](const std::string& name){if(name=="mode")t.host.system.loop_counter=27;if(name=="icon")t.drawing.point={99,99};};
     t.draw();check(t.drawing.icons.at(0).clip==104&&t.drawing.icons[0].frame==27&&t.drawing.gauges[0].x==40&&t.drawing.gauges[0].y==50);}
    {HumanDrawFixture t;t.drawing.on_call=[&](const std::string& name){if(name=="reset")t.f.state.attack_attribute=2;};t.draw();check(t.drawing.gauges.at(0).image==9);}
    for(std::int32_t attribute=0;attribute<5;++attribute){HumanDrawFixture t;t.f.state.attack_attribute=attribute;t.draw();
     check(t.drawing.gauges.at(0).image==std::array<std::int32_t,5>{{7,8,9,0,0}}.at(static_cast<std::size_t>(attribute)));}
    {HumanDrawFixture t;auto& e=t.hero();t.host.characters[0]->flags=0;t.host.characters[0]->weapon_id=3;t.host.gear[3]->attribute=1;t.f.state.attack_attribute=4;
     t.draw();check(t.drawing.gauges.at(0).image==8);}
    // Failure preserves the completed prefix; the icon has no finally-style render-mode reset.
    {HumanDrawFixture t;t.host.on_fps=[&]{if(t.host.fps_queries==2)throw std::runtime_error("burst");};
     fails([&]{t.draw();});check(t.host.fps_queries==2&&t.drawing.calls==std::vector<std::string>{"project"});}
    {HumanDrawFixture t;t.hero().remove(12);fails([&]{t.draw();});check(t.host.fps_queries==2&&t.drawing.calls==std::vector<std::string>{"project"});}
    for(const auto& stop:std::vector<std::string>{"mode","icon","reset","hp","mp"}){HumanDrawFixture t;t.drawing.fail=stop;fails([&]{t.draw();});
     check(t.drawing.calls.back()==stop&&t.drawing.mode==(stop=="icon"||stop=="reset"));
     check(t.drawing.icons.size()==(stop=="mode"||stop=="icon"?0U:1U)&&t.drawing.gauges.size()==(stop=="mp"?1U:0U));}
    for(auto attribute:{-1,5,std::numeric_limits<std::int32_t>::max()}){HumanDrawFixture t;t.f.state.attack_attribute=attribute;
     fails([&]{t.draw();});check(t.drawing.icons.size()==1&&!t.drawing.mode&&t.drawing.gauges.empty()&&t.drawing.calls.back()=="reset");}
    {HumanDrawFixture t;t.drawing.on_call=[&](const std::string& name){if(name=="reset")t.hero().remove(14);};
     fails([&]{t.draw();});check(t.drawing.icons.size()==1&&!t.drawing.mode&&t.drawing.gauges.empty());}
    // Live enumeration preserves insertion order and faults only after the current actor's suffix.
    {HumanDrawFixture t;auto& second=t.sub();t.selected.add(&second);t.draw();
     check(t.drawing.gauges.size()==4&&t.drawing.gauges[0].entity==&t.hero()&&t.drawing.gauges[2].entity==&second);}
    {HumanDrawFixture t;auto& second=t.sub();t.drawing.on_call=[&](const std::string& name){if(name=="hp")t.selected.add(&second);};
     fails([&]{t.draw();});check(t.drawing.gauges.size()==2&&t.drawing.calls.back()=="mp");}
}
}
int main()try{
    drawing_contracts();
    // Concrete Init uses the supplied world, stores nullable results and does not reset counters.
    {UpdateFixture f;FrameServices host(f);HumanWorld world(f,host);host.system.loop_counter=9;f.no_world=true;world.init();
     check(world.calls==std::vector<std::string>{"subset","map","stage","enemies","members","subscribe23","subscribe29"});
     check(host.system.map==f.active_map&&host.system.stage==&f.state&&host.system.enemies==&f.enemy_system&&host.system.members==&f.roster&&host.system.loop_counter==9);
     fails([&]{initialize_humans(host.system,nullptr,world.services);});world.null_results=true;world.init();
     check(!host.system.subset&&!host.system.map&&!host.system.stage&&!host.system.enemies&&!host.system.members);}
    for(const auto& stop:std::vector<std::string>{"subset","map","stage","enemies","members","subscribe23","subscribe29"}){
     UpdateFixture f;FrameServices host(f);HumanWorld world(f,host);world.fail=stop;fails([&]{world.init();});
     if(stop=="subset")check(!host.system.subset);if(stop=="map")check(host.system.subset&&!host.system.map);
     if(stop=="stage")check(host.system.map&&!host.system.stage);if(stop=="enemies")check(host.system.stage&&!host.system.enemies);
     if(stop=="members")check(host.system.enemies&&!host.system.members);
     auto& e=f.spawn(1).entity;host.events.send(23,&e);check(host.guards==(stop=="subscribe29"?1:0));}
    {UpdateFixture f;FrameServices host(f);HumanWorld world(f,host);std::array<EventHashes,2> hashes{{{4,104},{5,105}}};world.init(hashes);world.init();
     auto& e=f.spawn(1).entity;host.events.send(23,&e);check(host.guards==2&&host.events.unsubscribe_single(23,4)&&host.events.unsubscribe_pair(29,5));
     host.events.send(23,&e);check(host.guards==3);}
    {HumanFixture t;check(stage_attack_active(t.f.state));t.no_attack();check(!stage_attack_active(t.f.state));
     t.f.buttons[0].state=1;t.f.state.attack_buttons->at(1)=nullptr;check(stage_attack_active(t.f.state));
     t.f.buttons[0].state=0;fails([&]{stage_attack_active(t.f.state);});
     t.f.state.attack_buttons.reset();fails([&]{stage_attack_active(t.f.state);});}
    // Targeting uses XYZ, inclusive wrapped range, live slot order, state/HP guards and first ties.
    {HumanFixture t;ContactPosition origin;origin.x=100;origin.z=100;
     auto& a=t.enemy_at({100,100,100});auto& b=t.enemy_at({100,0,128});auto& c=t.enemy_at({100,0,72});
     check(nearest_attack_enemy(t.f.enemy_system,&origin,1)==&b);
     field<EnemyRecordLink>(b,24).state=4;check(nearest_attack_enemy(t.f.enemy_system,&origin,-1)==&c);
     resolve_combat_parameter(c,0)->value=0;check(!nearest_attack_enemy(t.f.enemy_system,&origin,1));
     field<ContactPosition>(a,0).y=std::numeric_limits<float>::quiet_NaN();check(!nearest_attack_enemy(t.f.enemy_system,&origin,100));
     field<ContactPosition>(a,0).y=28;check(nearest_attack_enemy(t.f.enemy_system,&origin,std::numeric_limits<std::int32_t>::max())==&a);}
    {HumanFixture t;check(!nearest_attack_enemy(t.f.enemy_system,nullptr,1));auto& e=t.enemy_at();field<EnemyRecordLink>(e,24).state=4;
     check(!nearest_attack_enemy(t.f.enemy_system,nullptr,1));field<EnemyRecordLink>(e,24).state=0;fails([&]{nearest_attack_enemy(t.f.enemy_system,nullptr,1);});}
    {HumanFixture t;t.flat_parameters();auto& h=t.hero();auto& record=t.host.gear.at(3).value();record.distance=20;record.burst_count=20;record.use_mp=8;
     check(human_attack_distance(t.host.system,&h,t.host)==12&&human_burst_count(t.host.system,&h,t.host)==10);
     auto& mp=*resolve_combat_parameter(h,1);mp.maximum=5;mp.value=5;check(human_has_mp(t.host.system,&h,0,t.host));
     mp.value=4;check(!human_has_mp(t.host.system,&h,0,t.host));mp.maximum=0;mp.value=0;check(human_has_mp(t.host.system,&h,0,t.host));
     mp.maximum=5;mp.extra_maximum=5;mp.value=4;mp.extra=4;check(human_has_mp(t.host.system,&h,0,t.host));
     record.distance=-3;record.burst_count=-2;check(human_attack_distance(t.host.system,&h,t.host)==-3&&human_burst_count(t.host.system,&h,t.host)==-2);}
    {HumanFixture t;auto& h=t.hero();t.f.state.attack_attribute=2;check(human_attack_attribute(t.host.system,&*t.host.characters[0],t.host)==2);
     t.host.characters[1]->weapon_id=7;t.host.gear[7]->attribute=1;auto& sub=t.sub();field<EquipmentRecordLinks>(sub,8).ids->at(0)=3;
     t.host.gear[3]->attribute=0;check(human_attack_attribute(t.host.system,&*t.host.characters[1],t.host)==1);
     t.host.parameters=[](ComponentSlots&,std::int32_t key){return key==12?120:0;};t.host.gear[3]->distance=2;t.host.gear[3]->burst_count=1;t.host.gear[3]->use_mp=3;
     check(human_mp_cost(t.host.system,&h,2,t.host)==33&&human_attack_distance(t.host.system,&h,t.host)==6&&human_burst_count(t.host.system,&h,t.host)==7);}
    {HumanFixture t;auto& h=t.hero();auto& mp=*resolve_combat_parameter(h,1);mp.value=30;t.host.gear[3]->use_mp=3;
     t.host.parameters=[](ComponentSlots&,std::int32_t key){return key==10?120:0;};
     check(human_mp_cost(t.host.system,&h,0,t.host)==33);use_human_mp(&h,t.host);check(mp.value==27);}
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();check(human_move_speed(t.host.system,&h,t.host)==2.23F);
     check(human_attack_speed(t.host.system,&h,t.host)==100&&human_attack_interval(t.host.system,&h,t.host)==60&&human_burst_wait(t.host.system,&h,t.host)==6);
     t.flat_parameters(-100);check(human_attack_interval(t.host.system,&h,t.host)==2&&human_burst_wait(t.host.system,&h,t.host)==2);
     t.flat_parameters(std::numeric_limits<std::int32_t>::max());check(human_move_speed(t.host.system,&h,t.host)<0);}
    // Rejection does not clear the actor's earlier target or speed, and does not try the next candidate.
    for(auto kind:{0,1,2}){HumanFixture t;auto& h=t.hero();auto& a=t.enemy_at();t.enemy_at({100,0,130});
     auto& human=field<HumanRecordLink>(h,14);human.opponent=&a;field<ProjectileSpeed>(h,1).x=9;
     if(kind==0)field<EnemyRecordLink>(a,24).state=5;
     if(kind==1)t.f.enemy_pools.records->at(2)->flags=1;
     if(kind==2)t.f.node(a).lifetime.destroyed=true;
     check(!search_human_attack(t.host.system,&h,t.host)&&human.opponent==&a&&field<ProjectileSpeed>(h,1).x==9);}
    {HumanFixture t;t.no_attack();check(!search_human_attack(t.host.system,nullptr,t.host));
     t.f.buttons[0].state=1;fails([&]{search_human_attack(t.host.system,nullptr,t.host);});}
    // The complete waiting -> acquisition -> state-2 path uses real catalog/parameter and state kernels.
    {HumanFixture t;auto& target=t.enemy_at();auto& h=t.hero();auto& v=field<ProjectileSpeed>(h,1);v.x=3;v.y=4;v.z=5;
     t.update();auto& link=field<HumanRecordLink>(h,14);check(link.state==2&&link.opponent==&target&&link.frame==1&&link.attack_frame==1);
     check(v.x==0&&v.y==0&&v.z==0&&t.host.system.loop_counter==1&&t.host.shots==0);}
    // Human -> Enemy -> Move follows factory order: holding movement delays attack; release acquires the live enemy.
    {HumanFixture t;t.flat_parameters(100);t.f.geometry.step_x=312;t.f.geometry.step_z=312;auto& h=t.hero();
     auto& p=field<ContactPosition>(h,0);p.x=312;p.z=1164;field<HumanRecordLink>(h,14).state=1;field<ProjectileSpeed>(h,1).z=-1;
     auto& target=t.enemy_at({312,0,1140});change_enemy_state(target,1);auto* move=t.enemy_world.subset({0,1});bool enabled=true;
     GameplayUpdate human_system(4,enabled,[&](std::int32_t){t.update();});
     GameplayUpdate enemy_system(5,enabled,[&](std::int32_t){update_enemies(t.f.enemy_system,t.f);});
     GameplayUpdate movement(11,enabled,[&](std::int32_t){move_subset(*move);});std::vector<UpdateSystem*> list{&human_system,&enemy_system,&movement};
     UpdateSchedule schedule{&list,nullptr};update_systems(schedule,1);check(p.z<1164&&field<HumanRecordLink>(h,14).state==1);
     field<ProjectileSpeed>(h,1).z=0;update_systems(schedule,1);
     check(field<HumanRecordLink>(h,14).state==2&&field<HumanRecordLink>(h,14).opponent==&target&&t.host.shots==0);}
    // Both actual Init methods deliver Human contact before Enemy contact, then the two frame passes recover.
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();auto& target=t.enemy_at({40,0,50});field<HumanRecordLink>(h,14).invincible_time=100;
     t.host.calls.clear();t.host.events.send(29,&h,&target);check(t.host.calls.front()=="human29");
     check(field<HumanRecordLink>(h,14).state==3&&field<EnemyRecordLink>(target,24).state==3);
     auto* move=t.enemy_world.subset({0,1});for(int i=0;i<21;++i){t.update();update_enemies(t.f.enemy_system,t.f);move_subset(*move);}
     check(field<HumanRecordLink>(h,14).state==0&&field<EnemyRecordLink>(target,24).state==1&&t.f.random.index==0);}
    // MoveBase preserves existing velocities on arrival/teleport. Move's nonpositive speed selects the stat default.
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();auto& p=field<ContactPosition>(h,0);auto& v=field<ProjectileSpeed>(h,1);
     p.x=0;p.z=0;p.y=7;v.x=9;v.y=8;v.z=7;
     check(!move_human_base(t.host.system,&h,3,4,1,t.host)&&v.x==0.6F&&v.z==0.8F&&v.y==8&&p.x==0&&p.y==7);
     check(move_human_base(t.host.system,&h,3,4,5,t.host)&&p.x==3&&p.z==4&&v.x==0.6F&&v.z==0.8F);
     check(move_human_base(t.host.system,&h,100,200,-1,t.host)&&p.x==100&&p.z==200);
     check(!move_human(t.host.system,&h,200,200,-1,t.host)&&p.x==100&&v.x==2.23F);
     check(!move_human_base(t.host.system,&h,200,200,std::numeric_limits<float>::quiet_NaN(),t.host)&&std::isnan(v.x)&&std::isnan(v.z));}
    // Positive travel is reduced by 0.7 after dash; negative travel is not. X/Y speed survive.
    for(auto direction:{-1.0F,1.0F}){HumanFixture t;t.flat_parameters(100);t.no_attack();auto& h=t.hero();auto& v=field<ProjectileSpeed>(h,1);
     auto& link=field<HumanRecordLink>(h,14);link.state=1;link.dash_speed=2;v.x=3;v.y=4;v.z=direction;t.host.system.loop_counter=1;
     update_human_walking(t.host.system,&h,t.host);const float amount=2.23F+2.0F;
     check(v.z==(direction>0?amount*0.7F:-amount)&&v.x==3&&v.y==4&&(field<EntitySprite>(h,2).animation.clip&~3)==8);}
    {HumanFixture t;t.no_attack();t.flat_parameters(100);auto& h=t.hero();field<HumanRecordLink>(h,14).state=1;
     field<ProjectileSpeed>(h,1).z=std::numeric_limits<float>::quiet_NaN();t.host.system.loop_counter=1;
     fails([&]{update_human_walking(t.host.system,&h,t.host);});check(t.f.random.index==0);}
    // Sub characters start walking toward formation; far formation adjustment updates only Z velocity.
    {HumanFixture t;t.no_attack();t.flat_parameters(100);auto& sub=t.sub();auto& p=field<ContactPosition>(sub,0);p.x=54;p.z=90;
     update_human_waiting(t.host.system,&sub,t.host);check(field<HumanRecordLink>(sub,14).state==1);
     auto& v=field<ProjectileSpeed>(sub,1);v.x=7;t.host.system.loop_counter=1;update_human_walking(t.host.system,&sub,t.host);
     check(p.x==54&&p.z==90&&v.x==7&&v.z<0);
     p.x=64;p.z=84;update_human_walking(t.host.system,&sub,t.host);check(field<HumanRecordLink>(sub,14).state==0&&v.z==0&&v.x==7);}
    {HumanFixture t;t.no_attack();t.flat_parameters(100);auto& sub=t.sub();field<HumanRecordLink>(sub,14).state=1;
     field<ProjectileSpeed>(t.hero(),1).z=1;field<ContactPosition>(sub,0).z=157;t.host.system.loop_counter=1;
     update_human_walking(t.host.system,&sub,t.host);check(field<ProjectileSpeed>(sub,1).z==0);
     field<ProjectileSpeed>(t.hero(),1).z=-1;update_human_walking(t.host.system,&sub,t.host);check(field<ProjectileSpeed>(sub,1).z==-2.23F);}
    // Walking recovery consumes exactly one inclusive 3..5 draw and uses total maxima but base-maximum clamping.
    {HumanFixture t;t.no_attack();t.flat_parameters();auto& h=t.hero();field<HumanRecordLink>(h,14).state=1;
     auto& hp=*resolve_combat_parameter(h,0);auto& mp=*resolve_combat_parameter(h,1);hp.value=50;hp.maximum=100;hp.extra_maximum=200;
     mp.value=10;mp.maximum=30;mp.extra_maximum=100;t.f.random.values={0};update_human_walking(t.host.system,&h,t.host);
     check(hp.value==59&&mp.value==13&&t.f.random.index==1);
     t.host.gear[3]->attribute=0;t.host.system.loop_counter=30;update_human_walking(t.host.system,&h,t.host);check(t.f.random.index==1);
     t.host.gear[3]->attribute=3;update_human_walking(t.host.system,&h,t.host);check(t.f.random.index==2);}
    {DamageParameter p{90,100,0,1000};check(add_human_parameter(p,20)==10&&p.value==100);
     p.maximum=0;check(add_human_parameter(p,-200)==0&&p.value==-100);
     p.value=std::numeric_limits<std::int32_t>::max();check(add_human_parameter(p,1)==0&&p.value==std::numeric_limits<std::int32_t>::min());}
    // Burst orchestration requires an explicit Shoot producer; tests supply only its observed result/MP side effect.
    {HumanFixture t;t.flat_parameters(100);t.f.fps=10;auto& h=t.hero();auto& target=t.enemy_at();auto& link=field<HumanRecordLink>(h,14);
     t.host.parameters=[](ComponentSlots&,std::int32_t key){return key>=10?0:100;};t.host.gear[3]->burst_count=2;
     link.state=2;link.opponent=&target;link.attack_frame=10;t.host.shooter=[](ComponentSlots&,ComponentSlots&){return true;};
     field<EntitySprite>(h,2).animation.frame=7;t.update();check(t.host.shots==1&&link.state==2&&link.attack_frame==11&&field<EntitySprite>(h,2).animation.frame==0);
     t.update();check(t.host.shots==1&&link.attack_frame==12);t.update();check(t.host.shots==2&&link.state==0&&!link.opponent&&link.attack_frame==1&&link.frame==1);}
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();auto& target=t.enemy_at();auto& link=field<HumanRecordLink>(h,14);
     link.state=2;link.opponent=&target;link.attack_frame=human_attack_interval(t.host.system,&h,t.host);
     const auto count=link.attack_frame;fails([&]{t.update();});check(t.host.shots==1&&link.attack_frame==count&&link.frame==0&&t.host.system.loop_counter==0);}
    {HumanFixture t;t.flat_parameters(100);t.f.fps=10;auto& h=t.hero();auto& target=t.enemy_at();auto& link=field<HumanRecordLink>(h,14);
     link.state=2;link.opponent=&target;link.attack_frame=10;
     t.host.shooter=[&](ComponentSlots& actor,ComponentSlots&){resolve_combat_parameter(actor,1)->value=0;return false;};
     t.update();check(t.host.shots==1&&link.state==0&&!link.opponent&&link.attack_frame==1);}
    // Captured Human writes survive replacement by the required producer.
    {HumanFixture t;t.flat_parameters(100);t.f.fps=10;auto& h=t.hero();auto& target=t.enemy_at();auto& old=field<HumanRecordLink>(h,14);
     old.state=2;old.opponent=&target;old.attack_frame=10;old.frame=6;t.host.gear[3]->burst_count=2;
     t.host.parameters=[](ComponentSlots&,std::int32_t key){return key>=10?0:100;};HumanRecordLink replacement;replacement.character_id=0;replacement.state=4;
     t.host.shooter=[&](ComponentSlots& e,ComponentSlots&){e.change(14,&replacement);return true;};t.update();
     check(old.state==2&&old.attack_frame==11&&old.frame==7&&replacement.frame==0&&replacement.attack_frame==0);}
    {HumanFixture t;auto& h=t.hero();field<HumanRecordLink>(h,14).state=4;auto& p=field<ContactPosition>(h,0);p.z=1000;
     t.update();check(p.z==192);p.z=-1000;t.update();check(p.z==16);}
    {HumanFixture t;auto& h=t.hero();auto& link=field<HumanRecordLink>(h,14);link.state=4;link.invincible_time=1;
     AnimationModificationRequest r;r.type=3;add_animation_modification(h,r,t.f);auto* old=h.get(15);t.update();
     check(link.invincible_time==0&&!h.contains(15)&&t.f.returned_modifiers.back()==old);
     add_animation_modification(h,r,t.f);link.invincible_time=std::numeric_limits<std::int32_t>::min();t.update();check(link.invincible_time==std::numeric_limits<std::int32_t>::min()&&h.contains(15));}
    {HumanFixture t;auto& h=t.hero();auto& link=field<HumanRecordLink>(h,14);link.state=4;link.invincible_time=1;
     AnimationModificationRequest r;r.type=3;add_animation_modification(h,r,t.f);
     t.f.on_release_modifier=[](){throw std::runtime_error("pool return");};fails([&]{t.update();});
     check(link.invincible_time==0&&!h.contains(15)&&link.frame==0&&t.host.system.loop_counter==0);}
    {HumanFixture t;auto& h=t.hero();auto& link=field<HumanRecordLink>(h,14);link.state=5;link.frame=std::numeric_limits<std::int32_t>::max();
     link.attack_frame=std::numeric_limits<std::int32_t>::max();t.host.system.loop_counter=std::numeric_limits<std::int32_t>::max();
     t.update();check(link.frame==-1&&link.attack_frame==std::numeric_limits<std::int32_t>::min()&&t.host.system.loop_counter==-1);}
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();auto& link=field<HumanRecordLink>(h,14);link.state=3;link.dash_speed=2;
     field<ProjectileSpeed>(h,1).z=0.12F;field<ContactDirection>(h,12).locked=true;t.update();
     check(link.state==0&&link.frame==1&&link.dash_speed==0&&!field<ContactDirection>(h,12).locked);}
    {HumanFixture t;auto& h=t.hero();auto& v=field<ProjectileSpeed>(h,1);v.x=3;v.y=4;v.z=5;t.f.random.values={3};
     update_human_celebration(&h,t.host);check(v.x==3&&v.y==4&&v.z==0&&field<ContactDirection>(h,12).direction==3&&field<EntitySprite>(h,2).animation.clip==3&&t.f.random.index==1);
     field<HumanRecordLink>(h,14).frame=1;update_human_celebration(&h,t.host);check(t.f.random.index==1);
     h.remove(12);field<HumanRecordLink>(h,14).frame=0;fails([&]{update_human_celebration(&h,t.host);});check(t.f.random.index==2);}
    // Structural mutation completes this actor's ordered writes before invalidating the live group cursor.
    {HumanFixture t;t.flat_parameters(100);t.no_attack();auto& h=t.hero();field<HumanRecordLink>(h,14).state=4;bool added=false;
     t.host.on_fps=[&]{if(!added){added=true;t.sub();}};fails([&]{t.update();});
     check(field<HumanRecordLink>(h,14).frame==1&&t.host.system.loop_counter==0);}
}catch(const std::exception& e){std::cerr<<"uncaught human frame error: "<<e.what()<<'\n';return 1;}
