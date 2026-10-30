#pragma once
#include "text_font_backend.hpp"
#include "core/gameplay/text_blocks.hpp"
#include "core/gameplay/text_fit.hpp"
#include "core/gameplay/text_font.hpp"
#include <optional>

namespace saga::desktop {
struct MissionFontScenario {
    std::filesystem::path path;
    std::int32_t requested_scale=0,load_flags=0,kerning_mode=0;
    float application_ratio=0,layout_scale=0;
    bool japanese=false,japanese_style=false;
};
// A concrete public font provider with an explicit unverified GUIStyle-equivalence boundary.
// Native layout/font numeric kernels run above that boundary.
class MissionTextLayout final:private core::TextBlockHost,private core::TextFitHost {
public:
    MissionTextLayout(MissionFontScenario,std::shared_ptr<DesktopTextFont>);
    void set_text(const std::u16string&);
    std::int32_t length()const;
    std::int32_t height()const;
    const core::TextBlockLayout& blocks()const{return blocks_;}
    std::uint64_t measurements()const{return measurements_;}
private:
    MissionFontScenario scenario_;
    std::shared_ptr<DesktopTextFont> backend_;
    core::TextFontState font_;
    core::TextFitState fit_{196,60,0,true,true,false};
    core::TextBlockLayout blocks_;
    std::u16string text_;
    std::vector<std::int32_t> presets_{12,24,30}; // Font.HEIGHT after native class initialization.
    std::uint64_t measurements_=0;
    std::int32_t length_=0;
    std::u16string layout_text(const std::u16string&)override;
    bool equal_tag(std::u16string_view,std::u16string_view)override;
    bool starts_with_tag(std::u16string_view,std::u16string_view)override;
    float measure_run(const std::u16string&,std::int32_t)override;
    std::int32_t parse_decimal(const std::u16string&)override;
    std::int32_t parse_hexadecimal(const std::u16string&)override;
    std::int32_t font_size()override{return font_.size;}
    void set_font_size(std::int32_t)override;
    void rebuild_blocks(core::TextFitState&)override;
    std::int32_t text_height()override{return height();}
};
}
