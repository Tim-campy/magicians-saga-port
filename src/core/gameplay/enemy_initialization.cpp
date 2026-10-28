#include "enemy_initialization.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing enemy initialization input");return *p;}
}
void initialize_enemies(EnemyCreationSystem& s,EnemyInitializationWorld* input,EnemyEventServices& services,
                       std::optional<std::array<EventHashes,3>> hashes){
    auto& world=required(input);
    s.subset=world.enemy_subset({24});
    s.gauge=world.enemy_gauge_system();
    s.stage=world.enemy_stage_system();
    s.members=world.enemy_member_system();
    s.maximum_by_section=std::make_shared<EventIntegers>(10,0);
    s.created_by_section=std::make_shared<EventIntegers>(10,0);
    const auto hash=[&](std::size_t index)->std::optional<EventHashes>{return hashes?std::optional<EventHashes>((*hashes)[index]):std::nullopt;};
    world.subscribe_enemy_impact([&services](ComponentSlots* e){
        enemy_projectile_impact(e,services.records,services.random,services.impact);
    },hash(0));
    world.subscribe_enemy_contact([&services](ComponentSlots* a,ComponentSlots* b){
        on_contact_collision(a,b,services.records,services.maxima,services.contact);
    },hash(1));
    world.subscribe_enemy_human_dead([&s,&services](ComponentSlots* dead){
        retarget_after_human_death(required(s.subset),dead,services.retarget);
    },hash(2));
}
}
