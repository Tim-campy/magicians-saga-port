#include "text_font.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
              "Font arithmetic requires IEEE binary32");
std::int32_t signed_bits(std::uint32_t bits) {
    std::int32_t value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}
// FCVTZS W,S saturates and maps NaN to zero; the original additionally
// replaces positive infinity with INT_MIN. Never cast out-of-range floats.
std::int32_t adjusted_integer(float value) {
    constexpr auto minimum = std::numeric_limits<std::int32_t>::min();
    if (std::isnan(value)) return 0;
    if (value == std::numeric_limits<float>::infinity()) return minimum;
    if (value >= 2147483648.0F) return std::numeric_limits<std::int32_t>::max();
    if (value <= -2147483648.0F) return minimum;
    return static_cast<std::int32_t>(value);
}
}
// Font._setSize17770, ELF VA 0x1b78f84. Stores precede table validation.
void set_text_font_size(TextFontState& state, std::int32_t requested,
                        bool use_preset, const std::vector<std::int32_t>* presets) {
    state.size = requested;
    state.requested_size = requested;
    state.use_preset = use_preset;
    if (!presets) throw std::runtime_error("missing font preset table");
    auto base = requested;
    if (use_preset && (requested < 0 ||
        static_cast<std::size_t>(requested) < presets->size())) {
        // Negative requests pass the signed length comparison, then fail the
        // unsigned array bounds check. Larger requests are literal sizes.
        base = presets->at(static_cast<std::size_t>(requested));
    }
    const auto product = static_cast<std::uint32_t>(base) *
                         static_cast<std::uint32_t>(state.font_scale);
    state.size = signed_bits(product) / 100;
    state.adjusted_size = state.size;
}
// UpdateFontScale17806, 0x1b7a8ac, with synchronous SetSize dispatch.
void update_text_font_scale(TextFontState& state, std::int32_t requested_scale,
                           const std::vector<std::int32_t>* presets) {
    if (state.font_scale == requested_scale) return;
    state.font_scale = requested_scale;
    set_text_font_size(state, state.requested_size, true, presets);
}
// _adjustFontSize17796, 0x1b7a858..0x1b7a89c, after UpdateFontScale.
void adjust_text_font_size(TextFontState& state, float percentage) {
    const float ratio = percentage / 100.0F;
    // Volatile intermediates force binary32 rounding and prevent cancellation.
    volatile float converted = static_cast<float>(state.size);
    volatile float product = ratio * converted;
    volatile float quotient = product / ratio;
    state.scale = ratio;
    volatile float biased = quotient + 0x1.47ae14p-7F; // bits 0x3c23d70a
    state.adjusted_size = adjusted_integer(biased);
}
std::int32_t text_gui_pixel_size(std::int32_t adjusted_size,float application_ratio){
    // Font._stringWidthF 0x1b79b54..0x1b79b8c; bias bits0x3a83126f.
    volatile float product=application_ratio*static_cast<float>(adjusted_size);
    volatile float scaled=product/100.0F;
    volatile float biased=scaled+0x1.0624dep-10F;
    return adjusted_integer(biased);
}
float text_run_logical_width(float measured_width,float application_ratio){
    // 0x1b79de4..0x1b79df4: multiply before divide, retaining binary32 steps.
    volatile float product=measured_width*100.0F;
    return product/application_ratio;
}
}
