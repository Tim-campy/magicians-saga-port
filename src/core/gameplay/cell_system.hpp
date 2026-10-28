#pragma once
#include "cell_component.hpp"
#include "mission_map.hpp"
#include "../ecs/entity_subset.hpp"
#include "../ecs/entity_events.hpp"
#include <unordered_map>

namespace saga::core {
using CellEntities=std::shared_ptr<std::vector<ComponentSlots*>>;
class CellInitializationWorld {
public:
    virtual ~CellInitializationWorld()=default;
    virtual EntitySubset* cell_subset(const std::vector<std::int32_t>&)=0;
    virtual MapComponent* cell_map()=0;
};
class CellChangeEvents {
public:
    using Receiver=std::function<void(ComponentSlots*,std::int32_t)>;
    virtual ~CellChangeEvents()=default;
    virtual void subscribe_cell_change(Receiver,std::optional<EventHashes>)=0; // Event 14, Entity/int32.
    virtual void send_cell_change(ComponentSlots*,std::int32_t old_key)=0;
};
class CellFrameHost {
public:
    virtual ~CellFrameHost()=default;
    virtual CellChangeEvents* current_cell_change_world()=0; // Fresh BaseSystem global world.
};
struct CellSystemState {
    EntitySubset* subset=nullptr;MapComponent* map=nullptr;CellEntities changed; // +0x20/+0x28/+0x30.
};
void initialize_cells(CellSystemState&,CellInitializationWorld*); // CellSystem.Init 0x1145920.
// Update 0x1145a18: centered conversion, immediate event 14, then clear scratch list.
void update_cells(CellSystemState&,CellFrameHost&);

struct CellIndexState {
    using Buckets=std::unordered_map<std::int32_t,CellEntities>;
    MapComponent* map=nullptr;std::shared_ptr<Buckets> buckets; // CellCullingSystem +0x20/+0x28.
};
class CellIndexHost:public CellFrameHost {
public:
    using ComponentReceiver=std::function<void(ComponentSlots*,std::int32_t,Component*)>;
    virtual CellInitializationWorld* current_cell_world()=0; // Key methods resolve this world's map afresh.
    virtual void subscribe_cell_added(EntitySubset&,ComponentReceiver)=0;
    virtual void subscribe_cell_removed(EntitySubset&,ComponentReceiver)=0;
};
// CellCullingSystem.Init 0x11448e0: map/dictionary, subset observers, population,
// then event-14 subscription through the current global world, not the Init world.
void initialize_cell_index(CellIndexState&,CellInitializationWorld*,CellIndexHost&,
                           std::optional<EventHashes> =std::nullopt);
std::int32_t cell_key(MapComponent*,std::int32_t xi,std::int32_t yi); // Static GetCellKey 0x11454dc.
std::int32_t cell_key(std::int32_t xi,std::int32_t yi,CellIndexHost&); // 0x1145124.
std::int32_t cell_key(CellComponent*,CellIndexHost&); // 0x11451ac; world lookup before cell validation.
void add_indexed_cell(CellIndexState&,ComponentSlots*,std::int32_t slot,Component*,CellIndexHost&); // 0x1144c44.
void remove_indexed_cell(CellIndexState&,ComponentSlots*,std::int32_t slot,Component*,CellIndexHost&); // 0x1145230.
void change_indexed_cell(CellIndexState&,ComponentSlots*,std::int32_t old_key,CellIndexHost&); // 0x1145324.
// OnCellChange 0x11455ac is a tail branch to ChangeCellEntity, not a no-op.
void on_cell_change(CellIndexState&,ComponentSlots*,std::int32_t old_key,CellIndexHost&);
CellEntities cell_entities(CellIndexState&,std::int32_t key); // 0x1145518: null for absent key.
CellEntities cell_entities(CellIndexState&,std::int32_t xi,std::int32_t yi,CellIndexHost&); // 0x11454fc.
// AppData.GetCellLocation 0x1111a50: mutate/return the shared int32[2] scratch.
// x/y are the caller's camera offsets; no implicit desktop viewport is invented.
EventInstruction project_cell_location(EventInstruction,std::int32_t x,std::int32_t y,
                                       std::int32_t step_x,std::int32_t half_step_z);
class CellVisibilityView {
public:
    virtual ~CellVisibilityView()=default;
    virtual std::int32_t cell_game_width()=0; // QuestForm.GetGameWidth, including current scale.
    virtual std::int32_t cell_game_height()=0;
    virtual EventInstruction cell_view_origin(std::int32_t step_x,std::int32_t half_step_z)=0;
};
class CellVisibilityHost:public CellIndexHost,public CellVisibilityView {
public:
    virtual void clear_cell_visibility()=0;
    virtual void add_cell_visibility(ComponentSlots&)=0; // Current global World's HashSet, deduplicated.
    virtual std::int32_t cell_visibility_count()=0;
    virtual void store_cell_visibility_count(std::int32_t)=0;
};
// CellCullingSystem.Update 0x1144e10. Pass-zero gate is owned by GameplayUpdate.
void update_cell_visibility(CellIndexState&,CellVisibilityHost&);
}
