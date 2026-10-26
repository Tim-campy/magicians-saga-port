#include "garbage_lifetime.hpp"
#include <stdexcept>

namespace saga::core {
ComponentSlots& add_garbage_lifetime(ComponentSlots& e,std::int32_t frames,GarbagePool& pool){
    auto& c=pool.acquire_garbage();c.frames=frames;return e.add(garbage_slot,&c);
}
bool advance_garbage_lifetime(ComponentSlots& e){
    auto* c=dynamic_cast<GarbageLifetime*>(e.get(garbage_slot));
    if(!c)throw std::invalid_argument("missing garbage component");
    const auto bits=static_cast<std::uint32_t>(c->frames)-1U;
    c->frames=static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
    return c->frames<=0;
}
void GarbagePass::update(GarbageUpdateHost& host){
    // Keep the active queue local: destruction callbacks can start another pass.
    std::vector<QuestMemberId> expired;expired.swap(expired_);expired.clear();
    {
        auto cursor=host.open_garbage_cursor();
        if(!cursor)throw std::invalid_argument("missing garbage subset cursor");
        while(const auto entry=cursor->next()){
            if(!entry->components)throw std::invalid_argument("null garbage entity");
            if(advance_garbage_lifetime(*entry->components))expired.push_back(entry->id);
        }
    }
    for(const auto id:expired)destroy_quest_entity(id,true,host);
    expired.clear();expired_.swap(expired);
}
}
