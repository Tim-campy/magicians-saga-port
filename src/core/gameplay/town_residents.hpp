#pragma once
#include "progression_tables.hpp"
#include "select_camera.hpp"
#include <array>

namespace saga::core {
// Borrowed native data/save projections. Their owners outlive initialization;
// retaining this wrapper preserves a selected row across catalog replacement.
struct TownResidentCharacter {
    const CharacterProgressionRecord* definition=nullptr;
    CharacterNewGameState* progress=nullptr;
};
using TownResidentCharacters=std::vector<std::shared_ptr<TownResidentCharacter>>;
struct TownResidentInitializationState {
    EventScript residents; // SelectForm +0x1d0; published before candidate work.
    std::int32_t present_value=0; // +0x1f0; retained unless the final Hits(10) succeeds.
};
class TownResidentInitializationHost {
public:
    virtual ~TownResidentInitializationHost()=default;
    virtual std::int32_t challenge_count()=0;
    virtual std::int32_t store_limit()=0; // Decoded persisted system-save value.
    virtual std::shared_ptr<TownResidentCharacters> characters()=0;
    // Capacity-indexed living array, including null gaps; queried after shuffle.
    virtual std::shared_ptr<TownResidentCharacters> living_submembers()=0;
    virtual EventInstruction active_range()=0;
    virtual std::array<std::int32_t,2> building_position(std::int32_t index)=0;
    virtual std::array<std::int32_t,2> area_position(std::int32_t index)=0;
    virtual bool conditions(const EventScript&)=0;
    virtual std::int32_t app_random(std::int32_t bound)=0;
    virtual std::int32_t lib_random(std::int32_t bound)=0;
    virtual std::int32_t lib_random(std::int32_t minimum,std::int32_t maximum)=0;
};
// SelectForm.Init 0x11ea550..0x11eaf78. Calls host RNG wrappers, never std::shuffle.
// Lib.Shuffle(source,null) returns a shuffled COPY which this caller discards;
// original candidate order survives, but all count-1 Lib draws still occur.
void initialize_town_residents(TownResidentInitializationState&,TownResidentInitializationHost&);

struct TownResidentGeometryState {
    const std::int32_t& camera_x;
    const std::int32_t& camera_y;
    const std::int32_t& left_extent;
    const std::int32_t& right_extent;
    const std::int32_t& view_x;
    const std::int32_t& view_y;
};
class TownResidentGeometryHost {
public:
    virtual ~TownResidentGeometryHost()=default;
    virtual std::int32_t resident_view_height()=0;
    virtual std::int32_t resident_town_capacity()=0;
    virtual std::int32_t resident_town_record_count()=0;
    virtual std::shared_ptr<CameraAreas> resident_geometry_areas()=0;
};
// GetBuildingPos 0x11eb8f4. Caller reads raw height before computing the pair.
std::array<std::int32_t,2> town_building_position(std::int32_t index,std::int32_t raw_height);
// GetPosObstacle 0x11f4088; capture camera/view origin before the height callback.
std::array<std::int32_t,2> town_obstacle_position(const TownResidentGeometryState&,TownResidentGeometryHost&);
// GetActiveX 0x11eb714; retain full callback order instead of simplifying bounds.
std::array<std::int32_t,2> town_resident_active_range(const TownResidentGeometryState&,TownResidentGeometryHost&);
}
