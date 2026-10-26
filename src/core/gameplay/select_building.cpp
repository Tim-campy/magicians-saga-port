#include "select_building.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> T& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("null building menu data");
    return *value;
}
std::int32_t saved_distance(float value) {
    if(std::isnan(value))return 0;
    if(value==std::numeric_limits<float>::infinity() || value<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(value>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(value);
}
std::int32_t kind(std::int32_t id,SelectBuildingHost& host) {
    const auto row=required(host.buildings()).at(static_cast<std::size_t>(id));
    return required(row).kind;
}
}
// BuildingData.IsShop, 0x116c9c4.
bool building_is_shop(std::int32_t value) {return (value>=1 && value<=5) || value==9 || value==10;}
// SelectForm.Update 0x11ecbe0..0x11ecce4. Capture each array before its value read.
void save_select_position(const SelectBuildingState& state,SelectBuildingHost& host) {
    auto save=host.save_integers();required(save).at(11)=state.camera_x;
    save=host.save_integers();required(save).at(12)=state.camera_y;
    save=host.save_integers();const auto distance=saved_distance(state.drag_distance);required(save).at(10)=distance;
}
bool process_select_building_button(SelectBuildingState& state,SelectFormPassState& form,SelectBuildingHost& host) {
    if(!host.pressed(0x08000000U))return false;
    host.play_button_sound();
    const auto slots=host.building_slots();
    if(state.selected_chip<0)return false;
    if(static_cast<std::size_t>(state.selected_chip)>=required(slots).size())return false;
    save_select_position(state,host);
    const auto row=slots->at(static_cast<std::size_t>(state.selected_chip));
    if(row) {
        const auto id=row->at(1);
        EventObject child;
        bool push=false;
        if(id==-1) {
            if(host.can_build(state.selected_chip)) {
                state.unlock_info_number=-1;state.unlock_info_frames=-1;
                child=host.make_build_list(state.selected_chip);push=true;
            }
        } else {
            const auto shop=building_is_shop(kind(id,host));
            const auto type=kind(id,host); // Original always fetches again, even for a shop.
            if(shop) {child=host.make_shop(type);push=true;}
            else if(type==6) {
                const auto members=host.living_members();
                const auto index=state.selected_chip;
                const auto member=required(members).at(static_cast<std::size_t>(index));
                if(member)child=host.make_human_house(member,index,false);
                else {
                    child=host.make_shop(6);
                    if(!child)throw std::invalid_argument("null house shop");
                    host.set_shop_selection(child,state.selected_chip);
                }
                push=true;
            } else if(kind(id,host)==7) {child=host.make_subform(15);push=true;}
            else if(kind(id,host)==12) {const auto value=host.system_share_value();child=host.make_share_window(value==0);push=true;}
        }
        if(push)push_select_form(form.form,child,form.auto_save,form.events.event_check_requested);
    }
    state.selected_chip=-1;
    return true;
}
SelectBuildingUpdate update_select_through_building(SelectUpdateBindings& bindings,SelectBuildingState& state,SelectBuildingHost& host) {
    const auto prior=update_select_through_navigation(bindings);
    if(prior.result==SelectNavigationResult::ReturnTrue)return {prior.snapshot,true};
    return {prior.snapshot,process_select_building_button(state,*bindings.state,host)};
}
}
