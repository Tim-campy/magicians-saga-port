#include "core/gameplay/entity_visual.hpp"
#include <cstdlib>
#include <limits>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
int main(){
    check(factory_cell_coordinate(23.99F)==0&&factory_cell_coordinate(24.0F)==1);
    check(factory_cell_coordinate(-23.99F)==0&&factory_cell_coordinate(-24.0F)==-1);
    check(factory_cell_coordinate(std::numeric_limits<float>::quiet_NaN())==0);
    const auto low=std::numeric_limits<std::int32_t>::min()/24;
    check(factory_cell_coordinate(std::numeric_limits<float>::infinity())==low);
    check(factory_cell_coordinate(-std::numeric_limits<float>::infinity())==low);
    check(factory_cell_coordinate(2147483648.0F)==std::numeric_limits<std::int32_t>::max()/24);
}
