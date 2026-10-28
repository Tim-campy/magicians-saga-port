#pragma once
#include "combat_parameters.hpp"

namespace saga::core {
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
    std::vector<DamageParameter> storage_;
    DamageParameterMap values_;
};
EventInstruction initial_character_parameter_ids();
bool character_parameter_has_maximum(std::int32_t);
// Param.Create(CharaData,level) 0x11782d8. Captured ID array, live growth/maxima;
// duplicate IDs still evaluate providers but retain the first stored Parameter.
std::shared_ptr<CharacterParameterSet> create_character_parameters(const CharacterParameters&,
    std::int32_t level,EventInstruction ids,CharacterParameterHost&);
} // namespace saga::core
