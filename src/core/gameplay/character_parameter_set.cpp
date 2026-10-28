#include "character_parameter_set.hpp"
#include "enemy_rewards.hpp"
#include <stdexcept>

namespace saga::core {
EventInstruction initial_character_parameter_ids(){
    auto result=std::make_shared<EventIntegers>();result->reserve(13);
    for(std::int32_t id=0;id<13;++id)result->push_back(id);
    return result;
}
bool character_parameter_has_maximum(std::int32_t id){return static_cast<std::uint32_t>(id)<2U;}
std::shared_ptr<CharacterParameterSet> create_character_parameters(const CharacterParameters& data,
    std::int32_t level,EventInstruction ids,CharacterParameterHost& host){
    auto result=std::make_shared<CharacterParameterSet>();
    if(!ids)throw std::invalid_argument("missing character parameter IDs");
    result->storage_.reserve(ids->size());
    for(std::size_t index=0;index<ids->size();++index){
        const auto id=ids->at(index);
        const auto value=character_parameter(data,id,level,host);
        const auto maximum=character_parameter_has_maximum(id)?value:0;
        const auto max_level=host.maximum_level(id);
        if(result->values_.find(id)==result->values_.end()){
            DamageParameter p;p.id=id;p.value=value;p.maximum=maximum;p.level=level;p.max_level=max_level;
            result->storage_.push_back(p);result->values_.emplace(id,&result->storage_.back());
        }
    }
    return result;
}
std::shared_ptr<CharacterParameterSet> create_enemy_parameters(const EnemyRecord& data,
    std::int32_t level,std::int32_t max_level,EventInstruction ids,const ParameterMaximumLevels& maxima){
    auto result=std::make_shared<CharacterParameterSet>();
    if(!ids)throw std::invalid_argument("missing enemy parameter IDs");
    result->storage_.reserve(ids->size());
    for(const auto id:*ids){
        const auto value=enemy_parameter(data,id,level,maxima);
        const auto maximum=character_parameter_has_maximum(id)?value:0; // Same unsigned <2 contract.
        if(result->values_.find(id)==result->values_.end()){
            DamageParameter p;p.id=id;p.value=value;p.maximum=maximum;p.level=level;p.max_level=max_level;
            result->storage_.push_back(p);result->values_.emplace(id,&result->storage_.back());
        }
    }
    return result;
}
} // namespace saga::core
