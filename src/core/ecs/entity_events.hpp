#pragma once
#include "component_slots.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <optional>
#include <array>

namespace saga::core {
struct EventHashes {std::int32_t original,wrapper;}; // Runtime delegate hashes; never inferred from function pointers.
// Entity-only projection of EventManager's one/two-argument channels.
// Public delegate combination semantics, not a .NET delegate implementation.
// Nonempty callbacks and externally managed receiver lifetimes are required.
class EntityEvents {
public:
    using Single=std::function<void(ComponentSlots*)>;
    using Pair=std::function<void(ComponentSlots*,ComponentSlots*)>;
    void subscribe(std::int32_t,Single,std::optional<EventHashes> =std::nullopt);
    void subscribe(std::int32_t,Pair,std::optional<EventHashes> =std::nullopt);
    void send(std::int32_t,ComponentSlots*)const;
    void send(std::int32_t,ComponentSlots*,ComponentSlots*)const;
    bool unsubscribe_single(std::int32_t event,std::int32_t delegate_hash);
    bool unsubscribe_pair(std::int32_t event,std::int32_t delegate_hash);
    void remove_all(); // Original wrapper-hash lookup and list invalidation behavior.
    std::function<void(std::int32_t)> removal_log; // Log.Println after successful removal.
private:
    template<class F>using List=std::vector<std::shared_ptr<F>>;
    template<class F>using Channels=std::unordered_map<std::int32_t,std::shared_ptr<const List<F>>>;
    Channels<Single> single_;
    Channels<Pair> pair_;
    template<class F>struct Record {std::int32_t event;std::optional<EventHashes> hashes;std::shared_ptr<F> wrapper;};
    template<class F>struct Tracking {std::vector<Record<F>> records;std::uint32_t version=0;};
    Tracking<Single> single_tracking_;
    Tracking<Pair> pair_tracking_;
    template<class F>static void append(Channels<F>&,Tracking<F>&,std::int32_t,F,std::optional<EventHashes>);
    template<class F>bool remove(Channels<F>&,Tracking<F>&,std::int32_t,std::int32_t);
    template<class F>void remove_tracking(Channels<F>&,Tracking<F>&);
};
// Registration slice of Stage, Human, Enemy and Projectile Init, in factory order.
// Other events and Init effects remain external. All seven callbacks are required.
struct CombatReceivers {
    EntityEvents::Single stage_dead,human_impact,enemy_impact,enemy_dead,projectile_impact;
    EntityEvents::Pair human_contact,enemy_contact;
    // Stage death, Human impact/contact, Enemy impact/contact/death, Projectile impact.
    std::optional<std::array<EventHashes,7>> hashes;
};
void bind_combat_receivers(EntityEvents&,const CombatReceivers&);
}
