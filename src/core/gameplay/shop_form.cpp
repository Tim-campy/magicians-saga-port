#include "shop_form.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t signed32(std::uint32_t bits){
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits)
        :-1-static_cast<std::int32_t>(0xffffffffU-bits);
}
std::int32_t add32(std::int32_t first,std::int32_t second){
    return signed32(static_cast<std::uint32_t>(first)+static_cast<std::uint32_t>(second));
}
std::int32_t remainder32(std::int32_t numerator,std::int32_t denominator){
    // AArch64 SDIV yields zero for divisor0; INT_MIN/-1 yields INT_MIN.
    if(denominator==0)return numerator;
    if(numerator==std::numeric_limits<std::int32_t>::min()&&denominator==-1)return 0;
    return numerator%denominator;
}
template<class T>const T& required(const T* value){
    if(!value)throw std::invalid_argument("null magic-shop record");
    return *value;
}
template<class T>T& required(const std::shared_ptr<T>& value){
    if(!value)throw std::invalid_argument("null magic-shop entries");
    return *value;
}
void require_kind(const MagicShopFormState& state){
    if(state.kind!=1)throw std::invalid_argument("only magic ShopForm kind1 is bound");
}
std::int32_t entry_count(const MagicShopFormState& state){
    const auto size=required(state.entries).size();
    if(size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("magic-shop list exceeds int32");
    return static_cast<std::int32_t>(size);
}
void check_filter(const MagicShopFormState& state,MagicShopFormHost& host){
    // ShopForm.CheckFilter0x120b0c0 queries TownSystem before the vector.
    host.shop_town_buildings();
    if(entry_count(state)!=0){
        (void)required(state.entries).at(0);
        // kind1 fails the unsigned(type-2)<=7 dispatch gate, leaving filters false.
    }
}
void change_state(MagicShopFormState& state,std::int32_t value,MagicShopFormHost& host){
    // ShopForm.ChangeState0x1209ea0: labels precede both stores.
    host.shop_change_soft_labels(value);state.state=value;state.frame=0;
}
void reset_preserving_selection(MagicShopFormState& state,MagicShopFormHost& host){
    const auto selected=state.selection;
    reset_magic_shop(state,host);
    state.selection=selected;
}
void update_main(MagicShopFormState& state,MagicShopFormHost& host){
    // Selected ShopForm._update0x120ba1c kind1/thread0 path.
    if(state.thread_state!=0)throw std::invalid_argument("magic-shop billing thread path is unbound");
    host.shop_find_building(state.kind);
    if(const auto result=host.shop_take_returned_child_result()){
        if(*result==0)throw std::invalid_argument("magic-shop zero-result unlock child is unbound");
        reset_magic_shop(state,host);return;
    }
    check_filter(state,host);
    if(host.shop_key_repeat(0x40000U))
        state.page=remainder32(add32(state.page,1),state.page_count);
    else if(host.shop_key_repeat(0x10000U))
        state.page=remainder32(add32(add32(state.page,state.page_count),-1),state.page_count);
    if(host.shop_key_pulse(0x100000U)&&state.selection>=9999){
        host.shop_execute_event(8);return;
    }
    if(host.shop_key_pulse(0x02000000U)){
        const auto index=required(state.entries).at(static_cast<std::size_t>(state.selection));
        const auto price=host.shop_price(required(host.shop_record(index)));
        if(host.shop_check_money(static_cast<std::int64_t>(price))){
            const auto payment=host.shop_price(required(host.shop_record(index)));
            host.shop_subtract_money(payment);
            host.shop_add_purchases(required(host.shop_record(index)),1);
            const auto parameter=host.shop_parameter_key(required(host.shop_record(index)));
            const auto& record=required(host.shop_record(index));
            const auto member=state.member_system;
            if(!member)throw std::invalid_argument("null magic-shop MemberSystem");
            const auto addition=record.value_per_purchase;
            const auto kind=state.kind;
            const auto total=host.shop_hero_total(member,parameter);
            const auto form=host.shop_make_buy_confirmation({kind,0,parameter,addition,total});
            host.shop_push_form(form);
        }
        // The native kind1 Reset executes for both successful and refused buys.
        reset_preserving_selection(state,host);
    }
    if(host.shop_key_pulse(0x10000000U))host.shop_open_tapjoy();
    if(host.shop_soft_label(12))host.shop_open_browser();
    if(host.shop_soft_label(21))host.shop_open_text_input();
}
}
void reset_magic_shop(MagicShopFormState& state,MagicShopFormHost& host){
    // ShopForm.Reset0x1209ec8: selected kind1 at0x120a110 and common tail0x120b048.
    require_kind(state);
    state.selection=-1;state.crossfade=-1;
    state.entries=std::make_shared<std::vector<std::int32_t>>();
    const auto count=host.shop_catalog_count();
    if(count<0)throw std::length_error("negative magic-shop catalog length");
    state.filters=std::make_shared<std::vector<std::uint8_t>>(static_cast<std::size_t>(count));
    for(std::int32_t index=0;index<host.shop_catalog_count();index=add32(index,1))
        if(host.shop_catalog_is_found(index))required(state.entries).push_back(index);
    const auto maximum=add32(add32(entry_count(state),-1)/12,1);
    state.page_count=maximum;
    // IntExtension.ClampMax0x10eda6c is inclusive, even though page_count is a count.
    if(state.page>maximum)state.page=maximum;
    check_filter(state,host);
}
void initialize_magic_shop(MagicShopFormState& state,MagicShopFormHost& host){
    // ShopForm.Init0x1209bc4 selected ordinary kind1.
    require_kind(state);
    host.shop_common_initialize();
    state.member_system=host.shop_member_system();
    state.touch_panel=host.shop_touch_panel();
    state.arrival_x=60.0F;state.witch_x=-20.0F;
    state.witch_y=static_cast<float>(add32(host.shop_game_height(),-156));
    change_state(state,0,host);
    host.shop_flag_system(0x100);
    reset_magic_shop(state,host);
}
bool update_magic_shop(MagicShopFormState& state,MagicShopFormHost& host){
    // ShopForm.Update0x120b774, with source binary32 motion and int32 counters.
    require_kind(state);
    state.update_pair={1,0};host.shop_base_update();
    if(state.loop_counter==0)host.shop_reset_image_animations();
    host.shop_update_image_animations();host.shop_update_main_blur();
    state.frame=add32(state.frame,1);
    state.loop_counter=remainder32(add32(state.loop_counter,1),2147483647);
    if(state.crossfade!=-1){
        state.crossfade=add32(state.crossfade,1);
        if(state.crossfade>=21){
            change_state(state,1,host);state.crossfade=-1;
        }
        return true; // Crossfade skips both main logic and BGM/Back.
    }
    if(state.state==1)update_main(state,host);
    else{
        const auto difference=state.arrival_x-state.witch_x;
        const auto step=host.shop_clamp(difference/3.0F,3.0F,4.0F);
        state.witch_x=host.shop_clamp_max(step+state.witch_x,state.arrival_x);
        if(state.witch_x>=state.arrival_x){
            if(host.shop_key_pulse(0x100000U)||state.loop_counter>=61)state.crossfade=0;
        }
    }
    host.shop_play_bgm();
    if(host.shop_soft_label(3)){
        host.shop_back_sound();return host.shop_change_to_town();
    }
    return true;
}
} // namespace saga::core
