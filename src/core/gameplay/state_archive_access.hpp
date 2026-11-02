#pragma once
#include "../ecs/component_slots.hpp"
#include "../ecs/dynamic_subsets.hpp"
#include "../ecs/entity_events.hpp"
#include "../ecs/entity_registration.hpp"
#include "../ecs/entity_subset.hpp"
#include "../ecs/filter_identity.hpp"
#include "character_parameter_set.hpp"
#include "collision_pass.hpp"
#include "game_random.hpp"
#include "garbage_lifetime.hpp"
#include "info_frame.hpp"
#include "motion_pass.hpp"
#include "../ecs/system_initialization.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace saga::core {
// Const field access for the canonical archive, never a gameplay query. The
// writer anchors an object's identity before entering visit and handles public
// containers, aliases and callback binding tags. Each call is an ordered record;
// a visit may emit several records. No allocator capacities or callback bytes
// are exposed. DynamicSubsets' private Entry references use const void* solely
// as archive identities; their contents are emitted once through order_.
struct StateArchiveAccess {
    template<class V> static void visit(const InitList& s,V&& v){v("InitList",s.entries_,s.version_);}
    template<class V> static void visit(const LegacySystemRandom& s,V&& v){
        v("LegacySystemRandom",s.seeds_,s.index_,s.paired_index_);
    }
    template<class V> static void visit(const ComponentSlots& s,V&& v){
        v("ComponentSlots",s.observer_,static_cast<std::uint64_t>(s.slots_.size()));
        for(const auto* component:s.slots_)v("slot",component);
    }
    template<class V> static void visit(const ComponentEvents& s,V&& v){
        v("ComponentEvents",s.added_,s.removed_,s.changed_);
    }
    template<class V> static void visit(const EntityRegistry& s,V&& v){
        v("EntityRegistry",static_cast<const ComponentObserver*>(&s.observer_),
          slot_index(s.free_),s.version_,static_cast<std::uint64_t>(s.slots_.size()));
        for(const auto& slot:s.slots_)v("slot",slot.entity,slot_index(slot.next_free));
        std::vector<std::pair<std::int32_t,std::size_t>> entries;
        entries.reserve(s.entries_.size());
        for(const auto& entry:s.entries_)entries.emplace_back(entry.first,entry.second);
        std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.first<b.first;});
        v("entries",static_cast<std::uint64_t>(entries.size()));
        for(const auto& entry:entries)v("entry",entry.first,slot_index(entry.second));
    }
    template<class V> static void visit(const SubsetFilter& s,V&& v){
        v("SubsetFilter",s.all,s.any,s.none);
    }
    template<class V> static void visit(const EntitySubset& s,V&& v){
        v("EntitySubset",s.notifications,s.cached_entities,s.filter_,s.identity_,s.predicate_,
          slot_index(s.free_),s.version_,static_cast<std::uint64_t>(s.slots_.size()));
        for(const auto& slot:s.slots_)v("slot",slot.entity,slot_index(slot.next_free));
        using Lookup=std::tuple<std::size_t,std::uint32_t,ComponentSlots*>;
        std::vector<Lookup> lookup;lookup.reserve(s.lookup_.size());
        for(const auto& item:s.lookup_)lookup.emplace_back(item.second,item.first.hash,item.first.entity);
        std::sort(lookup.begin(),lookup.end(),[](const auto& a,const auto& b){
            if(std::get<0>(a)!=std::get<0>(b))return std::get<0>(a)<std::get<0>(b);
            return std::get<1>(a)<std::get<1>(b);
        });
        v("lookup",static_cast<std::uint64_t>(lookup.size()));
        for(const auto& item:lookup)v("key",slot_index(std::get<0>(item)),std::get<1>(item),std::get<2>(item));
    }
    template<class V> static void visit(const ComponentGroupFilter& s,V&& v){
        v("ComponentGroupFilter",s.identity,s.diagnostic,s.owned_all_,s.owned_none_);
    }
    template<class V> static void visit(const FilterIdentity& s,V&& v){
        v("FilterIdentity",s.lists,s.cached_);
    }
    template<class V> static void visit(const DynamicSubsets& s,V&& v){
        v("DynamicSubsets",static_cast<const EntityRegistry*>(&s.registry_),s.identity_,s.version_,
          s.initialized_,static_cast<std::uint64_t>(s.order_.size()));
        for(const auto& entry:s.order_){
            v("entry_identity",static_cast<const void*>(entry.get()));
            if(entry)v("entry",entry->filter,entry->group,static_cast<const void*>(entry->next.get()));
        }
        std::vector<std::pair<std::uint32_t,const void*>> buckets;
        buckets.reserve(s.buckets_.size());
        for(const auto& bucket:s.buckets_)buckets.emplace_back(bucket.first,static_cast<const void*>(bucket.second.get()));
        std::sort(buckets.begin(),buckets.end(),[](const auto& a,const auto& b){return a.first<b.first;});
        v("buckets",static_cast<std::uint64_t>(buckets.size()));
        // Heads refer to the entries above. Their emitted next identities retain
        // each bucket's linked order without visiting the same entry twice.
        for(const auto& bucket:buckets)v("bucket",bucket.first,bucket.second);
    }
    template<class V> static void visit(const EventHashes& s,V&& v){
        v("EventHashes",s.original,s.wrapper);
    }
    template<class V> static void visit(const EntityEvents& s,V&& v){
        v("EntityEvents",s.removal_log);
        event_tracking<EntityEvents::Single>(s.single_tracking_,"single_tracking",v);
        event_tracking<EntityEvents::Pair>(s.pair_tracking_,"pair_tracking",v);
        event_channels<EntityEvents::Single>(s.single_,s.single_tracking_,"single_channels",v);
        event_channels<EntityEvents::Pair>(s.pair_,s.pair_tracking_,"pair_channels",v);
    }
    template<class V> static void visit(const CharacterParameterSet& s,V&& v){
        // Preserve the borrowed CombatParameterLinks.values alias to this exact
        // map, as well as each Parameter identity in the owning storage.
        v("CharacterParameterSet",s.storage_,&s.values_);
    }
    template<class V> static void visit(const ProjectilePass& s,V&& v){
        v("ProjectilePass",s.completed_,s.version_);
    }
    template<class V> static void visit(const CollisionPass& s,V&& v){
        v("CollisionPass",s.pending_,s.version_);
    }
    template<class V> static void visit(const GarbagePass& s,V&& v){
        v("GarbagePass",s.expired_);
    }
    template<class T,class V> static void visit(const InfoBars<T>& s,V&& v){
        v("InfoBars",s.values_,s.version_);
    }
