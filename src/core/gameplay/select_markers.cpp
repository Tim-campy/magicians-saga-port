#include "select_markers.hpp"
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> const T& required(const std::shared_ptr<T>& p) {
    if (!p) throw std::invalid_argument("null marker data");
    return *p;
}
}
// AreaData.CheckAllCompleted, 0x116c1dc. Data queries deliberately repeated.
bool area_all_completed(const AreaMarkerRecord& area,SelectMarkerHost& host) {
    const auto ids=area.quest_ids;
    required(ids);
    for (std::size_t i=0;i<ids->size();++i) {
        const auto id=ids->at(i);
        if (id==-1) continue;
        if (!host.debug_enabled() && (required(host.quest(id)).flags & 2U)!=0) continue;
        if (!host.quest(id)) continue;
        if (required(host.quest(id)).state!=1) continue;
        const auto quest=host.quest(id);
        if ((required(quest).flags & 2U)==0 && !quest->completed) return false;
    }
    return true;
}
// SelectForm predicates 0x12072e0/0x120732c/0x1207364.
bool equipment_marker(const std::shared_ptr<EquipmentMarkerState>& item,EquipmentMarkerKind kind) {
    if (!item || item->quantity<=0) return false;
    if (kind==EquipmentMarkerKind::Equipment && (item->flags & 4U)!=0) return false;
    return item->is_new;
}
// SelectForm.Update 0x11ec0c0..0x11ec498: ordered area and equipment refresh.
bool refresh_select_markers(SelectMarkerState& state,SelectMarkerHost& host) {
    if (!state.scene_resume || !host.is_top_form()) return false;
    state.area_marks=std::make_shared<EventIntegers>(required(host.areas()).size(),0);
    for (std::size_t i=0;i<required(host.areas()).size();++i) {
        if (!required(host.areas()).at(i)) continue;
        const auto area=required(host.areas()).at(i);
        const auto completed=area_all_completed(required(area),host);
        required(state.area_marks);
        state.area_marks->at(i)=completed ? 2 : 1;
    }
    bool any=false;
    for (auto kind:{EquipmentMarkerKind::Equipment,EquipmentMarkerKind::Hat,EquipmentMarkerKind::Cloth}) {
        const auto items=host.equipment(kind);
        for (const auto& item:required(items)) {
            const auto marked=equipment_marker(item,kind); // Do not short-circuit enumeration.
            any=marked || any;
        }
    }
    state.weapon_new=any;
    state.scene_resume=false;
    return true;
}
}
