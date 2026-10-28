#include "core/gameplay/member_roster.hpp"
#include "core/gameplay/member_selection.hpp"
#include "core/gameplay/animation_pass.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace saga::core;
void verify(bool v,int line){if(!v){std::cerr<<"party check failed at "<<line<<'\n';std::abort();}}
#define check(...) verify((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Member {
    ComponentSlots entity{35};HumanRecordLink human;ContactPosition position;ContactDirection direction;
    ProjectileSpeed speed;EntitySprite sprite;EntityPlayback playback;CombatParameterLinks parameters;
    DamageParameter hp{7,8,9,10},energy{11,12,13,14};DamageParameterMap values{{0,&hp},{1,&energy}};
    std::int32_t sorties=0;
    Member(){parameters.values=&values;playback.state={1,-1};sprite.animation={1,99,12};
        human.character_id=3;human.state=1;human.attack_frame=22;human.invincible_time=77;human.opponent=&entity;
        direction.direction=3;speed.z=4;position.y=25;position.offset_x=19;
        entity.add(0,&position).add(1,&speed).add(2,&sprite).add(10,&playback).add(12,&direction)
              .add(14,&human).add(23,&parameters);}
};
struct Host final:MemberInitializationHost,HumanStateHost,MemberParameterHost {
    MemberRoster roster;Member a,b;std::vector<std::string> calls;EquipmentParameters gear;
    std::int32_t left=100,top=200;std::array<std::int32_t,2> base{100,30};
    bool missing_system=false,missing_record=false,cut_in=false;
    std::function<void(ComponentSlots&,std::int32_t)> on_total;
    std::function<void()> on_origin,on_animation,on_record;
    Host(){gear.values=std::make_shared<std::vector<EventInstruction>>(std::vector<EventInstruction>{
        std::make_shared<EventIntegers>(EventIntegers{2,2}),std::make_shared<EventIntegers>(EventIntegers{3,3})});
        a.human.formation_x=36;a.human.formation_z=72;b.human.formation_x=0;b.human.formation_z=0;
        add_member(roster,&a.entity);add_member(roster,&b.entity);}
    std::int32_t formation_left()override{calls.push_back("left");return left;}
    std::int32_t formation_top()override{calls.push_back("top");if(on_origin)on_origin();return top;}
    std::int32_t total_parameter(ComponentSlots& e,std::int32_t k)override{
        calls.push_back("total"+std::to_string(k));if(on_total)on_total(e,k);return member_total_parameter(*this,k);}
    std::int32_t base_parameter(std::int32_t k)override{return base.at(static_cast<std::size_t>(k));}
    const EquipmentParameters& equipment(EquipmentSlot)override{return gear;}
    void change_animation(ComponentSlots& e,std::int32_t clip,bool reset)override{
        calls.push_back("animation");if(on_animation)on_animation();saga::core::change_animation(live_animation(e),clip,reset);}
    HumanStateHost* human_system()override{calls.push_back("system");return missing_system?nullptr:this;}
    bool playing_cut_in(ComponentSlots&)override{calls.push_back("cut-in");return cut_in;}
    std::int32_t attack_interval(ComponentSlots&)override{calls.push_back("interval");return 10;}
    std::int32_t* sortie_count(HumanRecordLink& h)override{
        calls.push_back("sortie");if(on_record)on_record();return missing_record?nullptr:(&h==&a.human?&a.sorties:&b.sorties);}
};
int main()try{
    {MemberRoster r;ComponentSlots player(35),other(35);Component tag;player.add(30,&tag);
     check(r.formation->size()==16&&(*r.formation)[10]==-1);
     add_member(r,&player);add_member(r,&player);add_member(r,&other);add_member(r,&other);
     check(r.members->entities()==std::vector<ComponentSlots*>({&player,&other}));
     check(r.sub_members->entities()==std::vector<ComponentSlots*>({&other}));
     player.remove(30);add_member(r,&player);check(r.sub_members->entities().back()==&player);
     other.add(30,&tag);add_member(r,&other);check(r.sub_members->contains(&other)); // No automatic reconciliation.
     r.members->append(&other);remove_member(r,&other);check(r.members->contains(&other)&&!r.sub_members->contains(&other));
     const auto v=r.members->version();remove_member(r,nullptr);check(v==r.members->version());
     fails([&]{add_member(r,nullptr);});check(r.members->contains(nullptr)); // Appended before null guard.
     remove_member(r,nullptr);check(!r.members->contains(nullptr));
     r.sub_members.reset();fails([&]{remove_member(r,&player);});check(!r.members->contains(&player));
     add_member(r,&other); // Player marker skips the absent sub-list.
     r.members.reset();fails([&]{add_member(r,&other);});}
    {MemberRoster r;std::vector<std::int32_t> queries;
     auto main=[&](std::int32_t id){queries.push_back(id);return id==5;};
     check(set_formation_character(r,10,5,main));check(!set_formation_character(r,10,5,main));
     check(!set_formation_character(r,10,-1,main)&&queries==std::vector<std::int32_t>{5});
     check(set_formation_empty_slot(r,7,main));check(set_formation_empty_slot(r,7,main)); // Duplicate IDs allowed.
     check(formation_number(r,7)==0&&formation_number(r,-1)==-1&&formation_number(r,99)==-1);
     check(set_formation_character(r,0,-1,main)&&queries.back()==7);
     check(set_formation_character(r,0,-1,{})); // Clearing an empty slot never resolves data.
     check(set_formation_character(r,0,-2,{})); // Only -1 is the empty sentinel.
     swap_formation_slots(r,0,10);check((*r.formation)[0]==5&&(*r.formation)[10]==-2);
     auto saved=*r.formation;swap_formation_slots(r,-1,0);swap_formation_slots(r,0,16);swap_formation_slots(r,0,0);
     check(*r.formation==saved);check(!set_formation_character(r,16,1,{})&&!set_formation_character(r,-1,1,{}));
     r.formation=std::make_shared<std::vector<std::int32_t>>(2,8);check(!set_formation_empty_slot(r,9,{}));
     auto old=r.formation;check(set_formation_character(r,0,-1,[&](std::int32_t){r.formation=std::make_shared<std::vector<std::int32_t>>(2,77);return false;}));
     check((*old)[0]==8&&(*r.formation)[0]==-1&&(*r.formation)[1]==77);
     fails([&]{set_formation_character(r,1,-1,[&](std::int32_t){r.formation.reset();return false;});});
     check(!set_formation_empty_slot(r,-1,{})&&formation_number(r,-1)==-1);
     swap_formation_slots(r,0,0);check(!set_formation_character(r,-1,5,{}));
     fails([&]{formation_number(r,5);});}
    {Host h;initialize_members(h.roster,h);
     check(h.calls==std::vector<std::string>({"left","top","total0","total1","animation","system","cut-in","animation","sortie",
                                            "total0","total1","animation","system","cut-in","animation","sortie"}));
     check(h.a.position.x==136&&h.a.position.y==0&&h.a.position.z==272&&h.a.position.offset_x==19);
     check(h.a.hp.maximum==108&&h.a.hp.value==108&&h.a.hp.extra==9&&h.a.hp.extra_maximum==10);
     check(h.a.energy.maximum==42&&h.a.energy.value==42&&h.a.energy.extra==13);
     check(h.a.human.state==0&&h.a.human.frame==0&&h.a.human.attack_frame==0&&!h.a.human.opponent&&h.a.human.invincible_time==0);
     check(h.a.sprite.animation.clip==36&&h.a.sprite.animation.frame==0&&h.a.speed.z==0&&h.a.sorties==1);
     check(front_line_human(&h.roster.members->entities())==&h.b.entity);
     remove_member(h.roster,&h.b.entity);check(front_line_human(&h.roster.members->entities())==&h.a.entity);}
    {Host h;h.base={-8,-12};h.a.sorties=std::numeric_limits<std::int32_t>::max();
     h.left=std::numeric_limits<std::int32_t>::max();h.a.human.formation_x=1;initialize_members(h.roster,h);
     check(h.a.position.x==static_cast<float>(std::numeric_limits<std::int32_t>::min()));
     check(h.a.hp.maximum==0&&h.a.hp.value==7&&h.a.energy.maximum==0&&h.a.energy.value==11);
     check(h.a.sorties==std::numeric_limits<std::int32_t>::min());}
    {DamageParameter p{7,-3,99,100};fill_parameter(p);check(p.value==-3&&p.extra==99&&p.extra_maximum==100);}
    {Host h;h.a.values[0]=nullptr;fails([&]{initialize_members(h.roster,h);});
     check(h.a.position.x==136&&h.calls.back()=="total0"&&h.a.energy.maximum==12);}
    {Host h;h.on_total=[&](ComponentSlots&,std::int32_t k){if(k==1)throw std::runtime_error("total");};
     fails([&]{initialize_members(h.roster,h);});check(h.a.hp.maximum==108&&h.a.hp.value==7&&h.a.energy.maximum==12);}
    {Host h;DamageParameter replacement{4,5,6,7};
     h.on_total=[&](ComponentSlots&,std::int32_t k){if(k==0)h.a.values[0]=&replacement;};
     initialize_members(h.roster,h);check(h.a.hp.maximum==108&&h.a.hp.value==7&&replacement.maximum==5&&replacement.value==5);}
    {Host h;h.missing_system=true;fails([&]{initialize_members(h.roster,h);});
     check(h.a.hp.value==108&&h.a.sprite.animation.clip==36&&h.a.human.state==1&&h.a.sorties==0);}
    {Host h;h.missing_record=true;fails([&]{initialize_members(h.roster,h);});
     check(h.a.human.state==0&&h.a.human.attack_frame==0&&!h.a.human.opponent&&h.a.human.invincible_time==77);}
    {Host h;h.roster.members->append(&h.a.entity);initialize_members(h.roster,h);check(h.a.sorties==2&&h.b.sorties==1);}
    {Host h;h.on_total=[&](ComponentSlots&,std::int32_t k){if(k==0)remove_member(h.roster,&h.b.entity);};
     fails([&]{initialize_members(h.roster,h);});check(h.a.sorties==1&&h.b.sorties==0);}
    {Host h;auto captured=h.roster.members;h.on_total=[&](ComponentSlots&,std::int32_t){h.roster.members=std::make_shared<PartyMembers>();};
     initialize_members(h.roster,h);check(h.a.sorties==1&&h.b.sorties==1&&h.roster.members->entities().empty()&&captured->entities().size()==2);}
    {Host h;h.on_origin=[&]{h.roster.members=std::make_shared<PartyMembers>();};initialize_members(h.roster,h);
     check(h.a.sorties==0&&h.calls==std::vector<std::string>({"left","top"}));}
    {Host h;h.roster.members->clear();initialize_members(h.roster,h);check(h.calls.size()==2);h.roster.members.reset();
     fails([&]{initialize_members(h.roster,h);});}
}catch(const std::exception& e){std::cerr<<"uncaught party error: "<<e.what()<<'\n';return 1;}
