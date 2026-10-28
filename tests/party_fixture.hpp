#pragma once
#include "core/gameplay/human_factory.hpp"
#include "core/gameplay/animation_pass.hpp"
#include "core/gameplay/stage_cleanup.hpp"
#include <cstdlib>
#include <deque>
#include <iostream>

using namespace saga::core;
inline void verify(bool v,int line){if(!v){std::cerr<<"party start check failed at "<<line<<'\n';std::abort();}}
#define check(...) verify((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Node {
    std::int32_t id;ComponentEvents events;ComponentSlots entity{35,&events};QuestEntityLifetime lifetime;
    HumanRecordLink human;ContactPosition position;ContactDirection direction;ProjectileSpeed speed;
    EntitySprite sprite;EntityPlayback playback;CombatParameterLinks parameters;Component tag;
    CellComponent cell;EntityDepth depth;EntityImage image;EquipmentRecordLinks equipment;HumanAI ai;Component input;
    DamageParameter hp{7,8,0,0},energy{9,10,0,0};DamageParameterMap values{{0,&hp},{1,&energy}};
    explicit Node(std::int32_t n,bool full=true):id(n){parameters.values=&values;playback.state={1,-1};sprite.animation={1,36,0};
        if(full)entity.add(0,&position).add(1,&speed).add(2,&sprite).add(10,&playback).add(12,&direction)
              .add(14,&human).add(23,&parameters);}
};
struct Fixture final:PartyStartHost,PartyHumanFactory,PartyDistanceSystem,MemberInitializationHost,
                     HumanStateHost,QuestEntityHost,EntityDeletionHost,ComponentObserver,HumanFactoryHost {
    std::deque<Node> nodes;std::deque<ProjectileCollision> collisions;std::vector<Component*> released;
    EntityRegistry registry{*this};DynamicSubsets groups{registry,[&](const ComponentSlots& e){return node(e).id;}};
    PlayerWorldState world{nullptr,&groups};MemberRoster roster;EntityIdState ids;HumanFactory factory{ids,*this};
    HumanHat hat{2,700};HumanHats hat_rows=std::make_shared<std::vector<HumanHat*>>(1,&hat);
    ParameterMaximumLevels maxima=initial_parameter_maximum_levels();EventInstruction parameter_keys=initial_character_parameter_ids();
    std::array<PartyCharacter,12> records;std::array<std::int32_t,12> sorties{};
    PartyCharacters table=std::make_shared<std::vector<PartyCharacter*>>();
    std::shared_ptr<std::vector<std::int32_t>> layout=std::make_shared<std::vector<std::int32_t>>(16,1);
    std::vector<HumanCreation> creations;std::vector<std::int32_t> distance_order;
    std::vector<std::int32_t> random_candidates;
    std::vector<std::string> calls;std::vector<QuestMemberId> destroyed;
    bool debug=false,editor=false,auto_party=false,no_factory=false,no_world=false,no_distance=false;
    int quest_queries=0,editor_queries=0,random_queries=0;std::int32_t layout_index=-1,random_id=1;
    std::function<void()> on_factory_world,on_release;std::function<void(HumanRecordLink&)> on_record;
    std::function<void(const HumanCreation&)> on_create;
    std::function<void(ComponentSlots&,std::int32_t)> on_added;
    Fixture(){for(std::size_t i=0;i<records.size();++i){records[i].id=static_cast<std::int32_t>(i);records[i].flags=i==0?2:4;
            records[i].saved={4,42};records[i].image=static_cast<std::int32_t>(50+i);records[i].weapon_id=static_cast<std::int32_t>(i+3);
            records[i].parameters.values=std::make_shared<std::vector<EventInstruction>>();
            for(std::int32_t key=0;key<13;++key)records[i].parameters.values->push_back(std::make_shared<EventIntegers>(EventIntegers{100+key,100+key}));
            table->push_back(&records[i]);}
        spawn(0).entity.add(30,&nodes.front().tag);(*roster.formation)[10]=0;}
    Node& node(const ComponentSlots& e){for(auto& n:nodes)if(&n.entity==&e)return n;throw std::invalid_argument("entity");}
    Node& spawn(std::int32_t character){nodes.emplace_back(static_cast<std::int32_t>(nodes.size()));auto& n=nodes.back();
        n.human.character_id=character;EntityRegistration r{n.id,&n.entity,&n.events};registry.add(&r,false);ids.counter=static_cast<std::int32_t>(nodes.size());return n;}
    void added(ComponentSlots& e,std::int32_t i,Component* c)override{groups.added(e,i,c);if(on_added)on_added(e,i);}
    void removed(ComponentSlots& e,std::int32_t i,Component* c)override{groups.removed(e,i,c);}
    void changed(ComponentSlots& e,std::int32_t i,Component* a,Component* b)override{groups.changed(e,i,a,b);}
    PlayerWorldState* current_world()override{calls.push_back("world");return no_world?nullptr:&world;}
    QuestEntityHost* destruction_host(PlayerWorldState& w)override{check(&w==&world);calls.push_back("destruction");return this;}
    QuestMemberId entity_id(ComponentSlots& e)override{return static_cast<QuestMemberId>(node(e).id);}
    PartyCharacter* character(HumanRecordLink& h)override{if(on_record)on_record(h);return table->at(static_cast<std::size_t>(h.character_id));}
    PartyCharacters character_table()override{calls.push_back("table");return table;}
    PartyHumanFactory* human_factory_world()override{calls.push_back("factory-world");if(on_factory_world)on_factory_world();return no_factory?nullptr:this;}
    ComponentSlots* create_human(const HumanCreation& args)override{
        calls.push_back("create");creations.push_back(args);if(on_create)on_create(args);return factory.create_human(args);}
    bool registered_id(std::int32_t id)override{return registry.contains(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{check(type==0);nodes.emplace_back(id,false);return &nodes.back().entity;}
    void register_entity(ComponentSlots& e,bool temporary)override{check(!temporary);auto& n=node(e);EntityRegistration r{n.id,&e,&n.events};registry.add(&r,temporary);}
    std::int32_t weapon_image(std::int32_t weapon)override{return 600+weapon;}
    HumanHats hats()override{return hat_rows;}
    EventInstruction parameter_ids()override{return parameter_keys;}
    std::int32_t maximum_level(std::int32_t id)override{return maxima.at(static_cast<std::size_t>(id));}
    const ShopParameterUpgrade& shop_upgrade(std::int32_t)override{throw std::runtime_error("unexpected shop query");}
    ContactPosition& acquire_position()override{return nodes.back().position;}
    ProjectileSpeed& acquire_speed()override{return nodes.back().speed;}
    EntitySprite& acquire_sprite()override{return nodes.back().sprite;}
    EntityDepth& acquire_depth()override{return nodes.back().depth;}
    CellComponent& acquire_cell()override{return nodes.back().cell;}
    EntityImage& acquire_image()override{return nodes.back().image;}
    EntityPlayback& acquire_playback()override{return nodes.back().playback;}
    HumanRecordLink& acquire_human()override{return nodes.back().human;}
    CombatParameterLinks& acquire_parameters()override{return nodes.back().parameters;}
    EquipmentRecordLinks& acquire_equipment()override{return nodes.back().equipment;}
    ContactDirection& acquire_direction()override{return nodes.back().direction;}
    HumanAI& acquire_ai()override{return nodes.back().ai;}
    Component& acquire_input()override{return nodes.back().input;}
    ProjectileCollision& acquire_collision()override{calls.push_back("collision");collisions.emplace_back();return collisions.back();}
    void release_collision(Component* c)override{calls.push_back("release");released.push_back(c);if(on_release)on_release();}
    bool debug_enabled()override{calls.push_back("debug");return debug;}
    bool editor_enabled()override{++editor_queries;return editor;}
    bool quest_has_auto_party()override{++quest_queries;return auto_party;}
    PartyCharacter* random_character(const std::vector<PartyCharacter*>& candidates)override{
        if(random_queries==0)for(auto* c:candidates)random_candidates.push_back(c->id);
        ++random_queries;return candidates.empty()?nullptr:&records.at(static_cast<std::size_t>(random_id));}
    PartyDistanceSystem* distance_system(PlayerWorldState&)override{calls.push_back("distance-system");return no_distance?nullptr:this;}
    std::int32_t distance(PartyCharacter* data)override{if(!data)throw std::invalid_argument("distance record");
        distance_order.push_back(data->id);return data->id%3;}
    std::shared_ptr<std::vector<std::int32_t>> auto_formation(std::int32_t index)override{layout_index=index;return layout;}
    MemberInitializationHost& member_initialization()override{calls.push_back("initialize");return *this;}
    std::int32_t formation_left()override{return 100;}
    std::int32_t formation_top()override{return 200;}
    std::int32_t total_parameter(ComponentSlots&,std::int32_t k)override{return k==0?100:30;}
    void change_animation(ComponentSlots& e,std::int32_t clip,bool reset)override{saga::core::change_animation(live_animation(e),clip,reset);}
    HumanStateHost* human_system()override{return this;}
    bool playing_cut_in(ComponentSlots&)override{return false;}
    std::int32_t attack_interval(ComponentSlots&)override{return 10;}
    std::int32_t* sortie_count(HumanRecordLink& h)override{return &sorties.at(static_cast<std::size_t>(h.character_id));}
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{return nodes.at(static_cast<std::size_t>(id)).lifetime;}
    void delete_from_manager(QuestMemberId id,bool remove)override{destroyed.push_back(id);delete_entity_from_manager(id,remove,*this);}
    void remove_all_components(QuestMemberId id)override{nodes.at(static_cast<std::size_t>(id)).entity.remove_all();}
    void dispose_entity_events(QuestMemberId id)override{nodes.at(static_cast<std::size_t>(id)).events.dispose();}
    void erase_registration(QuestMemberId id)override{registry.erase(static_cast<std::int32_t>(id));}
};
