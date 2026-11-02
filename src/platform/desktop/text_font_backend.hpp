#pragma once
#include "decoded_image.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace saga::desktop {
struct TextRunBitmap {
    DecodedImage pixels;
    std::int32_t left=0,top=0;
    float advance=0,ascent=0; // Pixel coordinates relative to the FreeType baseline.
};
// Public FreeType run metrics, not a claim of Unity GUIStyle metric equivalence.
// Load flags and kerning mode are explicit scenario/backend policy inputs.
class DesktopTextFont {
public:
    explicit DesktopTextFont(const std::filesystem::path&);
    ~DesktopTextFont();
    DesktopTextFont(const DesktopTextFont&)=delete;
    DesktopTextFont& operator=(const DesktopTextFont&)=delete;
    float run_width(std::u16string_view,std::int32_t pixel_size,std::int32_t load_flags,std::int32_t kerning_mode);
    // Public grayscale rasterization; placement policy does not establish Unity GUIStyle equivalence.
    TextRunBitmap rasterize_run(std::u16string_view,std::int32_t pixel_size,std::int32_t load_flags,std::int32_t kerning_mode);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
