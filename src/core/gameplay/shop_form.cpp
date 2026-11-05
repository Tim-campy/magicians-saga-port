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
    if(!value)throw std::invalid_argument("null shop record");
    return *value;
}
template<class T>T& required(const std::shared_ptr<T>& value){
    if(!value)throw std::invalid_argument("null shop entries");
    return *value;
}
void require_kind(const ShopFormState& state){
    if(state.kind<1||state.kind>5)throw std::invalid_argument("only ShopForm kinds1..5 are bound");
}
std::int32_t entry_count(const ShopFormState& state){
    const auto size=required(state.entries).size();
    if(size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("shop list exceeds int32");
    return static_cast<std::int32_t>(size);
}
void check_filter(const ShopFormState& state,ShopFormHost& host){
    // ShopForm.CheckFilter0x120b0c0 queries TownSystem before the vector.
    host.shop_town_buildings();
    for(std::int32_t item=0;item<entry_count(state);item=add32(item,1)){
        const auto id=required(state.entries).at(static_cast<std::size_t>(item));
        // kind1 fails the unsigned(type-2)<=7 dispatch gate after reading the first item.
        if(state.kind==1)return;
        const auto catalog_kind=state.kind;
        if(catalog_kind==5){
            // Kind5 captures the filter after the live tier query but before
            // the fresh cost record and eager CheckCrystal (0x120b4bc).
            const auto unlock=required(host.shop_character_record(id)).unlock_level;
            const auto tier=host.shop_unlock_value(state.kind);
            const auto filters=state.filters;
            const auto cost=required(host.shop_character_record(id)).recruit_cost;
            const auto affordable=host.shop_check_crystals(static_cast<std::int64_t>(cost));
            required(filters).at(static_cast<std::size_t>(id))=static_cast<std::uint8_t>(unlock<=tier&&!affordable);
            continue;
        }
        const auto unlock=required(host.shop_equipment_record(catalog_kind,id)).unlock_level;
        const auto price=required(host.shop_equipment_record(catalog_kind,id)).buy_price;
        const auto kind=state.kind;
        const auto tier=host.shop_unlock_value(kind);
        const auto filters=state.filters;
        // Native calls CheckMoney even for locked rows, and only afterward
        // checks the captured filter array's null/bounds and combines the bits.
        const auto affordable=host.shop_check_money(static_cast<std::int64_t>(price));
        required(filters).at(static_cast<std::size_t>(id))=static_cast<std::uint8_t>(unlock<=tier&&!affordable);
    }
}
void change_state(ShopFormState& state,std::int32_t value,ShopFormHost& host){
    // ShopForm.ChangeState0x1209ea0: labels precede both stores.
    host.shop_change_soft_labels(value);state.state=value;state.frame=0;
}
void reset_preserving_selection(ShopFormState& state,ShopFormHost& host){
    const auto selected=state.selection;
    reset_shop_form(state,host);
    state.selection=selected;
}
void update_main(ShopFormState& state,ShopFormHost& host){
    // Selected ShopForm._update0x120ba1c kinds1..5, thread0 path.
    if(state.thread_state!=0)throw std::invalid_argument("shop billing thread path is unbound");
    host.shop_find_building(state.kind);
    if(const auto result=host.shop_take_returned_child_result()){
        if(*result==0)throw std::invalid_argument("shop zero-result unlock child is unbound");
        reset_shop_form(state,host);return;
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
        const auto dispatch_kind=state.kind;
        if(dispatch_kind==1){
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
            // Only kinds1/9 preserve selection after a successful or refused buy.
            reset_preserving_selection(state,host);
        }else if(dispatch_kind==5){
            // Kind5 (0x120c8bc) checks a quoted crystal cost, then reloads
            // payment and the recruitment receiver before its separate writes.
            const auto cost=required(host.shop_character_record(index)).recruit_cost;
            if(host.shop_check_crystals(static_cast<std::int64_t>(cost))){
                const auto payment=required(host.shop_character_record(index)).recruit_cost;
                host.shop_subtract_crystals(payment);
                (void)required(host.shop_character_record(index));
                host.shop_recruit_character(index);
                const auto kind=state.kind;
                const auto form=host.shop_make_equipment_confirmation(kind,index,0);
                host.shop_push_form(form);
            }
            reset_shop_form(state,host);
        }else{
            // Ordinary item branches retain their dispatched catalog across callbacks.
            // Shipped kind4 checks Cloth.buyPrice but debits Hat.buyPrice at the same ID.
            const auto price=required(host.shop_equipment_record(dispatch_kind,index)).buy_price;
            if(host.shop_check_money(static_cast<std::int64_t>(price))){
                const auto debit_kind=dispatch_kind==4?3:dispatch_kind;
                const auto payment=required(host.shop_equipment_record(debit_kind,index)).buy_price;
                host.shop_subtract_money(payment);
                (void)required(host.shop_equipment_record(dispatch_kind,index));
                host.shop_add_equipment_stock(dispatch_kind,index,1);
                const auto kind=state.kind;
                const auto form=host.shop_make_equipment_confirmation(kind,index,0);
                host.shop_push_form(form);
            }
            reset_shop_form(state,host);
        }
    }
    if(host.shop_key_pulse(0x10000000U))host.shop_open_tapjoy();
    if(host.shop_soft_label(12))host.shop_open_browser();
    if(host.shop_soft_label(21))host.shop_open_text_input();
}
}
void reset_shop_form(ShopFormState& state,ShopFormHost& host){
    // ShopForm.Reset0x1209ec8: kinds1..5 at0x120a110/a1fc/a448/a694/a8e0; common tail0x120b048.
    require_kind(state);
    state.selection=-1;state.crossfade=-1;
    state.entries=std::make_shared<std::vector<std::int32_t>>();
    if(state.kind==1){
        const auto count=host.shop_catalog_count();
        if(count<0)throw std::length_error("negative magic-shop catalog length");
        state.filters=std::make_shared<std::vector<std::uint8_t>>(static_cast<std::size_t>(count));
        for(std::int32_t index=0;index<host.shop_catalog_count();index=add32(index,1))
            if(host.shop_catalog_is_found(index))required(state.entries).push_back(index);
    }else if(state.kind==5){
        const auto count=host.shop_character_catalog_count();
        if(count<0)throw std::length_error("negative recruitment-shop catalog length");
        state.filters=std::make_shared<std::vector<std::uint8_t>>(static_cast<std::size_t>(count));
        const auto catalog=host.shop_character_catalog();
        append_character_shop_entries(required(state.entries),required(catalog),
            [&](const CharacterProgressionRecord& record){return host.shop_character_found(record);},
            [&](const CharacterProgressionRecord& record){return host.shop_character_recruited(record);});
    }else{
        const auto kind=state.kind;
        const auto count=host.shop_equipment_catalog_count(kind);
        if(count<0)throw std::length_error("negative equipment-shop catalog length");
        state.filters=std::make_shared<std::vector<std::uint8_t>>(static_cast<std::size_t>(count));
        const auto catalog=host.shop_equipment_catalog(kind);
        const auto& source=required(catalog);
        const auto mask=kind==2?0x10U:0x4U;
        append_equipment_shop_entries(required(state.entries),source,mask,
            [&](const EquipmentCombatRecord& record){return host.shop_equipment_owned(kind,record);});
    }
    const auto maximum=add32(add32(entry_count(state),-1)/12,1);
    state.page_count=maximum;
    // IntExtension.ClampMax0x10eda6c is inclusive, even though page_count is a count.
    if(state.page>maximum)state.page=maximum;
    check_filter(state,host);
}
void initialize_shop_form(ShopFormState& state,ShopFormHost& host){
    // ShopForm.Init0x1209bc4 selected ordinary kinds1..5.
    require_kind(state);
    host.shop_common_initialize();
    state.member_system=host.shop_member_system();
    state.touch_panel=host.shop_touch_panel();
    state.arrival_x=60.0F;state.witch_x=-20.0F;
    state.witch_y=static_cast<float>(add32(host.shop_game_height(),-156));
    change_state(state,0,host);
    host.shop_flag_system(0x100);
    reset_shop_form(state,host);
}
bool update_shop_form(ShopFormState& state,ShopFormHost& host){
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
