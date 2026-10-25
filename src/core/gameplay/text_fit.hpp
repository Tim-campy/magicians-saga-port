#pragma once
#include <cstdint>

namespace saga::core {
struct TextFitState {
    float width;             // TextLayout +0x38
    float height;            // +0x3c
    float text_width;        // +0x68, updated by block construction
    bool auto_scale;         // +0x78
    bool multiline;          // +0x84
    bool font_size_adjusted = false; // +0x86
};
class TextFitHost {
public:
    virtual ~TextFitHost() = default;
    virtual std::int32_t font_size() = 0;
    // Original Font.SetSize(size, false); may change the effective size.
    virtual void set_font_size(std::int32_t size) = 0;
    virtual void rebuild_blocks(TextFitState& state) = 0;
    virtual std::int32_t text_height() = 0;
};
// Run after initial block construction and the original font snapshot.
// Does not provide font metrics, parse blocks, count reveal length, build ruby
// annotations or restore the saved font; those belong to the surrounding flow.
void fit_text_blocks(TextFitState& state, TextFitHost& host);
// Before initial block construction. requested_scale is Font's static +0x44.
// Independent of auto_scale; marks adjusted only after the setter returns.
void prepare_text_font_size(TextFitState& state, std::int32_t requested_scale,
                            TextFitHost& host);
}
