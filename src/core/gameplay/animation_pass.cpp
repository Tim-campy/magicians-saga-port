#include "animation_pass.hpp"
#include "contact_collision.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t add_word(std::int32_t a,std::int32_t b) {
    const auto bits=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t remainder(std::int32_t value,std::int32_t divisor) {
    // ARM SDIV returns zero for divisor zero, and INT_MIN for INT_MIN/-1.
    // W-register MSUB therefore retains value in the first case, gives 0 in the second.
    return divisor==0?value:static_cast<std::int32_t>(static_cast<std::int64_t>(value)%divisor);
}
EntitySprite& sprite(ComponentSlots& e) {
    auto* p=dynamic_cast<EntitySprite*>(e.get(2));
    if(!p)throw MissingAnimationComponent("seb");
    return *p;
}
EntityPlayback& playback(ComponentSlots& e) {
    auto* p=dynamic_cast<EntityPlayback*>(e.get(10));
    if(!p)throw MissingAnimationComponent("animation");
    return *p;
}
}
void initialize_animation(AnimationSystemState& state,AnimationInitializationHost* host) {
    if(!host)throw std::invalid_argument("missing animation initialization host");
    state.subset=host->subset({2,10});
    state.resources=std::make_shared<std::vector<AnimationResource*>>(24,nullptr);
    auto destination=state.resources;
    for(std::int32_t i=0;i<24;++i) {
        auto* value=host->resource(i);
        if(!destination)throw std::invalid_argument("missing animation resource array");
        destination->at(static_cast<std::size_t>(i))=value;
        if(i!=23)destination=state.resources; // Callback replacement affects the next store.
    }
}
void advance_shared_animation(SharedAnimationClip& clip,const SharedAnimationGates& gates) {
    if(gates.automatic||!gates.application_update)return;
    clip.frame=remainder(add_word(clip.frame,1),clip.max_frame);
}
EntityAnimationView live_animation(ComponentSlots& e) {
    auto* s=dynamic_cast<EntitySprite*>(e.get(2));
    auto* p=dynamic_cast<EntityPlayback*>(e.get(10));
    auto* d=dynamic_cast<ContactDirection*>(e.get(contact_direction_slot));
    return {s?&s->animation:nullptr,p?&p->state:nullptr,d?&d->direction:nullptr,
            e.contains(combat_human_slot),e.contains(16),e.contains(combat_enemy_slot)};
}
void update_animations(AnimationSystemState& state,const SharedAnimationGates& gates) {
    const auto resources=state.resources; // Shared phase captures this array.
    if(!resources)throw std::invalid_argument("missing animation resource array");
    for(std::size_t r=0;r<resources->size();++r) {
        auto* resource=resources->at(r);
        if(!resource)continue;
        const auto initial=resource->clips;
        if(!initial)throw std::invalid_argument("missing animation clip array");
        const auto count=initial->size();
        for(std::size_t i=0;i<count;++i) {
            const auto clips=resource->clips;
            if(!clips)throw std::invalid_argument("missing animation clip array");
            if(auto* clip=clips->at(i))advance_shared_animation(*clip,gates);
        }
    }
    if(!state.subset)throw std::invalid_argument("missing animation subset");
    auto cursor=state.subset->cursor();
    while(const auto entry=cursor.next()) {
        if(!*entry)throw std::invalid_argument("null animation entity");
        auto& entity=**entry;
        const auto resource_id=sprite(entity).animation.resource;
        auto& selection=sprite(entity).animation;
        if(resource_id<0)continue;
        const auto current_resources=state.resources; // Per-entity selection re-fetches it.
        if(!current_resources)throw std::invalid_argument("missing animation resource array");
        const auto clip_id=selection.clip;
        if(clip_id<0||static_cast<std::size_t>(resource_id)>=current_resources->size())continue;
        auto* resource=current_resources->at(static_cast<std::size_t>(resource_id));
        if(!resource)continue;
        const auto clips=resource->clips;
        if(!clips)throw std::invalid_argument("missing animation clip array");
        auto* clip=clips->at(static_cast<std::size_t>(clip_id));
        if(!clip)throw std::invalid_argument("missing animation clip");
        const auto max_frame=clip->max_frame;
        auto& destination=sprite(entity).animation;
        const auto previous=destination.frame;
        destination.frame=add_word(previous,playback(entity).state.rate);
        if(playback(entity).state.return_clip!=-1&&sprite(entity).animation.frame>=max_frame)
            restore_animation(live_animation(entity));
        auto& frame=sprite(entity).animation.frame;
        frame=remainder(frame,max_frame); // Captured old clip maximum, including after restoration.
    }
}
} // namespace saga::core
