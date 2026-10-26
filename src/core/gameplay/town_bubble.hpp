#pragma once
#include "form_push.hpp"
#include "select_touch_move.hpp"
namespace saga::core {
struct TownVoice {std::int32_t type;std::shared_ptr<std::vector<FormText>> texts;};
struct TownSpeaker {std::int32_t voice_id;std::int32_t living_id;std::int32_t recruit_value;};
using TownVoices=std::vector<std::shared_ptr<TownVoice>>;
using TownSpeakers=std::vector<std::shared_ptr<TownSpeaker>>;
struct TownBubbleState {std::int32_t index;FormText text;std::int32_t frames;};
class TownBubbleHost {
public:
    virtual ~TownBubbleHost() = default;
    virtual std::shared_ptr<TownHumanRows> town_humans() = 0;
    virtual std::shared_ptr<TownVoices> voices() = 0;
    virtual std::shared_ptr<TownSpeakers> speakers() = 0;
    // SelectForm.GetGameWidth/Height (scaled dimensions), not raw window pixels.
    virtual std::int32_t game_width() = 0;
    virtual std::int32_t game_height() = 0;
};
void set_town_bubble(TownBubbleState&,std::int32_t index,bool option,TownBubbleHost&);
bool check_select_view_area(std::int32_t x,std::int32_t y,std::int32_t margin_x,std::int32_t margin_y,TownBubbleHost&);
// Called at original late Update position 0x11ed538, not immediately after contact.
void update_town_bubble_lifetime(TownBubbleState&,std::int32_t saved_camera_x,std::int32_t saved_view_x,TownBubbleHost&);
class TownBubbleTouchHost final:public SelectTouchMoveHost {
public:
    TownBubbleTouchHost(TownBubbleState& state,TownBubbleHost& host):state_(state),host_(host) {}
    std::shared_ptr<TownHumanRows> town_humans() override {return host_.town_humans();}
    void set_talk_bubble(std::int32_t index,bool option) override {set_town_bubble(state_,index,option,host_);}
private:
    TownBubbleState& state_;
    TownBubbleHost& host_;
};
}
