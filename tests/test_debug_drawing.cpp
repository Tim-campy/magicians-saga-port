#include "core/gameplay/debug_drawing.hpp"
#include "core/gameplay/ecs_combat.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace saga::core;
namespace {
void check(bool value){static int count=0;++count;if(!value){std::cerr<<"debug drawing assertion "<<count<<'\n';std::abort();}}
template<class F>void fails(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught);}
DebugText label(const std::u16string& value){return std::make_shared<const std::u16string>(value);}
DebugNames names(std::size_t count,char16_t prefix){auto rows=std::make_shared<std::vector<DebugText>>();
    for(std::size_t i=0;i<count;++i)rows->push_back(label(std::u16string{prefix,static_cast<char16_t>(u'0'+i)}));return rows;}
struct Tag final:Component {std::int32_t id=0;};
std::int32_t identity(const ComponentSlots& e){return dynamic_cast<Tag*>(e.get(34))->id;}
bool present(ComponentSlots* e){return e!=nullptr;}
struct Actor {
    ComponentSlots entity{35};Tag tag;Component speed;ContactPosition position;
    HumanRecordLink human;EnemyRecordLink enemy;ProjectileCollision collision;
    DamageParameter hp,mp;DamageParameterMap parameters;CombatParameterLinks links;
    explicit Actor(std::int32_t id){tag.id=id;entity.add(34,&tag);entity.add(0,&position);entity.add(1,&speed);
        entity.add(14,&human);entity.add(24,&enemy);entity.add(32,&collision);
        hp.value=9;hp.extra=1;hp.maximum=18;hp.extra_maximum=2;mp.value=5;mp.maximum=9;
        parameters={{0,&hp},{1,&mp}};links.values=&parameters;entity.add(23,&links);
        position.x=100;position.y=10;position.z=20;human.state=2;human.attack_frame=12;enemy.state=1;enemy.level=1;
    }
};
struct TextDraw {DebugText value;float x,y;std::int32_t anchor;};
struct Host final:DebugDrawingHost {
    std::vector<std::string> calls;std::vector<TextDraw> texts;
    std::vector<std::array<float,5>> lines;std::vector<std::array<float,4>> rectangles;
    std::vector<std::array<float,7>> arcs;std::vector<std::array<float,3>> coordinates;
    std::vector<const ContactPosition*> projected;std::vector<EventIntegers> filters;
    std::vector<std::int32_t> total_keys;std::vector<DebugPalette> palettes;
    std::vector<DebugSystemContext> member_queries,human_queries;
    std::vector<EquipmentSlot> equipment_reads;std::function<void(const std::string&)> callback;
    std::string failure;bool controls=true;int font_depth=0;std::array<std::int32_t,3> mode{{0,255,0}};
    StageGeometry map_value{24,24,50};StageGeometry* map=&map_value;
    StageQuest quest_value;StageQuest* quest=&quest_value;
    Actor hero{1};ComponentSlots* selected_hero=&hero.entity;
    int member_token=1,human_token=2;DebugSystemContext member=&member_token,system=&human_token;
    EntitySubset stored{present,identity},humans{present,identity},collisions{present,identity},enemies{present,identity};
    EventInstruction ids=std::make_shared<EventIntegers>(EventIntegers{0,1,2});
    DebugNames parameter_names=names(13,u'p'),human_names=names(7,u'h'),enemy_names=names(6,u'e');
    std::array<DebugText,3> equipment_names{{label(u"w"),label(u"t"),label(u"c")}};
    DebugText actor_name=label(u"actor");EnemyRecord enemy_record;const EnemyRecord* selected_enemy=&enemy_record;
    DebugText attack_pattern=label(u"{0}|{1}|{2}");
    std::int32_t interval_value=4,wait_value=3,count_value=2,distance_value=2;
    GaugePoint point{30,40},endpoint{50,60};
    Host(){quest_value.distance=4;enemy_record.experience=EventIntegers{10,110};}
    void step(const std::string& name){calls.push_back(name);if(callback)callback(name);if(failure==name)throw std::runtime_error(name);}
    bool debug_controls()override{step("controls");return controls;}
    StageGeometry* debug_map()override{auto* result=map;step("map");return result;}
    std::int32_t debug_game_width()override{step("width");return 240;}
    std::int32_t debug_game_height()override{step("height");return 320;}
    void debug_validate_quest_form()override{step("form");}
    EntitySubset* debug_subset(const EventIntegers& all)override{
        filters.push_back(all);step("subset"+std::to_string(all.at(0)));
        if(all==EventIntegers{14})return &humans;if(all==EventIntegers{32})return &collisions;if(all==EventIntegers{24})return &enemies;
        throw std::invalid_argument("unexpected filter");
    }
    GaugePoint debug_project(const ContactPosition* p)override{projected.push_back(p);step("project");return p?point:GaugePoint{};}
    GaugePoint debug_project(float x,float y,float z)override{coordinates.push_back({x,y,z});step("xyz");return endpoint;}
    StageQuest* debug_quest()override{auto* result=quest;step("quest");return result;}
    DebugSystemContext debug_member_system()override{auto result=member;step("member");return result;}
    ComponentSlots* debug_hero(DebugSystemContext c)override{check(c==&member_token);member_queries.push_back(c);auto* result=selected_hero;step("hero");return result;}
    std::int32_t debug_total_parameter(DebugSystemContext c,ComponentSlots& e,std::int32_t key)override{
        check(c==&member_token && &e==selected_hero);member_queries.push_back(c);total_keys.push_back(key);step("total");return 100+key;
    }
    DebugSystemContext debug_human_system()override{auto result=system;step("system");return result;}
    std::int32_t debug_attack_interval(DebugSystemContext c,ComponentSlots&)override{check(c==&human_token);human_queries.push_back(c);step("interval");return interval_value;}
    std::int32_t debug_burst_interval(DebugSystemContext c,ComponentSlots&)override{check(c==&human_token);human_queries.push_back(c);step("wait");return wait_value;}
    std::int32_t debug_burst_count(DebugSystemContext c,ComponentSlots&)override{check(c==&human_token);human_queries.push_back(c);step("count");return count_value;}
    std::int32_t debug_distance(DebugSystemContext c,ComponentSlots&)override{check(c==&human_token);human_queries.push_back(c);step("distance");return distance_value;}
    std::size_t equipment_index(EquipmentSlot slot){if(slot==EquipmentSlot::weapon)return 0;if(slot==EquipmentSlot::hat)return 1;check(slot==EquipmentSlot::cloth);return 2;}
    DebugText debug_equipment_name(ComponentSlots& e,EquipmentSlot slot)override{
        check(&e==selected_hero);equipment_reads.push_back(slot);const auto result=equipment_names[equipment_index(slot)];step("equipment_name");return result;
    }
    std::int32_t debug_equipment_level(ComponentSlots& e,EquipmentSlot slot)override{check(&e==selected_hero);step("equipment_level");return static_cast<std::int32_t>(7+equipment_index(slot));}
    DebugText debug_character_name(ComponentSlots&)override{auto result=actor_name;step("character_name");return result;}
    const EnemyRecord* debug_enemy_record(ComponentSlots&)override{auto* result=selected_enemy;step("enemy_record");return result;}
    EventInstruction debug_parameter_ids()override{auto result=ids;step("ids");return result;}
    DebugNames debug_parameter_names()override{auto result=parameter_names;step("parameter_names");return result;}
    DebugNames debug_human_state_names()override{auto result=human_names;step("human_names");return result;}
    DebugNames debug_enemy_state_names()override{auto result=enemy_names;step("enemy_names");return result;}
    DebugText debug_text(DebugTextKey key)override{
        step("literal");switch(key){
        case DebugTextKey::level_separator:return label(u"#");case DebugTextKey::parameter_separator:return label(u"=");
        case DebugTextKey::value_separator:return label(u":");case DebugTextKey::attack_format:return attack_pattern;
        case DebugTextKey::percent_suffix:return label(u"pct");case DebugTextKey::hp_prefix:return label(u"H=");
        case DebugTextKey::experience_prefix:return label(u"E=");}throw std::invalid_argument("bad text key");
    }
    std::int32_t debug_palette(DebugPalette palette)override{palettes.push_back(palette);step("palette");return palette==DebugPalette::white?11:22;}
    void debug_set_color(std::int32_t color)override{check(color==11||color==22);step("color");}
    void debug_push_font(std::int32_t size)override{check(size==6);step("push");++font_depth;}
    void debug_pop_font()override{step("pop");--font_depth;}
    void debug_render_mode(std::int32_t m,std::int32_t a,std::int32_t b)override{step("mode");mode={m,a,b};}
    void debug_reset_render_mode()override{step("reset");mode={0,255,0};}
    void debug_draw_text(DebugText value,float x,float y,std::int32_t anchor)override{step("text");texts.push_back({std::move(value),x,y,anchor});}
    void debug_draw_line(float x,float y,float x2,float y2,float width)override{step("line");lines.push_back({x,y,x2,y2,width});}
    void debug_fill_rect(float x,float y,float width,float height)override{step("rect");rectangles.push_back({x,y,width,height});}
    void debug_draw_arc(float x,float y,float width,float height,float start,float sweep,float line_width)override{
        step("arc");arcs.push_back({x,y,width,height,start,sweep,line_width});
    }
};
std::size_t at(const Host& host,const std::string& name){return static_cast<std::size_t>(std::find(host.calls.begin(),host.calls.end(),name)-host.calls.begin());}
std::size_t count(const Host& host,const std::string& name){return static_cast<std::size_t>(std::count(host.calls.begin(),host.calls.end(),name));}
void gates_and_dispatch(){
    Host host;DebugVisualizationState state;state.flags=511;host.controls=false;
    draw_debug_dynamic(state,host);check(host.calls==std::vector<std::string>{"controls"});
    host.controls=true;host.calls.clear();state.flags=0;draw_debug_dynamic(state,host);check(host.calls==std::vector<std::string>{"controls"});
    host.calls.clear();state.flags=1|2|4|16|32|512;draw_debug_dynamic(state,host);check(host.calls==std::vector<std::string>{"controls"});
    draw_debug_visualize_system(host);draw_debug_node_cost(host);draw_debug_node_connect(host);draw_debug_map_chip_info(host);
    draw_debug_town_area_info(host);draw_debug_node_info(host,-1);check(host.calls==std::vector<std::string>{"controls"});
    host.calls.clear();state.flags=8|64|128|256;state.subset=&host.stored;draw_debug_dynamic(state,host);
    check(at(host,"map")<at(host,"quest")&&at(host,"quest")<at(host,"subset32")&&at(host,"subset32")<at(host,"subset24"));
    check(host.filters==std::vector<EventIntegers>({{14},{32},{24}}));check(host.font_depth==0&&count(host,"push")==2&&count(host,"pop")==2);
    check(host.lines.size()==11&&count(host,"reset")==1);
    Host changing;DebugVisualizationState fresh{&changing.stored,8|64|256};
    changing.callback=[&](const std::string& s){if(s=="pop")fresh.flags=128;};draw_debug_dynamic(fresh,changing);
    check(changing.filters==std::vector<EventIntegers>{{32}}&&count(changing,"quest")==0);
}
void human_layout_and_live_inputs(){
    Host host;Actor actor(2);host.stored.add(&actor.entity);DebugVisualizationState state{&host.stored,8};
    host.callback=[&](const std::string& name){if(name=="equipment_level")host.equipment_names[0]=label(u"changed");};
    draw_debug_humans(state,host);check(host.font_depth==0&&host.texts.size()==9&&host.lines.size()==1);
    check(host.calls[0]=="map"&&host.calls[1]=="width"&&host.calls[2]=="height"&&host.calls[3]=="form");
    check(*host.texts[0].value==u"w#7"&&*host.texts[1].value==u"t#8"&&*host.texts[2].value==u"c#9");
    for(std::size_t i=0;i<6;++i)check(host.texts[i].anchor==17&&host.texts[i].x==2);
    check(host.texts[0].y==110&&host.texts[1].y==117&&host.texts[2].y==124);
    check(*host.texts[3].value==u"p0=10:20"&&host.texts[3].y==132);
    check(*host.texts[4].value==u"p1=5:9"&&*host.texts[5].value==u"p2=102");
    check(host.total_keys==std::vector<std::int32_t>({0,1,2}));
    check(host.texts[6].value==host.actor_name&&host.texts[6].x==30&&host.texts[6].y==51&&host.texts[6].anchor==2);
    check(*host.texts[7].value==u"h2"&&host.texts[7].y==58&&*host.texts[8].value==u"12|4|10"&&host.texts[8].y==65);
    check(host.coordinates==std::vector<std::array<float,3>>{{100,10,-36}});
    check(host.lines[0]==std::array<float,5>{30,40,50,60,1});
    check(at(host,"interval")<at(host,"wait")&&at(host,"wait")<at(host,"count")&&at(host,"count")<at(host,"distance"));
    Host captured;captured.stored.add(&actor.entity);DebugVisualizationState captured_state{&captured.stored,8};
    captured.callback=[&](const std::string& name){if(name=="member")captured.member=nullptr;if(name=="system")captured.system=nullptr;};
    draw_debug_humans(captured_state,captured);
    check(captured.member_queries.size()==4&&captured.human_queries.size()==4&&captured.font_depth==0);
    Host replaced;DebugVisualizationState empty{&replaced.stored,8};int reads=0;
    replaced.callback=[&](const std::string& name){if(name=="ids"&&++reads==1)replaced.ids=std::make_shared<EventIntegers>(EventIntegers{2});};
    draw_debug_humans(empty,replaced);check(replaced.total_keys==std::vector<std::int32_t>{2}&&replaced.texts.size()==4);
    check(*replaced.texts.back().value==u"p2=102");
    Host overflow;overflow.hero.hp.value=std::numeric_limits<std::int32_t>::max();overflow.hero.hp.extra=1;
    DebugVisualizationState overflow_state{&overflow.stored,8};draw_debug_humans(overflow_state,overflow);
    check(*overflow.texts[3].value==u"p0=-2147483648:20");
    Host arithmetic;actor.human.attack_frame=std::numeric_limits<std::int32_t>::min();arithmetic.stored.add(&actor.entity);
    arithmetic.wait_value=2;arithmetic.count_value=std::numeric_limits<std::int32_t>::max();arithmetic.distance_value=std::numeric_limits<std::int32_t>::min();
    DebugVisualizationState arithmetic_state{&arithmetic.stored,8};draw_debug_humans(arithmetic_state,arithmetic);
    check(*arithmetic.texts.back().value==u"-2147483648|4|2"&&arithmetic.coordinates.back()[2]==20);
    actor.human.attack_frame=12;
    Host no_system;no_system.system=nullptr;DebugVisualizationState no_actors{&no_system.stored,8};
    draw_debug_humans(no_actors,no_system);check(no_system.font_depth==0); // Empty group never requires the captured HumanSystem.
    no_system.stored.add(&actor.entity);fails([&]{draw_debug_humans(no_actors,no_system);});check(no_system.font_depth==1);
}
void human_failures(){
    for(const auto& failure:std::vector<std::string>{"map","width","height","form","push","color","member","hero","equipment_name","equipment_level","ids","total","parameter_names","system"}){
        Host host;Actor actor(2);host.stored.add(&actor.entity);DebugVisualizationState state{&host.stored,8};host.failure=failure;
        fails([&]{draw_debug_humans(state,host);});check(count(host,"pop")==0);
        if(at(host,"push")<at(host,failure))check(host.font_depth==1);
    }
    Host host;host.ids.reset();DebugVisualizationState state{&host.stored,8};fails([&]{draw_debug_humans(state,host);});
    check(host.texts.size()==3&&host.font_depth==1);
    Host missing;missing.member=nullptr;DebugVisualizationState second{&missing.stored,8};fails([&]{draw_debug_humans(second,missing);});
    check(count(missing,"hero")==0&&missing.font_depth==1);
    Host invalid;Actor actor(2);actor.human.state=-1;invalid.stored.add(&actor.entity);DebugVisualizationState third{&invalid.stored,8};
    fails([&]{draw_debug_humans(third,invalid);});check(invalid.texts.size()==7&&invalid.font_depth==1);
    Host null_format;null_format.attack_pattern.reset();null_format.stored.add(&actor.entity);actor.human.state=2;
    DebugVisualizationState null_format_state{&null_format.stored,8};fails([&]{draw_debug_humans(null_format_state,null_format);});
    check(null_format.texts.size()==8&&null_format.font_depth==1);
    // A callback invalidating the live group is detected on the next cursor advance; font is not restored.
    Host changed;Actor a(2),b(3);changed.stored.add(&a.entity);DebugVisualizationState fourth{&changed.stored,8};
    changed.callback=[&](const std::string& event){if(event=="line")changed.stored.add(&b.entity);};
    fails([&]{draw_debug_humans(fourth,changed);});check(changed.lines.size()==1&&changed.font_depth==1);
}
void quest_grid_and_fresh_opponent(){
    Host host;Actor actor(2),first(3),second(4);actor.human.opponent=&first.entity;host.humans.add(&actor.entity);
    StageQuest replacement;replacement.distance=100;host.callback=[&](const std::string& name){
        if(name=="mode")host.quest=&replacement;
        if(name=="project"&&host.projected.size()==1)actor.human.opponent=&second.entity;
    };
    draw_debug_quest(host);check(host.lines.size()==12&&host.texts.size()==11&&host.coordinates.size()==22);
    check(host.coordinates[0]==std::array<float,3>{-86,0,84}&&host.coordinates[1]==std::array<float,3>{124,0,84});
    check(host.coordinates[20][2]==12&&host.coordinates[21][2]==12&&count(host,"map")==33);
    check(host.texts.front().x==45&&host.texts.front().y==55&&host.texts.front().anchor==36);
    check(*host.texts.front().value==u"0pct"&&*host.texts.back().value==u"100pct");
    check(host.projected==std::vector<const ContactPosition*>({&actor.position,&second.position}));
    check(host.lines.back()==std::array<float,5>{30,40,30,40,1}&&host.mode==std::array<std::int32_t,3>{0,255,0});
    Host fresh;StageGeometry other{70,25,50};int reads=0;
    fresh.callback=[&](const std::string& name){if(name=="map"&&++reads==1)fresh.map=&other;};draw_debug_quest(fresh);
    check(fresh.coordinates[0]==std::array<float,3>{-40,0,84}); // First stepZ captured before the map callback, then fresh X/Z.
    Host wrapped;wrapped.map_value.step_x=std::numeric_limits<std::int32_t>::min();draw_debug_quest(wrapped);
    check(wrapped.coordinates[0][0]>0&&wrapped.coordinates[1][0]<0);
    Host null_quest;null_quest.quest=nullptr;fails([&]{draw_debug_quest(null_quest);});
    check(null_quest.mode==std::array<std::int32_t,3>{1,255,128}&&count(null_quest,"reset")==0);
    Host failing;failing.failure="text";fails([&]{draw_debug_quest(failing);});
    check(failing.lines.size()==1&&count(failing,"reset")==0&&failing.mode==std::array<std::int32_t,3>{1,255,128});
}
void collisions_and_wrap(){
    Host host;Actor actor(2);actor.collision.offset_x=-3;actor.collision.offset_y=7;actor.collision.offset_z=-5;actor.collision.radius=9;
    host.collisions.add(&actor.entity);ProjectileCollision replacement;replacement.radius=std::numeric_limits<std::int32_t>::max();
    host.callback=[&](const std::string& event){
        if(event=="xyz"){actor.entity.remove(32);actor.entity.add(32,&replacement);}
        if(event=="rect")replacement.radius=3;
    };
    draw_debug_collisions(host);check(host.filters==std::vector<EventIntegers>{{32}});
    check(host.coordinates==std::vector<std::array<float,3>>{{97,17,15}});
    check(host.rectangles==std::vector<std::array<float,4>>{{50,60,1,1}});
    check(host.arcs==std::vector<std::array<float,7>>{{51,61,-2,-2,0,360,1}});
    check(count(host,"push")==0&&count(host,"mode")==0&&host.palettes==std::vector<DebugPalette>{DebugPalette::collision});
    Host missing;Actor no_position(2);no_position.entity.remove(0);missing.collisions.add(&no_position.entity);
    fails([&]{draw_debug_collisions(missing);});check(missing.rectangles.empty()&&missing.arcs.empty());
    Host failure;Actor last(2);failure.collisions.add(&last.entity);failure.failure="arc";
    fails([&]{draw_debug_collisions(failure);});check(failure.rectangles.size()==1&&failure.arcs.empty());
}
void enemies_capture_and_failures(){
    Host host;Actor actor(2);host.enemies.add(&actor.entity);DamageParameter replacement;replacement.value=500;
    EnemyRecordLink fresh;fresh.state=5;fresh.level=1000;
    host.callback=[&](const std::string& event){
        if(event=="text"&&host.texts.empty()){actor.parameters[0]=&replacement;actor.hp.value=19;}
        if(event=="enemy_record"){actor.entity.remove(24);actor.entity.add(24,&fresh);}
    };
    draw_debug_enemies(host);check(host.font_depth==0&&host.filters==std::vector<EventIntegers>{{24}}&&host.texts.size()==3);
    check(*host.texts[0].value==u"e1"&&*host.texts[1].value==u"H=20:20"&&*host.texts[2].value==u"E=110");
    for(std::size_t i=0;i<3;++i)check(host.texts[i].x==30&&host.texts[i].y==55+7*static_cast<float>(i)&&host.texts[i].anchor==2);
    Host absent_position;Actor without_position(2);without_position.entity.remove(0);absent_position.enemies.add(&without_position.entity);
    draw_debug_enemies(absent_position);check(absent_position.projected==std::vector<const ContactPosition*>{nullptr});
    check(absent_position.texts[0].x==0&&absent_position.texts[0].y==15);
    Host missing;Actor hp_missing(2);hp_missing.parameters[0]=nullptr;missing.enemies.add(&hp_missing.entity);
    fails([&]{draw_debug_enemies(missing);});check(missing.texts.size()==1&&missing.font_depth==1);
    Host bad_state;Actor state_missing(2);state_missing.enemy.state=6;bad_state.enemies.add(&state_missing.entity);
    fails([&]{draw_debug_enemies(bad_state);});check(bad_state.texts.empty()&&bad_state.font_depth==1);
    Host data;Actor data_missing(2);data.enemies.add(&data_missing.entity);data.selected_enemy=nullptr;
    fails([&]{draw_debug_enemies(data);});check(data.texts.size()==2&&data.font_depth==1);
}
}
int main(){gates_and_dispatch();human_layout_and_live_inputs();human_failures();quest_grid_and_fresh_opponent();collisions_and_wrap();enemies_capture_and_failures();
    std::cout<<"debug drawing contracts ok\n";}
