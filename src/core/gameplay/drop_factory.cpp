#include "drop_factory.hpp"
#include <stdexcept>
namespace saga::core {
ComponentSlots* create_drop(std::int32_t type,std::int32_t value,ProjectilePoint p,std::int32_t scale,
                            const TreasureTable& treasures,EntityIdState& ids,DropFactoryHost& host){
    if(type<0||type>3)return nullptr;
    std::int32_t clip=42,image=98;bool sync=true;
    if(type==1)image=value==1?99:105;
    if(type==2){scale=100;clip=43;image=-1;}
    if(type==3){
        const auto& record=treasures.at(static_cast<std::size_t>(value));
        if(!record)throw std::invalid_argument("missing treasure drop record");
        image=record->image;scale=100;clip=41;sync=false;
    }
    auto& entity=create_entity(ids,0,false,host);
    auto& drop=host.acquire_drop();drop.type=type;drop.value=value;drop.x0=p.x;drop.z0=p.z;drop.frame=-1;entity.add(34,&drop);
    auto& position=host.acquire_position();position.x=p.x;position.y=p.y;position.z=p.z;
    position.offset_x=0;position.offset_y=0;position.offset_z=0;position.parent=nullptr;entity.add(0,&position);
    auto& speed=host.acquire_speed();speed.x=0;speed.y=0;speed.z=0;entity.add(1,&speed);
    host.add_cell(entity,factory_cell_coordinate(p.x),factory_cell_coordinate(p.z));
    auto& depth=host.acquire_depth();depth.value=0;depth.extra=10;entity.add(4,&depth);
    auto& sprite=host.acquire_sprite();sprite.animation={7,clip,0};sprite.layers=std::make_shared<std::vector<std::int32_t>>(1,-1);entity.add(2,&sprite);
    host.add_image(entity,{{image,7,-1,-1,-1,-1}}); // Invoked even for image -1.
    auto& modification=host.acquire_modification();modification.type=10;
    modification.x=0;modification.y=0;modification.z=0;modification.scale_x=static_cast<float>(scale)/100.0F;modification.scale_y=modification.scale_x;
    modification.angle=0;modification.frame=0;modification.max_frame=0;modification.anchor=34;
    modification.destroy=false;modification.loop=true;modification.alpha=255;entity.add(15,&modification);
    if(sync)entity.add(13,host.sync_animation());
    return &entity;
}
}
