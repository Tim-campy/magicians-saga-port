#pragma once
#include "select_camera.hpp"
#include "select_guidance_save.hpp"
namespace saga::core {
class SelectReviewHost {
public:
    virtual ~SelectReviewHost() = default;
    virtual std::int32_t random(std::int32_t bound) = 0; // Original Lib.Random; no assumed backend.
    virtual std::int32_t area_state(std::int32_t index) = 0; // Requires catalog and non-null record.
    virtual std::shared_ptr<EventIntegers> system_save_integers() = 0; // Requires sysSave; nullable intArray.
    virtual std::int32_t reviewed_slot() = 0; // AppData static +0x310; no guessed index.
    virtual std::int32_t review_coin_slot() = 0; // AppData static +0x30c.
    virtual std::int32_t platform_code() = 0; // Config static int32 +0x0.
    virtual EventObject make_reviewed(bool option) = 0;
};
bool select_is_reviewed(SelectReviewHost&);
bool select_has_review_coin(SelectReviewHost&);
// Entry 0x11ed520, only after all buttons fall through and guidance/save completes.
void process_select_late(SelectFormPassState&,SelectBuildingState&,TownBubbleState&,SelectPreludeSnapshot,
    SelectCameraState&,SelectCameraHost&,TownBubbleHost&,SelectFormPassHost&,SelectReviewHost&,
    EventDelayQueue&,EventExecutionHost&);
// Full recovered Update control flow with explicit callbacks; concrete desktop bindings remain required.
bool update_select_form(SelectUpdateBindings&,SelectBuildingState&,SelectBuildingHost&,SelectTownButtonsState&,
    TownBubbleState&,SelectTownButtonsHost&,SelectGuidanceSaveHost&,SelectCameraState&,SelectCameraHost&,SelectReviewHost&);
}
