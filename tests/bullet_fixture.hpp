#pragma once
#include "human_frame_fixture.hpp"
#include "core/gameplay/human_shoot.hpp"

struct BulletPools final:BulletFactoryHost,EffectFactoryHost {
    UpdateFixture& f;BulletRecords records=std::make_shared<BulletTable>(3);
    BulletFactoryWorld global;bool missing_world=false;std::size_t queries=0,map_queries=0;
    std::function<void()> on_records;std::function<void(std::size_t)> on_map;
    std::deque<BulletComponent> bullets;std::deque<EmitterComponent> emitters;
    std::deque<ProjectileDamage> damage;
    std::deque<DefeatColor> colors;
    explicit BulletPools(UpdateFixture& value):f(value),global{f.ids,static_cast<Fixture&>(f)}{
        for(std::int32_t i=0;i<3;++i){BulletRecord r;r.id=i;r.category=0;r.attribute=i;r.image=100+i;r.effect_image=200+i;
            records->at(static_cast<std::size_t>(i))=r;}
    }
    BulletRecords bullet_records()override{++queries;if(on_records)on_records();return records;}
    BulletFactoryWorld* bullet_global_world()override{return missing_world?nullptr:&global;}
    StageGeometry* bullet_global_map()override{++map_queries;if(on_map)on_map(map_queries);return f.active_map;}
    bool registered_id(std::int32_t id)override{return f.registered_id(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{return f.create(type,id);}
    void register_entity(ComponentSlots& e,bool temporary)override{f.register_entity(e,temporary);}
    ContactPosition& acquire_position()override{return f.acquire_position();}
    CellComponent& acquire_cell()override{return f.acquire_cell();}
    EntityDepth& acquire_depth()override{return f.acquire_depth();}
    EntityImage& acquire_image()override{return f.acquire_image();}
    EntitySprite& acquire_sprite()override{return f.acquire_sprite();}
    ProjectileSpeed& acquire_speed()override{return f.acquire_speed();}
    BulletComponent& acquire_bullet()override{bullets.emplace_back();return bullets.back();}
    EmitterComponent& acquire_emitter()override{emitters.emplace_back();return emitters.back();}
    ProjectileCollision& acquire_bullet_collision()override{return f.acquire_collision();}
    ProjectileDamage& acquire_bullet_damage()override{damage.emplace_back();return damage.back();}
    DefeatColor& acquire_bullet_color()override{colors.emplace_back();return colors.back();}
    EntityPlayback& acquire_playback()override{return f.acquire_playback();}
    EffectComponent& acquire_effect()override{return f.acquire_effect();}
    GarbageLifetime& acquire_garbage()override{return f.acquire_garbage();}
    std::int32_t clip_max_frame(std::int32_t resource,std::int32_t clip)override{
        check(resource==20&&(clip==0||clip==101));return 12; // Explicit synthetic effect catalog.
    }
};
struct Shooter final:HumanShootHost {
    HumanFixture& t;BulletPools pools;EventInstruction clips=initial_attack_animation_ids();
    DropEffectWorld effect_world;
    std::vector<bool> sounds;std::vector<ComponentSlots*> made;std::int32_t effects=0;
    bool missing_bullet_world=false,missing_effect_world=false;
    std::function<void()> on_sound,on_effect_world,on_bullet_world;
    explicit Shooter(HumanFixture& value):t(value),pools(t.f),effect_world{t.f.ids,static_cast<EffectFactoryHost&>(pools)}{
        t.host.shooter=[this](ComponentSlots& e,ComponentSlots& target){return shoot_human(t.host.system,&e,&target,t.host,*this);};
        t.flat_parameters();t.f.active_map=&t.f.geometry;t.f.geometry={24,24,6};
        auto& w=*t.host.gear.at(0);w.bullet_id=0;w.bullet_speed=8;w.use_mp=3;w.max_height=11;
        set_combat_equipment(t.hero(),EquipmentSlot::weapon,0);
        field<ContactDirection>(t.hero(),12).direction=0;
        *resolve_combat_parameter(t.hero(),1)={100,100,0,0};
    }
    BulletRecords shoot_bullet_records()override{return pools.bullet_records();}
    BulletFactoryHost* shoot_bullet_world()override{if(on_bullet_world)on_bullet_world();return missing_bullet_world?nullptr:&pools;}
    DropEffectWorld* shoot_effect_world()override{++effects;if(on_effect_world)on_effect_world();return missing_effect_world?nullptr:&effect_world;}
    ProjectileLaunchHost& shoot_launch_host()override{return static_cast<EnemyImpactHost&>(t.host);}
    EventInstruction shoot_attack_animations()override{return clips;}
    void play_attack_sound(bool elemental)override{sounds.push_back(elemental);if(on_sound)on_sound();}
    bool shoot(ComponentSlots* target,ComponentSlots* actor=nullptr){return shoot_human(t.host.system,actor?actor:&t.hero(),target,t.host,*this);}
};
struct BulletImpactServices final:ImpactEffectHost {
    Shooter& s;HumanFixture& t;
    explicit BulletImpactServices(Shooter& value):s(value),t(s.t){}
    const BulletImpactData* bullet_data(ComponentSlots& e)override{
        const auto id=field<BulletComponent>(e,29).data_id;
        auto& row=s.pools.records->at(static_cast<std::size_t>(id));return row?&*row:nullptr;
    }
    std::int32_t attribute_value(ComponentSlots& e,std::int32_t a)override{return static_cast<std::uint32_t>(a)<=2U?t.host.human_parameter(e,a+10):0;}
    ComponentSlots* create_effect(std::int32_t clip,std::int32_t image,std::int32_t loop,ProjectilePoint p)override{
        return &create_visual_effect(p,clip,image,loop!=0,s.effect_world.ids,s.effect_world.factory);
    }
    ComponentSlots* create_entity(std::int32_t type)override{return &saga::core::create_entity(t.f.ids,type,false,static_cast<Fixture&>(t.f));}
    ContactPosition& acquire_position()override{return t.f.acquire_position();}
    ProjectileCollision& acquire_collision()override{return s.pools.acquire_bullet_collision();}
    ProjectileDamage& acquire_damage()override{return s.pools.acquire_bullet_damage();}
    ProjectileLink& acquire_projectile()override{return t.host.acquire_projectile();}
    ProjectileModification& acquire_modification()override{return t.f.acquire_modification();}
    void return_component(std::int32_t slot,Component* p)override{t.host.return_component(slot,p);}
    void add_animation(ComponentSlots& e,std::int32_t rate,std::int32_t backup)override{t.host.add_animation(e,rate,backup);}
    void add_garbage(ComponentSlots& e,std::int32_t frames)override{add_garbage_lifetime(e,frames,s.pools);}
    void add_color(ComponentSlots& e,std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d)override{
        auto& color=s.pools.acquire_bullet_color();color.values={{a,b,c,d}};e.add(6,&color);
    }
};
