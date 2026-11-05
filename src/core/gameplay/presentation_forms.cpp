#include "presentation_forms.hpp"
#include "table_catalog.hpp"
#include <limits>
#include <utility>

namespace saga::core {
namespace {
std::int32_t increment_form_int32(std::int32_t value) {
    const auto bits=static_cast<std::uint32_t>(value)+1U;
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits)
        : -1-static_cast<std::int32_t>(0xffffffffU-bits);
}
void read_event_message(EventMessageDefinition& row,detail::TableTokens& tokens,
                        const PresentationStringDecoder& decode) {
    row.id=tokens.integer();
    row.type=tokens.integer();
    row.text=decode(tokens.take());
}
void read_talk(TalkDefinition& row,detail::TableTokens& tokens,const PresentationStringDecoder& decode) {
    row.id=tokens.integer();
    row.character_index=tokens.integer();
    const auto count=tokens.integer();
    std::shared_ptr<std::vector<std::u16string>> texts;
    if(count>=0) {
        texts=std::make_shared<std::vector<std::u16string>>();
        for(std::int32_t i=0;i<count;++i)texts->push_back(decode(tokens.take()));
    }
    row.texts=std::move(texts);
}
}
EventMessageDefinition& load_event_message_definition(EventMessageDefinition& row,
    std::optional<std::string_view> line,const EventIntegerParser& parser,const PresentationStringDecoder& decode) {
    // EventMsgData.Load 0x116fc90: empty/null input preserves the receiver.
    if(line&&!line->empty()){detail::TableTokens tokens(*line,parser);read_event_message(row,tokens,decode);}
    return row;
}
EventMessageTable load_event_message_table(std::string_view text,bool normalize,
    const EventIntegerParser& parser,const PresentationStringDecoder& decode) {
    return detail::load_table_records<EventMessageDefinition>(text,normalize,parser,[&](detail::TableTokens& tokens){
        EventMessageDefinition row;read_event_message(row,tokens,decode);return row;
    });
}
TalkDefinition& load_talk_definition(TalkDefinition& row,std::optional<std::string_view> line,
                                    const EventIntegerParser& parser,const PresentationStringDecoder& decode) {
    // TalkData.Load 0x117347c; GetStringArray 0x1127384 returns null for count<0.
    if(line&&!line->empty()){detail::TableTokens tokens(*line,parser);read_talk(row,tokens,decode);}
    return row;
}
TalkTable load_talk_table(std::string_view text,bool normalize,const EventIntegerParser& parser,
                         const PresentationStringDecoder& decode) {
    return detail::load_table_records<TalkDefinition>(text,normalize,parser,[&](detail::TableTokens& tokens){
        TalkDefinition row;read_talk(row,tokens,decode);return row;
    });
}
std::u16string join_talk_text(const TalkDefinition& row) {
    // BootForm.<Load>b__40_2 0x11d4f7c: String.Join(TAB, TalkData.text).
    if(!row.texts)throw std::invalid_argument("null talk string array");
    std::u16string result;
    for(std::size_t i=0;i<row.texts->size();++i){if(i)result.push_back(u'\t');result+=row.texts->at(i);}
    return result;
}
void update_talk_page(PreparedTalkForm& form,std::int32_t& close_counter,TalkPageHost& host) {
    // SubForm.Update 0x11f829c..0x11f8378 and 0x11f8854..0x11f8a48.
    host.text_frame();
    host.portrait_frame();
    if(!host.advance_pulse()&&(!host.held_advance_enabled()||!host.advance_held()))return;
    if(!host.text_ended()){host.flush_text();return;}
    const auto bits=static_cast<std::uint32_t>(form.text_index)+1U;
    form.text_index=bits<=0x7fffffffU?static_cast<std::int32_t>(bits)
        : -1-static_cast<std::int32_t>(0xffffffffU-bits);
    if(static_cast<std::int64_t>(form.text_index)>=static_cast<std::int64_t>(form.texts.size())) {
        if(form.next_index==-1)close_counter=8;
        else host.continue_talk(form);
    }else host.set_text(form.texts.at(static_cast<std::size_t>(form.text_index)));
}
void update_event_notice(const PreparedEventForm& form,std::int32_t& page,
    std::int32_t& animation_frame,std::int32_t& close_counter,EventNoticeHost& host) {
    // SubForm.Update 0x11f80cc..0x11f81e0, close store at0x11f8a44.
    if(host.debug_command_enabled()&&host.held_advance())animation_frame=17;
    if(animation_frame<17)return;
    host.frame_resource();
    if(host.advance_pulse()||(host.debug_command_enabled()&&host.held_advance())) {
        const auto bits=static_cast<std::uint32_t>(page)+1U;
        page=bits<=0x7fffffffU?static_cast<std::int32_t>(bits)
            : -1-static_cast<std::int32_t>(0xffffffffU-bits);
        if(static_cast<std::int64_t>(page)>=static_cast<std::int64_t>(form.event_ids.size())) {
            close_counter=8;
            return;
        }
        host.set_event(page);
    }
    if(host.cancel_pressed())close_counter=8;
}
bool update_popup_production(PopupProductionState& production,PopupContentsHost& host) {
    // SubForm.UpdateWindowProduction 0x11fa02c..0x11fa098.
    const auto step=production.step;
    if(step!=1&&step!=2)return false;
    const auto bits=static_cast<std::uint32_t>(production.frame)+1U;
    production.frame=bits<=0x7fffffffU?static_cast<std::int32_t>(bits)
        : -1-static_cast<std::int32_t>(0xffffffffU-bits);
    if(production.frame>=5) {
        if(step==2)host.pop_popup();
        // The native 64-bit zero store clears both fields after successful Pop,
        // including replacements made by that callback. Failure keeps the prefix.
        production.step=0;production.frame=0;
    }
    return true;
}
bool update_popup_contents(std::int32_t& animation_frame,PopupProductionState& production,
                           PopupContentsHost& host) {
    // SubForm._update type19 0x11f947c..0x11f94cc; production suffix0x11f9838.
    if(production.step==1||production.step==2) {
        update_popup_production(production,host);
        return false;
    }
    const auto bits=static_cast<std::uint32_t>(animation_frame)+1U;
    const auto next=bits<=0x7fffffffU?static_cast<std::int32_t>(bits)
        : -1-static_cast<std::int32_t>(0xffffffffU-bits);
    animation_frame=next%std::numeric_limits<std::int32_t>::max();
    if(host.popup_close_pulse())production.step=2;
    return true;
}
void initialize_king_hall(KingHallState& state,KingHallHost& host) {
    // SubForm.Init type15 0x11f6ab8..0x11f6bb8.
    state.entrance_x=-40;state.production_step=1;
    state.speech=host.king_hall_voice();
    host.king_hall_initialize_news(60);
}
bool update_king_hall(KingHallState& state,KingHallHost& host) {
    // SubForm._update 0x11f9438, 0x11f9848 and the shared 0x11f9b98 suffix.
    const auto step=state.production_step;
    if(step==1||step==2) {
        state.frame=increment_form_int32(state.frame);
        if(state.frame>=8) {
            state.frame=0;
            if(step==2)host.king_hall_pop();
            state.production_step=0;
        }
    }else{
        state.movement_frame=increment_form_int32(state.movement_frame);
        state.entrance_x=host.king_hall_easing(state.movement_frame);
        // Native reloads this field after Easing; do not compare the argument.
        if(state.movement_frame==39)host.execute_king_rewards();
        if(host.king_hall_confirm_pulse()) {
            host.king_hall_play_sound();
            host.king_hall_save_game();host.king_hall_save_system();
            if(state.selection==2)host.king_hall_push_tapjoy();
            else if(state.selection==1)host.king_hall_push_notes();
        }else if(host.king_hall_soft_label(3)) {
            host.king_hall_play_sound();state.production_step=2;
        }else if(host.king_hall_soft_label(11))host.king_hall_push_notes();
    }
    // This service also runs after Pop; the manager processes Finish afterward.
    host.king_hall_update_news();return true;
}
PreparedTalkForm prepare_talk_form(std::int32_t character_index,
                                 NullablePresentationText text,
                                 NullablePresentationText arguments,
                                 PresentationTextHost& host) {
    // Original overload skips Replace altogether when arguments is null.
    if (arguments) text = substitute_form_arguments(std::move(text), arguments, host);
    auto parts = split_form_text(text ? &*text : nullptr);
    // The original loads textAry_[0] for TextLayout.SetText. No fabricated page
    // is supplied for a missing first element. Layout side effects remain open.
    (void)parts.at(0);
    return {{0, 0, 0, 0}, character_index, std::move(parts), 0, -1, std::move(arguments)};
}
PreparedEventForm prepare_event_form(NullablePresentationText text,
                                    NullablePresentationText arguments,
                                    PresentationFormHost& host) {
    text = substitute_form_arguments(std::move(text), std::move(arguments), host);
    const auto parts = split_form_text(text ? &*text : nullptr);
    PreparedEventForm result{{4, 0, 0, 0}, std::vector<std::int32_t>(parts.size()),
                             std::vector<std::u16string>(parts.size()), {0, 0, 0, 0}};
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const auto fields = split_event_text_fields(&parts[i]);
        result.event_ids[i] = host.parse_event_type(fields.at(0));
        // The original parses first; only then checks whether field1 exists.
        result.texts[i] = fields.at(1);
        // Fields2+ are not consumed by this factory.
    }
    return result;
}
}
