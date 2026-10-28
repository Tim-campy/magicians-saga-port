#pragma once
#include "ecs_combat.hpp"
#include "../ecs/entity_events.hpp"
#include <array>

namespace saga::core {
using ParameterExperienceQueue=std::vector<ComponentSlots*>;
// ExpSystem..ctor 0x115194c: thirteen distinct lists, with keys 0 through 12.
// Only constructor-created state is covered; save restoration remains external.
struct ParameterExperienceSystem {
    using Queues=std::array<std::shared_ptr<ParameterExperienceQueue>,13>;
    ParameterExperienceSystem();
    std::shared_ptr<Queues> queues; // Native dictionary +0x20; no reset in Init.
};
class ParameterExperienceInitializationWorld {
public:
    using Receiver=std::function<void(ComponentSlots*,std::int32_t)>;
    virtual ~ParameterExperienceInitializationWorld()=default;
    virtual void subscribe_parameter_experience(Receiver,std::optional<EventHashes>)=0; // Event 21.
};
// ExpSystem.Init 0x1151a90. Receiver lifetime borrows the system, not the world.
void initialize_parameter_experience(ParameterExperienceSystem&,ParameterExperienceInitializationWorld*,
                                     std::optional<EventHashes> =std::nullopt);
// Parameter.AddLevel 0x1178e2c: wrapping addition, upper clamp then lower clamp.
std::int32_t add_parameter_level(DamageParameter&,std::int32_t amount);
// ExpSystem.OnGetExp 0x1151f0c queues a pointer; it does not add experience.
void queue_parameter_experience(ParameterExperienceSystem&,ComponentSlots*,std::int32_t key);
void level_up_parameter(ComponentSlots*,std::int32_t key); // LevelUp 0x1151e68.
// ExpSystem.Update 0x1151b40. Enabled/pass/pause gates belong to the scheduler.
void update_parameter_experience(ParameterExperienceSystem&);
}
