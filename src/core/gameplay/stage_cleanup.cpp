#include "stage_cleanup.hpp"

namespace saga::core {
void delete_entity_from_manager(QuestMemberId id,bool remove_registration,EntityDeletionHost& host){
    host.remove_all_components(id);
    host.dispose_entity_events(id);
    if(remove_registration)host.erase_registration(id);
}
void cleanup_quest_stage(StageCleanupHost& host){
    for(const auto slot:stage_cleanup_slots){
        // Enumerable.ToArray<Entity> produces a separate snapshot for each live group.
        const auto snapshot=host.snapshot_group(slot);
        auto& entities=host.destruction_host();
        for(const auto& id:snapshot){
            // World.DestroyEntities forces remove_registration=true; except=null skips null members.
            if(id)destroy_quest_entity(*id,true,entities);
        }
    }
    if(const auto extra=host.extra_entity()){
        destroy_quest_entity(*extra,true,host.destruction_host());
        host.clear_extra_entity();
    }
    host.clear_collecting_drops();
}
}
