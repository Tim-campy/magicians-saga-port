#include "presentation_forms.hpp"
#include "table_catalog.hpp"
#include <utility>

namespace saga::core {
namespace {
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
