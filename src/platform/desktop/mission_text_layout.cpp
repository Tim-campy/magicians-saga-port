#include "mission_text_layout.hpp"
#include "core/gameplay/event_catalog.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
std::string ascii(const std::u16string& value){
    std::string result;for(const auto unit:value){if(unit>127)throw ManagedTextParseError("invariant text number outside ASCII subset");result.push_back(static_cast<char>(unit));}return result;
}
}
MissionTextLayout::MissionTextLayout(MissionFontScenario scenario,std::shared_ptr<DesktopTextFont> backend)
    :scenario_(std::move(scenario)),backend_(std::move(backend)),font_{0,0,true,0,1,scenario_.requested_scale}{
    if(!backend_)throw std::invalid_argument("missing explicit font provider");
    if(scenario_.requested_scale<1||scenario_.requested_scale>400||!std::isfinite(scenario_.application_ratio)||scenario_.application_ratio<=0||
       !std::isfinite(scenario_.layout_scale)||scenario_.layout_scale<=0)throw std::invalid_argument("unsupported explicit font scale context");
    set_text_font_size(font_,0,true,&presets_); // Fresh Font._init's preset0.
    fit_.auto_scale=!scenario_.japanese; // Default TextLayout constructor queries Japanese separately from wrap style.
}
void MissionTextLayout::set_text(const std::u16string& text){
    text_=text;
    prepare_text_font_size(fit_,scenario_.requested_scale,*this);
    rebuild_blocks(fit_);
    const auto saved=font_; // Native SetText snapshots after initial block construction.
    fit_text_blocks(fit_,*this);
    length_=text_blocks_reveal_length(blocks_);
    font_=saved; // Restore only after normal completion; failed preparation retains mutations.
}
void MissionTextLayout::set_font_size(std::int32_t size){set_text_font_size(font_,size,false,&presets_);}
void MissionTextLayout::rebuild_blocks(TextFitState& state){
    TextBlockConfig config{state.width,state.height,4,font_.size,0xff252336U,0x11U,scenario_.japanese_style};
    build_talk_text_blocks(blocks_,text_,config,*this);state.text_width=blocks_.text_width;
}
std::int32_t MissionTextLayout::length()const{return length_;}
std::int32_t MissionTextLayout::height()const{return text_blocks_height(blocks_);}
std::u16string MissionTextLayout::layout_text(const std::u16string& text){
    // SAGA_STUB(mission-text-translation): declared invariant translation/Chanakya/localization provider.
    return text;
}
bool MissionTextLayout::equal_tag(std::u16string_view expected,std::u16string_view candidate){return expected==candidate;}
bool MissionTextLayout::starts_with_tag(std::u16string_view candidate,std::u16string_view prefix){return candidate.substr(0,prefix.size())==prefix;}
float MissionTextLayout::measure_run(const std::u16string& text,std::int32_t size){
    if(size!=font_.size)throw std::logic_error("unbound text block font mutation");
    auto work=font_;update_text_font_scale(work,scenario_.requested_scale,&presets_);adjust_text_font_size(work,scenario_.layout_scale);
    const auto pixels=text_gui_pixel_size(work.adjusted_size,scenario_.application_ratio);
    auto content=text;std::replace(content.begin(),content.end(),u' ',u'.'); // Native GUIStyle content rule.
    ++measurements_;
    // SAGA_STUB(mission-talk-font-metrics): public pixel advances; GUIStyle equivalence needs the original oracle.
    return text_run_logical_width(backend_->run_width(content,pixels,scenario_.load_flags,scenario_.kerning_mode),scenario_.application_ratio);
}
std::int32_t MissionTextLayout::parse_decimal(const std::u16string& value){
    try{return parse_event_decimal(ascii(value));}
    catch(const std::invalid_argument& e){throw ManagedTextParseError(e.what());}
    catch(const std::out_of_range& e){throw ManagedTextParseError(e.what());}
}
std::int32_t MissionTextLayout::parse_hexadecimal(const std::u16string& value){
    const auto token=ascii(value);std::uint32_t bits=0;
    const auto result=std::from_chars(token.data(),token.data()+token.size(),bits,16);
    if(token.empty()||result.ec!=std::errc{}||result.ptr!=token.data()+token.size())throw ManagedTextParseError("invalid invariant hexadecimal text number");
    std::int32_t number;std::memcpy(&number,&bits,sizeof number);return number;
}
}
