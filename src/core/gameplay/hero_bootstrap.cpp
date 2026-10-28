#include "hero_bootstrap.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing hero bootstrap state");return *p;}
bool main_character(const PartyCharacter& data){return (static_cast<std::uint32_t>(data.flags)&2U)!=0;}
HumanRecordLink& human(ComponentSlots* e){return required(dynamic_cast<HumanRecordLink*>(required(e).get(14)));}
}
bool world_is_new_game(const PlayerWorldState& world){return world.loaded==0;}
ComponentSlots& create_hero(PartyHumanFactory* receiver,HeroBootstrapHost& host){
    const auto records=host.character_table();PartyCharacter* selected=nullptr;
    for(auto* data:required(records.get()))if(data&&main_character(*data)){selected=data;break;}
    const auto id=required(selected).id;
    auto& entity=required(required(receiver).create_human({0,id,1,0,0,0,0,0,0}));
    entity.add(30,host.player_marker());
    return entity;
}
void initialize_member_system(MemberRoster& roster,PlayerWorldState* argument,HeroBootstrapHost& host){
    if(!world_is_new_game(required(argument)))return;
    auto& creation_world=required(host.current_world());
    create_hero(host.world_factory(creation_world),host);
    auto& data=required(host.character(human(member_hero(host.current_world()))));
    set_formation_character(roster,10,data.id,[&host](std::int32_t id){
        const auto records=host.character_table();
        return main_character(required(required(records.get()).at(static_cast<std::size_t>(id))));
    });
}
void MemberSystemInitializer::init(InitializationWorld* world){
    auto& binding=required(dynamic_cast<MemberBootstrapWorld*>(world));
    initialize_member_system(roster_,binding.state,host_);
}
} // namespace saga::core
