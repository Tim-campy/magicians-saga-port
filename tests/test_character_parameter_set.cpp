#include "core/gameplay/character_parameter_set.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host final:CharacterParameterHost {
    ParameterMaximumLevels maxima=initial_parameter_maximum_levels();std::vector<std::int32_t> calls;
    std::int32_t maximum_level(std::int32_t id)override{calls.push_back(id);return maxima.at(static_cast<std::size_t>(id));}
    const ShopParameterUpgrade& shop_upgrade(std::int32_t)override{throw std::runtime_error("unexpected shop");}
};
int main(){
    CharacterParameters data;data.values=std::make_shared<std::vector<EventInstruction>>();
    for(std::int32_t i=0;i<13;++i)data.values->push_back(std::make_shared<EventIntegers>(EventIntegers{10+i,10+i}));
    Host host;auto ids=std::make_shared<EventIntegers>(EventIntegers{1,0,2,1});
    auto set=create_character_parameters(data,7,ids,host);
    check(host.calls==EventIntegers({1,1,0,0,2,2,1,1})&&set->values().size()==3);
    auto& p=*set->values().at(1);check(p.id==1&&p.value==11&&p.maximum==11&&p.level==7&&p.max_level==1000);
    check(p.extra==0&&p.extra_maximum==0&&p.experience==0&&p.max_experience==0&&p.experience_buffer==0);
    check(set->values().at(2)->value==12&&set->values().at(2)->maximum==0);
    check(character_parameter_has_maximum(0)&&character_parameter_has_maximum(1)&&!character_parameter_has_maximum(-1)&&!character_parameter_has_maximum(2));
    auto full=create_character_parameters(data,1,initial_character_parameter_ids(),host);check(full->values().size()==13&&full->values().at(12)->value==22);
    auto empty=create_character_parameters(data,1,std::make_shared<EventIntegers>(),host);check(empty->values().empty());
    fails([&]{create_character_parameters(data,1,nullptr,host);});
    data.values->at(12).reset();fails([&]{create_character_parameters(data,1,std::make_shared<EventIntegers>(EventIntegers{12}),host);});
}
