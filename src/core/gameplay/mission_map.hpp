#pragma once
#include "stage_start.hpp"
#include <utility>

namespace saga::core {
using MapRow=std::shared_ptr<std::vector<ComponentSlots*>>;
using MapGrid=std::shared_ptr<std::vector<MapRow>>;
struct MapComponent final:Component,StageGeometry {
    std::int32_t width=0,size_width=0,size_height=0,area_width=0,area_height=0;
    MapGrid chips,areas; // MapComponent +0x30/+0x38, jagged arrays; rows then columns.
};
struct MapChipComponent final:Component {
    std::int32_t data_id=0,staff_id=0;
    EventInstruction visiting_human_ids;
};
class MapFactoryHost:public EntityVisualHost,public PlaybackPool {
public:
    virtual MapChipComponent& acquire_map_chip()=0;
    // ComponentFactory.map_ is a shared singleton, not a per-call allocation.
    virtual MapComponent* shared_map_component()=0;
};
struct MapCreation {
    std::int32_t width=0,height=0,size_width=0,size_height=0,step_x=0,step_z=0,area_width=0,area_height=0;
    MapGrid chips;
};
struct MapChipCreation {
    std::int32_t column=0,row=0,y=0,resource=0,clip=0,layer=0,frame=0,texture=0,step_x=0,step_z=0;
    bool animate=false;
    std::int32_t data_id=0,extra_depth=0;
};
// AppData.InitWorld's actual CreateMap arguments, not a port of its system/Continue gates.
MapCreation initial_mission_map_creation();
// ext.util.Lib.CreateDoubleArray<Entity>: standard jagged allocation, no runtime reconstruction.
MapGrid make_map_grid(std::int32_t rows,std::int32_t columns);
// Entity.AddMap / World.CreateMap 0x11300e8 / 0x1133588; areas are retained.
ComponentSlots& add_map_component(ComponentSlots&,MapCreation,MapFactoryHost&);
ComponentSlots& create_map(MapCreation,EntityIdState&,MapFactoryHost&);
// Entity.AddMapChip / World.CreateMapChip 0x112fe58 / 0x113368c.
ComponentSlots& add_map_chip_component(ComponentSlots&,std::int32_t data_id,std::int32_t staff_id,MapFactoryHost&);
ComponentSlots& create_map_chip(MapChipCreation,EntityIdState&,MapFactoryHost&);

class MissionMapCursor {
public:virtual ~MissionMapCursor()=default;virtual void set_cursor(std::int32_t x,std::int32_t y)=0;
};
struct MapFactoryWorld {EntityIdState& ids;MapFactoryHost& factory;};
class MissionMapHost {
public:
    virtual ~MissionMapHost()=default;
    // Each query resolves current BaseSystem state; a missing world must fault,
    // while its map or the current quest-form result may be null until required.
    virtual MapComponent* current_mission_map()=0;
    virtual MissionMapCursor* current_map_cursor()=0;
    virtual MapFactoryWorld* current_map_factory()=0;
};
struct MissionMapSystem {MapComponent* map=nullptr;}; // MapSystem.map_ +0x20.
void initialize_mission_map(MissionMapSystem&,MissionMapHost&); // 0x115bcc8; global world, not Init argument.
std::int32_t quest_map_width(); // QuestData.get_widthNum 0x1172d44: always 3.
std::int32_t quest_map_start_image(const StageQuest&);
std::int32_t quest_map_middle_image(const StageQuest&);
std::int32_t quest_map_end_image(const StageQuest&);
std::int32_t quest_map_background_image(const StageQuest&);
bool map_location_in_range(MapComponent*,std::int32_t x,std::int32_t z,std::int32_t width,std::int32_t height,MissionMapHost&);
void set_mission_map_chip(MapComponent*,std::int32_t x,std::int32_t z,ComponentSlots*,MissionMapHost&);
// MapSystem.LoadMap 0x115bd38: captured dimensions, current image/map/world per tile.
void load_mission_map(MissionMapSystem&,std::shared_ptr<StageQuest>,MissionMapHost&);
class MissionMapEntry final:public StageMapLoader {
public:
    MissionMapEntry(MissionMapSystem& state,MissionMapHost& host):state_(state),host_(host){}
    void load_map(std::shared_ptr<StageQuest> quest)override{load_mission_map(state_,std::move(quest),host_);}
private:MissionMapSystem& state_;MissionMapHost& host_;
};
}
