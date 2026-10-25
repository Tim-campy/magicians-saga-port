#include "text_fit.hpp"
#include <cstring>
#include <limits>

namespace saga::core {
namespace {
std::int32_t multiply100(std::int32_t value) {
    const auto bits = static_cast<std::uint32_t>(value) * 100U;
    std::int32_t result;
    std::memcpy(&result, &bits, sizeof result);
    return result;
}
std::int32_t arm_divide(std::int32_t value, std::int32_t divisor) {
    if (divisor == 0) return 0;
    if (value == std::numeric_limits<std::int32_t>::min() && divisor == -1) return value;
    return value / divisor;
}
std::int32_t decrement(std::int32_t size) {
    const auto bits = static_cast<std::uint32_t>(size) - 1U;
    std::int32_t result;
    std::memcpy(&result, &bits, sizeof result);
    return result;
}
}
// SetText19198, ELF VA 0x1bbd298..0x1bbd300. Two SDIVs, not one scale ratio.
void prepare_text_font_size(TextFitState& state, std::int32_t requested_scale,
                            TextFitHost& host) {
    if (state.font_size_adjusted || !state.multiline) return;
    auto size = arm_divide(multiply100(host.font_size()), requested_scale);
    size = arm_divide(multiply100(size), requested_scale);
    host.set_font_size(size);
    state.font_size_adjusted = true;
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
