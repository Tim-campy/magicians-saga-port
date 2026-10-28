#include "human_initialization.hpp"
#include <stdexcept>

namespace saga::core {
void initialize_humans(HumanFrameSystem& s,HumanInitializationWorld* input,HumanEventServices& services,
                       std::optional<std::array<EventHashes,2>> hashes){
    if(!input)throw std::invalid_argument("missing human initialization world");
    s.subset=input->human_subset({14});s.map=input->human_map();
    s.stage=input->human_stage_system();s.enemies=input->human_enemy_system();s.members=input->human_member_system();
    const auto hash=[&](std::size_t i)->std::optional<EventHashes>{return hashes?std::optional<EventHashes>((*hashes)[i]):std::nullopt;};
    input->subscribe_human_impact([&services](ComponentSlots* e){human_projectile_impact(e,services.impact);},hash(0));
    input->subscribe_human_contact([&services](ComponentSlots* a,ComponentSlots* b){on_human_contact(a,b,services.records,services.contact);},hash(1));
}
}
