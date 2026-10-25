#pragma once
#include <cstdint>

namespace saga::core {
struct TextFitState {
    float width;             // TextLayout +0x38
    float height;            // +0x3c
    float text_width;        // +0x68, updated by block construction
    bool auto_scale;         // +0x78
    bool multiline;          // +0x84
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
}
