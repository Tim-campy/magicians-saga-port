#pragma once
#include "../ecs/component_slots.hpp"
#include "quest_return.hpp"
#include <memory>

namespace saga::core {
inline constexpr std::int32_t garbage_slot=22;
struct GarbageLifetime final:Component {std::int32_t frames=0;}; // GarbageComponent +0x10.
class GarbagePool {
public:
    virtual ~GarbagePool()=default;
    virtual GarbageLifetime& acquire_garbage()=0;
};
// Entity.AddGarbage 0x112eccc; acquire/write/add, without an existing-slot guard.
ComponentSlots& add_garbage_lifetime(ComponentSlots&,std::int32_t,GarbagePool&);
bool advance_garbage_lifetime(ComponentSlots&);
struct GarbageEntity {QuestMemberId id;ComponentSlots* components;};
class GarbageCursor {
public:
    virtual ~GarbageCursor()=default;
    // Enumerate live slot-22 subset in original order with its version checks.
    virtual std::optional<GarbageEntity> next()=0;
};
class GarbageUpdateHost:public QuestEntityHost {
public:
    virtual std::unique_ptr<GarbageCursor> open_garbage_cursor()=0;
};
// GarbageSystem.Update normal path: finish enumeration before destroying any entity.
// Reuses queue capacity between passes. The scheduler owns enabled/pass gating.
class GarbagePass {
public:
    void update(GarbageUpdateHost&);
private:
    std::vector<QuestMemberId> expired_;
};
}
