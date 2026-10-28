#pragma once
#include "startup_fixture.hpp"
#include "core/gameplay/enemy_creation.hpp"

struct EnemyPoolFixture final:EnemyFactoryHost {
    StartupFixture& f;
    EnemyRecords records=std::make_shared<EnemyTable>(3);
    ParameterMaximumLevels maxima=initial_enemy_parameter_maximum_levels();
    EventInstruction keys=initial_enemy_parameter_ids();
    std::int32_t resource=3;std::size_t record_queries=0;
    std::function<void(std::size_t)> on_records;std::function<void()> on_resource;
    explicit EnemyPoolFixture(StartupFixture& fixture):f(fixture){
        for(std::size_t i=0;i<records->size();++i){EnemyRecord r;r.id=static_cast<std::int32_t>(i);r.enemy_seb=2;
            r.image=90;r.seb=-1;r.frame=-1;r.resource="synthetic-enemy";
            r.parameters=std::make_shared<std::vector<EventInstruction>>();
            for(std::int32_t key=0;key<13;++key)r.parameters->push_back(std::make_shared<EventIntegers>(EventIntegers{100+key,100+key}));
            records->at(i)=std::move(r);}
    }
    EnemyRecords enemy_records()override{++record_queries;if(on_records)on_records(record_queries);return records;}
    const ParameterMaximumLevels& enemy_maximum_levels()override{return maxima;}
    EventInstruction enemy_parameter_ids()override{return keys;}
    std::int32_t enemy_resource_id(std::string_view name)override{check(name=="synthetic-enemy");if(on_resource)on_resource();return resource;}
    bool registered_id(std::int32_t id)override{return f.registered_id(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{return f.create(type,id);}
    void register_entity(ComponentSlots& e,bool temporary)override{f.register_entity(e,temporary);}
    ContactPosition& acquire_position()override{return f.acquire_position();}
    CellComponent& acquire_cell()override{return f.acquire_cell();}
    EntityDepth& acquire_depth()override{return f.acquire_depth();}
    EntityImage& acquire_image()override{return f.acquire_image();}
    EntitySprite& acquire_sprite()override{return f.acquire_sprite();}
    ProjectileSpeed& acquire_speed()override{return f.acquire_speed();}
    EntityPlayback& acquire_playback()override{return f.acquire_playback();}
    EnemyRecordLink& acquire_enemy()override{return f.nodes.back().enemy;}
    HumanAI& acquire_enemy_ai()override{return f.nodes.back().ai;}
    CombatParameterLinks& acquire_enemy_parameters()override{return f.nodes.back().parameters;}
    DefeatColor& acquire_enemy_color()override{return f.nodes.back().color;}
    ProjectileCollision& acquire_collision()override{return f.acquire_collision();}
    void release_collision(Component* p)override{f.release_collision(p);}
    ContactDirection& acquire_enemy_direction()override{return f.acquire_direction();}
};
