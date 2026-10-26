#include "select_late.hpp"
#include <stdexcept>
namespace saga::core {
namespace {
EventIntegers& required(const std::shared_ptr<EventIntegers>& value) {
    if(!value)throw std::invalid_argument("missing system save integers");
    return *value;
}
}
// AppData.IsReviewed 0x111750c / HasReviewedCoinGotten 0x11175a4.
bool select_is_reviewed(SelectReviewHost& host) {
    const auto save=host.system_save_integers();
    required(save);
    const auto slot=host.reviewed_slot();
    return save->at(static_cast<std::size_t>(slot))==1;
}
bool select_has_review_coin(SelectReviewHost& host) {
    const auto save=host.system_save_integers();
    required(save);
    const auto slot=host.review_coin_slot();
    return save->at(static_cast<std::size_t>(slot))==1;
}
void process_select_late(SelectFormPassState& form,SelectBuildingState& building,TownBubbleState& bubble,
    SelectPreludeSnapshot snapshot,SelectCameraState& camera,SelectCameraHost& camera_host,
    TownBubbleHost& bubble_host,SelectFormPassHost& pass,SelectReviewHost& review,
    EventDelayQueue& queue,EventExecutionHost& execution) {
    move_select_camera(camera,camera_host);
    if(building.unlock_info_frames>=0)--building.unlock_info_frames;
    update_town_bubble_lifetime(bubble,snapshot.camera_x,snapshot.view_x,bubble_host);
    pass.check_guide_arrow(false);
    if(form.events.loop_counter==90 && review.random(100)<40 && review.area_state(4)==1 &&
       !select_is_reviewed(review) && !select_has_review_coin(review)) {
        const auto child=review.make_reviewed(false);
        push_select_form(form.form,child,form.auto_save,form.events.event_check_requested);
        return; // No platform reward gate or BGM after this Push.
    }
    if(review.platform_code()==2 && select_is_reviewed(review) && !select_has_review_coin(review) &&
       !event_already_executed(14,execution)) {
        const auto save=review.system_save_integers();
        const auto slot=review.review_coin_slot();
        required(save).at(static_cast<std::size_t>(slot))=1;
        execute_indexed_event(14,{},queue,execution); // Form result intentionally discarded.
        form.auto_save=true; // Only after the event returns; failures do not roll back the flag store.
    }
    pass.play_bgm();
}
bool update_select_form(SelectUpdateBindings& bindings,SelectBuildingState& building,SelectBuildingHost& inputs,
    SelectTownButtonsState& town,TownBubbleState& bubble,SelectTownButtonsHost& town_host,
    SelectGuidanceSaveHost& saves,SelectCameraState& camera,SelectCameraHost& camera_host,SelectReviewHost& review) {
    const auto prior=update_select_through_guidance_save(bindings,building,inputs,town,bubble,town_host,saves);
    if(!prior.returned)process_select_late(*bindings.state,building,bubble,prior.snapshot,camera,camera_host,
        town_host,bindings.pass_host,review,bindings.queue,bindings.execution);
    return true;
}
}
