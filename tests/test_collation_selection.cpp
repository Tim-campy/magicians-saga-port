#include "core/gameplay/collation_selection.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Host:CollationHost {
    std::optional<std::u16string> environment;bool ready=true,fail=false;
    int env_calls=0,ready_calls=0,managed_calls=0,compare_calls=0;
    std::u16string source,prefix;std::int32_t options=0,result=0;
    std::optional<std::u16string> disable_managed_environment()override{++env_calls;return environment;}
    bool managed_tables_ready()override{++ready_calls;if(fail)throw std::runtime_error("resource failure");return ready;}
    bool managed_prefix(std::u16string_view s,std::u16string_view p,std::int32_t o)override{
        ++managed_calls;source=s;prefix=p;options=o;return true;
    }
    std::int32_t compare_slices(std::u16string_view s,std::u16string_view p,std::int32_t o)override{
        ++compare_calls;source=s;prefix=p;options=o;return result;
    }
};
int main(){
    {Host h;CollationSelection s;h.environment=u"yes";
        check(!use_managed_collation(s,h)&&h.ready_calls==0);
        h.environment.reset();check(!use_managed_collation(s,h)&&h.env_calls==1);}
    for(auto text:{u"YES",u"Yes",u"",u"yes "}){
        Host h;CollationSelection s;h.environment=text;check(use_managed_collation(s,h)&&h.ready_calls==1);
    }
    {Host h;CollationSelection s;h.fail=true;
        try{use_managed_collation(s,h);check(false);}catch(const std::runtime_error&){}
        check(!s.initialized);h.fail=false;check(use_managed_collation(s,h)&&h.env_calls==2);
        check(cultural_prefix(s,h,u"a",u"long",7)&&h.managed_calls==1&&h.compare_calls==0);
        check(h.source==u"a"&&h.prefix==u"long"&&h.options==7);}
    {Host h;CollationSelection s;h.ready=false;
        check(!cultural_prefix(s,h,u"a",u"long",0)&&h.compare_calls==0);
        h.ready=true;check(cultural_prefix(s,h,u"abcd",u"xy",3));
        check(h.source==u"ab"&&h.prefix==u"xy"&&h.options==3&&h.ready_calls==1);
        h.result=-1;check(!cultural_prefix(s,h,u"abcd",u"xy",0));
        h.result=1;check(!cultural_prefix(s,h,u"abcd",u"xy",0));}
}
