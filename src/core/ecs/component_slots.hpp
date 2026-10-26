#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace saga::core {

// Portable type boundary; not an IL2CPP object layout. Ownership stays with ECS.
struct Component {
    virtual ~Component() = default;
};

class ComponentSlots;
struct ComponentObserver {
    virtual ~ComponentObserver() = default;
    virtual void added(ComponentSlots&, std::int32_t, Component*) {}
    virtual void removed(ComponentSlots&, std::int32_t, Component*) {}
    virtual void changed(ComponentSlots&, std::int32_t, Component*, Component*) {}
};

// BaseEntity's fixed slot storage and notifications (docs/archive/reports/35-component-slots.md).
// Borrowed components/observer must remain alive through each operation/callback.
// One allocation on construction; operations don't allocate, clone or delete.
class ComponentSlots {
public:
    explicit ComponentSlots(std::int32_t count, ComponentObserver* observer = nullptr);
    ComponentSlots(const ComponentSlots&) = delete;
    ComponentSlots& operator=(const ComponentSlots&) = delete;
    ComponentSlots(ComponentSlots&&) = delete;
    ComponentSlots& operator=(ComponentSlots&&) = delete;

    Component* get(std::int32_t index) const;
    bool contains(std::int32_t index) const;
    bool contains_all(const std::vector<std::int32_t>& indices) const;
    bool contains_any(const std::vector<std::int32_t>& indices) const;
    ComponentSlots& add(std::int32_t index, Component* component);
    ComponentSlots& change(std::int32_t index, Component* component);
    ComponentSlots& remove(std::int32_t index);
    ComponentSlots& remove_all();

private:
    std::vector<Component*> slots_;
    ComponentObserver* observer_;
    std::size_t checked_index(std::int32_t index) const;
};
} // namespace saga::core
