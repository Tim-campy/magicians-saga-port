#pragma once
#include "town_background.hpp"
namespace saga::desktop {
struct TownWindowOptions {
    bool interactive=false,hidden=true;
    std::uint64_t passes=0;
    std::int32_t pulse_every=0;
    std::optional<std::filesystem::path> capture;
};
void run_town_window(TownBackground&,const TownWindowOptions&);
void capture_town_background(TownBackground&,const std::filesystem::path&);
}
