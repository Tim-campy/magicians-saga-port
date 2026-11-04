#include "town_residents.hpp"
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace saga::core {
namespace {
template<class T>T& required(const std::shared_ptr<T>& value){
    if(!value)throw std::invalid_argument("missing town resident data");return *value;
}
const CharacterProgressionRecord& definition(const std::shared_ptr<TownResidentCharacter>& value){
    const auto* row=required(value).definition;
    if(!row)throw std::invalid_argument("missing resident character definition");return *row;
}
CharacterNewGameState& progress(const std::shared_ptr<TownResidentCharacter>& value){
    auto* row=required(value).progress;
    if(!row)throw std::invalid_argument("missing resident character save state");return *row;
}
std::int32_t count(std::size_t size){
    if(size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("resident count exceeds int32");
    return static_cast<std::int32_t>(size);
}
std::int32_t wrap(std::int64_t value){
    const auto bits=static_cast<std::uint32_t>(value);
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
}
bool has(const std::shared_ptr<TownResidentCharacter>& value,std::int32_t flag){
    return (static_cast<std::uint32_t>(definition(value).flags)&static_cast<std::uint32_t>(flag))!=0;
}
void consume_discarded_shuffle(const TownResidentCharacters& source,TownResidentInitializationHost& host){
    // Lib.Shuffle generic instance 0x12d5248: copy, draw n..2, swap, return copy.
    // The caller ignores its return; retaining swaps also preserves index failures.
    auto destination=source;
    for(auto remaining=count(source.size());remaining>=2;--remaining){
        const auto index=host.lib_random(remaining);
        auto& chosen=destination.at(static_cast<std::size_t>(index));
        auto& last=destination.at(static_cast<std::size_t>(remaining-1));
        const auto previous=chosen;chosen=last;last=previous;
    }
}
}
bool update_town_residents(TownResidentUpdateHost& host){
    const auto range=host.active_range();
    for(std::int32_t index=0;;++index){
        const auto source=host.resident_rows();
        if(index>=count(required(source).size()))break;
        const auto row=source->at(static_cast<std::size_t>(index));auto& fields=required(row);
        static_cast<void>(fields.at(9));
        auto x=fields.at(1);const auto type=fields.at(8);const auto frame=wrap(static_cast<std::int64_t>(fields.at(9))+1);
        auto velocity=fields.at(3);x=wrap(static_cast<std::int64_t>(x)+velocity);
        fields.at(9)=frame;fields.at(1)=x;
        bool right_clamped=false;
        if(type!=2){
            const auto left=required(range).at(0);
            if(x<left){velocity=0;x=left;fields.at(1)=left;fields.at(3)=0;}
            const auto right=range->at(1);
            if(x>right){x=right;fields.at(1)=right;fields.at(3)=0;right_clamped=true;}
        }
        const auto move=fields.at(7);
        if(move==1){
            if(!right_clamped&&velocity!=0){
                const auto direction=velocity<1?2:1;fields.at(6)=direction;fields.at(5)=40|direction;
            }else fields.at(5)=wrap(static_cast<std::int64_t>(fields.at(6))+36);
            if(frame>=61&&host.app_random(100)==0){
                const auto selected=host.app_random(5);fields.at(3)=selected<3?wrap(static_cast<std::int64_t>(selected)-1):0;
                fields.at(9)=0;
            }
        }else if(move==0){
            const auto hero=host.resident_hero_x();const auto direction=hero<static_cast<float>(x)?2:1;
            fields.at(5)=36|direction;fields.at(6)=direction;
        }
        if(fields.at(8)==2){fields.at(3)=8;fields.at(5)=9;fields.at(6)=1;}
    }
    return true;
}
void initialize_town_residents(TownResidentInitializationState& state,TownResidentInitializationHost& host){
    const auto challenges=host.challenge_count();const auto store_limit=host.store_limit();
    state.residents=std::make_shared<std::vector<EventInstruction>>();
    TownResidentCharacters candidates;
    const auto initial=host.characters();
    for(const auto& row:required(initial))if(row&&has(row,0x20))candidates.push_back(row);
    consume_discarded_shuffle(candidates,host);
    const auto living=host.living_submembers();std::int32_t living_count=0;
    for(const auto& row:required(living))if(row){candidates.push_back(row);++living_count;}
    if(living_count<=8){
        TownResidentCharacters visitors;const auto records=host.characters();
        for(const auto& row:required(records))if(row&&progress(row).recruited!=0&&progress(row).living_address==-1)visitors.push_back(row);
        if(!visitors.empty()){
            const auto selected=host.app_random(count(visitors.size()));
            candidates.push_back(visitors.at(static_cast<std::size_t>(selected)));
        }
    }
    std::unordered_set<std::int32_t> used_positions;
    const auto active=host.active_range();
    for(const auto& character:candidates){
        const auto rate=definition(character).appear_rate;
        if(host.lib_random(100)>=rate)continue; // Lib.Hits always consumes the draw.
        const auto terms=definition(character).resident_terms;
        if(terms&&!terms->empty()&&!host.conditions(terms))continue;
        std::int32_t x=-1,lane=0;std::int32_t key=0;bool deduplicate=false;
        if(has(character,4)){
            if(progress(character).living_address!=-1){
                const auto address=progress(character).living_address;
                x=host.building_position(address)[0];
                key=wrap(static_cast<std::int64_t>(progress(character).living_address)+100);deduplicate=true;
            }else x=wrap(static_cast<std::int64_t>(host.building_position(0)[0])+45);
        }else if(definition(character).town_id!=-1){
            x=host.building_position(definition(character).town_id)[0];
            key=definition(character).town_id;deduplicate=true;
        }else if(definition(character).area_id!=-1){
            x=host.area_position(definition(character).area_id)[0];
            key=wrap(static_cast<std::int64_t>(definition(character).area_id)+1000);deduplicate=true;
        }else lane=host.app_random(3);
        if(deduplicate){
            if(used_positions.find(key)!=used_positions.end())continue;
            used_positions.insert(key);
        }
        const auto store=has(character,0x80);
        if(challenges==store_limit&&store)continue;
        const auto destination=state.residents;
        if(store)lane=0;
        auto row=std::make_shared<EventIntegers>(11,0);
        row->at(0)=definition(character).id;
        if(x==-1)x=host.app_random(required(active).at(1));
        row->at(1)=x;row->at(2)=lane;row->at(5)=38;row->at(6)=2;
        row->at(7)=has(character,0x40)?0:1;
        row->at(8)=has(character,0x80)?1:0;
        const auto frame=host.app_random(60);
        row->at(9)=wrap(static_cast<std::int64_t>(frame)+(has(character,0x40)?60:0));
        row->at(10)=host.app_random(9999);
        required(destination).push_back(std::move(row));
    }
    if(host.lib_random(100)>=10)return;
    for(std::int32_t index=0;index<count(required(state.residents).size());++index){
        const auto row=state.residents->at(static_cast<std::size_t>(index));
        if(required(row).at(1)<1)continue;
        if(row->at(8)!=0)continue;
        row->at(8)=3;break;
    }
    state.present_value=wrap(static_cast<std::int64_t>(host.lib_random(2,5))*100);
}
std::array<std::int32_t,2> town_building_position(std::int32_t index,std::int32_t height){
    return {wrap(static_cast<std::int64_t>(index)*90+130),wrap(static_cast<std::int64_t>(height)-100)};
}
std::array<std::int32_t,2> town_obstacle_position(const TownResidentGeometryState& state,TownResidentGeometryHost& host){
    const auto camera_x=state.camera_x,camera_y=state.camera_y,view_x=state.view_x,view_y=state.view_y;
    std::array<std::int32_t,2> result{wrap(static_cast<std::int64_t>(camera_x)+view_x+state.right_extent-120),0};
    const auto height=host.resident_view_height();
    result[1]=wrap(static_cast<std::int64_t>(view_y)+camera_y+height-70);return result;
}
std::array<std::int32_t,2> town_resident_active_range(const TownResidentGeometryState& state,TownResidentGeometryHost& host){
    const auto camera_x=state.camera_x,view_x=state.view_x;
    std::array<std::int32_t,2> result{wrap(static_cast<std::int64_t>(state.left_extent)+60),wrap(static_cast<std::int64_t>(state.right_extent)-60)};
    const auto capacity=host.resident_town_capacity();const auto towns=host.resident_town_record_count();
    if(capacity<towns){
        const auto obstacle=town_obstacle_position(state,host);
        result[1]=wrap(static_cast<std::int64_t>(obstacle[0])-wrap(static_cast<std::int64_t>(camera_x)+view_x)-30);
    }
    const auto first=host.resident_geometry_areas();const auto second=host.resident_geometry_areas();
    const auto index=wrap(static_cast<std::int64_t>(count(required(second).size()))-1);
    const auto row=required(first).at(static_cast<std::size_t>(index));
    if(required(row).state!=1)result[0]=wrap(static_cast<std::int64_t>(state.left_extent)+90);
    return result;
}
}
