#pragma once
#include <cstdint>
#include <vector>

namespace saga::core {
// Numeric subset of Font; not a font resource, renderer or thread dispatcher.
struct TextFontState {
    std::int32_t requested_size; // +0x10
    std::int32_t size;           // +0x1c
    bool use_preset;            // +0x20
    std::int32_t adjusted_size; // +0x2c
    float scale;               // +0x40
    std::int32_t font_scale;    // +0x44
};
// Synchronous bodies, after managed class initialization/thread dispatch.
// nullptr models a missing preset table, including when use_preset is false.
void set_text_font_size(TextFontState&, std::int32_t requested, bool use_preset,
                        const std::vector<std::int32_t>* presets);
void update_text_font_scale(TextFontState&, std::int32_t requested_scale,
                           const std::vector<std::int32_t>* presets);
// Arithmetic AFTER UpdateFontScale has completed. Caller owns its dispatch.
void adjust_text_font_size(TextFontState&, float percentage);
// GUIStyle branch of Font._stringWidthF, after scope/localization preparation.
std::int32_t text_gui_pixel_size(std::int32_t adjusted_size,float application_ratio);
float text_run_logical_width(float measured_width,float application_ratio);
}
