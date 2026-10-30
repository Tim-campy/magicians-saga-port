#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace saga::desktop {
// Public FreeType run metrics, not a claim of Unity GUIStyle metric equivalence.
// Load flags and kerning mode are explicit scenario/backend policy inputs.
class DesktopTextFont {
public:
    explicit DesktopTextFont(const std::filesystem::path&);
    ~DesktopTextFont();
    DesktopTextFont(const DesktopTextFont&)=delete;
    DesktopTextFont& operator=(const DesktopTextFont&)=delete;
    float run_width(std::u16string_view,std::int32_t pixel_size,std::int32_t load_flags,std::int32_t kerning_mode);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
