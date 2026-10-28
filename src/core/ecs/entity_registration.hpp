#pragma once
#include "component_slots.hpp"
#include <memory>
#include <unordered_map>
#include <optional>
#include <limits>

namespace saga::core {
class ComponentSubscriptions {
public:
    virtual ~ComponentSubscriptions()=default;
    virtual void subscribe_added(ComponentObserver&)=0;
    virtual void subscribe_removed(ComponentObserver&)=0;
    virtual void subscribe_changed(ComponentObserver&)=0;
};
// Entity's three multicast channels; observers and entities remain externally owned.
class ComponentEvents final:public ComponentObserver,public ComponentSubscriptions {
public:
    void subscribe_added(ComponentObserver&)override;
    void subscribe_removed(ComponentObserver&)override;
    void subscribe_changed(ComponentObserver&)override;
    void added(ComponentSlots&,std::int32_t,Component*)override;
    void removed(ComponentSlots&,std::int32_t,Component*)override;
    void changed(ComponentSlots&,std::int32_t,Component*,Component*)override;
    void dispose(); // Entity.DisposeEvents 0x1131adc; existing delivery snapshots survive.
private:
    using List=std::vector<ComponentObserver*>;
    using Channel=std::shared_ptr<const List>;
    Channel added_,removed_,changed_;
    static void append(Channel&,ComponentObserver&);
};
struct EntityRegistration {
    std::int32_t id;
    ComponentSlots* components;
    ComponentSubscriptions* events;
};
class EntityRegistry {
public:
    explicit EntityRegistry(ComponentObserver& observer):observer_(observer){}
    // EntityManager.AddEntity 0x1137758: insert first unless temporary, then subscribe A/R/C.
    void add(EntityRegistration*,bool temporary);
    bool contains(std::int32_t id)const{return entries_.find(id)!=entries_.end();}
    ComponentSlots* find(std::int32_t)const;
    void erase(std::int32_t); // Host primitive, not full entity destruction.
    void clear();
    class Cursor {
    public:
        explicit Cursor(const EntityRegistry& owner):owner_(owner),version_(owner.version_){}
        std::optional<ComponentSlots*> next();
    private:
        const EntityRegistry& owner_;std::uint32_t version_;std::size_t index_=0;
    };
    Cursor values()const{return Cursor(*this);}
private:
    ComponentObserver& observer_;
    static constexpr auto no_slot=std::numeric_limits<std::size_t>::max();
    struct Slot {ComponentSlots* entity;std::size_t next_free=no_slot;};
    std::unordered_map<std::int32_t,std::size_t> entries_;
    std::vector<Slot> slots_;
    std::size_t free_=no_slot;
    std::uint32_t version_=0;
};
}
