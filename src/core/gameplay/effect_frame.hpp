#pragma once
#include "emitter_frame.hpp"

namespace saga::core {
struct EffectSystemState {
    EntitySubset* subset=nullptr; // EffectSystem +0x20: And26/Nor4.
    std::shared_ptr<EmitterExpiryQueue> expired; // +0x28; same List<Entity> contract.
};
class EffectInitializationWorld {
public:
    virtual ~EffectInitializationWorld()=default;
    virtual EntitySubset* effect_subset(const EventIntegers& all,const EventIntegers& none)=0;
};
class EffectFrameHost {
public:
    virtual ~EffectFrameHost()=default;
    virtual bool effect_parent_destroyed(ComponentSlots&)=0;
    virtual void destroy_effect_entity(ComponentSlots&)=0;
};
void initialize_effects(EffectSystemState&,EffectInitializationWorld*); // Init 0x1149cf0.
void update_normal_effect(ComponentSlots*); // 0x114a294.
void update_damage_number_effect(ComponentSlots*); // 0x114a318.
void update_time_effect(ComponentSlots*) noexcept; // 0x114a36c; not called by Update.
void update_level_up_effect(ComponentSlots*); // 0x114a370.
void update_jump_number_effect(ComponentSlots*); // 0x114a3a0.
void update_warehouse_balloon_effect(ComponentSlots*); // 0x114a3d0.
void update_string_background_effect(ComponentSlots*); // 0x114a400.
// EffectSystem.Update 0x1149e6c; scheduler owns enabled/pass/pause gates.
void update_effects(EffectSystemState&,EffectFrameHost&);
}
