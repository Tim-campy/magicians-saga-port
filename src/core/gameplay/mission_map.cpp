#include "mission_map.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mission map input");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t negate(std::int32_t a){return word(0U-static_cast<std::uint32_t>(a));}
std::int32_t image(const StageQuest& q,std::size_t index){return required(q.chip_images.get()).at(index);}
}
MapGrid make_map_grid(std::int32_t rows,std::int32_t columns){
    if(rows<0)throw std::length_error("negative map row count");
    auto grid=std::make_shared<std::vector<MapRow>>(static_cast<std::size_t>(rows));
    // Zero rows do not allocate or validate any inner row.
    for(auto& row:*grid){if(columns<0)throw std::length_error("negative map column count");
        row=std::make_shared<std::vector<ComponentSlots*>>(static_cast<std::size_t>(columns),nullptr);}
    return grid;
}
MapCreation initial_mission_map_creation(){return {3,8,624,312,312,312,16,16,nullptr};} // 0x1101d04..0x1101d2c.
ComponentSlots& add_map_component(ComponentSlots& e,MapCreation r,MapFactoryHost& host){
    auto& map=required(host.shared_map_component());
    map.width=r.width;map.height=r.height;map.size_width=r.size_width;map.size_height=r.size_height;
    map.step_x=r.step_x;map.step_z=r.step_z;map.area_width=r.area_width;map.area_height=r.area_height;map.chips=std::move(r.chips);
    return e.add(11,&map);
}
ComponentSlots& create_map(MapCreation r,EntityIdState& ids,MapFactoryHost& host){
    auto& e=create_entity(ids,0,false,host);
    if(!r.chips)r.chips=make_map_grid(r.height,r.width);
    return add_map_component(e,std::move(r),host);
}
ComponentSlots& add_map_chip_component(ComponentSlots& e,std::int32_t data_id,std::int32_t staff_id,MapFactoryHost& host){
    auto& chip=host.acquire_map_chip();chip.data_id=data_id;chip.staff_id=staff_id;
    chip.visiting_human_ids=std::make_shared<EventIntegers>(5,-1);return e.add(9,&chip);
}
ComponentSlots& create_map_chip(MapChipCreation r,EntityIdState& ids,MapFactoryHost& host){
    auto& e=create_entity(ids,0,false,host);auto& p=host.acquire_position();
    p.x=static_cast<float>(multiply(r.step_x,r.column));p.y=static_cast<float>(r.y);p.z=static_cast<float>(multiply(r.step_z,r.row));
    p.offset_x=0;p.offset_y=0;p.offset_z=0;p.parent=nullptr;e.add(0,&p);
    host.add_cell(e,r.column,r.row);
    auto& sprite=host.acquire_sprite();sprite.animation={r.resource,r.clip,r.frame};sprite.layers={r.layer};e.add(2,&sprite);
    host.add_image(e,{{r.texture,-1,-1,-1,-1,-1}});
    auto& depth=host.acquire_depth();depth.value=0;depth.extra=r.extra_depth;e.add(4,&depth);
    add_map_chip_component(e,r.data_id,-1,host);
    if(r.animate)add_animation_component(e,1,-1,host);
    return e;
}
void initialize_mission_map(MissionMapSystem& s,MissionMapHost& host){s.map=host.current_mission_map();}
std::int32_t quest_map_width(){return 3;}
std::int32_t quest_map_start_image(const StageQuest& q){return image(q,0);}
std::int32_t quest_map_middle_image(const StageQuest& q){return image(q,1);}
std::int32_t quest_map_end_image(const StageQuest& q){return image(q,2);}
std::int32_t quest_map_background_image(const StageQuest& q){return image(q,3);}
bool map_location_in_range(MapComponent* map,std::int32_t x,std::int32_t z,std::int32_t width,std::int32_t height,MissionMapHost& host){
    if(!map)map=host.current_mission_map();
    if(x<0||z<0)return false;
    if(add(add(x,width),-1)>=required(map).width)return false;
    return add(add(z,height),-1)<map->height;
}
void set_mission_map_chip(MapComponent* map,std::int32_t x,std::int32_t z,ComponentSlots* e,MissionMapHost& host){
    if(!map_location_in_range(map,x,z,1,1,host))return;
    auto grid=required(map).chips;auto row=required(grid.get()).at(static_cast<std::size_t>(z));
    required(row.get()).at(static_cast<std::size_t>(x))=e;
}
void load_mission_map(MissionMapSystem& s,std::shared_ptr<StageQuest> quest,MissionMapHost& host){
    auto& q=required(quest.get());auto* output=s.map;const auto width_value=quest_map_width();required(output).width=width_value;
    output=s.map;const auto height_value=stage_quest_height(q);required(output).height=height_value;
    output=s.map;auto& initial=required(output);const auto width=initial.width,height=initial.height;
    initial.chips=make_map_grid(height,width);
    auto* cursor=host.current_map_cursor();auto& current=required(s.map);
    const auto cursor_x=multiply(add(2,negate(height)),current.step_x);
    const auto cursor_y=multiply(current.step_z,add(height,-2))/2;
    required(cursor).set_cursor(cursor_x,cursor_y);
    const auto last=add(height,-1);
    if(last<0)return;
    auto base_depth=add(-300,negate(multiply(last,add(width,-1))));
    auto depth_step=last;
    for(std::int32_t z=0;z<height;++z){
        auto depth=base_depth;
        for(std::int32_t x=0;x<width;++x){
            const auto texture=z==last?quest_map_start_image(q):z==0?quest_map_end_image(q):
                (x==0||x==add(width,-1))?quest_map_background_image(q):quest_map_middle_image(q);
            auto* world=host.current_map_factory();auto& map=required(s.map);auto& binding=required(world);
            const auto step_x=map.step_x,step_z=map.step_z;
            auto& e=create_map_chip({x,z,0,18,0,0,0,texture,step_x,step_z,false,0,0},binding.ids,binding.factory);
            if(z<last&&x<=1)required(dynamic_cast<EntityDepth*>(e.get(4))).extra=depth;
            set_mission_map_chip(s.map,x,z,&e,host);
            depth=add(depth,depth_step);
        }
        --depth_step;base_depth=add(base_depth,add(width,-1));
    }
}
}
