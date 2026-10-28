#include "core/gameplay/drop_collection.hpp"
#include "core/gameplay/enemy_rewards.hpp"
#include "core/gameplay/stage_cleanup.hpp"
#include "core/gameplay/subset_cursors.hpp"
#include "core/gameplay/member_selection.hpp"
#include "core/ecs/dynamic_subsets.hpp"
#include <cstdlib>
#include <deque>
#include <limits>
#include <iostream>

using namespace saga::core;
void verify(bool value,int line){if(!value){std::cerr<<"drop collection check failed at "<<line<<'\n';std::abort();}}
#define check(...) verify((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Node {
    std::int32_t id;ComponentEvents events;ComponentSlots entity{35,&events};EntityRegistration registration{id,&entity,&events};
    QuestEntityLifetime lifetime;DropComponent drop;EffectComponent effect;ContactPosition position;ProjectileSpeed speed;
    EntitySprite sprite;EntityDepth depth;CellComponent cell;EntityImage image;EntityPlayback playback;ProjectileModification modify;
    explicit Node(std::int32_t n):id(n){}
};
struct Scene final:DropFactoryHost,EffectFactoryHost,DropCollectionHost,GarbageUpdateHost,EntityDeletionHost,ComponentObserver,EnemyRewardHost {
    std::deque<Node> nodes;std::deque<GarbageLifetime> garbage;Component sync;
    EntityRegistry registry{*this};DynamicSubsets groups{registry,[&](const ComponentSlots& e){return &e==&human?std::int32_t{-100}:node(e).id;}};
    EntityIdState ids;DropEffectWorld binding{ids,static_cast<EffectFactoryHost&>(*this)};
    StageRewards rewards{0,0,0,{}};DropCollectionState state{&rewards,false,false,std::make_shared<std::vector<ComponentSlots*>>()};
    TreasureTable treasures{TreasureRecord{}};EnemyRecord enemy;
    ComponentEvents human_events;ComponentSlots human{35,&human_events};ContactPosition human_position;Component player_tag;
    std::vector<ComponentSlots*> members{&human};PlayerWorldState player_world{nullptr,&groups};
    std::optional<ComponentSlots*> front_override,hero_override;
    std::vector<std::string> calls;int sounds=0,jingles=0,rolls=0;
    bool fail_duration=false,null_world=false,fail_jingle=false;std::function<void()> on_sound,on_world,on_treasure;
    Scene(){EntityRegistration registration{-100,&human,&human_events};registry.add(&registration,false);
        human_position.x=100;human_position.y=20;human_position.z=150;human.add(0,&human_position).add(30,&player_tag);
        enemy.experience=EventIntegers{10,10};enemy.treasures=EnemyTreasureGroups{{{0,1000},{9,1000}}};}
    Node& node(const ComponentSlots& e){for(auto& n:nodes)if(&n.entity==&e)return n;throw std::invalid_argument("entity");}
    bool registered_id(std::int32_t id)override{return registry.contains(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{check(type==0);nodes.emplace_back(id);return &nodes.back().entity;}
    void register_entity(ComponentSlots& e,bool temporary)override{check(&e==&nodes.back().entity);registry.add(&nodes.back().registration,temporary);}
    void added(ComponentSlots& e,std::int32_t i,Component* c)override{groups.added(e,i,c);}
    void removed(ComponentSlots& e,std::int32_t i,Component* c)override{groups.removed(e,i,c);}
    void changed(ComponentSlots& e,std::int32_t i,Component* a,Component* b)override{groups.changed(e,i,a,b);}
    DropComponent& acquire_drop()override{return nodes.back().drop;}
    EffectComponent& acquire_effect()override{return nodes.back().effect;}
    ContactPosition& acquire_position()override{return nodes.back().position;}
    ProjectileSpeed& acquire_speed()override{return nodes.back().speed;}
    EntitySprite& acquire_sprite()override{return nodes.back().sprite;}
    EntityDepth& acquire_depth()override{return nodes.back().depth;}
    CellComponent& acquire_cell()override{return nodes.back().cell;}
    EntityImage& acquire_image()override{return nodes.back().image;}
    EntityPlayback& acquire_playback()override{return nodes.back().playback;}
    ProjectileModification& acquire_modification()override{return nodes.back().modify;}
    Component* sync_animation()override{return &sync;}
    GarbageLifetime& acquire_garbage()override{garbage.emplace_back();return garbage.back();}
    std::int32_t clip_max_frame(std::int32_t r,std::int32_t clip)override{
        check(r==20&&clip==0);calls.push_back("duration");if(fail_duration)throw std::runtime_error("duration");return 12;}
    ComponentSlots* front_line_human()override{calls.push_back("front");return front_override?*front_override:saga::core::front_line_human(&members);}
    EntitySubset* drop_subset()override{calls.push_back("group");return &groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{34}));}
    const TreasureRecord& treasure(std::int32_t id)override{
        calls.push_back("treasure");if(on_treasure)on_treasure();auto& value=treasures.at(static_cast<std::size_t>(id));
        if(!value)throw std::invalid_argument("treasure");return *value;}
    ComponentSlots* hero()override{calls.push_back("hero");return hero_override?*hero_override:member_hero(&player_world);}
    DropEffectWorld* effect_world()override{calls.push_back("world");if(on_world)on_world();return null_world?nullptr:&binding;}
    void play_collection_sound()override{++sounds;calls.push_back("sound");if(on_sound)on_sound();}
    void play_reward_jingle()override{++jingles;calls.push_back("jingle");if(fail_jingle)throw std::runtime_error("jingle");}
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{return nodes.at(static_cast<std::size_t>(id)).lifetime;}
    void delete_from_manager(QuestMemberId id,bool remove)override{delete_entity_from_manager(id,remove,*this);}
    void remove_all_components(QuestMemberId id)override{nodes.at(static_cast<std::size_t>(id)).entity.remove_all();}
    void dispose_entity_events(QuestMemberId id)override{nodes.at(static_cast<std::size_t>(id)).events.dispose();}
    void erase_registration(QuestMemberId id)override{registry.erase(nodes.at(static_cast<std::size_t>(id)).id);}
    std::unique_ptr<GarbageCursor> open_garbage_cursor()override{
        auto& group=groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{22}));
        return open_garbage_subset(group,[&](const ComponentSlots& e){return static_cast<QuestMemberId>(node(e).id);});}
    ComponentSlots& spawn(std::int32_t type,std::int32_t value,ProjectilePoint p={5,7,50}){
        return *create_drop(type,value,p,25,treasures,ids,static_cast<DropFactoryHost&>(*this));}
    const EnemyRecord& enemy_data()override{return enemy;}
    bool hits1000(std::int32_t threshold)override{check(threshold==1000);++rolls;return true;}
    void spawn_experience_drop(std::int32_t value)override{spawn(0,value);}
    void spawn_treasure_drop(std::int32_t value)override{spawn(3,value);}
};
int main()try{
    {Scene s;spawn_enemy_rewards(1,s);check(s.nodes.size()==2&&s.rolls==1);
     collect_drops(s.state,s);check(s.state.collecting->size()==2&&s.rewards.experience==0);
     collect_drops(s.state,s);check(s.state.collecting->size()==2);
     for(int i=0;i<30;++i)update_collecting_drops(s.state,s);
     check(s.nodes[0].drop.frame==30&&s.rewards.experience==0&&s.nodes.size()==2);
     update_collecting_drops(s.state,s);
     check(s.state.collecting->empty()&&s.rewards.experience==10&&s.rewards.treasure_ids==std::vector<std::int32_t>{0});
     check(s.player_world.cached_player==&s.human);
     check(s.nodes.size()==5&&s.ids.counter==5&&s.sounds==2);
     check(s.nodes[0].position.x==100&&s.nodes[0].position.z==150&&s.nodes[0].position.y==7);
     check(s.nodes[2].effect.type==3&&s.nodes[2].effect.value==10&&s.nodes[2].sprite.animation.resource==2&&s.nodes[2].sprite.animation.clip==58);
     check(s.nodes[2].position.y==27&&!s.nodes[2].entity.contains(5)&&!s.nodes[2].entity.contains(10));
     check(s.nodes[3].sprite.animation.resource==20&&s.nodes[3].sprite.animation.clip==0&&s.nodes[3].image.arrays.textures->at(0)==25);
     check(dynamic_cast<GarbageLifetime&>(*s.nodes[0].entity.get(22)).frames==0);
     GarbagePass pass;pass.update(s);check(s.nodes[0].lifetime.destroyed&&s.nodes[1].lifetime.destroyed);
     check(!s.registry.contains(0)&&!s.registry.contains(1)&&s.registry.contains(2)&&s.drop_subset()->size()==0);
     check(s.rewards.experience==10&&s.rewards.treasure_ids.size()==1);
    }
    {Scene s;auto& e=s.spawn(3,0);s.treasures[0]->flags=1;s.human_position.z=51;s.state.boss_defeated=true;
     collect_drops(s.state,s);check(s.rewards.treasure_ids.empty());
     s.human_position.z=50;s.state.boss_defeated=false;collect_drops(s.state,s);check(s.rewards.treasure_ids.empty());
     s.state.boss_defeated=true;collect_drops(s.state,s);
     check(s.state.reward_box_picked_up&&s.state.collecting->empty()&&s.nodes[0].drop.frame==-1&&e.contains(22)&&s.jingles==1);
     collect_drops(s.state,s);check(s.rewards.treasure_ids==std::vector<std::int32_t>{0,0}&&s.jingles==2); // No local boss dedup before Garbage.
    }
    for(bool hero_nan:{false,true}){Scene s;s.spawn(0,1);s.human_position.z=hero_nan?std::numeric_limits<float>::quiet_NaN():150;
     if(!hero_nan)s.nodes[0].position.z=std::numeric_limits<float>::quiet_NaN();
     if(hero_nan)fails([&]{collect_drops(s.state,s);});else collect_drops(s.state,s);
     check(s.state.collecting->empty()&&s.nodes[0].drop.frame==-1);}
    {Scene s;s.front_override=nullptr;collect_drops(s.state,s);check(s.calls==std::vector<std::string>{"front","group"});
     s.spawn(0,1);fails([&]{collect_drops(s.state,s);});check(s.nodes[0].drop.frame==-1);
    }
    {Scene s;s.spawn(0,1);s.state.collecting.reset();fails([&]{collect_drops(s.state,s);});check(s.nodes[0].drop.frame==0);}
    {Scene s;s.spawn(0,9);collect_drops(s.state,s);s.nodes[0].drop.frame=30;s.fail_duration=true;
     fails([&]{update_collecting_drops(s.state,s);});
     check(s.rewards.experience==9&&s.nodes[0].drop.frame==31&&s.state.collecting->size()==1&&s.nodes.size()==2&&!s.nodes[0].entity.contains(22));
     s.fail_duration=false;update_collecting_drops(s.state,s);check(s.rewards.experience==18&&s.state.collecting->empty()&&s.sounds==1);
    }
    {Scene s;auto& first=s.spawn(1,7);auto& second=s.spawn(2,99);s.nodes[0].drop.frame=30;
     auto old=s.state.collecting;old->push_back(&first);
     s.on_sound=[&]{s.state.collecting=std::make_shared<std::vector<ComponentSlots*>>(1,&second);};
     update_collecting_drops(s.state,s);
     check(s.rewards.money==7&&s.rewards.prisoners==0&&s.state.collecting->empty()&&old->size()==1&&s.nodes[1].drop.frame==-1);
    }
    {Scene s;auto& e=s.spawn(2,99);on_collected_drop(s.state,&e,s);check(s.rewards.prisoners==1&&s.nodes.size()==2&&s.sounds==1);
     s.nodes[0].drop.type=-1;s.state.rewards=nullptr;on_collected_drop(s.state,&e,s);check(s.nodes.size()==3&&s.sounds==2);
    }
    {Scene s;auto& e=s.spawn(0,1);s.null_world=true;fails([&]{on_collected_drop(s.state,&e,s);});
     check(s.rewards.experience==1&&s.nodes.size()==1&&!e.contains(22));}
    {Scene s;s.hero_override=nullptr;fails([&]{update_collecting_drops(s.state,s);});}
    {Scene s;auto& e=s.spawn(0,1);s.state.collecting->push_back(&e);s.nodes[0].drop.frame=std::numeric_limits<std::int32_t>::max();
     update_collecting_drops(s.state,s);check(s.nodes[0].drop.frame==std::numeric_limits<std::int32_t>::min()&&s.rewards.experience==0);
    }
    {Scene s;s.spawn(3,0);s.on_treasure=[&]{s.spawn(0,2);};
     fails([&]{collect_drops(s.state,s);});check(s.nodes.size()==2&&s.state.collecting->size()==1&&s.nodes[0].drop.frame==0&&s.nodes[1].drop.frame==-1);
    }
    {Scene s;auto& e=s.spawn(0,4);s.nodes[0].drop.frame=30;s.state.collecting->push_back(&e);
     s.on_sound=[]{throw std::runtime_error("sound");};fails([&]{update_collecting_drops(s.state,s);});
     check(s.rewards.experience==4&&s.nodes.size()==3&&e.contains(22)&&s.state.collecting->size()==1&&s.nodes[0].drop.frame==31);
    }
    {Scene s;s.spawn(3,0);s.treasures[0]->flags=1;s.state.boss_defeated=true;s.human_position.z=50;s.fail_jingle=true;
     fails([&]{collect_drops(s.state,s);});check(s.state.reward_box_picked_up&&s.rewards.treasure_ids.size()==1&&s.nodes[0].entity.contains(22));
    }
    {Scene s;ComponentSlots companion(35);ContactPosition front;front.z=50;HumanRecordLink dead;dead.state=4;
     companion.add(0,&front).add(14,&dead);s.members.push_back(&companion);s.human_position.z=300;
     s.spawn(0,4);collect_drops(s.state,s);check(s.state.collecting->size()==1); // Dead-state member still supplies the minimum Z.
     s.members.pop_back();s.spawn(0,8);collect_drops(s.state,s);
     check(s.state.collecting->size()==1&&s.nodes[1].drop.frame==-1); // Roster removal changes the pickup gate.
     for(int i=0;i<31;++i)update_collecting_drops(s.state,s);
     check(s.rewards.experience==4&&s.nodes[0].position.z==300&&s.player_world.cached_player==&s.human);
    }
    TreasureRecord treasure;treasure.flags=2;check(!treasure_is_boss_reward(treasure));treasure.flags=3;check(treasure_is_boss_reward(treasure));
}catch(const std::exception& error){std::cerr<<"drop collection exception: "<<error.what()<<'\n';return 1;}
