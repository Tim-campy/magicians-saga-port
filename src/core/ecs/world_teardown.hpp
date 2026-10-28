#pragma once
#include "entity_registration.hpp"

namespace saga::core {
class RegisteredEntityDestruction {
public:
    virtual ~RegisteredEntityDestruction()=default;
    virtual void destroy(ComponentSlots&,bool remove_registration)=0; // Bind Entity.Destroy's existing guard/sequence.
};
struct RegistryDestruction {EntityRegistry* registry=nullptr;};
// EntityManager.DestroyAllEntities 0x11379e8: live Values, Destroy(false), then reread registry and Clear.
void destroy_all_entities(RegistryDestruction&,RegisteredEntityDestruction&);
class WorldTeardownHost {
public:
    virtual ~WorldTeardownHost()=default;
    virtual void clear_visible_entities()=0;
    virtual void destroy_registered_entities()=0;
    virtual void remove_systems()=0;
    virtual void remove_event_handlers()=0;
};
// World.Destroy 0x11356f4. No group-directory, ID-counter or loaded-flag reset.
void destroy_world(WorldTeardownHost&);
}
