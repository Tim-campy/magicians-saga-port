#include "core/gameplay/culture_setters.hpp"
#include "core/gameplay/apply_culture.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"culture setter contract failure\n";std::exit(1);}}
template<class F>void fails(F action,CultureMutationFailure expected){
    try{action();check(false);}catch(const CultureMutationError& error){check(error.kind==expected);}
}
struct Population:CulturePopulationHost {
    int calls=0;std::int32_t observed=0;bool fail=false,readonly=false;
    std::shared_ptr<CultureNumberFormat> intermediate=std::make_shared<CultureNumberFormat>();
    bool populate(CultureInstance& value,std::int32_t lcid)override{
        ++calls;observed=lcid;value.name=u"populated";value.read_only=readonly;value.number_format=intermediate;
        if(fail)throw CultureManagedException("population failed");
        return false;
    }
};
int main(){
    CultureNumberFormat format;
    set_positive_sign(format,u"P");set_negative_sign(format,u"N");
    check(parse_apply_integer(u"P42",{format.positive,format.negative})==42);
    check(parse_apply_integer(u"N42",{format.positive,format.negative})==-42);
    set_positive_sign(format,u"");check(format.positive.empty());
    set_negative_sign(format,std::u16string{0,0xd800,u'-'});check(format.negative.size()==3);
    format.invariant=true;set_positive_sign(format,u"P");check(format.invariant);
    format.read_only=true;
    fails([&]{set_positive_sign(format,std::nullopt);},CultureMutationFailure::NullValue);
    fails([&]{set_negative_sign(format,std::nullopt);},CultureMutationFailure::NullValue);
    fails([&]{set_positive_sign(format,u"X");},CultureMutationFailure::ReadOnly);
    fails([&]{set_negative_sign(format,u"X");},CultureMutationFailure::ReadOnly);
    check(format.positive==u"P" && format.negative.size()==3);
    {CultureInstance c;Population h;c.identity.lcid=44;auto incoming=std::make_shared<CultureNumberFormat>();
        set_culture_number_format(c,incoming,h);
        check(h.calls==1 && h.observed==44 && c.identity.constructed && c.number_format==incoming);
        set_positive_sign(*incoming,u"Q");check(c.number_format->positive==u"Q");
        set_culture_number_format(c,incoming,h);check(h.calls==1);}
    {CultureInstance c;Population h;h.readonly=true;
        fails([&]{set_culture_number_format(c,nullptr,h);},CultureMutationFailure::ReadOnly);
        check(c.identity.constructed && c.number_format==h.intermediate && c.name==u"populated");}
    {CultureInstance c;Population h;
        fails([&]{set_culture_number_format(c,nullptr,h);},CultureMutationFailure::NullValue);
        check(c.identity.constructed && c.number_format==h.intermediate);}
    {CultureInstance c;Population h;h.fail=true;
        try{set_culture_number_format(c,nullptr,h);check(false);}catch(const CultureManagedException&){}
        check(!c.identity.constructed && c.number_format==h.intermediate && c.name==u"populated");}
    {CultureInstance c;Population h;c.identity.constructed=true;c.read_only=true;
        fails([&]{set_culture_number_format(c,nullptr,h);},CultureMutationFailure::ReadOnly);check(h.calls==0);}
}
