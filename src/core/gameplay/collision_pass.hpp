#pragma once
#include "collision_check.hpp"
#include <memory>
#include <optional>
#include <utility>

namespace saga::core {
class CollisionCursor {
public:
    virtual ~CollisionCursor()=default;
    // Fresh live And({32,0}) subset cursor, preserving order/version checks.
    // nullopt ends enumeration; a contained null pointer represents a corrupt entry.
    virtual std::optional<ComponentSlots*> next()=0;
};
class CollisionPassHost {
public:
    virtual ~CollisionPassHost()=default;
    virtual std::unique_ptr<CollisionCursor> open_collision_cursor()=0;
    virtual void send_collision(std::int32_t event_id,ComponentSlots&,ComponentSlots&)=0;
};
// CollisionSystem.Update 0x1145f40. Scheduler owns enabled gating.
class CollisionPass {
public:
    void update(CollisionPassHost&);
private:
    // Original list is persistent and only cleared after successful event delivery.
    std::vector<std::pair<ComponentSlots*,ComponentSlots*>> pending_;
    std::uint32_t version_=0;
};
}
