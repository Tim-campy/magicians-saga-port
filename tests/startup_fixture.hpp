#pragma once
#include "core/gameplay/stage_start.hpp"
#include "party_fixture.hpp"

struct Draws final:RandomSource {
    std::vector<std::int32_t> values;std::size_t index=0;std::int32_t fallback=99;
    std::function<void(std::size_t)> on_draw;
    std::int32_t next()override{if(on_draw)on_draw(index);const auto value=index<values.size()?values[index]:fallback;++index;return value;}
};
inline StageScriptGroups scripts(EventScript first){return std::make_shared<std::vector<EventScript>>(1,std::move(first));}
struct StartupFixture:Fixture,DropFactoryHost,EffectFactoryHost,DropCollectionHost,StageStartHost,
                            StageCleanupHost,StageMapLoader,StageEnemyStart {
    StageStartState state;StageHumanStart human_system;StageQuestForm form;
    StageGeometry geometry{100,24},other_geometry{200,48};StageGeometry* active_map=&geometry;
    TreasureTable treasures{TreasureRecord{},TreasureRecord{},TreasureRecord{}};
    DropPlacementWorld placement{ids,static_cast<DropFactoryHost&>(*this),treasures};
    DropEffectWorld effects{ids,static_cast<EffectFactoryHost&>(*this)};
    Component sync;std::deque<GarbageLifetime> garbage;
    std::array<AttackButtonState,3> buttons{};std::optional<QuestMemberId> extra;
    std::vector<std::string> trace;std::shared_ptr<StageQuest> mapped,enemy_quest;
    Draws random;std::int32_t fps=60;int map_queries=0,drop_queries=0,sounds=0,jingles=0;
    bool paused=true,no_drop_world=false,no_map_system=false,no_form=false;
    std::function<void()> on_cleanup,on_fps,on_map_load,on_enemy_start,on_drop,on_bgm;
    std::function<void(int)> on_map_query;
    StartupFixture(){state.quest=std::make_shared<StageQuest>();state.quest->distance=4;state.quest->time_limit=10;
        state.quest->events=scripts(std::make_shared<std::vector<EventInstruction>>(2));
        state.quest->fixed_treasure=scripts(std::make_shared<std::vector<EventInstruction>>());
        state.humans=&human_system;state.members=&roster;state.enemies=this;
        state.collection.collecting=std::make_shared<std::vector<ComponentSlots*>>();
        state.attack_buttons=std::make_shared<std::vector<AttackButtonState*>>();for(auto& b:buttons)state.attack_buttons->push_back(&b);
        for(std::size_t i=0;i<treasures.size();++i)treasures[i]->id=static_cast<std::int32_t>(i);}
    bool registered_id(std::int32_t id)override{return Fixture::registered_id(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{return Fixture::create(type,id);}
    void register_entity(ComponentSlots& e,bool temporary)override{Fixture::register_entity(e,temporary);}
    ContactPosition& acquire_position()override{return Fixture::acquire_position();}
    ProjectileSpeed& acquire_speed()override{return Fixture::acquire_speed();}
    EntitySprite& acquire_sprite()override{return Fixture::acquire_sprite();}
    EntityDepth& acquire_depth()override{return Fixture::acquire_depth();}
    CellComponent& acquire_cell()override{return Fixture::acquire_cell();}
    EntityImage& acquire_image()override{return Fixture::acquire_image();}
    EntityPlayback& acquire_playback()override{return Fixture::acquire_playback();}
    DropComponent& acquire_drop()override{return nodes.back().drop;}
    ProjectileModification& acquire_modification()override{return nodes.back().modification;}
    EffectComponent& acquire_effect()override{return nodes.back().effect;}
    GarbageLifetime& acquire_garbage()override{garbage.emplace_back();return garbage.back();}
    Component* sync_animation()override{return &sync;}
    std::int32_t clip_max_frame(std::int32_t resource,std::int32_t clip)override{check(resource==20&&clip==0);return 12;}
    StageGeometry* stage_map()override{++map_queries;if(on_map_query)on_map_query(map_queries);return active_map;}
    DropPlacementWorld* drop_world()override{++drop_queries;trace.push_back("drop");if(on_drop)on_drop();return no_drop_world?nullptr:&placement;}
    std::int32_t formation_left()override{return stage_formation_left(*this);}
    std::int32_t formation_top()override{return stage_formation_top(state,*this);}
    StageCleanupHost& cleanup_host()override{trace.push_back("cleanup");if(on_cleanup)on_cleanup();return *this;}
    std::int32_t frames_per_second()override{trace.push_back("fps");if(on_fps)on_fps();return fps;}
    StageMapLoader* current_map_system()override{trace.push_back("map-system");return no_map_system?nullptr:this;}
    void load_map(std::shared_ptr<StageQuest> q)override{trace.push_back("map");mapped=std::move(q);if(on_map_load)on_map_load();}
    PartyStartHost& party_host()override{trace.push_back("party");return static_cast<PartyStartHost&>(*this);}
    void start_enemies(std::shared_ptr<StageQuest> q)override{trace.push_back("enemies");enemy_quest=std::move(q);if(on_enemy_start)on_enemy_start();}
    StageQuestForm* quest_form()override{trace.push_back("form");return no_form?nullptr:&form;}
    bool systems_paused()override{trace.push_back("paused");return paused;}
    void resume_systems()override{trace.push_back("resume");paused=false;}
    void play_stage_bgm()override{trace.push_back("bgm");if(on_bgm)on_bgm();}
    void play_stage_sound()override{trace.push_back("start-sound");}
    std::vector<std::optional<QuestMemberId>> snapshot_group(std::int32_t slot)override{
        std::vector<std::optional<QuestMemberId>> out;auto& group=groups.get(std::make_shared<ComponentGroupFilter>(EventIntegers{slot}));
        auto cursor=group.cursor();while(const auto e=cursor.next())out.push_back(entity_id(**e));return out;}
    QuestEntityHost& destruction_host()override{return *this;}
    std::optional<QuestMemberId> extra_entity()override{return extra;}
    void clear_extra_entity()override{extra.reset();}
    void clear_collecting_drops()override{if(!state.collection.collecting)throw std::runtime_error("collecting");state.collection.collecting->clear();}
    ComponentSlots* front_line_human()override{return saga::core::front_line_human(&state.members->members->entities());}
    EntitySubset* drop_subset()override{return &groups.get(std::make_shared<ComponentGroupFilter>(EventIntegers{34}));}
    const TreasureRecord& treasure(std::int32_t id)override{return *treasures.at(static_cast<std::size_t>(id));}
    ComponentSlots* hero()override{return member_hero(current_world());}
    DropEffectWorld* effect_world()override{return &effects;}
    void play_collection_sound()override{++sounds;}
    void play_reward_jingle()override{++jingles;}
};
