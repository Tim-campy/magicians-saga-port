#pragma once
#include "bullet_fixture.hpp"
#include "core/gameplay/emitter_frame.hpp"
#include "core/gameplay/animation_resources.hpp"

struct EmitterPools final:EmitterFactoryHost,ParticleFactoryHost {
    Shooter& s;UpdateFixture& f;BulletFactoryWorld global;
    SharedAnimationClip clock{0,8};AnimationResource bullet_clips{std::make_shared<std::vector<SharedAnimationClip*>>(2,nullptr)};
    ApplicationAnimationResources resources;
    std::size_t clip_queries=0,map_queries=0,world_queries=0;
    bool missing_world=false;std::function<void()> on_clip,on_world;
    std::function<void(std::size_t)> on_map;
    explicit EmitterPools(Shooter& value):s(value),f(s.t.f),global{f.ids,static_cast<Fixture&>(f)}{
        bullet_clips.clips->at(1)=&clock;resources.bullets=&bullet_clips;
    }
    BulletFactoryWorld* world(){++world_queries;if(on_world)on_world();return missing_world?nullptr:&global;}
    BulletFactoryWorld* emitter_global_world()override{return world();}
    BulletFactoryWorld* particle_global_world()override{return world();}
    StageGeometry* particle_global_map()override{++map_queries;if(on_map)on_map(map_queries);return f.active_map;}
    std::int32_t particle_clip_max_frame(std::int32_t resource,std::int32_t clip)override{
        ++clip_queries;if(on_clip)on_clip();return animation_clip_max_frame(resources,resource,clip);
    }
    bool registered_id(std::int32_t id)override{return f.registered_id(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{return f.create(type,id);}
    void register_entity(ComponentSlots& e,bool temporary)override{f.register_entity(e,temporary);}
    ContactPosition& acquire_position()override{return f.acquire_position();}
    CellComponent& acquire_cell()override{return f.acquire_cell();}
    EntityDepth& acquire_depth()override{return f.acquire_depth();}
    EntityImage& acquire_image()override{return f.acquire_image();}
    EntitySprite& acquire_sprite()override{return f.acquire_sprite();}
    ProjectileSpeed& acquire_speed()override{return f.acquire_speed();}
    EmitterComponent& acquire_emitter()override{return s.pools.acquire_emitter();}
    GarbageLifetime& acquire_garbage()override{return f.acquire_garbage();}
};
struct EmitterServices final:EmitterFrameHost,EmitterInitializationWorld,AnimationInitializationHost {
    Shooter& s;UpdateFixture& f;EmitterPools pools;EmitterSystemState state;
    std::vector<ComponentSlots*> destroyed;
    std::function<void()> on_subset,on_particle_world,on_effect_world;
    std::function<void(ComponentSlots&)> on_destroy;
    bool null_subset=false,missing_particle_world=false,missing_effect_world=false;
    explicit EmitterServices(Shooter& value):s(value),f(s.t.f),pools(s){initialize_emitters(state,this);}
    EntitySubset* emitter_subset(const EventIntegers& all)override{check(all==EventIntegers{31});if(on_subset)on_subset();
        return null_subset?nullptr:&f.groups.get(std::make_shared<ComponentGroupFilter>(all));}
    EntitySubset* subset(const EventIntegers& all)override{return &f.groups.get(std::make_shared<ComponentGroupFilter>(all));}
    AnimationResource* resource(std::int32_t id)override{return id==19?&pools.bullet_clips:nullptr;}
    BulletRecords bullet_records()override{return s.pools.bullet_records();}
    ParticleFactoryHost* emitter_particle_world()override{if(on_particle_world)on_particle_world();return missing_particle_world?nullptr:&pools;}
    DropEffectWorld* emitter_effect_world()override{if(on_effect_world)on_effect_world();return missing_effect_world?nullptr:&s.effect_world;}
    DefeatColor& acquire_emitter_color()override{return s.pools.acquire_bullet_color();}
    ProjectileModification& acquire_modification()override{return f.acquire_modification();}
    EntityPlayback& acquire_playback()override{return f.acquire_playback();}
    void destroy_emitter_entity(ComponentSlots& e)override{destroyed.push_back(&e);if(on_destroy)on_destroy(e);
        destroy_quest_entity(f.entity_id(e),true,static_cast<Fixture&>(f));}
    ComponentSlots& emitter(std::int32_t type=0,std::int32_t interval=1,std::int32_t maximum=-1){return create_emitter({0,type,interval,maximum,0,{10,20,30}},pools);}
    ComponentSlots& bullet(){auto& e=create_bullet({0,17,0,&s.t.hero(),{10,20,30}},s.pools);
        AnimationModificationRequest m;m.type=7;m.angle=45;m.scale_x=2;m.scale_y=3;m.x=4;m.y=5;
        add_animation_modification(e,m,*this);return e;}
    std::vector<ComponentSlots*> particles(){std::vector<ComponentSlots*> result;
        for(auto& n:f.nodes)if(n.entity.contains(22)&&n.entity.contains(2)&&n.sprite.animation.resource==19&&n.sprite.animation.clip==1)result.push_back(&n.entity);
        return result;
    }
    void update(){update_emitters(state,*this);}
};
struct LiveGarbage final:GarbageUpdateHost {
    HumanFixture& t;explicit LiveGarbage(HumanFixture& value):t(value){}
    struct Cursor final:GarbageCursor {
        HumanFixture& t;EntitySubset::Cursor cursor;
        Cursor(HumanFixture& value,EntitySubset& group):t(value),cursor(group.cursor()){}
        std::optional<GarbageEntity> next()override{auto e=cursor.next();if(!e)return std::nullopt;return GarbageEntity{t.f.entity_id(**e),*e};}
    };
    std::unique_ptr<GarbageCursor> open_garbage_cursor()override{
        return std::make_unique<Cursor>(t,t.f.groups.get(std::make_shared<ComponentGroupFilter>(EventIntegers{22})));
    }
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{return static_cast<Fixture&>(t.f).entity_lifetime(id);}
    void delete_from_manager(QuestMemberId id,bool remove)override{static_cast<Fixture&>(t.f).delete_from_manager(id,remove);}
};
