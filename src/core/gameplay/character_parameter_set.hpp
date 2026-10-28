#pragma once
#include "combat_parameters.hpp"

namespace saga::core {
struct EnemyRecord;
// ParamSet owns Parameter objects; the keyed view is the existing combat adapter.
// Build once, then borrow stable pointees. Copy/move of the owning set is disabled.
class CharacterParameterSet {
public:
    CharacterParameterSet()=default;
    CharacterParameterSet(const CharacterParameterSet&)=delete;
    CharacterParameterSet& operator=(const CharacterParameterSet&)=delete;
    const DamageParameterMap& values()const{return values_;}
private:
    friend std::shared_ptr<CharacterParameterSet> create_character_parameters(
        const CharacterParameters&,std::int32_t,EventInstruction,CharacterParameterHost&);
    friend std::shared_ptr<CharacterParameterSet> create_enemy_parameters(
        const EnemyRecord&,std::int32_t,std::int32_t,EventInstruction,const ParameterMaximumLevels&);
    std::vector<DamageParameter> storage_;
    DamageParameterMap values_;
};
EventInstruction initial_character_parameter_ids();
bool character_parameter_has_maximum(std::int32_t);
// Param.Create(CharaData,level) 0x11782d8. Captured ID array, live growth/maxima;
// duplicate IDs still evaluate providers but retain the first stored Parameter.
std::shared_ptr<CharacterParameterSet> create_character_parameters(const CharacterParameters&,
    std::int32_t level,EventInstruction ids,CharacterParameterHost&);
// Param.Create(EnemyData,level,maxLevel) 0x11785f8; reuse the same owning set.
std::shared_ptr<CharacterParameterSet> create_enemy_parameters(const EnemyRecord&,
    std::int32_t level,std::int32_t max_level,EventInstruction ids,const ParameterMaximumLevels&);
} // namespace saga::core
