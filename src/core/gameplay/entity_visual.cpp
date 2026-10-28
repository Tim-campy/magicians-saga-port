#include "entity_visual.hpp"
#include <cmath>
#include <limits>
namespace saga::core {
std::int32_t factory_cell_coordinate(float v){
    if(std::isnan(v))return 0;
    if(v==std::numeric_limits<float>::infinity()||v<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min()/24;
    if(v>=2147483648.0F)return std::numeric_limits<std::int32_t>::max()/24;
    return static_cast<std::int32_t>(v)/24;
}
}
