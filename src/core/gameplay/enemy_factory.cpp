#include "enemy_factory.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing enemy factory input");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
}
EventInstruction initial_enemy_parameter_ids(){return initial_character_parameter_ids();} // Independent 0..12 array.
ParameterMaximumLevels initial_enemy_parameter_maximum_levels(){return initial_parameter_maximum_levels();}
bool enemy_parameter_has_maximum(std::int32_t id){return character_parameter_has_maximum(id);}
float enemy_move_speed(const EnemyRecord& data,std::int32_t level,const ParameterMaximumLevels& maxima){
    if((data.flags&1)!=0)return 0;
    const float parameter=static_cast<float>(enemy_parameter(data,5,level,maxima));
    const float scaled=parameter*2.23F;
    const float speed=scaled/100.0F;
    return speed<0.5575F?0.5575F:speed; // Ordered float clamp; NaN passes through.
}
ComponentSlots* create_enemy(EnemyCreation args,EntityIdState& ids,EnemyFactoryHost& host){
    if(args.id<0)return nullptr;
    const auto initial=host.enemy_records();
    if(static_cast<std::size_t>(args.id)>=required(initial.get()).size())return nullptr;
    const auto records=host.enemy_records();
    auto& row=required(records.get()).at(static_cast<std::size_t>(args.id));auto* data=row?&*row:nullptr;
    float speed_z=0;
    if((args.flags&3)==0){auto& source=required(data);const auto& maxima=host.enemy_maximum_levels();
        speed_z=enemy_move_speed(source,args.level,maxima);}
    const auto resource=host.enemy_resource_id(required(data).resource);
    const auto clip=data->seb==-1?word(static_cast<std::uint32_t>(data->enemy_seb)*4U):data->seb;
    const auto frame=data->frame==-1?0:data->frame;
    auto& e=create_entity(ids,0,false,host);
    auto& position=host.acquire_position();position.x=args.position.x;position.y=args.position.y;position.z=args.position.z;
    position.offset_x=0;position.offset_y=0;position.offset_z=0;position.parent=nullptr;e.add(0,&position);
    host.add_cell(e,factory_cell_coordinate(args.position.x),factory_cell_coordinate(args.position.z));
    auto& depth=host.acquire_depth();depth.value=0;depth.extra=10;e.add(4,&depth);
    host.add_image(e,{{data->image,resource,-1,-1,-1,-1}});
    auto& sprite=host.acquire_sprite();sprite.animation={resource,clip,frame};sprite.layers=std::make_shared<std::vector<std::int32_t>>(1,-1);e.add(2,&sprite);
    auto& speed=host.acquire_speed();speed.x=0;speed.y=0;speed.z=speed_z;e.add(1,&speed);
    auto& enemy=host.acquire_enemy();enemy.level=args.level;enemy.flags=args.flags;enemy.state=0;enemy.frame=0;
    enemy.enemy_id=args.id;enemy.opponent=nullptr;e.add(24,&enemy);
    auto& ai=host.acquire_enemy_ai();ai.scripts=std::make_shared<HumanScriptCache>();ai.state=0;e.add(20,&ai);
    const auto keys=host.enemy_parameter_ids();const auto& maxima=host.enemy_maximum_levels();
    const auto parameters=create_enemy_parameters(*data,args.level,1000,keys,maxima);
    auto& links=host.acquire_enemy_parameters();links.storage=parameters;links.values=&parameters->values();
    links.extras=std::make_shared<std::vector<ComponentSlots*>>();e.add(23,&links);
    auto& color=host.acquire_enemy_color();color.values={{255,255,255,255}};e.add(6,&color);
    add_party_collision(e,12,0,5,0,false,host);
    if(args.direction!=-1){auto& direction=host.acquire_enemy_direction();direction.direction=args.direction;direction.locked=false;e.add(12,&direction);}
    if((data->flags&1)==0)add_animation_component(e,1,-1,host);
    return &e;
}
} // namespace saga::core
