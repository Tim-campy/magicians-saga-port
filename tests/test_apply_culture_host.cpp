#include "core/gameplay/apply_culture_host.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"apply culture host contract failure\n";std::exit(1);}}
struct Providers:ThreadCultureHost,CurrentCultureHost,CultureNumberHost,ApplyGiftContentHost {
    std::shared_ptr<CultureInstance> culture=std::make_shared<CultureInstance>();
    std::shared_ptr<CultureNumberFormat> format=std::make_shared<CultureNumberFormat>();
    InvariantCultureState invariant;
    int failure=0,reads=0,formats=0,catalogs=0;
    bool no_thread=false,fail_create=false,fail_content=false,null_format=false;
    Providers(){culture->inherited=true;format->positive=u"P";format->negative=u"N";}
    std::shared_ptr<CultureInstance> thread_culture()override{
        ++reads;
        if(failure==1)throw CultureManagedException("thread exception");
        if(failure==2)throw CultureManagedError("non-Exception object");
        if(failure==3)throw std::invalid_argument("bad host data");
        return no_thread?nullptr:culture;
    }
    std::shared_ptr<CultureInstance> default_thread_culture()override{return nullptr;}
    bool invariant_mode()override{return false;}
    std::optional<std::u16string> native_locale_name()override{return u"supplied";}
    std::shared_ptr<CultureInstance> create_specific(const std::u16string&)override{
        if(fail_create)throw CultureManagedError("caught by factory Object filter");return culture;
    }
    std::shared_ptr<CultureInstance> invariant_culture()override{return invariant.instance;}
    std::shared_ptr<CultureNumberFormat> construct_number_format(const CultureInstance&)override{check(false);return {};}
    std::shared_ptr<CultureNumberFormat> get_format(CultureInstance&)override{++formats;return null_format?nullptr:format;}
    bool starts_special_gift(const std::u16string&)override{
        if(fail_content)throw ApplyManagedError("prefix");return false;
    }
    std::shared_ptr<ApplyCatalog> catalog()override{++catalogs;return std::make_shared<ApplyCatalog>();}
};
struct Fixture {
    Providers providers;CurrentCultureState current;ApplyTaskState task{};
    CultureApplyGiftHost host{current,providers,providers,providers,providers};
};
int main(){
    {Fixture f;initialize_apply_gift(f.task,u"reward_P42",0,true,f.host);
        check(f.task.provision_value==42 && f.providers.reads==2 && f.providers.formats==1);
        initialize_apply_gift(f.task,u"N7\tdialogue",0,true,f.host);
        check(f.task.talk_character==-7 && f.task.talk_text==u"dialogue");}
    {Fixture f;f.providers.failure=1;initialize_apply_gift(f.task,u"reward_P42",0,true,f.host);
        check(f.task.provision_value==0 && f.providers.catalogs==1);
        f.task.talk_character=9;
        try{initialize_apply_gift(f.task,u"P42\tdialogue",0,true,f.host);check(false);}catch(const ApplyManagedError&){}
        check(f.task.talk_character==9 && f.providers.catalogs==1);}
    {Fixture f;f.providers.failure=2;
        try{initialize_apply_gift(f.task,u"reward_P42",0,true,f.host);check(false);}catch(const CultureManagedError&){}
        check(f.providers.catalogs==0);}
    {Fixture f;f.providers.failure=3;
        try{initialize_apply_gift(f.task,u"reward_P42",0,true,f.host);check(false);}catch(const std::invalid_argument&){}
        check(f.providers.catalogs==0);}
    {Fixture f;f.providers.null_format=true;initialize_apply_gift(f.task,u"reward_P42",0,true,f.host);
        check(f.task.provision_value==0 && f.providers.catalogs==1);}
    {Fixture f;f.providers.no_thread=true;f.providers.fail_create=true;
        initialize_apply_gift(f.task,u"reward_+42",0,true,f.host);
        check(f.task.provision_value==42 && f.current.cached==f.providers.invariant.instance);}
    {Fixture f;f.providers.failure=1;initialize_apply_gift(f.task,u"reward_on",0,true,f.host);
        check(f.task.provision_value==1 && f.providers.reads==0);
        initialize_apply_gift(f.task,u"reward_off",0,true,f.host);check(f.task.provision_value==0 && f.providers.reads==0);}
    {Fixture f;f.providers.fail_content=true;
        try{initialize_apply_gift(f.task,u"reward_P42",0,true,f.host);check(false);}catch(const ApplyManagedError&){}
        check(f.providers.reads==0 && f.providers.catalogs==0);}
}
