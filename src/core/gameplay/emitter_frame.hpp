#pragma once
#include "emitter_factory.hpp"
#include "drop_collection.hpp"
#include "../ecs/entity_subset.hpp"

namespace saga::core {
// The original persistent List<Entity>, including version invalidation on Clear.
class EmitterExpiryQueue {
public:
    void add(ComponentSlots* e){entities_.push_back(e);++version_;}
    void clear(){entities_.clear();++version_;}
    std::size_t size()const{return entities_.size();}
    std::uint32_t version()const{return version_;}
    ComponentSlots* at(std::size_t index)const{return entities_.at(index);}
private:
    std::vector<ComponentSlots*> entities_;std::uint32_t version_=0;
};
struct EmitterSystemState {
    EntitySubset* subset=nullptr; // +0x20.
    std::shared_ptr<EmitterExpiryQueue> expired; // +0x28, required after Init, retained on failures.
    std::int32_t loop_counter=0; // +0x30; incremented before group validation.
};
class EmitterInitializationWorld {
public:
    virtual ~EmitterInitializationWorld()=default;
    virtual EntitySubset* emitter_subset(const EventIntegers& all)=0;
};
void initialize_emitters(EmitterSystemState&,EmitterInitializationWorld*); // Init 0x114b950.
class EmitterFrameHost:public BulletCatalogHost,public AnimationModificationPool,public PlaybackPool {
public:
    virtual ParticleFactoryHost* emitter_particle_world()=0; // Capture before source coordinates/speed.
    virtual DropEffectWorld* emitter_effect_world()=0;
    virtual DefeatColor& acquire_emitter_color()=0;
    virtual void destroy_emitter_entity(ComponentSlots&)=0; // Actual Entity.Destroy(true), after group pass.
};
// EmitterSystem.Update 0x114ba28. Scheduler owns enabled/pass/pause gating.
void update_emitters(EmitterSystemState&,EmitterFrameHost&);
}
