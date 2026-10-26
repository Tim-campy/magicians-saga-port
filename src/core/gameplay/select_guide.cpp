#include "select_guide.hpp"
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> T& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("missing guide data");
    return *value;
}
template<class T,class Predicate> std::int32_t count(const std::shared_ptr<T>& values,Predicate predicate) {
    auto& rows=required(values);
    std::int32_t total=0;
    for(std::size_t i=0;i<rows.size();++i)if(predicate(rows.at(i))) {
        if(total==std::numeric_limits<std::int32_t>::max())throw std::overflow_error("guide count overflow");
        ++total;
    }
    return total;
}
}
// SelectForm.<CheckGuidArrow>b__162_0, 0x11f5a4c.
bool guide_buildable_slot(const std::shared_ptr<EventIntegers>& row,SelectGuideHost& host) {
    return row && row->at(1)==-1 && host.can_build(row->at(0));
}
// <>c.<CheckGuidArrow>b__162_1, 0x12073bc.
bool guide_active_town(const std::shared_ptr<GuideTownRecord>& row) {
    return row && (static_cast<std::uint32_t>(row->flags)&1U)!=0 && row->id!=-1;
}
// <>c.<CheckGuidArrow>b__162_2, 0x12073f8.
bool guide_occupied_slot(const std::shared_ptr<EventIntegers>& row) {return row && row->at(1)!=-1;}
// SelectForm.CheckGuidArrow, 0x11ee8b8.
void check_select_guide(SelectGuideState& state,bool force,SelectGuideHost& host) {
    if(force)state.flags=std::make_shared<std::vector<std::uint8_t>>(2,0);
    auto flags=state.flags;
    if(force || required(flags).at(0)!=0) {
        const auto challenges=host.sum_quest_challenges();
        required(flags).at(0)=static_cast<std::uint8_t>(challenges==0);
        if(!force)flags=state.flags;
    }
    if(force || required(flags).at(1)!=0) {
        const auto available=count(host.building_slots(),[&](const auto& row){return guide_buildable_slot(row,host);});
        const auto active=count(host.towns(),guide_active_town);
        const auto occupied=count(host.building_slots(),guide_occupied_slot);
        required(state.flags).at(1)=static_cast<std::uint8_t>(available>0 && occupied==active);
    }
}
}
