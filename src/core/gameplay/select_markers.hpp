#pragma once
#include "event_conditions.hpp"
namespace saga::core {
struct AreaMarkerRecord {std::shared_ptr<EventIntegers> quest_ids;};
struct AreaQuestState {std::int32_t state;std::uint8_t flags;bool completed;};
struct EquipmentMarkerState {std::int32_t quantity;std::uint32_t flags;bool is_new;};
enum class EquipmentMarkerKind { Equipment, Hat, Cloth };
struct SelectMarkerState {
    bool scene_resume;
    bool weapon_new;
    std::shared_ptr<EventIntegers> area_marks;
};
class SelectMarkerHost {
public:
    virtual ~SelectMarkerHost()=default;
    virtual bool is_top_form()=0;
    virtual bool debug_enabled()=0;
    virtual std::shared_ptr<ConditionCatalog<AreaMarkerRecord>> areas()=0;
    // Must perform Data.questData[id] bounds/null-catalog checks each call;
    // null entries themselves are returned, because DEBUG changes null handling.
    virtual std::shared_ptr<AreaQuestState> quest(std::int32_t id)=0;
    virtual std::shared_ptr<ConditionCatalog<EquipmentMarkerState>> equipment(EquipmentMarkerKind)=0;
};
bool area_all_completed(const AreaMarkerRecord&,SelectMarkerHost&);
bool equipment_marker(const std::shared_ptr<EquipmentMarkerState>&,EquipmentMarkerKind);
// Called after cloud updates, before UpdateNotifyValue. Returns whether refreshed.
bool refresh_select_markers(SelectMarkerState&,SelectMarkerHost&);
}
