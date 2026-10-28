#include "entity_visual.hpp"
#include <cmath>
#include <limits>
namespace saga::core {
std::int32_t factory_cell_coordinate(float v){
    return factory_cell_coordinate(v,24);
}
std::int32_t factory_cell_coordinate(float v,std::int32_t step){
    std::int32_t integer=0;
    if(v==std::numeric_limits<float>::infinity()||v<=-2147483648.0F)integer=std::numeric_limits<std::int32_t>::min();
    else if(v>=2147483648.0F)integer=std::numeric_limits<std::int32_t>::max();
    else if(!std::isnan(v))integer=static_cast<std::int32_t>(v);
    if(!step)return 0;
    if(step==-1&&integer==std::numeric_limits<std::int32_t>::min())return integer;
    return integer/step;
}
ComponentSlots& create_z_object(ProjectilePoint point,std::int32_t resource,std::int32_t clip,EntityIdState& ids,EntityVisualHost& host){
    auto& entity=create_entity(ids,0,false,host);
    auto& position=host.acquire_position();position.x=point.x;position.y=point.y;position.z=point.z;
    position.offset_x=0;position.offset_y=0;position.offset_z=0;position.parent=nullptr;entity.add(0,&position);
    auto& speed=host.acquire_speed();speed.x=0;speed.y=0;speed.z=0;entity.add(1,&speed);
    host.add_cell(entity,factory_cell_coordinate(point.x),factory_cell_coordinate(point.z));
    auto& depth=host.acquire_depth();depth.value=0;depth.extra=200;entity.add(4,&depth);
    auto& sprite=host.acquire_sprite();sprite.animation={resource,clip,0};sprite.layers={-1};entity.add(2,&sprite);
    return entity;
}
}
