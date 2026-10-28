#pragma once
#include "../ecs/dynamic_subsets.hpp"

namespace saga::core {
// Narrow World projection. The host retains cached entities even after their
// components/registration are removed, until the world/cache owner releases them.
struct PlayerWorldState {
    ComponentSlots* cached_player=nullptr; // World +0x18.
    DynamicSubsets* groups=nullptr; // EntityManager.GetSubSet service.
};
// World.get_playerEntity 0x1133210; cache miss queries And({30}), then first entity.
ComponentSlots* world_player_entity(PlayerWorldState&);
// MemberSystem.get_hero 0x1156370 ignores its receiver. The host supplies the
// current AppData.world_ reference; a missing world is an error, not a null hero.
ComponentSlots* member_hero(PlayerWorldState*);
// StageSystem.GetFrontLineHuman 0x11657c4. Borrowed current members_ list is
// captured for this call; valid list/entity lifetimes and no concurrent mutation.
// Reads only Position.z: no Human, HP, destroyed or state eligibility checks.
ComponentSlots* front_line_human(const std::vector<ComponentSlots*>* members);
} // namespace saga::core
