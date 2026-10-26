#include "combat_event_receivers.hpp"
#include <stdexcept>

namespace saga::core {
void projectile_impact_cleanup(ComponentSlots* entity,CombatEventHost& host){
    if(!entity)throw std::invalid_argument("missing impact entity");
    if(entity->contains(31))host.destroy(*entity,true);
    if(host.destroyed(*entity)||!entity->contains(5))return;
    // Original validates this reference but does not use its payload.
    if(!entity->get(27))throw std::invalid_argument("missing impact projectile");
}
void retarget_after_human_death(EntitySubset& enemies,ComponentSlots* dead,CombatEventHost& host){
    auto cursor=enemies.cursor();
    while(auto next=cursor.next()){
        auto* entity=*next;
        if(!entity||host.destroyed(*entity))continue;
        auto* enemy=dynamic_cast<EnemyRecordLink*>(entity->get(combat_enemy_slot));
        if(!enemy)throw std::invalid_argument("missing enemy while retargeting");
        if(enemy->opponent!=dead)continue;
        // Capture the component before SearchTarget; callbacks may replace its slot.
        auto* replacement=host.search_opponent(*entity);
        enemy->opponent=replacement;
    }
}
}
