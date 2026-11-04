#include "town_bubble.hpp"
#include "table_catalog.hpp"
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
void read_voice(TownVoice& voice,detail::TableTokens& tokens){
    voice.id=tokens.integer();voice.type=tokens.integer();
    const auto count=tokens.integer();
    std::shared_ptr<std::vector<FormText>> texts;
    if(count>=0){
        texts=std::make_shared<std::vector<FormText>>();
        for(std::int32_t i=0;i<count;++i)texts->push_back(std::make_shared<const std::string>(tokens.take()));
    }
    voice.texts=std::move(texts);
}
template<class T> T& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("null town bubble data");
    return *value;
}
std::int32_t wrap(std::int64_t value) {
    const auto bits=static_cast<std::uint32_t>(value);
    return bits<=0x7fffffffU ? static_cast<std::int32_t>(bits)
        : static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
FormText select_text(const std::shared_ptr<TownVoice>& voice,std::int32_t sequence) {
    const auto texts=required(voice).texts;
    const auto size=required(texts).size();
    if(size==0 || size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::out_of_range("invalid voice text count");
    const auto index=sequence%static_cast<std::int32_t>(size);
    return texts->at(static_cast<std::size_t>(index)); // Negative nonzero remainder fails.
}
}
TownVoice& load_town_voice_definition(TownVoice& voice,std::optional<std::string_view> line,const EventIntegerParser& parser){
    // String.IsNullOrEmpty gates all writes. Native id/type stores precede the
    // GetStringArray call; a failed array read retains the previous text owner.
    if(line&&!line->empty()){detail::TableTokens tokens(*line,parser);read_voice(voice,tokens);}
    return voice;
}
TownVoices load_town_voice_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    auto records=detail::load_table_records<TownVoice>(text,normalize,parser,[](detail::TableTokens& tokens){
        TownVoice voice{};read_voice(voice,tokens);return voice;
    });
    TownVoices result(records.size());
    for(std::size_t i=0;i<records.size();++i)if(records[i])result[i]=std::make_shared<TownVoice>(std::move(*records[i]));
    return result;
}
// SelectForm.SetTalkFuki, 0x11eedd8. The second argument is unused in this body.
void set_town_bubble(TownBubbleState& state,std::int32_t index,bool,TownBubbleHost& host) {
    state.index=index;
    const auto row=required(host.town_humans()).at(static_cast<std::size_t>(index));
    const auto speaker_id=required(row).at(0);
    const auto sequence=row->at(10);
    const auto voices=host.voices();
    const auto speaker=required(host.speakers()).at(static_cast<std::size_t>(speaker_id));
    const auto voice_id=required(speaker).voice_id;
    auto text=select_text(required(voices).at(static_cast<std::size_t>(voice_id)),sequence);
    const auto recruit=required(host.speakers()).at(static_cast<std::size_t>(speaker_id));
    if(required(recruit).recruit_value!=0) {
        const auto living=required(host.speakers()).at(static_cast<std::size_t>(speaker_id));
        if(required(living).living_id==-1) {
            const auto alternatives=host.voices();
            std::shared_ptr<TownVoice> selected;
            for(const auto& voice:required(alternatives)) {
                // <SetTalkFuki>b__135_0, 0x120739c: null rows fail before a match.
                if(required(voice).type==4) {selected=voice;break;}
            }
            text=select_text(selected,sequence);
        }
    }
    state.text=text;
    state.frames=120;
}
// SelectForm.CheckViewArea, 0x11f43ec: query order matters on short circuits.
bool check_select_view_area(std::int32_t x,std::int32_t y,std::int32_t mx,std::int32_t my,SelectViewportHost& host) {
    if(wrap(-static_cast<std::int64_t>(mx))>x)return false;
    const auto width=host.game_width();
    if(wrap(-static_cast<std::int64_t>(my))>y)return false;
    if(wrap(static_cast<std::int64_t>(width)+mx)<x)return false;
    return wrap(static_cast<std::int64_t>(host.game_height())+my)>=y;
}
// SelectForm.Update, 0x11ed538..0x11ed660. Offscreen clearing differs from expiry.
void update_town_bubble_lifetime(TownBubbleState& state,std::int32_t camera,std::int32_t view,TownBubbleHost& host) {
    if(state.frames<0)return;
    if(state.index!=-1) {
        const auto row=required(host.town_humans()).at(static_cast<std::size_t>(state.index));
        const auto x=wrap(static_cast<std::int64_t>(camera)+view+required(row).at(1));
        const auto y=wrap(static_cast<std::int64_t>(host.game_height())-1);
        if(!check_select_view_area(x,y,16,0,host)) {
            state.index=-1;state.frames=-1;
            static const auto empty=std::make_shared<const std::string>();
            state.text=empty;
            return;
        }
    }
    state.frames=wrap(static_cast<std::int64_t>(state.frames)-1);
    if(state.frames<0)state.index=-1; // Text is retained on ordinary expiry.
}
}
