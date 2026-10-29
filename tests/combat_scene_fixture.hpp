#pragma once
#include "modification_fixture.hpp"
#include "core/gameplay/parameter_experience.hpp"
#include "cell_fixture.hpp"
#include "quest_camera_fixture.hpp"
#include "render_fixture.hpp"
#include "render_draw_fixture.hpp"
#include <limits>


    // Actual shots, trails, collision/death/EXP, landing debris and expiry in recovered partial schedule order.
inline void run_combat_scene(bool physical,const std::function<void(RenderServices&,EmitterServices&,HumanFixture&,int)>& observe={}){HumanFixture t;Shooter s(t);EmitterServices em(s);ModificationServices modifiers(t);BulletImpactServices impacts(s);auto& actor=t.hero();auto& enemy=t.enemy_at({0,0,0});
     s.pools.records->at(0)->category=physical?1:0;
     auto& pos=field<ContactPosition>(actor,0);pos.x=0;pos.z=100;field<ProjectileSpeed>(actor,1).z=0;field<ProjectileSpeed>(enemy,1).z=0;
     auto* hp=resolve_combat_parameter(enemy,0);hp->maximum=1;hp->value=1;
     CellServices cells([&](const auto& all){return &t.f.groups.get(std::make_shared<ComponentGroupFilter>(all));});
     cells.map.step_x=t.f.active_map->step_x;cells.map.step_z=t.f.active_map->step_z;cells.map.width=quest_map_width();cells.init();
     CameraServices camera(t,cells.map);camera.init();cells.view=&camera.cell_view;
     RenderServices renderer(t,camera,cells);renderer.resource_provider=[&](std::int32_t id){return id==19?&em.pools.bullet_clips:get_animation_resource(renderer.catalog,id);};
     RenderDrawServices drawing;drawing.state.queue=&renderer.output;drawing.supplied=renderer.bounds;
     // Explicit synthetic image slots cover this fixture's clothing/weapon/bullet IDs (up to 700).
     drawing.catalog.images->assign(1024,&drawing.first_image);
     RenderDrawServices::Resource bullet_images{&em.pools.bullet_clips,drawing.catalog.images};
     drawing.catalog.animations=&renderer.resource;
     drawing.resource_provider=[&](std::int32_t id){return id==19?&bullet_images:&drawing.catalog;};
     drawing.global_provider=drawing.resource_provider;
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
     std::vector<int> order;ParameterExperienceSystem parameter_exp;
     GameplayUpdate camera_frame(8,enabled,[&](std::int32_t){order.push_back(8);update_quest_camera(camera.state,camera);});
     GameplayUpdate exp(9,enabled,[&](std::int32_t){order.push_back(9);update_parameter_experience(parameter_exp);});
     GameplayUpdate flight(10,enabled,[&](std::int32_t){order.push_back(10);projectiles.pass->update(*projectiles.subset,t.host);});
     GameplayUpdate motion(11,enabled,[&](std::int32_t){order.push_back(11);move_subset(*move);});
     GameplayUpdate contact(12,enabled,[&](std::int32_t){order.push_back(12);collision.update(collisions);});
     GameplayUpdate parameters(14,enabled,[&](std::int32_t){order.push_back(14);update_parameters();});
     GameplayUpdate emit(15,enabled,[&](std::int32_t){order.push_back(15);em.update();});
     GameplayUpdate cell(17,enabled,[&](std::int32_t){order.push_back(17);update_cells(cells.frame,cells);});
     GameplayUpdate modify(18,enabled,[&](std::int32_t){order.push_back(18);modifiers.update();});
     GameplayUpdate anim(19,enabled,[&](std::int32_t){order.push_back(19);update_animations(animation,{false,true});});
     GameplayUpdate culling(20,enabled,[&](std::int32_t){order.push_back(20);update_cell_visibility(cells.index,cells);});
     GameplayUpdate render(21,enabled,[&](std::int32_t){order.push_back(21);renderer.update();});
     GameplayUpdate collect(31,enabled,[&](std::int32_t){order.push_back(31);expiry.update(garbage);});
     std::vector<UpdateSystem*> systems{&camera_frame,&exp,&flight,&motion,&contact,&parameters,&emit,&cell,&modify,&anim,&culling,&render,&collect};UpdateSchedule schedule{&systems,&camera.clock};
     t.f.random.index=0;t.f.random.values={0,5,99,0,0,0};std::size_t peak_particles=0;bool advanced=false;
     int landed=-1,removed=-1;
     std::int32_t peak_commands=0;
     for(int frame=0;frame<120;++frame){order.clear();clear_render_queue(renderer.output);update_systems(schedule,0);check(order==std::vector<int>{8,9,10,11,12,14,15,17,18,19,20,21,31});
         check(renderer.errors.empty());peak_commands=std::max(peak_commands,renderer.output.counts->at(0));
         drawing.draws.clear();drawing.calls.clear();drawing.draw();
         if(observe)observe(renderer,em,t,frame);
         check(drawing.draws.size()==static_cast<std::size_t>(renderer.output.counts->at(0))&&drawing.missing.empty()&&drawing.invalid.empty());
         for(int i=1;i<renderer.output.counts->at(0);++i)check(renderer.row(static_cast<std::size_t>(i-1)).at(render_depth)<=renderer.row(static_cast<std::size_t>(i)).at(render_depth));
         if(debris_created==3&&landed==-1)landed=frame;
         if(t.f.node(enemy).lifetime.destroyed&&removed==-1)removed=frame;
         auto particles=em.particles();peak_particles=std::max(peak_particles,particles.size());
         for(auto* p:particles)if(field<EntitySprite>(*p,2).animation.frame>0)advanced=true;}
     check(hp->value==0&&t.host.defeat_counts[2]==1&&t.f.node(bullet).lifetime.destroyed);
     check(landed>=0&&removed-landed==10&&t.f.node(enemy).lifetime.destroyed&&!enemy.contains(24));
     check(peak_particles>0&&advanced&&em.particles().empty()&&em.state.subset->size()==0&&debris_created==3);
     for(const auto& queue:*parameter_exp.queues)check(queue->empty()); // No unproven event-21 producer.
     check(!cells.changes.empty()&&cells.frame.changed->empty());
     check(cells.visible_count>=0&&camera.clock.stops==120*static_cast<int>(systems.size()));
     check(peak_commands>0&&!renderer.queries.empty());
     for(auto& n:t.f.nodes)if(n.entity.contains(5)){
         auto& c=field<CellComponent>(n.entity,5);auto bucket=cell_entities(cells.index,cell_key(&cells.map,c.xi,c.yi));
         check(bucket&&std::find(bucket->begin(),bucket->end(),&n.entity)!=bucket->end());}
     for(const auto& [key,bucket]:*cells.index.buckets){static_cast<void>(key);
         for(auto* e:*bucket)check(e&&e->contains(5));}
     bool experience=false;for(auto& n:t.f.nodes)if(n.entity.contains(34)&&n.drop.type==0&&n.drop.value==10)experience=true;check(experience);
    }
