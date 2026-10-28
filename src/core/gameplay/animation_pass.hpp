#pragma once
#include "entity_visual.hpp"
#include "../ecs/entity_subset.hpp"

namespace saga::core {
// Runtime SEB clock fields, distinct from EntitySprite.animation.frame.
struct SharedAnimationClip { std::int32_t frame=0, max_frame=0; }; // Seb +0x18/+0x1c.
using AnimationClips = std::shared_ptr<std::vector<SharedAnimationClip*>>;
struct AnimationResource { AnimationClips clips; }; // ResourceManager.seb +0x18.
using AnimationResources = std::shared_ptr<std::vector<AnimationResource*>>;
// Effective engine flags and loaded clip clocks come from the host; catalog
// construction and the flags' live writers remain unbound.
struct SharedAnimationGates { bool automatic=false, application_update=false; };
struct AnimationSystemState {
    EntitySubset* subset=nullptr; // +0x20; borrowed.
    AnimationResources resources; // +0x28; fixed-length managed-array contract.
};
// Resource/clip array lengths stay fixed and fit int32; borrowed pointees outlive
// initialization/updates. Replacing an array reference is distinct from resizing it.
class AnimationInitializationHost {
public:
    virtual ~AnimationInitializationHost()=default;
    virtual EntitySubset* subset(const std::vector<std::int32_t>& all)=0;
    virtual AnimationResource* resource(std::int32_t index)=0;
};
// AnimationSystem.Init 0x1141a58: store And(2,10), publish 24 entries, fill in order.
void initialize_animation(AnimationSystemState&,AnimationInitializationHost*);
// Seb.Frame 0x1bb16c4 (FDE entry); respects both gates and ARM signed remainder.
void advance_shared_animation(SharedAnimationClip&,const SharedAnimationGates&);
// Borrowed view used by the established Change/CutIn/Restore kernels.
EntityAnimationView live_animation(ComponentSlots&);
// AnimationSystem.Update 0x1141bc0. Scheduler owns enabled/pass gating.
void update_animations(AnimationSystemState&,const SharedAnimationGates&);
} // namespace saga::core
