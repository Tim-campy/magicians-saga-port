#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <memory>
#include <cstddef>
namespace saga::desktop {
class MissionProfile;
class GlBitmapRenderer;
struct NativeMissionFrameStats {
    std::size_t quads=0,textures=0;
    std::int32_t surface_width=0,surface_height=0;
};
// The same native world/offscreen/HUD renderer serves mission and town frontends.
// Profile and GL renderer outlive this frame owner; SDL/input/stepping stay outside.
class NativeMissionFrame {
public:
    NativeMissionFrame(MissionProfile&,GlBitmapRenderer&);
    ~NativeMissionFrame();
    void draw(std::int32_t width,std::int32_t height);
    NativeMissionFrameStats stats()const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
// Shared SDL/OpenGL frontend. No frame limit opens the interactive window;
// Bounded runs use a hidden window. Capture without a limit draws the prepared queue;
// capture with a limit renders every step and saves the final frame with retained render history.
int run_native_mission(MissionProfile&,std::int32_t width,std::int32_t height,
    std::optional<std::int32_t> frames=std::nullopt,
    std::optional<std::filesystem::path> capture=std::nullopt);
}
