#pragma once
#include "emitter_frame.hpp"

namespace saga::core {
struct ModificationSystemState {
    EntitySubset* subset=nullptr; // ModifyAnimatinSystem +0x20.
    // Same original List<Entity> contract as Emitter; no duplicate collection implementation.
    std::shared_ptr<EmitterExpiryQueue> expired; // +0x28.
};
class ModificationInitializationWorld {
public:
    virtual ~ModificationInitializationWorld()=default;
    virtual EntitySubset* modification_subset(const EventIntegers& all)=0;
};
class ModificationFrameHost {
public:
    virtual ~ModificationFrameHost()=default;
    virtual void destroy_modified_entity(ComponentSlots&)=0; // Actual Entity.Destroy(true).
    virtual void return_modification(Component*)=0; // Required pool push; null captures are valid.
};
// ModifyAnimatinSystem.Init 0x115f1d8; supplied-world And15 and fresh persistent queue.
void initialize_modifications(ModificationSystemState&,ModificationInitializationWorld*);
// Entity.RemoveModifyAnimation 0x1130414: capture (nullable), remove notification, then pool return.
ComponentSlots& remove_animation_modification(ComponentSlots&,ModificationFrameHost&);
// ModifyAnimatinSystem.Update 0x115f2b4: all type paths, counters and deferred current-component cleanup.
// Scheduler owns enabled/pass/pause gating; no world/rendering APIs enter this numeric pass.
void update_modifications(ModificationSystemState&,ModificationFrameHost&);
}
