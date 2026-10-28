#include "cell_system.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing cell-system data");return *p;}
template<class T>T& field(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
std::int32_t signed32(std::uint32_t bits){return static_cast<std::int32_t>(bits<0x80000000U?
    static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return signed32(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t subtract(std::int32_t a,std::int32_t b){return signed32(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return signed32(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t divide(std::int32_t a,std::int32_t b){
    if(!b)return 0;
    if(b==-1&&a==std::numeric_limits<std::int32_t>::min())return a;
    return a/b; // ARM64 SDIV: truncate toward zero.
}
std::int32_t centered(float value,std::int32_t step){
    const auto integer=factory_cell_coordinate(value,1); // Existing FCVTZS + positive-infinity quirk.
    return divide(signed32(static_cast<std::uint32_t>(integer)+static_cast<std::uint32_t>(step/2)),step);
}
CellComponent* cast_cell(Component* p){
    auto* cell=dynamic_cast<CellComponent*>(p);
    if(p&&!cell)throw std::invalid_argument("incorrect cell component type");
    return cell;
}
void remove_first(const CellEntities& list,ComponentSlots* e){
    if(!list)return;
    const auto at=std::find(list->begin(),list->end(),e);
    if(at!=list->end())list->erase(at);
}
CellEntities bucket(CellIndexState& state,std::int32_t key){
    auto& values=required(state.buckets.get());const auto at=values.find(key);
    return at==values.end()?nullptr:at->second;
}
void append(CellIndexState& state,std::int32_t key,ComponentSlots* e){
    auto list=bucket(state,key);
    if(!list){list=std::make_shared<std::vector<ComponentSlots*>>();required(state.buckets.get())[key]=list;}
    list->push_back(e); // No deduplication; empty buckets remain after removals.
}
}
void initialize_cells(CellSystemState& state,CellInitializationWorld* world){
    auto& w=required(world);state.subset=w.cell_subset({5,0,1});state.map=w.cell_map();
    state.changed=std::make_shared<std::vector<ComponentSlots*>>();
}
std::int32_t cell_key(MapComponent* map,std::int32_t xi,std::int32_t yi){
    const auto width=required(map).width;
    return signed32(static_cast<std::uint32_t>(xi)+2U*static_cast<std::uint32_t>(yi)*static_cast<std::uint32_t>(width));
}
void update_cells(CellSystemState& state,CellFrameHost& host){
    auto cursor=required(state.subset).cursor();
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);auto& position=field<ContactPosition>(e,0);auto& map=required(state.map);
        const auto xi=centered(position.x,map.step_x),yi=centered(position.z,map.step_z);
        if(xi==field<CellComponent>(e,5).xi&&yi==field<CellComponent>(e,5).yi)continue;
        const auto old_x=field<CellComponent>(e,5).xi,old_y=field<CellComponent>(e,5).yi;
        field<CellComponent>(e,5).xi=xi;field<CellComponent>(e,5).yi=yi;
        required(state.changed.get()).push_back(&e);
        auto* events=host.current_cell_change_world();
        auto* current_map=&required(state.map); // Validate map before the captured world.
        auto& target=required(events);
        target.send_cell_change(&e,cell_key(current_map,old_x,old_y));
    }
    required(state.changed.get()).clear(); // Fresh field after any subscriber replacement.
}
std::int32_t cell_key(std::int32_t xi,std::int32_t yi,CellIndexHost& host){
    return cell_key(required(host.current_cell_world()).cell_map(),xi,yi);
}
std::int32_t cell_key(CellComponent* cell,CellIndexHost& host){
    auto* map=required(host.current_cell_world()).cell_map();auto& c=required(cell);
    return cell_key(map,c.xi,c.yi);
}
void add_indexed_cell(CellIndexState& state,ComponentSlots* e,std::int32_t slot,Component* component,CellIndexHost& host){
    if(slot!=5)return;
    const auto key=cell_key(cast_cell(component),host);append(state,key,e);
}
void remove_indexed_cell(CellIndexState& state,ComponentSlots* e,std::int32_t slot,Component* component,CellIndexHost& host){
    if(slot!=5)return;
    const auto key=cell_key(cast_cell(component),host);remove_first(bucket(state,key),e);
}
void change_indexed_cell(CellIndexState& state,ComponentSlots* e,std::int32_t old_key,CellIndexHost& host){
    remove_first(bucket(state,old_key),e); // Removal precedes entity/Cell/global-world validation.
    auto& entity=required(e);const auto xi=field<CellComponent>(entity,5).xi,yi=field<CellComponent>(entity,5).yi;
    const auto key=cell_key(xi,yi,host);append(state,key,e);
}
void on_cell_change(CellIndexState& state,ComponentSlots* e,std::int32_t old_key,CellIndexHost& host){
    change_indexed_cell(state,e,old_key,host);
}
CellEntities cell_entities(CellIndexState& state,std::int32_t key){return bucket(state,key);}
CellEntities cell_entities(CellIndexState& state,std::int32_t xi,std::int32_t yi,CellIndexHost& host){
    const auto key=cell_key(xi,yi,host);return cell_entities(state,key);
}
void initialize_cell_index(CellIndexState& state,CellInitializationWorld* world,CellIndexHost& host,
                           std::optional<EventHashes> hashes){
    auto& w=required(world);state.map=w.cell_map();state.buckets=std::make_shared<CellIndexState::Buckets>();
    auto* group=w.cell_subset({5});
    host.subscribe_cell_added(required(group),[&state,&host](ComponentSlots* e,std::int32_t i,Component* c){add_indexed_cell(state,e,i,c,host);});
    host.subscribe_cell_removed(*group,[&state,&host](ComponentSlots* e,std::int32_t i,Component* c){remove_indexed_cell(state,e,i,c,host);});
    auto cursor=group->cursor();
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);add_indexed_cell(state,&e,5,e.get(5),host);
    }
    required(host.current_cell_change_world()).subscribe_cell_change(
        [&state,&host](ComponentSlots* e,std::int32_t key){on_cell_change(state,e,key,host);},hashes);
}
EventInstruction project_cell_location(EventInstruction storage,std::int32_t x,std::int32_t y,
                                       std::int32_t step_x,std::int32_t half_step_z){
    auto& values=required(storage.get());
    const auto first=add(add(subtract(x,step_x),multiply(step_x,100)),divide(multiply(step_x,y),half_step_z));
    values.at(0)=first; // A one-element scratch retains this write before the next bounds fault.
    if(values.size()<2)throw std::out_of_range("cell location scratch needs two entries");
    const auto second=subtract(add(add(half_step_z,y),multiply(half_step_z,100)),divide(multiply(half_step_z,x),step_x));
    values[0]=subtract(divide(first,step_x),100)/2;
    values[1]=subtract(divide(second,half_step_z),100)/2;
    return storage;
}
void update_cell_visibility(CellIndexState& state,CellVisibilityHost& host){
    host.clear_cell_visibility();const auto width=host.cell_game_width(),height=host.cell_game_height();
    auto& initial_map=required(state.map);
    auto origin=host.cell_view_origin(initial_map.step_x,initial_map.step_z/2);
    auto& coordinates=required(origin.get());
    if(coordinates.size()<2)throw std::out_of_range("cell visibility origin needs two entries");
    const auto rows=add(divide(height,required(state.map).step_z/2),4);
    if(rows>0){
        const auto x=coordinates[0],y=coordinates[1];
        for(std::int32_t row=0;row<rows;++row){
            const auto columns=add(divide(width,required(state.map).size_width),3);
            if(columns<=0)continue;
            const auto base_x=add(x,row/2),base_y=add(y,(row+1)/2);
            for(std::int32_t column=0;column<columns;++column){
                auto list=cell_entities(state,add(base_x,column),subtract(base_y,column),host);
                if(!list)continue;
                for(std::size_t index=0;index<list->size();++index){
                    auto& e=required(list->at(index));
                    if(!e.contains(21))host.add_cell_visibility(e); // Entity.get_hasInvisible.
                }
            }
        }
    }
    const auto count=host.cell_visibility_count();host.store_cell_visibility_count(count);
}
}
