#include "core/gameplay/specific_culture.hpp"
#include "core/gameplay/culture_lower.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"specific culture contract failure at "<<line<<"\n";std::exit(1);}}
#define check(value) check_at((value),__LINE__)
void word(std::string& bytes,std::size_t pos,std::size_t value){bytes[pos]=static_cast<char>(value&255);bytes[pos+1]=static_cast<char>((value>>8)&255);}
struct Fixture {
    std::string names=std::string(12,'\0'),records=std::string(168,'\0'),pool=std::string(1,'\0');
    std::vector<NeutralCultureEntry> neutral{{u"aa",2}};
    std::size_t add(const std::string& value){auto offset=pool.size();pool+=value;pool+='\0';return offset;}
    Fixture(){
        const auto aa=add("aa"),region=add("aa-ZZ"),key=add("aa-zz"),bb=add("bb"),territory=add("ZZ");
        word(names,0,aa);word(names,2,0);word(names,4,key);word(names,6,1);word(names,8,bb);word(names,10,2);
        word(records,0,1);word(records,8,aa);word(records,0x20,8);
        word(records,56,2);word(records,64,region);word(records,56+0x14,territory);word(records,56+0x20,9);
        word(records,112,3);word(records,120,bb);word(records,112+0x20,10);
    }
    CultureTables tables()const{return {names,records,pool,neutral};}
};
struct Host:SpecificCultureHost {
    std::vector<std::u16string> lowered;
    InvariantCultureState state;
    std::shared_ptr<CultureInstance> invariant=state.instance;
    std::shared_ptr<CultureNumberFormat> invariant_number_format()override{return state.number_format;}
    std::u16string lower_invariant(const std::u16string& value)override{
        lowered.push_back(value);
        // ASCII fixture needs no table reads; use the recovered lowering kernel.
        return lower_invariant_text(value,{});
    }
    std::shared_ptr<CultureInstance> invariant_culture()override{return invariant;}
};
template<class F>void fails(F run,CultureSelectionFailure expected){try{run();check(false);}catch(const CultureSelectionError& e){check(e.kind==expected);}}
struct Current:CurrentCultureHost {
    Fixture& fixture;Host& host;std::u16string name;
    Current(Fixture& f,Host& h,std::u16string n):fixture(f),host(h),name(std::move(n)){}
    bool invariant_mode()override{return false;}
    std::optional<std::u16string> native_locale_name()override{return name;}
    std::shared_ptr<CultureInstance> create_specific(const std::u16string& n)override{return create_specific_culture(n,fixture.tables(),host);}
    std::shared_ptr<CultureInstance> invariant_culture()override{return host.invariant;}
};
int main(){
    Fixture f;Host h;
    fails([&]{create_specific_culture(std::nullopt,f.tables(),h);},CultureSelectionFailure::NullName);
    check(create_specific_culture(u"",f.tables(),h)==h.invariant && h.lowered.empty());
    auto direct=create_specific_culture(u"AA-ZZ",f.tables(),h);
    check(direct->name==u"aa-ZZ" && direct->identity.lcid==2 && direct->identity.territory==u"ZZ");
    check(direct->identity.constructed && direct->number_index==9 && !direct->use_user_override && !direct->read_only);
    h.lowered.clear();auto neutral=create_specific_culture(u"AA-extra",f.tables(),h);
    check(neutral->name==u"aa-ZZ" && neutral->use_user_override && !neutral->read_only);
    check((h.lowered==std::vector<std::u16string>{u"AA-extra",u"aa"}));
    fails([&]{create_specific_culture(u"bb",f.tables(),h);},CultureSelectionFailure::UnknownNeutral);
    fails([&]{create_specific_culture(u"missing",f.tables(),h);},CultureSelectionFailure::NameNotFound);
    fails([&]{create_culture_by_lcid(-1,false,false,f.tables(),h);},CultureSelectionFailure::NegativeLcid);
    fails([&]{create_culture_by_lcid(99,false,false,f.tables(),h);},CultureSelectionFailure::MissingLcid);
    auto alias=create_culture_by_lcid(65538,false,true,f.tables(),h);
    check(alias->identity.lcid==2 && alias->read_only && !alias->use_user_override);
    auto inv=create_culture_by_lcid(127,true,true,f.tables(),h);
    check(inv!=h.invariant && inv->identity.lcid==127 && inv->identity.constructed && inv->name.empty() && !inv->number_index && inv->read_only && inv->use_user_override);
    f.neutral[0].lcid=99;
    fails([&]{create_specific_culture(u"aa",f.tables(),h);},CultureSelectionFailure::MissingLcid);
    Current current(f,h,u"aa");CurrentCultureState state;
    check(construct_current_culture(state,current)==h.invariant && state.cached==h.invariant);
    check(!h.invariant->use_user_override);
    f.neutral[0].lcid=2;state.cached.reset();
    auto selected=construct_current_culture(state,current);
    check(selected->number_index==9 && selected->read_only && selected->use_user_override);
    // A nonzero pointer to an empty territory is specific, not neutral.
    word(f.records,0x14,f.add(""));
    check(create_specific_culture(u"aa",f.tables(),h)->number_index==8);
    // Invalid extracted bytes must not be converted into a managed fallback.
    word(f.records,8,65535);state.cached.reset();
    try{construct_current_culture(state,current);check(false);}catch(const std::invalid_argument&){}
    check(!state.cached);
}
