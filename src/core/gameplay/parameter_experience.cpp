#include "parameter_experience.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing parameter experience data");return *p;}
std::int32_t signed32(std::uint32_t bits){
    return static_cast<std::int32_t>(bits<0x80000000U?static_cast<std::int64_t>(bits):
                                   static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t add(std::int32_t a,std::int32_t b){return signed32(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t subtract(std::int32_t a,std::int32_t b){return signed32(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
}
ParameterExperienceSystem::ParameterExperienceSystem():queues(std::make_shared<Queues>()){
    for(auto& list:*queues)list=std::make_shared<ParameterExperienceQueue>();
}
void initialize_parameter_experience(ParameterExperienceSystem& state,ParameterExperienceInitializationWorld* world,
                                     std::optional<EventHashes> hashes){
    if(!world)throw std::invalid_argument("missing parameter experience initialization world");
    world->subscribe_parameter_experience([&state](ComponentSlots* e,std::int32_t key){queue_parameter_experience(state,e,key);},hashes);
}
std::int32_t add_parameter_level(DamageParameter& p,std::int32_t amount){
    const auto old=p.level;
    if(old>=p.max_level)return 0;
    p.level=std::max(std::min(add(old,amount),p.max_level),std::int32_t{1});
    return subtract(p.level,old);
}
void queue_parameter_experience(ParameterExperienceSystem& state,ComponentSlots* entity,std::int32_t key){
    auto queue=required(state.queues.get()).at(static_cast<std::size_t>(key));
    required(queue.get()).push_back(entity); // Nulls and duplicates are retained.
}
void level_up_parameter(ComponentSlots* entity,std::int32_t key){
    auto& p=required(resolve_combat_parameter(required(entity),key));
    p.experience=std::max(subtract(p.experience,p.max_experience),std::int32_t{0});
    add_parameter_level(p,1); // No stat-value, maximum, buffer or threshold update.
}
void update_parameter_experience(ParameterExperienceSystem& state){
    required(state.queues.get());
    for(std::int32_t key=0;key<13;++key){
        auto list=required(state.queues.get()).at(static_cast<std::size_t>(key));
        auto& entries=required(list.get());
        for(std::size_t index=0;index<entries.size();){
            auto* entity=entries.at(index);
            auto& p=required(resolve_combat_parameter(required(entity),key));
            const auto step=std::min(p.experience_buffer,std::int32_t{5});
            p.experience_buffer=subtract(p.experience_buffer,step);
            for(std::int32_t point=0;point<step;++point){
                p.experience=add(p.experience,1);
                if(p.experience>=p.max_experience)level_up_parameter(entity,key);
            }
            if(p.experience_buffer<=0)entries.erase(entries.begin()+static_cast<std::ptrdiff_t>(index));
            else ++index;
        }
    }
}
}
