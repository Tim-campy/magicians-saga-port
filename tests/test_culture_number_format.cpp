#include "core/gameplay/apply_culture.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"culture number format contract failure\n";std::exit(1);}}
struct Formats:CultureNumberHost {
    int creates=0,gets=0;
    bool fail=false,return_null=false,custom=false;
    CultureInstance* mutate=nullptr;
    std::shared_ptr<CultureNumberFormat> replacement=std::make_shared<CultureNumberFormat>();
    std::string records=std::string(48,'\0'),pool=std::string("\0P\0N\0",5);
    Formats(){records[0x14]=3;records[0x16]=1;}
    std::shared_ptr<CultureNumberFormat> construct_number_format(const CultureInstance& c)override{
        ++creates;
        if(fail)throw CultureManagedError("construction");
        auto result=read_culture_number_format(c,false,records,pool);
        if(mutate){mutate->read_only=true;mutate->number_format=replacement;}
        return return_null?nullptr:result;
    }
    std::shared_ptr<CultureNumberFormat> get_format(CultureInstance& c)override{
        ++gets;
        if(custom)return return_null?nullptr:replacement;
        return culture_number_format(c,*this);
    }
};
struct Thread:ThreadCultureHost {
    std::shared_ptr<CultureInstance> culture=std::make_shared<CultureInstance>();
    std::shared_ptr<CultureInstance> thread_culture()override{return culture;}
    std::shared_ptr<CultureInstance> default_thread_culture()override{return nullptr;}
};
struct User:CurrentCultureHost {
    bool invariant_mode()override{check(false);return false;}
    std::optional<std::u16string> native_locale_name()override{check(false);return {};}
    std::shared_ptr<CultureInstance> create_specific(const std::u16string&)override{check(false);return {};}
    std::shared_ptr<CultureInstance> invariant_culture()override{check(false);return {};}
};
int main(){
    InvariantCultureState state;
    auto original=state.number_format;
    check(original->read_only && original->invariant && original->positive==u"+" && original->negative==u"-");
    check(state.instance->number_format==original && readonly_number_format(original)==original);
    auto mutable_copy=clone_number_format(original);
    check(mutable_copy!=original && !mutable_copy->read_only && mutable_copy->invariant);
    mutable_copy->positive=u"P";
    auto ro=readonly_number_format(mutable_copy);
    check(ro!=mutable_copy && ro->read_only && !mutable_copy->read_only && ro->positive==u"P");
    mutable_copy->positive=u"Q";check(ro->positive==u"P" && original->positive==u"+");
    auto writable=make_invariant_culture(false,false,original);
    auto shared=make_invariant_culture(true,true,original);
    check(writable->number_format!=original && !writable->number_format->read_only && shared->number_format==original);
    try{readonly_number_format(nullptr);check(false);}catch(const CultureManagedError&){}
    try{clone_number_format(nullptr);check(false);}catch(const CultureManagedError&){}
    CultureInstance c;c.name=u"synthetic";c.number_index=0;
    Formats f;auto parsed=culture_number_format(c,f);
    check(parsed==c.number_format && parsed->positive==u"P" && parsed->negative==u"N" && !parsed->invariant);
    check(parse_apply_integer(u"P42",{parsed->positive,parsed->negative})==42);
    check(parse_apply_integer(u"N42",{parsed->positive,parsed->negative})==-42);
    c.read_only=true;check(culture_number_format(c,f)==parsed && f.creates==1 && !parsed->read_only);
    c.number_format.reset();f.mutate=&c;
    auto after=culture_number_format(c,f);
    check(after==c.number_format && after!=f.replacement && after->read_only);
    f.mutate=nullptr;c.number_format.reset();f.fail=true;
    try{culture_number_format(c,f);check(false);}catch(const CultureManagedError&){}
    check(!c.number_format);f.fail=false;f.return_null=true;
    try{culture_number_format(c,f);check(false);}catch(const CultureManagedError&){}
    check(!c.number_format);f.return_null=false;
    auto invariant=read_culture_number_format(c,true,{},{});
    check(invariant->invariant && invariant->positive==u"+");
    c.name.clear();auto empty=read_culture_number_format(c,false,{},{});
    check(empty->invariant && empty->negative==u"-");
    c.name=u"synthetic";c.number_index.reset();
    try{read_culture_number_format(c,false,{},{});check(false);}catch(const std::invalid_argument&){}
    Thread t;User user;CurrentCultureState current;
    t.culture->name=u"synthetic";t.culture->number_index=0;
    auto first=current_number_format(current,t,user,f);
    check(first==t.culture->number_format && f.gets==1);
    check(current_number_format(current,t,user,f)==first && f.gets==1);
    t.culture->inherited=true;f.custom=true;
    check(current_number_format(current,t,user,f)==f.replacement && f.gets==2);
    check(t.culture->number_format==first);
    f.return_null=true;check(!current_number_format(current,t,user,f) && f.gets==3);
}
