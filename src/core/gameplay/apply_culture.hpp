#pragma once
#include "apply_task.hpp"
#include <cstddef>
#include <string_view>
namespace saga::core {
// Validated extracted data; culture selection remains the caller's responsibility.
ApplyNumberSigns read_apply_number_signs(std::string_view records,std::string_view pool,std::size_t index);
}
