#include "combat_scene_fixture.hpp"

int main()try{
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto old=h.state.expired;old->add(&t.hero());h.state.loop_counter=9;
     h.null_subset=true;initialize_emitters(h.state,&h);check(!h.state.subset&&h.state.expired!=old&&h.state.expired->size()==0&&h.state.loop_counter==9);
     auto current=h.state.expired;fails([&]{initialize_emitters(h.state,nullptr);});check(h.state.expired==current);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto* old=h.state.subset;auto queue=h.state.expired;
     h.on_subset=[]{throw std::runtime_error("subset");};fails([&]{initialize_emitters(h.state,&h);});check(h.state.subset==old&&h.state.expired==queue);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);h.state.subset=nullptr;fails([&]{h.update();});check(h.state.loop_counter==1);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);h.state.expired.reset();fails([&]{h.update();});check(h.state.loop_counter==1);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);h.state.loop_counter=std::numeric_limits<std::int32_t>::max();h.update();check(h.state.loop_counter==-1);
     h.state.loop_counter=std::numeric_limits<std::int32_t>::max()-1;h.update();check(h.state.loop_counter==0);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.bullet();auto& speed=field<ProjectileSpeed>(e,1);speed.x=10;speed.y=-20;speed.z=30;
     s.pools.records->at(0)->trail_image=321;s.pools.records->at(0)->flags=1;h.update();check(h.particles().empty());h.update();
     auto particles=h.particles();check(particles.size()==1);auto& p=*particles[0];auto& m=field<ProjectileModification>(p,15);
     check(field<ContactPosition>(p,0).x==10&&field<ProjectileSpeed>(p,1).x==1&&field<ProjectileSpeed>(p,1).y==-2&&field<ProjectileSpeed>(p,1).z==3);
     check(field<EntityImage>(p,7).arrays.textures->at(0)==321&&field<EntityPlayback>(p,10).state.return_clip==-1);
     check(m.type==7&&m.angle==45&&m.loop&&!m.destroy&&m.scale_x==2&&m.scale_y==3&&m.x==4&&m.y==5&&m.alpha==255);
     check(field<DefeatColor>(p,6).values[3]==3192&&field<EmitterComponent>(e,31).frame==2&&h.state.loop_counter==2);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.emitter(1,2,1);h.update();check(h.destroyed.empty());h.update();
     check(t.f.nodes.back().effect.value==0&&t.f.nodes.back().image.arrays.textures->at(0)==27);h.update();h.update();
     check(h.destroyed==std::vector<ComponentSlots*>{&e}&&t.f.node(e).lifetime.destroyed&&h.state.expired->size()==0);}
    for(const auto type:{0,2,3,-1}){HumanFixture t;Shooter s(t);EmitterServices h(s);h.emitter(type);const auto count=t.f.nodes.size();h.update();
     check(t.f.nodes.size()==count&&h.destroyed.empty());}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.emitter(2);e.remove(0);fails([&]{h.update();});check(field<EmitterComponent>(e,31).frame==1);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.emitter(1);field<EmitterComponent>(e,31).category=7;e.remove(0);h.update();check(h.destroyed.empty());}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.emitter(1,0);h.update();check(t.f.nodes.back().entity.contains(31));
     field<EmitterComponent>(e,31).frame=-1;h.update();check(t.f.nodes.back().entity.contains(26));}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.emitter(1,-1);field<EmitterComponent>(e,31).frame=std::numeric_limits<std::int32_t>::max();
     h.update();check(field<EmitterComponent>(e,31).frame==std::numeric_limits<std::int32_t>::min());}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.bullet();e.remove(15);h.update();const auto count=t.f.nodes.size();
     fails([&]{h.update();});check(t.f.nodes.size()==count+1);auto& p=t.f.nodes.back().entity;
     check(p.contains(10)&&p.contains(15)&&p.contains(22)&&!p.contains(6)&&field<EmitterComponent>(e,31).frame==2);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.bullet();h.missing_particle_world=true;field<EmitterComponent>(e,31).frame=1;
     e.remove(0);fails([&]{h.update();});check(h.pools.clip_queries==0);}
    // Expiry is queued before category dispatch and destroyed only after the whole group pass.
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& a=h.emitter(1,1,0);auto& b=h.emitter(1,1,0);bool finished=false;
     h.on_destroy=[&](ComponentSlots& e){check(field<EmitterComponent>(b,31).frame==1);if(&e==&a)finished=true;};
     h.update();check(finished&&h.destroyed==std::vector<ComponentSlots*>{&a,&b});}
    // Failed drain retains the persistent list; retry adds another entry and replays the earlier one.
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.emitter(1,1,0);h.on_destroy=[](ComponentSlots&){throw std::runtime_error("destroy");};
     fails([&]{h.update();});check(h.state.expired->size()==1&&!t.f.node(e).lifetime.destroyed);
     h.on_destroy={};h.update();check(h.destroyed.size()==3&&h.state.expired->size()==0&&t.f.node(e).lifetime.destroyed);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.emitter(1,1,0);h.on_destroy=[&](ComponentSlots&){h.state.expired->add(&e);};
     fails([&]{h.update();});check(h.state.expired->size()==2&&t.f.node(e).lifetime.destroyed);}
    {HumanFixture t;Shooter s(t);EmitterServices h(s);h.emitter(1,1,0);auto original=h.state.expired;
     h.on_destroy=[&](ComponentSlots&){h.state.expired=std::make_shared<EmitterExpiryQueue>();h.state.expired->add(&t.hero());};
     h.update();check(original->size()==1&&h.state.expired->size()==0);}
    // A source's current modifier fields are read after the particle's modification insertion.
    {HumanFixture t;Shooter s(t);EmitterServices h(s);auto& e=h.bullet();field<EmitterComponent>(e,31).frame=1;
     const auto captured=s.pools.records;auto replacement=std::make_shared<BulletTable>(*captured);
     t.f.on_added=[&](ComponentSlots& out,std::int32_t slot){if(slot==15&&&out!=&e){auto& m=field<ProjectileModification>(e,15);m.scale_x=7;m.x=12;
         captured->at(0)->flags=1;s.pools.records=replacement;}};
     h.update();check(field<ProjectileModification>(*h.particles()[0],15).scale_x==7&&field<ProjectileModification>(*h.particles()[0],15).x==12);
     check(h.particles()[0]->contains(6)&&s.pools.records->at(0)->flags==0);}
    // Adding another emitter while iterating invalidates the live source group, preserving queued expiries.
    {HumanFixture t;Shooter s(t);EmitterServices h(s);h.emitter(1,1,0);h.emitter(1);bool added=false;
     h.on_effect_world=[&]{if(!added){added=true;h.emitter(2);}};fails([&]{h.update();});check(h.state.expired->size()==1&&h.destroyed.empty());}
    for(const bool physical:{false,true})run_combat_scene(physical);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
