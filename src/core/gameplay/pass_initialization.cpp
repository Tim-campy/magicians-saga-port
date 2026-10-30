#include "pass_initialization.hpp"
#include "input_frame.hpp"
#include "member_selection.hpp"
#include "table_catalog.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& town_required(const std::shared_ptr<T>& value){if(!value)throw std::invalid_argument("missing fresh town state");return *value;}
TownDefinition& town_record(const std::shared_ptr<TownTable>& table,std::size_t index){
    auto& row=town_required(table).at(index);if(!row)throw std::invalid_argument("missing town record");return *row;
}
void read_town(TownDefinition& r,detail::TableTokens& t){
    r.id=t.integer();r.first_build=t.integer();r.open_term=t.script();r.rock_term=t.script();r.flags=t.integer();
}
}
TownDefinition& load_town_definition(TownDefinition& r,std::optional<std::string_view> line,const EventIntegerParser& parser){
    // TownData.Load 0x11735c8: ordered stores, null/empty receiver preservation.
    if(line&&!line->empty()){detail::TableTokens t(*line,parser);read_town(r,t);}return r;
}
TownTable load_town_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    return detail::load_table_records<TownDefinition>(text,normalize,parser,[](detail::TableTokens& t){TownDefinition r;read_town(r,t);return r;});
}
void set_town_building(TownInitializationState& state,std::int32_t town,std::int32_t building,std::int32_t value,bool deferred,FreshTownHost& host){
    // TownSystem.SetBuildingList 0x1169934. Native forward RemoveAt skips a shifted consecutive match.
    auto list=state.building_list;
    for(std::size_t i=0;i<town_required(list).size();++i){
        const auto row=town_required(list).at(i);
        if(town_required(row).at(0)==town)town_required(state.building_list).erase(town_required(state.building_list).begin()+static_cast<std::ptrdiff_t>(i));
        list=state.building_list;
    }
    town_required(list).push_back(std::make_shared<EventIntegers>(EventIntegers{town,building,value}));
    if(!deferred)host.rebuild_town();
}
void initialize_fresh_town(TownInitializationState& state,FreshTownHost& host){
    // TownSystem.Init 0x1169678..0x1169894, after event31 and the global loaded-byte gate.
    state.build_enable=std::make_shared<std::vector<std::uint8_t>>(town_required(host.town_records()).size(),std::uint8_t{0});
    std::int32_t first_count=0;
    for(std::size_t i=0;i<town_required(host.town_records()).size();++i){
        const auto term=town_record(host.town_records(),i).open_term;
        if(!term||term->empty())town_required(state.build_enable).at(i)=1;
        if((static_cast<std::uint32_t>(town_record(host.town_records(),i).flags)&1U)!=0){
            const auto building=town_record(host.town_records(),i).first_build;
            set_town_building(state,static_cast<std::int32_t>(i),building,0,true,host);++first_count;
        }
    }
    if(first_count>host.saved_town_count())host.save_town_count(first_count);
}
void initialize_town_entry(TownInitializationWorld* world,std::function<void(const TownReturnEvent&)> receiver){
    if(!world)throw std::invalid_argument("missing town initialization world");
    world->subscribe_town_return(std::move(receiver));
    auto* global=world->current_town_world();if(!global)throw std::invalid_argument("missing current town world");
    if(global->loaded==0)world->initialize_fresh_town();
}
MenuRowShapes initial_menu_row_shapes(){return std::make_shared<std::vector<std::optional<std::size_t>>>(7,std::size_t{4});}
void initialize_submenu(AuxiliaryInitializationWorld* world,AuxiliaryInitializationWorld::Button receiver){
    if(!world)throw std::invalid_argument("missing submenu initialization world");world->subscribe_auxiliary_button(27,std::move(receiver));
}
void initialize_menu(MenuRowShapes rows,AuxiliaryInitializationWorld* world,AuxiliaryInitializationWorld::Button button,std::function<void()> orientation){
    if(!rows)throw std::invalid_argument("missing menu rows");
    for(const auto& row:*rows){if(!row)throw std::invalid_argument("missing menu row");if(*row<3)throw std::out_of_range("short menu row");}
    if(!world)throw std::invalid_argument("missing menu initialization world");
    world->subscribe_auxiliary_button(28,std::move(button));world->subscribe_menu_orientation(std::move(orientation));
}
void initialize_status_render() noexcept {}
void submenu_button_touch(const InputTouch* event){if(!event||!event->component)throw std::invalid_argument("missing submenu touch");}
void menu_button_touch(bool visible,const InputTouch* event,const std::function<void()>& off){
    if(!visible)return;
    if(!event||!event->component)throw std::invalid_argument("missing menu touch");
    if(event->component->group==35&&event->type==5)off();
}
void initialize_rotation(EntitySubset*& destination,PassInitializationWorld* world){
    if(!world)throw std::invalid_argument("missing rotation initialization world");
    destination=world->subset({12,1,2,7});
}
namespace {
PassInitializationWorld& require(PassInitializationWorld* world){
    if(!world)throw std::invalid_argument("missing gameplay initialization world");
    return *world;
}
}
void initialize_battle(BattleSystemState& state,PassInitializationWorld* world){
    auto& target=require(world);state.humans=target.subset({20,14});state.monsters=target.subset({20,16});
}
void update_battle(BattleSystemState& state){
    if(!state.humans)throw std::invalid_argument("missing battle human subset");
    auto cursor=state.humans->cursor();while(cursor.next()){}
}
void initialize_debug_visualization(DebugVisualizationState& state,DebugInitializationWorld* world){
    if(!world)throw std::invalid_argument("missing debug initialization world");
    state.subset=world->debug_subset({14,0,1},{9});state.flags=0;
}
namespace {
std::int32_t debug_word(std::uint32_t bits){return static_cast<std::int32_t>(bits<0x80000000U?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);}
}
bool check_debug_flag(const DebugVisualizationState& state,std::int32_t mask){return (static_cast<std::uint32_t>(state.flags)&static_cast<std::uint32_t>(mask))!=0;}
void enable_debug_flag(DebugVisualizationState& state,std::int32_t mask){state.flags=debug_word(static_cast<std::uint32_t>(state.flags)|static_cast<std::uint32_t>(mask));}
void disable_debug_flag(DebugVisualizationState& state,std::int32_t mask){state.flags=debug_word(static_cast<std::uint32_t>(state.flags)&~static_cast<std::uint32_t>(mask));}
void switch_debug_flag(DebugVisualizationState& state,std::int32_t mask){if(check_debug_flag(state,mask))disable_debug_flag(state,mask);else enable_debug_flag(state,mask);}
void reset_debug_flags(DebugVisualizationState& state){state.flags=0;}
void update_debug_visualization(const DebugVisualizationState& state,bool application_debug){
    if(!application_debug||state.flags==0)return;
    // Native tests bits1..256 with BitUtil.Check and discards every result.
    // Preserve the observed inert body; do not introduce pathfinding or drawing.
    for(std::int32_t mask=1;mask<=256;mask*=2)static_cast<void>(check_debug_flag(state,mask));
}
void debug_visualization_enabled(DebugPathVisualization& path){path.visualize=true;path.wait_milliseconds=0;}
void debug_visualization_disabled(DebugPathVisualization& path){path.visualize=false;path.wait_milliseconds=0;}
void initialize_projectiles(ProjectileSystemState& state,PassInitializationWorld* world,CombatEventHost& host,
                            std::optional<EventHashes> hashes){
    auto& target=require(world);
    state.subset=target.subset({27,0,1});
    state.pass.emplace(); // Fresh persistent completion list, before subscription.
    target.subscribe_impact([&host](ComponentSlots* e){projectile_impact_cleanup(e,host);},hashes);
}
void initialize_movement(EntitySubset*& destination,PassInitializationWorld* world){
    destination=require(world).subset({0,1});
}
void initialize_collisions(CollisionSystemState& state,PassInitializationWorld* world){
    state.subset=require(world).subset({32,0});
    state.pass.emplace(); // Fresh persistent pair list after storing the subset.
}
void initialize_garbage(EntitySubset*& destination,PassInitializationWorld* world){
    destination=require(world).subset({22});
}
void initialize_parameters(ParameterInitializationWorld* world,std::optional<EventHashes> hashes){
    if(!world)throw std::invalid_argument("missing parameter initialization world");
    world->subscribe_dropoff(parameter_dropoff,hashes);
}
void parameter_dropoff(ComponentSlots*) noexcept {}
void update_parameters() noexcept {}
}
