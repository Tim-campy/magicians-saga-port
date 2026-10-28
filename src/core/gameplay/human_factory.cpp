#include "human_factory.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing human factory data");return *p;}
std::int32_t word(std::uint32_t bits){
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
}
bool is_main(const PartyCharacter& record){return (static_cast<std::uint32_t>(record.flags)&2U)!=0;}
std::int32_t main_hat(HumanFactoryHost& host){
    const auto hats=host.hats();
    for(auto* hat:required(hats.get()))if((static_cast<std::uint32_t>(required(hat).flags)&2U)!=0)return hat->image;
    throw std::invalid_argument("missing main character hat"); // FirstOrDefault null is then dereferenced.
}
}
ComponentSlots& create_human(const HumanCreation& args,EntityIdState& ids,HumanFactoryHost& host){
    const auto records=host.character_table();
    auto& record=required(required(records.get()).at(static_cast<std::size_t>(args.character)));
    const auto weapon=record.weapon_id;
    const auto weapon_image=host.weapon_image(weapon);
    const auto hat_image=is_main(record)?main_hat(host):-2;
    auto textures=std::make_shared<EventIntegers>(5,0);
    textures->at(0)=200;textures->at(1)=record.image;textures->at(2)=record.image;
    textures->at(3)=hat_image;textures->at(4)=weapon_image;
    auto resources=std::make_shared<EventIntegers>(EventIntegers{1,1,1,21,22});
    auto& entity=create_entity(ids,0,false,host);
    auto& position=host.acquire_position();position.x=args.x;position.y=args.y;position.z=args.z;
    position.offset_x=0;position.offset_y=0;position.offset_z=0;position.parent=nullptr;entity.add(0,&position);
    host.add_cell(entity,factory_cell_coordinate(args.x),factory_cell_coordinate(args.z));
    auto& depth=host.acquire_depth();depth.value=0;depth.extra=10;entity.add(4,&depth);
    add_image_component(entity,textures,resources,host);
    auto& sprite=host.acquire_sprite();sprite.animation={1,word(static_cast<std::uint32_t>(args.direction)+36U),0};sprite.layers=std::make_shared<std::vector<std::int32_t>>(1,-1);entity.add(2,&sprite);
    auto& speed=host.acquire_speed();speed.x=0;speed.y=0;speed.z=0;entity.add(1,&speed);
    add_animation_component(entity,1,-1,host);
    auto& direction=host.acquire_direction();direction.direction=args.direction;direction.locked=false;entity.add(12,&direction);
    auto& human=host.acquire_human();
    human.scripts=std::make_shared<std::vector<std::shared_ptr<void>>>();
    human.character_id=args.character;human.level=args.level;human.experience=args.experience;
    human.state=0;human.frame=0;human.formation_x=args.formation_x;human.formation_z=args.formation_z;
    human.attack_frame=0;human.opponent=nullptr;human.invincible_time=0;human.formation_index=0;human.dash_speed=0;
    entity.add(14,&human);
    auto parameters=create_character_parameters(record.parameters,args.level,host.parameter_ids(),host);
    auto& links=host.acquire_parameters();links.storage=parameters;links.values=&parameters->values();
    links.extras=std::make_shared<std::vector<ComponentSlots*>>();entity.add(23,&links);
    auto equipment=std::make_shared<EventIntegers>(4,0);equipment->at(0)=weapon;
    auto& equip=host.acquire_equipment();equip.ids=equipment;entity.add(8,&equip);
    add_party_collision(entity,12,0,5,0,false,host);
    if(is_main(record))entity.add(28,&host.acquire_input());
    else{
        auto& ai=host.acquire_ai();ai.scripts=std::make_shared<HumanScriptCache>();ai.state=0;entity.add(20,&ai);
    }
    return entity;
}
} // namespace saga::core
