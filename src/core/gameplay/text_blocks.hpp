#pragma once
#include "text_parameters.hpp"
#include <vector>

namespace saga::core {
struct TextBlockConfig {
    float width,height;
    std::int32_t line_space,font_size;
    std::uint32_t color,alignment;
    bool japanese_style;
};
struct LayoutTextBlock {
    std::u16string text;
    float x=0,y=0,width=0;
    std::int32_t font_size=0;
    std::uint32_t color=0;
    std::int32_t source_begin=-1,source_length=-1;
};
struct TextBlockLayout {
    std::u16string processed_source;
    std::vector<LayoutTextBlock> blocks;
    float text_width=0;
};
class TextBlockHost:public TextParameterHost {
public:
    // Whole-text TranslateText/UnicodeToChanakya boundary. A missing/null
    // result must fail here; the builder does not invent a replacement.
    virtual std::u16string layout_text(const std::u16string& original)=0;
    virtual bool equal_tag(std::u16string_view expected,std::u16string_view candidate)=0;
    virtual bool starts_with_tag(std::u16string_view candidate,std::u16string_view prefix)=0;
    // Font.StringWidthF on the ENTIRE run, with the caller's configured font.
    virtual float measure_run(const std::u16string&,std::int32_t font_size)=0;
};
// Reached fresh talk subset of TextLayout._makeTextBlocks: multiline=true,
// format=false, ASCII ordinary units, co/endco; no guessed Unicode pair,
// pictograph, font-changing, ruby or ff path. Unsupported paths fail explicitly.
// text_width mutates during construction; blocks are replaced only at the
// original array-assignment phase, before final source-substring assignment.
// Empty original text clears blocks without invoking the host or resetting width.
TextBlockLayout& build_talk_text_blocks(TextBlockLayout&,const std::u16string& original,
                                       const TextBlockConfig&,TextBlockHost&);
std::int32_t text_blocks_height(const TextBlockLayout&); // TextLayout.GetHeight.
std::int32_t text_blocks_reveal_length(const TextBlockLayout&); // Existing reveal_length kernel.
}
