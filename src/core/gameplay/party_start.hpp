#pragma once
#include "member_roster.hpp"
#include "member_selection.hpp"
#include "quest_return.hpp"

namespace saga::core {
// Mutable CharaData projection, separate from the read-only table loader.
struct PartyCharacter {
    std::int32_t id=0,flags=0;
    SavedCharacterProgression saved;
    std::int32_t image=0,weapon_id=0;
    CharacterParameters parameters; // CharaData growth rows; saved level remains separate.
};
using PartyCharacters=std::shared_ptr<std::vector<PartyCharacter*>>;
PartyCharacter bind_party_character(const CharacterProgressionRecord&,SavedCharacterProgression);
bool character_is_sub(const PartyCharacter&); // CharaData.get_isSub 0x116abdc.
struct HumanCreation {
    std::int32_t direction=0,character=0,level=0,formation_x=0,formation_z=0,experience=0;
    float x=0,y=0,z=0;
};
class PartyHumanFactory {
public:
    virtual ~PartyHumanFactory()=default;
    // Concrete implementation: HumanFactory in human_factory.hpp; storage remains host-owned.
    virtual ComponentSlots* create_human(const HumanCreation&)=0;
};
class PartyCreationHost {
public:
    virtual ~PartyCreationHost()=default;
    virtual PartyCharacters character_table()=0;
    // World.world_, not necessarily BaseSystem.world_ or the wrapper receiver.
    virtual PartyHumanFactory* human_factory_world()=0;
};
// World.CreateSubChara 0x1133edc. Resolves live saved level/EXP and delegates to
// World.world_.CreateHuman with direction and absolute position zero.
ComponentSlots* create_sub_character(std::int32_t x,std::int32_t z,std::int32_t id,PartyCreationHost&);
class PartyCollisionPool {
public:
    virtual ~PartyCollisionPool()=default;
    virtual ProjectileCollision& acquire_collision()=0;
    virtual void release_collision(Component*)=0; // Includes null captures.
};
ComponentSlots& add_party_collision(ComponentSlots&,std::int32_t radius,std::int32_t x,
    std::int32_t y,std::int32_t z,bool visited,PartyCollisionPool&);
ComponentSlots& remove_party_collision(ComponentSlots&,PartyCollisionPool&);
class PartyDistanceSystem {
public:
    virtual ~PartyDistanceSystem()=default;
    // HumanSystem.GetDistance(CharaData). Equipment/attribute inputs are still
    // explicit; this method is called once per buffered record, in input order.
    virtual std::int32_t distance(PartyCharacter*)=0;
};
class PartyStartHost:public PartyCreationHost,public PartyCollisionPool {
public:
    virtual PlayerWorldState* current_world()=0; // BaseSystem.world_, read afresh.
    virtual QuestEntityHost* destruction_host(PlayerWorldState&)=0;
    virtual QuestMemberId entity_id(ComponentSlots&)=0;
    virtual PartyCharacter* character(HumanRecordLink&)=0;
    virtual bool debug_enabled()=0;
    virtual bool editor_enabled()=0;
    virtual bool quest_has_auto_party()=0; // BaseData.Check(2), only queried if DEBUG.
    // IListExtension.RandomOrDefault(candidates,null): live RNG remains a service.
    // Candidate array length/references stay fixed; record fields remain live.
    virtual PartyCharacter* random_character(const std::vector<PartyCharacter*>&)=0;
    virtual PartyDistanceSystem* distance_system(PlayerWorldState&)=0;
    // AUTO_FORMATIONS[sorted_count + 1]; return the source row, which is copied
    // before marking occupied cells. Missing/out-of-range rows must fault.
    virtual std::shared_ptr<std::vector<std::int32_t>> auto_formation(std::int32_t index)=0;
    virtual MemberInitializationHost& member_initialization()=0;
};
// MemberSystem.OnStart 0x115c4a0. Both stored and debug auto-party paths;
// preserves existing lists, destroys sub-character snapshots and finally InitMembers.
void start_party(MemberRoster&,PartyStartHost&);
} // namespace saga::core