private:
    static std::uint64_t slot_index(std::size_t value){
        return value==std::numeric_limits<std::size_t>::max()
            ?std::numeric_limits<std::uint64_t>::max():static_cast<std::uint64_t>(value);
    }
    template<class F,class V> static void event_tracking(const typename EntityEvents::Tracking<F>& tracking,
                                                        const char* tag,V& v){
        v(tag,tracking.version,static_cast<std::uint64_t>(tracking.records.size()));
        for(const auto& record:tracking.records){
            if(record.wrapper&&*record.wrapper&&record.semantic_id.empty())
                throw std::logic_error("state archive requires a semantic entity event binding");
            v("record",record.event,record.hashes,record.wrapper,record.semantic_id);
        }
    }
    template<class F,class V> static void event_channels(const typename EntityEvents::Channels<F>& channels,
                                                        const typename EntityEvents::Tracking<F>& tracking,
                                                        const char* tag,V& v){
        std::vector<std::int32_t> keys;keys.reserve(channels.size());
        for(const auto& channel:channels)keys.push_back(channel.first);
        std::sort(keys.begin(),keys.end());
        v(tag,static_cast<std::uint64_t>(keys.size()));
        for(const auto key:keys){
            const auto& callbacks=channels.at(key);
            if(callbacks)for(const auto& callback:*callbacks){
                if(!callback||!*callback)continue;
                const auto record=std::find_if(tracking.records.begin(),tracking.records.end(),
                    [&](const auto& item){return item.event==key&&item.wrapper==callback;});
                if(record==tracking.records.end()||record->semantic_id.empty())
                    throw std::logic_error("state archive requires a tracked semantic entity event binding");
            }
            v("channel",key,callbacks);
        }
    }
};
} // namespace saga::core
