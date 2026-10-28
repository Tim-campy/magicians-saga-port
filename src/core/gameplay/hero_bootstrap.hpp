#pragma once
#include "party_start.hpp"
#include "../ecs/system_initialization.hpp"

namespace saga::core {
class HeroBootstrapHost:public PartyCreationHost {
public:
    // BaseSystem.world_, independent of the World argument used by Init's gate.
    virtual PlayerWorldState* current_world()=0;
    // Receiver-specific CreateHuman service, distinct from World.world_ singleton.
    virtual PartyHumanFactory* world_factory(PlayerWorldState&)=0;
    virtual Component* player_marker()=0; // Shared Entity.playerComponent_ (+0xd8); raw pointer may be null.
    virtual PartyCharacter* character(HumanRecordLink&)=0;
};
// World.get_isNewGame 0x1135644; tests the whole byte, not its low bit.
bool world_is_new_game(const PlayerWorldState&);
// World.CreateHero 0x11338a8. First nonnull main record; create level 1/EXP 0
// through the supplied receiver, then AddPlayer. Does not publish/clear player cache.
ComponentSlots& create_hero(PartyHumanFactory* receiver,HeroBootstrapHost&);
// MemberSystem.Init 0x115c2e0. Loaded argument worlds cause no global queries;
// new ones create in the current world, then re-query hero and attempt slot 10.
void initialize_member_system(MemberRoster&,PlayerWorldState* argument,HeroBootstrapHost&);
// Explicit bridge into the existing SystemManager.Init dispatcher.
struct MemberBootstrapWorld final:InitializationWorld {PlayerWorldState* state=nullptr;};
class MemberSystemInitializer final:public InitSystem {
public:
    MemberSystemInitializer(MemberRoster& roster,HeroBootstrapHost& host):roster_(roster),host_(host){}
    void init(InitializationWorld*)override;
private:
    MemberRoster& roster_;HeroBootstrapHost& host_;
};
} // namespace saga::core
