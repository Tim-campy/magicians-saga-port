#pragma once
#include "select_building.hpp"
namespace saga::core {
struct GuideTownRecord {std::int32_t flags,id;}; // BaseData+0x1c and town record+0x20.
using GuideTowns=std::vector<std::shared_ptr<GuideTownRecord>>;
struct SelectGuideState {std::shared_ptr<std::vector<std::uint8_t>> flags;}; // SelectForm+0x1f8.
class SelectGuideHost {
public:
    virtual ~SelectGuideHost() = default;
    virtual std::int32_t sum_quest_challenges() = 0;
    virtual std::shared_ptr<BuildingSlots> building_slots() = 0;
    virtual bool can_build(std::int32_t chip) = 0;
    virtual std::shared_ptr<GuideTowns> towns() = 0;
};
bool guide_buildable_slot(const std::shared_ptr<EventIntegers>&,SelectGuideHost&);
bool guide_active_town(const std::shared_ptr<GuideTownRecord>&);
bool guide_occupied_slot(const std::shared_ptr<EventIntegers>&);
void check_select_guide(SelectGuideState&,bool force,SelectGuideHost&);
// Bind the recovered guide callback while retaining the other required pass services.
class SelectGuidePassHost final:public SelectFormPassHost {
public:
    SelectGuidePassHost(SelectFormPassHost& pass,SelectGuideState& state,SelectGuideHost& guide)
        :pass_(pass),state_(state),guide_(guide) {}
    bool capture_input(std::uint32_t mask) override {return pass_.capture_input(mask);}
    void change_state(std::int32_t value) override {pass_.change_state(value);}
    bool soft_label(std::int32_t index) override {return pass_.soft_label(index);}
    void finish_capture() override {pass_.finish_capture();}
    void capture() override {pass_.capture();}
    void play_bgm() override {pass_.play_bgm();}
    void update_notifications() override {pass_.update_notifications();}
    bool acquire_balance() override {return pass_.acquire_balance();}
    void set_active_x() override {pass_.set_active_x();}
    void update_decorations() override {pass_.update_decorations();}
    void update_town_humans() override {pass_.update_town_humans();}
    std::int32_t town_enable_count() override {return pass_.town_enable_count();}
    bool update_town_enable(std::int32_t index) override {return pass_.update_town_enable(index);}
    void check_guide_arrow(bool force) override {check_select_guide(state_,force,guide_);}
private:
    SelectFormPassHost& pass_;SelectGuideState& state_;SelectGuideHost& guide_;
};
}
