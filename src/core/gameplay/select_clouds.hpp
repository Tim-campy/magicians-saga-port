#pragma once
#include "event_flow.hpp"
#include <memory>
#include <vector>
namespace saga::core {
using SelectClouds = std::vector<std::shared_ptr<EventIntegers>>;
struct SelectCloudHost {
    virtual ~SelectCloudHost() = default;
    virtual std::shared_ptr<SelectClouds> clouds() = 0;
    virtual std::int32_t right_edge() = 0; // Wrapped SelectForm+0x160 + AppData static+0x394.
    virtual std::int32_t cloud_resource() = 0; // AppData static+0x19c, not an invented id.
    virtual std::int32_t game_height() = 0;
    virtual bool low_aspect() = 0;
    virtual std::int32_t random(std::int32_t bound) = 0;
};
void update_select_clouds(const std::int32_t& loop_counter, SelectCloudHost& host);
}
