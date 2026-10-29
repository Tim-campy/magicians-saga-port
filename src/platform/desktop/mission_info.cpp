#include "mission_scene.hpp"
#include "core/ecs/update_scheduler.hpp"

namespace saga::desktop {
using namespace core;
namespace {
struct ScenarioPopup final:PopupBar {
    MissionPopupScenario values;std::uint64_t& disposals;
    ScenarioPopup(MissionPopupScenario s,std::uint64_t& counter):PopupBar(s.id),values(s),disposals(counter){}
    std::int32_t category()override{return values.category;}
    std::int32_t type()override{return values.type;}
    std::int32_t priority()override{return values.priority;}
    bool single()override{return values.single;}
    std::int32_t height()override{return values.height;}
    std::int32_t duration()override{return values.duration;}
    void dispose()override{++disposals;} // Explicit scenario owns no widget/entity resources.
    bool same_popup(PopupBar*)override{throw std::logic_error("concrete popup equality is unbound; diagnostic insertion requires force");}
};
struct ScenarioSlide final:SlideBar {
    MissionSlideScenario values;
    explicit ScenarioSlide(MissionSlideScenario s):SlideBar(s.id),values(s){}
    std::int32_t width()override{return values.width;}
    std::int32_t height()override{return values.height;}
    std::int32_t duration()override{return values.duration;}
};
}
void MissionScene::advance_info(std::int32_t pass,bool enabled){if(gameplay_needs_update(25,enabled,pass)){update_info(info_,*this);++info_steps_;}}
void MissionScene::deliver_info_click(CursorPointRef point){info_click_(std::move(point));++info_clicks_;}
void MissionScene::add_info_popup(MissionPopupScenario s){
    auto bar=std::make_shared<ScenarioPopup>(s,info_popup_disposals_);scenario_popups_.push_back(bar);add_popup(info_,std::move(bar),true);
} // SAGA_STUB(mission-info-draw): real controller/base clocks; scenario dimensions/content are explicit, text output is unbound.
void MissionScene::add_info_slide(MissionSlideScenario s){
    auto bar=std::make_shared<ScenarioSlide>(s);scenario_slides_.push_back(bar);add_slide(info_,std::move(bar));
}
std::shared_ptr<PopupBar> MissionScene::make_monster_info(ComponentSlots*){throw std::logic_error("native MonsterInfo constructor is unbound");}
std::shared_ptr<SlideBar> MissionScene::make_attack_notice(EventObject){throw std::logic_error("native AttackNoticeBar constructor is unbound");}
}
