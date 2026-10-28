#pragma once
#include "filter_identity.hpp"
#include "entity_registration.hpp"

namespace saga::core {
class GroupFilter {
public:
    virtual ~GroupFilter()=default;
    virtual std::int32_t hash()=0;
    virtual bool equals(GroupFilter&)=0;
    virtual bool matches(ComponentSlots*)=0;
};
class ComponentGroupFilter final:public GroupFilter {
public:
    ComponentGroupFilter()=default;
    explicit ComponentGroupFilter(std::vector<std::int32_t> all)
        :owned_all_(std::make_shared<const std::vector<std::int32_t>>(std::move(all))){identity.lists.all=owned_all_.get();}
    FilterIdentity identity;
    std::function<void()> diagnostic;
    std::int32_t hash()override{return identity.hash(diagnostic);}
    bool equals(GroupFilter&)override;
    bool matches(ComponentSlots*)override;
private:
    std::shared_ptr<const std::vector<std::int32_t>> owned_all_;
};
class DynamicSubsets final:public ComponentObserver {
public:
    DynamicSubsets(EntityRegistry& registry,EntitySubset::Identity identity):registry_(registry),identity_(std::move(identity)){}
    // EntityManager.GetSubSet 0x1138408. Retains the key/filter, populates before publication.
    EntitySubset& get(std::shared_ptr<GroupFilter>);
    void added(ComponentSlots&,std::int32_t,Component*)override;
    void removed(ComponentSlots&,std::int32_t,Component*)override;
    void changed(ComponentSlots&,std::int32_t,Component*,Component*)override;
private:
    struct Entry {
        std::shared_ptr<GroupFilter> filter;
        std::unique_ptr<EntitySubset> group;
        std::shared_ptr<Entry> next;
    };
    std::shared_ptr<Entry> find(std::uint32_t,GroupFilter&);
    EntityRegistry& registry_;EntitySubset::Identity identity_;
    std::unordered_map<std::uint32_t,std::shared_ptr<Entry>> buckets_;
    std::vector<std::shared_ptr<Entry>> order_;
    std::uint32_t version_=0;bool initialized_=false;
};
}
