#include "modification_fixture.hpp"
#include <limits>

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
    // Actual shots, trails, collision/death/EXP, landing debris and expiry in recovered partial schedule order.
    for(const bool physical:{false,true}){HumanFixture t;Shooter s(t);EmitterServices em(s);ModificationServices modifiers(t);BulletImpactServices impacts(s);auto& actor=t.hero();auto& enemy=t.enemy_at({0,0,0});
     s.pools.records->at(0)->category=physical?1:0;
     auto& pos=field<ContactPosition>(actor,0);pos.x=0;pos.z=100;field<ProjectileSpeed>(actor,1).z=0;field<ProjectileSpeed>(enemy,1).z=0;
     auto* hp=resolve_combat_parameter(enemy,0);hp->maximum=1;hp->value=1;
     t.host.parameters=[](ComponentSlots&,std::int32_t k){return k==3?1000:0;};
     t.host.on_impact=[&](ComponentSlots& e,ComponentSlots* parent){create_impact_effect(&e,parent,t.host.records,impacts);};
     t.host.on_read_bullet=[&](ComponentSlots& e){check(impacts.bullet_data(e));};
     t.host.on_bullet_impact=[&](ComponentSlots& e,ComponentSlots& owner){create_impact_effect(&e,&owner,t.host.records,impacts);};
     t.host.on_create_effect=[&](std::int32_t a,std::int32_t b,std::int32_t c,ProjectilePoint p){return impacts.create_effect(a,b,c,p);};
     int debris_created=0;t.host.on_create_emitter=[&](std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d,std::int32_t arg,ProjectilePoint p){
         ++debris_created;return &create_emitter({a,b,c,d,arg,p},em.pools);};
     ProjectileSystemState projectiles;initialize_projectiles(projectiles,&t.enemy_world,t.host);
     AnimationSystemState animation;SharedAnimationClip shot_clock{0,1};em.pools.bullet_clips.clips->at(0)=&shot_clock;initialize_animation(animation,&em);
     auto& human=field<HumanRecordLink>(actor,14);human.state=2;human.opponent=&enemy;human.attack_frame=human_attack_interval(t.host.system,&actor,t.host);
     update_human_attacking(t.host.system,&actor,t.host);auto& bullet=t.f.nodes.back().entity;
     auto* move=t.enemy_world.subset({0,1});LiveCollisions collisions(t);CollisionPass collision;LiveGarbage garbage(t);GarbagePass expiry;bool enabled=true;
     std::vector<int> order;
     GameplayUpdate flight(10,enabled,[&](std::int32_t){order.push_back(10);projectiles.pass->update(*projectiles.subset,t.host);});
     GameplayUpdate motion(11,enabled,[&](std::int32_t){order.push_back(11);move_subset(*move);});
     GameplayUpdate contact(12,enabled,[&](std::int32_t){order.push_back(12);collision.update(collisions);});
     GameplayUpdate emit(15,enabled,[&](std::int32_t){order.push_back(15);em.update();});
     GameplayUpdate modify(18,enabled,[&](std::int32_t){order.push_back(18);modifiers.update();});
     GameplayUpdate anim(19,enabled,[&](std::int32_t){order.push_back(19);update_animations(animation,{false,true});});
     GameplayUpdate collect(31,enabled,[&](std::int32_t){order.push_back(31);expiry.update(garbage);});
     std::vector<UpdateSystem*> systems{&flight,&motion,&contact,&emit,&modify,&anim,&collect};UpdateSchedule schedule{&systems,nullptr};
     t.f.random.index=0;t.f.random.values={0,5,99,0,0,0};std::size_t peak_particles=0;bool advanced=false;
     int landed=-1,removed=-1;
     for(int frame=0;frame<120;++frame){order.clear();update_systems(schedule,1);check(order==std::vector<int>{10,11,12,15,18,19,31});
         if(debris_created==3&&landed==-1)landed=frame;
         if(t.f.node(enemy).lifetime.destroyed&&removed==-1)removed=frame;
         auto particles=em.particles();peak_particles=std::max(peak_particles,particles.size());
         for(auto* p:particles)if(field<EntitySprite>(*p,2).animation.frame>0)advanced=true;}
     check(hp->value==0&&t.host.defeat_counts[2]==1&&t.f.node(bullet).lifetime.destroyed);
     check(landed>=0&&removed-landed==10&&t.f.node(enemy).lifetime.destroyed&&!enemy.contains(24));
     check(peak_particles>0&&advanced&&em.particles().empty()&&em.state.subset->size()==0&&debris_created==3);
     bool experience=false;for(auto& n:t.f.nodes)if(n.entity.contains(34)&&n.drop.type==0&&n.drop.value==10)experience=true;check(experience);
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
