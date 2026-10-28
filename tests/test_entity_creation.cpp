#include "core/ecs/entity_creation.hpp"
#include <cstdlib>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:EntityCreationHost {
    ComponentSlots entity{35};std::vector<std::string> calls;
    std::vector<std::int32_t> probes;std::int32_t free_id=0,type_seen=0,id_seen=0;
    bool null_factory=false,fail_factory=false,fail_registration=false,temporary_seen=false;
    std::function<void(std::int32_t)> on_probe;
    bool registered_id(std::int32_t id)override{probes.push_back(id);calls.push_back("probe");if(on_probe)on_probe(id);return id!=free_id;}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{
        calls.push_back("create");type_seen=type;id_seen=id;
        if(fail_factory)throw std::runtime_error("factory");return null_factory?nullptr:&entity;
    }
    void register_entity(ComponentSlots& e,bool temporary)override{
        check(&e==&entity);calls.push_back("register");temporary_seen=temporary;
        if(fail_registration)throw std::runtime_error("registration");
    }
};
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
int main(){
    {EntityIdState state;Host h;check(&create_entity(state,0,false,h)==&h.entity);
        check(state.counter==1&&h.id_seen==0&&!h.temporary_seen&&h.probes.empty());
        create_entity(state,7,true,h);check(state.counter==2&&h.id_seen==-1&&h.type_seen==7&&h.temporary_seen);
        create_entity(state,0,false,h);check(state.counter==3&&h.id_seen==2);}
    for(auto initial:{0x7ffffffc,0x7ffffffd,0x7ffffffe}){EntityIdState state{initial};Host h;h.free_id=2;
        h.on_probe=[&](std::int32_t){check(state.counter==0x7ffffffd);};
        create_entity(state,0,false,h);check(state.counter==0x7ffffffd&&h.id_seen==2&&h.probes==std::vector<std::int32_t>{0,1,2});
        check(h.calls==std::vector<std::string>{"probe","probe","probe","create","register"});}
    {EntityIdState state{0x7ffffffc};Host h;h.free_id=1;create_entity(state,0,true,h);
        check(state.counter==0x7ffffffd&&h.id_seen==-1&&h.probes==std::vector<std::int32_t>{0,1});}
    {EntityIdState state{std::numeric_limits<std::int32_t>::max()};Host h;create_entity(state,0,false,h);
        check(state.counter==std::numeric_limits<std::int32_t>::min()&&h.id_seen==std::numeric_limits<std::int32_t>::max()&&h.probes.empty());}
    for(int failure=0;failure<3;++failure){EntityIdState state{9};Host h;
        h.null_factory=failure==0;h.fail_factory=failure==1;h.fail_registration=failure==2;
        fails([&]{create_entity(state,8,false,h);});check(state.counter==10&&h.id_seen==9);
        check(h.calls.size()==(failure==2?2U:1U));}
    {EntityIdState state{0x7ffffffc};Host h;h.on_probe=[](std::int32_t){throw std::runtime_error("lookup");};
        fails([&]{create_entity(state,0,true,h);});check(state.counter==0x7ffffffd&&h.calls==std::vector<std::string>{"probe"});}
}
