#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
namespace saga::desktop {
class MissionProfile;
// Shared SDL/OpenGL frontend. No frame limit opens the interactive window;
// Bounded runs use a hidden window. Capture without a limit draws the prepared queue;
// capture with a limit renders every step and saves the final frame with retained render history.
int run_native_mission(MissionProfile&,std::int32_t width,std::int32_t height,
    std::optional<std::int32_t> frames=std::nullopt,
    std::optional<std::filesystem::path> capture=std::nullopt);
}
