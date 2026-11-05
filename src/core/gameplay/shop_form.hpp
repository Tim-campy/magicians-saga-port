#pragma once
#include "combat_tables.hpp"
#include "event_flow.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace saga::core {
// Selected ShopForm(kind1) fields. Other kinds and billing thread states are external.
struct MagicShopFormState {
    std::int32_t kind=1,state=0,selected_item=0; // +0xfc/+0xf8/+0x100.
    std::int32_t selection=0,page=0,page_count=0; // +0x134/+0x140/+0x144.
    std::int32_t crossfade=0,frame=0,loop_counter=0,thread_state=0;
    float arrival_x=0,witch_x=0,witch_y=0; // +0x104/+0x108/+0x10c.
    std::array<std::int32_t,2> frame_size{{98,32}}; // ctor literal at +0x148/+0x14c.
    std::array<std::int32_t,2> update_pair{{0,0}}; // +0xf0/+0xf4.
    EventObject member_system,touch_panel; // +0x158/+0xe8.
    std::shared_ptr<std::vector<std::int32_t>> entries; // +0x138.
    std::shared_ptr<std::vector<std::uint8_t>> filters; // +0x160.
};
struct MagicShopBuyRequest {
    std::int32_t kind,character_index,parameter_key,add_value,total;
};
class MagicShopFormHost {
public:
    virtual ~MagicShopFormHost()=default;
    virtual void shop_common_initialize()=0;
    virtual EventObject shop_member_system()=0;
    virtual EventObject shop_touch_panel()=0;
    virtual std::int32_t shop_game_height()=0;
    virtual void shop_change_soft_labels(std::int32_t state)=0;
    virtual void shop_flag_system(std::int32_t bits)=0;
    // Each call resolves the live catalog. Row null/bounds checks belong to the provider.
    virtual std::int32_t shop_catalog_count()=0;
    virtual bool shop_catalog_is_found(std::int32_t index)=0;
    virtual void shop_town_buildings()=0;
    virtual void shop_base_update()=0;
    virtual void shop_reset_image_animations()=0;
    virtual void shop_update_image_animations()=0;
    virtual void shop_update_main_blur()=0;
    virtual bool shop_key_repeat(std::uint32_t mask)=0;
    virtual bool shop_key_pulse(std::uint32_t mask)=0;
    virtual bool shop_soft_label(std::int32_t label)=0;
    virtual float shop_clamp(float value,float lower,float upper)=0;
    virtual float shop_clamp_max(float value,float maximum)=0;
    virtual void shop_find_building(std::int32_t kind)=0;
    // Native clears lastForm before examining its result. Zero belongs to the
    // separate unlock-shop confirmation path, not the ordinary kind1 path.
    virtual std::optional<std::int32_t> shop_take_returned_child_result()=0;
    virtual const MagicShopCombatRecord* shop_record(std::int32_t index)=0;
    virtual std::int32_t shop_price(const MagicShopCombatRecord&)=0;
    virtual bool shop_check_money(std::int64_t amount)=0;
    virtual void shop_subtract_money(std::int32_t amount)=0;
    virtual void shop_add_purchases(const MagicShopCombatRecord&,std::int32_t amount)=0;
    virtual std::int32_t shop_parameter_key(const MagicShopCombatRecord&)=0;
    virtual std::int32_t shop_hero_total(const EventObject& member_system,std::int32_t key)=0;
    virtual EventObject shop_make_buy_confirmation(const MagicShopBuyRequest&)=0;
    virtual void shop_push_form(EventObject)=0;
    virtual void shop_execute_event(std::int32_t id)=0;
    virtual void shop_open_tapjoy()=0;
    virtual void shop_open_browser()=0;
    virtual void shop_open_text_input()=0;
    virtual void shop_play_bgm()=0;
    virtual void shop_back_sound()=0;
    virtual bool shop_change_to_town()=0;
};
void initialize_magic_shop(MagicShopFormState&,MagicShopFormHost&);
void reset_magic_shop(MagicShopFormState&,MagicShopFormHost&);
bool update_magic_shop(MagicShopFormState&,MagicShopFormHost&);
} // namespace saga::core
