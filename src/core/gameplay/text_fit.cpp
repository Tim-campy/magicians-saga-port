#include "text_fit.hpp"
#include <cstring>

namespace saga::core {
namespace {
std::int32_t decrement(std::int32_t size) {
    const auto bits = static_cast<std::uint32_t>(size) - 1U;
    std::int32_t result;
    std::memcpy(&result, &bits, sizeof result);
    return result;
}
}
// TextLayout.SetText19198, ELF VA 0x1bbd32c..0x1bbd420.
void fit_text_blocks(TextFitState& state, TextFitHost& host) {
    if (!state.auto_scale || !(state.height > 0.0F)) return;
    if (state.multiline) {
        float height = state.height;
        auto measured = host.text_height();
        while (height + 1.0F < static_cast<float>(measured)) {
            const auto requested = decrement(host.font_size());
            host.set_font_size(requested);
            host.rebuild_blocks(state);
            if (requested == 1) break;
            height = state.height;
            measured = host.text_height();
        }
        height = state.height;
        measured = host.text_height();
        if (height < static_cast<float>(measured)) {
            state.height = static_cast<float>(host.text_height());
        }
    } else {
        if (!(state.width > 0.0F)) return;
        while (state.width < state.text_width) {
            const auto requested = decrement(host.font_size());
            host.set_font_size(requested);
            host.rebuild_blocks(state);
            if (requested == 1) break;
        }
    }
}
}
