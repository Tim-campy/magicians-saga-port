#pragma once
#include "component_filter.hpp"
#include <functional>
#include <limits>
#include <optional>
#include <unordered_map>

namespace saga::core {
struct SubsetFilter {
    const std::vector<std::int32_t> *all=nullptr,*any=nullptr,*none=nullptr;
};
class EntitySubset {
public:
    // IDs are supplied by the registry; allocation/restoration policy is not invented here.
    using Predicate=std::function<bool(ComponentSlots*)>;
    using Identity=std::function<std::int32_t(const ComponentSlots&)>;
    EntitySubset(const SubsetFilter& filter,Identity identity):filter_(&filter),identity_(std::move(identity)){}
    EntitySubset(Predicate predicate,Identity identity);
    EntitySubset(const EntitySubset&)=delete;
    EntitySubset& operator=(const EntitySubset&)=delete;
    ComponentObserver* notifications=nullptr;
    // Original cache is invalidated here; cache production/get_entity remains unported.
    const std::vector<ComponentSlots*>* cached_entities=nullptr;
    bool add(ComponentSlots*);
    bool add(ComponentSlots*,std::int32_t,Component*);
    void remove(ComponentSlots*,std::int32_t,Component*);
    void change(ComponentSlots&,std::int32_t,Component*,Component*);
    std::size_t size()const{return lookup_.size();}
    class Cursor {
    public:
        explicit Cursor(const EntitySubset& owner):owner_(owner),version_(owner.version_){}
        std::optional<ComponentSlots*> next();
    private:
        const EntitySubset& owner_;std::uint32_t version_;std::size_t index_=0;
    };
    Cursor cursor()const{return Cursor(*this);}
    // Subset.get_entity 0x1139434: empty -> null, otherwise public Enumerable.First.
    ComponentSlots* first_entity()const;
private:
    struct Key {
        ComponentSlots* entity;std::uint32_t hash;
        bool operator==(const Key& other)const{return entity==other.entity&&hash==other.hash;}
    };
    struct Hash {std::size_t operator()(const Key& k)const{return std::hash<ComponentSlots*>{}(k.entity)^static_cast<std::size_t>(k.hash);}};
    static constexpr auto no_slot=std::numeric_limits<std::size_t>::max();
    struct Slot {ComponentSlots* entity;std::size_t next_free=no_slot;};
    const SubsetFilter* filter_=nullptr;Identity identity_;Predicate predicate_;
    std::unordered_map<Key,std::size_t,Hash> lookup_;
    std::vector<Slot> slots_;std::size_t free_=no_slot;std::uint32_t version_=0;
    Key key(ComponentSlots*)const;
    bool matches(ComponentSlots*)const;
    void insert(ComponentSlots*);
    bool erase(ComponentSlots*);
};
// Fixed ordered group wiring for the gameplay host, not World.GetSubSet/dedup.
// Register before component writes; constructor does not scan existing entities.
class SubsetMembership final:public ComponentObserver {
public:
    explicit SubsetMembership(std::vector<EntitySubset*> groups):groups_(std::move(groups)){}
    void added(ComponentSlots&,std::int32_t,Component*)override;
    void removed(ComponentSlots&,std::int32_t,Component*)override;
    void changed(ComponentSlots&,std::int32_t,Component*,Component*)override;
private:
    const std::vector<EntitySubset*> groups_;
};
}
