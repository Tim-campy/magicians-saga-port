#include "party_start.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing party start state");return *p;}
HumanRecordLink& human(ComponentSlots* e){return required(dynamic_cast<HumanRecordLink*>(required(e).get(14)));}
PartyCharacter& character(PartyCreationHost& host,std::int32_t id){
    auto table=host.character_table();return required(required(table.get()).at(static_cast<std::size_t>(id)));
}
std::int32_t word(std::uint32_t bits){
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::pair<std::int32_t,std::int32_t> offsets(std::int32_t index){
    return {static_cast<std::int32_t>((index%4)*36),word(static_cast<std::uint32_t>(index/4)*36U)};
}
void place_character(MemberRoster& state,PartyStartHost& host,PartyCharacter& data,std::int32_t id,std::int32_t index){
    const auto [x,z]=offsets(index);
    if((static_cast<std::uint32_t>(data.flags)&2U)!=0){
        human(member_hero(host.current_world())).formation_x=x;
        human(member_hero(host.current_world())).formation_z=z;
        human(member_hero(host.current_world())).formation_index=index;
    }else{
        auto* entity=create_sub_character(x,z,id,host);
        human(entity).formation_index=index;
        add_member(state,entity);
    }
}
std::vector<PartyCharacter*> priority_characters(const std::vector<PartyCharacter*>& candidates){
    std::vector<PartyCharacter*> result;
    for(auto* c:candidates){
        if((static_cast<std::uint32_t>(required(c).flags)&0x100U)!=0){result.push_back(c);if(result.size()==8)break;}
    }
    return result;
}
}
PartyCharacter bind_party_character(const CharacterProgressionRecord& record,SavedCharacterProgression saved){
    PartyCharacter result;result.id=record.id;result.flags=record.flags;result.saved=saved;
    result.image=record.image;result.weapon_id=record.weapon_id;
    result.parameters=bind_character_parameters(record,saved.level);return result;
}
bool character_is_sub(const PartyCharacter& data){return (static_cast<std::uint32_t>(data.flags)&4U)!=0;}
ComponentSlots* create_sub_character(std::int32_t x,std::int32_t z,std::int32_t id,PartyCreationHost& host){
    auto& data=character(host,id);
    auto* world=host.human_factory_world();
    // Read saved values only after resolving World.world_, as in the wrapper.
    const auto level=data.saved.level,experience=data.saved.experience;
    return required(world).create_human({0,id,level,x,z,experience,0,0,0});
}
ComponentSlots& add_party_collision(ComponentSlots& e,std::int32_t radius,std::int32_t x,
    std::int32_t y,std::int32_t z,bool visited,PartyCollisionPool& pool){
    auto& c=pool.acquire_collision();c.radius=radius;c.offset_x=x;c.offset_y=y;c.offset_z=z;c.visited=visited;
    return e.add(32,&c);
}
ComponentSlots& remove_party_collision(ComponentSlots& e,PartyCollisionPool& pool){
    auto* c=e.get(32);e.remove(32);pool.release_collision(c);return e;
}
void start_party(MemberRoster& state,PartyStartHost& host){
    start_party(state,host,[&host]{return host.quest_has_auto_party();});
}
void start_party(MemberRoster& state,PartyStartHost& host,const std::function<bool()>& auto_party){
    auto& world=required(host.current_world());
    auto& subset=required(world.groups).get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{14}));
    std::vector<ComponentSlots*> old_subs;
    auto cursor=subset.cursor();
    while(const auto member=cursor.next()){
        if(character_is_sub(required(host.character(human(*member)))))old_subs.push_back(*member);
    }
    auto& destroy_world=required(host.current_world());
    auto& destruction=required(host.destruction_host(destroy_world));
    for(auto* entity:old_subs)if(entity)destroy_quest_entity(host.entity_id(*entity),true,destruction);

    auto* hero=member_hero(host.current_world());
    if(required(hero).contains(32))remove_party_collision(required(member_hero(host.current_world())),host);
    add_party_collision(required(member_hero(host.current_world())),12,0,5,0,false,host);
    const auto members=state.members;
    hero=member_hero(host.current_world());
    required(members.get()).append(hero); // Direct List.Add, no duplicate/Player guard.

    if(host.debug_enabled()&&auto_party()){
        std::vector<PartyCharacter*> candidates;
        const auto table=host.character_table();
        for(auto* data:required(table.get()))if(data&&character_is_sub(*data))candidates.push_back(data);
        // Count enumerates Take/Where before Concat enumerates it again.
        const auto priority_count=priority_characters(candidates).size();
        auto selected=priority_characters(candidates);
        for(std::size_t i=priority_count;i<8;++i)selected.push_back(host.random_character(candidates));
        if(host.debug_enabled()&&host.editor_enabled()){
            for(auto* data:selected)set_saved_character_level(required(data).saved,1200);
        }
        selected.push_back(host.character(human(member_hero(host.current_world()))));
        auto* distance=host.distance_system(required(host.current_world()));
        // Public LINQ OrderBy buffers before evaluating keys, once per element;
        // stable sorting uses the captured keys and performs no further provider calls.
        const auto buffered=selected;
        struct Ranked{PartyCharacter* data;std::int32_t key;};
        std::vector<Ranked> ranked;ranked.reserve(buffered.size());
        for(auto* data:buffered)ranked.push_back({data,required(distance).distance(data)});
        std::stable_sort(ranked.begin(),ranked.end(),[](const Ranked& a,const Ranked& b){return a.key<b.key;});
        auto source=host.auto_formation(static_cast<std::int32_t>(ranked.size()+1));
        auto formation=required(source.get()); // Enumerable.ToArray: independent mutable row.
        for(const auto& entry:ranked){
            const auto at=std::find(formation.begin(),formation.end(),1);
            const auto index=at==formation.end()?-1:static_cast<std::int32_t>(at-formation.begin());
            if(at!=formation.end())*at=999;
            // Re-resolve Data.charaData[record.id] before its main flag/factory.
            auto records=host.character_table();
            const auto id=required(entry.data).id;
            auto& data=required(required(records.get()).at(static_cast<std::size_t>(id)));
            place_character(state,host,data,id,index);
        }
    }else{
        for(std::size_t index=0;index<required(state.formation.get()).size();++index){
            const auto id=state.formation->at(index);
            if(id!=-1){auto& data=character(host,id);place_character(state,host,data,id,static_cast<std::int32_t>(index));}
        }
    }
    initialize_members(state,host.member_initialization());
}
} // namespace saga::core
