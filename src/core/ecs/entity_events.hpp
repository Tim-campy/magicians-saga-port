#pragma once
#include "component_slots.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace saga::core {
// Entity-only projection of EventManager's one/two-argument channels.
// Public delegate combination semantics, not a .NET delegate implementation.
// Nonempty callbacks and externally managed receiver lifetimes are required.
class EntityEvents {
public:
    using Single=std::function<void(ComponentSlots*)>;
    using Pair=std::function<void(ComponentSlots*,ComponentSlots*)>;
    void subscribe(std::int32_t,Single);
    void subscribe(std::int32_t,Pair);
    void send(std::int32_t,ComponentSlots*)const;
    void send(std::int32_t,ComponentSlots*,ComponentSlots*)const;
private:
    template<class F>using List=std::vector<std::shared_ptr<F>>;
    template<class F>using Channels=std::unordered_map<std::int32_t,std::shared_ptr<const List<F>>>;
    Channels<Single> single_;
    Channels<Pair> pair_;
    template<class F>static void append(Channels<F>&,std::int32_t,F);
};
// Registration slice of Stage, Human, Enemy and Projectile Init, in factory order.
// Other events and Init effects remain external. All seven callbacks are required.
struct CombatReceivers {
    EntityEvents::Single stage_dead,human_impact,enemy_impact,enemy_dead,projectile_impact;
    EntityEvents::Pair human_contact,enemy_contact;
};
void bind_combat_receivers(EntityEvents&,const CombatReceivers&);
}
