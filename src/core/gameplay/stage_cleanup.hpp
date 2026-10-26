#pragma once
#include "quest_return.hpp"
#include <array>

namespace saga::core {
inline constexpr std::array<std::int32_t,5> stage_cleanup_slots{{34,29,26,9,31}};
class EntityDeletionHost {
public:
    virtual ~EntityDeletionHost()=default;
    virtual void remove_all_components(QuestMemberId)=0;
    virtual void dispose_entity_events(QuestMemberId)=0;
    // Resolve the entity's current registry key at this point, after component notifications.
    virtual void erase_registration(QuestMemberId)=0;
};
// EntityManager.DestroyEntity 0x1137950: removal callbacks run before event disposal.
void delete_entity_from_manager(QuestMemberId,bool remove_registration,EntityDeletionHost&);
class StageCleanupHost {
public:
    virtual ~StageCleanupHost()=default;
    // Query the current World.GetSubSet(And({slot})) and copy its entities_ collection.
    // Preserve original HashSet iteration order; use nullopt for null references.
    virtual std::vector<std::optional<QuestMemberId>> snapshot_group(std::int32_t slot)=0;
    virtual QuestEntityHost& destruction_host()=0;
    virtual std::optional<QuestMemberId> extra_entity()=0;
    virtual void clear_extra_entity()=0;
    virtual void clear_collecting_drops()=0;
};
// StageSystem.CleanUp 0x11654b8; optional StageSystem+0xa0 remains unnamed.
void cleanup_quest_stage(StageCleanupHost&);
}
