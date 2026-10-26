#include "core/gameplay/current_culture.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"current culture contract failure\n";std::exit(1);}}
struct Host:CurrentCultureHost {
    std::vector<std::string> calls;
    bool mode=false,fail_name=false,fail_specific=false,fail_native=false,fail_invariant=false;
    std::optional<std::u16string> name=u"synthetic";
    std::shared_ptr<CultureInstance> specific=std::make_shared<CultureInstance>(),invariant=std::make_shared<CultureInstance>();
    CurrentCultureState* mutate=nullptr;
    bool invariant_mode() override {calls.push_back("mode");return mode;}
    std::optional<std::u16string> native_locale_name() override {calls.push_back("name");if(fail_name)throw CultureManagedError("name");return name;}
    std::shared_ptr<CultureInstance> create_specific(const std::u16string& value) override {
        calls.push_back("create");check(name && value==*name);
        if(mutate)mutate->cached=invariant;
        if(fail_specific)throw CultureManagedError("specific");
        if(fail_native)throw std::logic_error("native failure");
        return specific;
    }
    std::shared_ptr<CultureInstance> invariant_culture() override {calls.push_back("invariant");if(fail_invariant)throw CultureManagedError("invariant");return invariant;}
};
int main(){
    {Host h;CurrentCultureState s{h.specific};check(construct_current_culture(s,h)==h.specific && h.calls.empty());check(!s.cached->read_only);}
    {Host h;CurrentCultureState s;h.mode=true;check(construct_current_culture(s,h)==h.invariant && !s.cached);check((h.calls==std::vector<std::string>{"mode","invariant"}));}
    {Host h;CurrentCultureState s;h.mutate=&s;check(construct_current_culture(s,h)==h.specific && s.cached==h.specific);check(s.cached->read_only && s.cached->use_user_override);check((h.calls==std::vector<std::string>{"mode","name","create"}));h.calls.clear();check(construct_current_culture(s,h)==h.specific && h.calls.empty());}
    {Host h;CurrentCultureState s;h.name=u"";h.specific=h.invariant;check(construct_current_culture(s,h)==h.invariant);check(h.invariant->read_only && h.invariant->use_user_override && h.calls.back()=="create");}
    {Host h;CurrentCultureState s;h.name.reset();check(construct_current_culture(s,h)==h.invariant && s.cached==h.invariant);check((h.calls==std::vector<std::string>{"mode","name","invariant"}));check(!h.invariant->use_user_override);}
    {Host h;CurrentCultureState s;h.specific.reset();check(construct_current_culture(s,h)==h.invariant && s.cached==h.invariant);}
    {Host h;CurrentCultureState s;h.fail_specific=true;check(construct_current_culture(s,h)==h.invariant && s.cached==h.invariant);check(h.calls.back()=="invariant");}
    {Host h;CurrentCultureState s;h.fail_name=true;try{construct_current_culture(s,h);check(false);}catch(const CultureManagedError&){}check(!s.cached && h.calls.back()=="name");}
    {Host h;CurrentCultureState s;h.fail_native=true;try{construct_current_culture(s,h);check(false);}catch(const std::logic_error&){}check(!s.cached && h.calls.back()=="create");}
    {Host h;CurrentCultureState s;h.fail_specific=true;h.fail_invariant=true;try{construct_current_culture(s,h);check(false);}catch(const CultureManagedError&){}check(!s.cached && h.calls.back()=="invariant");}
}
