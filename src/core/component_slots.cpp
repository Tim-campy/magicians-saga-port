#include "core/component_slots.hpp"
#include <stdexcept>

namespace saga::core {
ComponentSlots::ComponentSlots(std::int32_t count, ComponentObserver* observer)
    : observer_(observer) {
    if (count < 0) throw std::length_error("negative component slot count");
    slots_.resize(static_cast<std::size_t>(count), nullptr);
}

std::size_t ComponentSlots::checked_index(std::int32_t index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= slots_.size())
        throw std::out_of_range("component index");
    return static_cast<std::size_t>(index);
}

// ELF 0x1136c84 / 0x1136b64: GetComponent37068 / Contains37065.
Component* ComponentSlots::get(std::int32_t index) const {
    return slots_[checked_index(index)];
}
bool ComponentSlots::contains(std::int32_t index) const { return get(index) != nullptr; }

// ELF 0x1136b9c / 0x1136c10: preserve left-to-right short circuit, including
// which invalid indices are reached. References represent non-null arrays.
bool ComponentSlots::contains_all(const std::vector<std::int32_t>& indices) const {
    for (const auto index : indices)
        if (!contains(index)) return false;
    return true;
}
bool ComponentSlots::contains_any(const std::vector<std::int32_t>& indices) const {
    for (const auto index : indices)
        if (contains(index)) return true;
    return false;
}

// ELF 0x113693c, Add37061: occupied slots ignore the supplied replacement.
ComponentSlots& ComponentSlots::add(std::int32_t index, Component* component) {
    const auto slot = checked_index(index);
    if (slots_[slot]) return *this;
    slots_[slot] = component;
    if (observer_) observer_->added(*this, index, component);
    return *this;
}

// ELF 0x1136a94, Change37064: null or identical replacement still notifies.
ComponentSlots& ComponentSlots::change(std::int32_t index, Component* component) {
    const auto slot = checked_index(index);
    auto* old = slots_[slot];
    if (!old) return add(index, component);
    slots_[slot] = component;
    if (observer_) observer_->changed(*this, index, old, component);
    return *this;
}

// ELF 0x1136a2c, Remove37063: detach before notifying with the old pointer.
ComponentSlots& ComponentSlots::remove(std::int32_t index) {
    const auto slot = checked_index(index);
    auto* old = slots_[slot];
    if (!old) return *this;
    slots_[slot] = nullptr;
    if (observer_) observer_->removed(*this, index, old);
    return *this;
}

// ELF 0x11369d4, RemoveAll37062: ascending live scan, not a snapshot/bulk clear.
ComponentSlots& ComponentSlots::remove_all() {
    for (std::size_t slot = 0; slot < slots_.size(); ++slot)
        remove(static_cast<std::int32_t>(slot));
    return *this;
}
} // namespace saga::core
