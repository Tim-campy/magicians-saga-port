#include "core/gameplay/apply_task.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"apply catalog contract failure\n";std::exit(1);}}
template<class F> void fails(F f){try{f();check(false);}catch(const std::exception&) {}}
std::shared_ptr<ApplyCatalogItem> item(const char16_t* type,const char16_t* text,std::int32_t value) {
    return std::make_shared<ApplyCatalogItem>(ApplyCatalogItem{u"synthetic",type,text,value});
}
struct Host:ApplyGiftHost {
    std::shared_ptr<ApplyCatalog> rows=std::make_shared<ApplyCatalog>();
    int sign_calls=0,catalog_calls=0;
    bool managed_failure=false,native_failure=false;
    ApplyNumberSigns current_number_signs() override {++sign_calls;if(managed_failure)throw ApplyManagedError("format provider");if(native_failure)throw std::runtime_error("host");return {u"+",u"-"};}
    bool starts_special_gift(const std::u16string& value) override {return value.front()==u'@';} // Synthetic prefix provider.
    std::shared_ptr<ApplyCatalog> catalog() override {++catalog_calls;return rows;}
};
int main(){
    const ApplyNumberSigns signs{u"+",u"-"};
    check(parse_apply_integer(u" \t-2147483648\r",signs)==(-2147483647-1));
    check(parse_apply_integer(u"+2147483647",signs)==2147483647);
    check(parse_apply_integer(u"000000000000000000000000000001",signs)==1);
    check(parse_apply_integer(std::u16string(u"12\0\0",4),signs)==12);
    check(parse_apply_integer(u"minus9",{u"plus",u"minus"})==-9);
    check(parse_apply_integer(u"same9",{u"same",u"same"})==9);
    for(const auto* text:{u"",u"+",u"- 1",u"1.0",u"\u00a01",u"\uff11",u"999999999999bad"}) {
        try{parse_apply_integer(text,signs);check(false);}catch(const ApplyNumberError& e){check(e.kind==ApplyNumberFailure::Format);}
    }
    for(const auto* text:{u"2147483648",u"-2147483649",u"999999999999999999999999"}) {
        try{parse_apply_integer(text,signs);check(false);}catch(const ApplyNumberError& e){check(e.kind==ApplyNumberFailure::Overflow);}
    }
    {Host h;ApplyGiftState s{};s.talk_character=7;s.talk_text=u"previous";initialize_apply_gift(s,u"a_on, named",3,false,h);check(s.provision_value==1 && s.name==u"named" && s.product_id==u"a_on" && s.type==3 && !s.notice && !s.locked);check(s.talk_character==7 && s.talk_text==u"previous" && h.sign_calls==0 && h.catalog_calls==0);}
    {Host h;ApplyGiftState s{};initialize_apply_gift(s,u" 4\t words \t ignored",0,true,h);check(s.talk_character==4 && s.talk_text==u"words" && h.catalog_calls==0);}
    {Host h;ApplyGiftState s{};s.talk_character=7;fails([&]{initialize_apply_gift(s,u"bad\twords",0,true,h);});check(s.talk_character==7 && h.catalog_calls==0);}
    {Host h;ApplyGiftState s{};h.rows->push_back(item(u"a",u"bad",0));initialize_apply_gift(s,u"a_bad",0,true,h);check(s.provision_value==0 && s.name==u"synthetic" && h.catalog_calls==1);}
    {Host h;ApplyGiftState s{};h.managed_failure=true;initialize_apply_gift(s,u"a_3",0,true,h);check(s.provision_value==0 && h.catalog_calls==1);h.native_failure=true;h.managed_failure=false;fails([&]{initialize_apply_gift(s,u"a_3",0,true,h);});check(h.catalog_calls==1);}
    {Host h;ApplyGiftState s{};initialize_apply_gift(s,u"@special",0,true,h);check(h.catalog_calls==0 && h.sign_calls==0 && s.product_id.empty());}
    {Host h;ApplyGiftState s{};initialize_apply_gift(s,u"a_2_extra,chosen,ignored",0,true,h);check(s.provision_value==2 && s.name==u"chosen" && s.provision_text==u"2");}
    {Host h;const auto task=make_notification_gift(std::make_shared<const std::u16string>(u"a_3"),h);auto s=std::static_pointer_cast<ApplyGiftState>(task->original);check(notification_task_enabled(*task) && s->provision_value==3 && s->type==0 && s->notice && s->talk_character==0 && !s->talk_text);check(!notification_task_enabled(*make_notification_gift(nullptr,h)));}
    auto rows=std::make_shared<ApplyCatalog>();
    check(!resolve_apply_name_item(rows,u"a",u"x",2));
    fails([&]{resolve_apply_name_item(nullptr,u"a",u"x",2);});
    auto fallback=item(u"a",u"other",1),numeric=item(u"a",u"other",2),exact=item(u"a",u"x",0);
    *rows={nullptr,fallback,numeric,exact};
    check(resolve_apply_name_item(rows,u"a",u"x",2)==exact);
    *rows={exact,numeric,fallback};check(resolve_apply_name_item(rows,u"a",u"x",2)==exact); // Complete scan, fixed category priority.
    rows->erase(rows->begin());check(resolve_apply_name_item(rows,u"a",u"x",2)==numeric);
    check(resolve_apply_name_item(rows,u"a",u"x",1)==fallback);
    auto exact2=item(u"a",u"x",9);*rows={exact,numeric,exact2,fallback};check(resolve_apply_name_item(rows,u"a",u"x",2)==exact2);
    auto numeric2=item(u"a",u"new",3);*rows={numeric,fallback,numeric2};check(resolve_apply_name_item(rows,u"a",u"x",2)==numeric2);
    auto fallback2=item(u"a",u"new",-1);*rows={fallback,numeric,fallback2};check(resolve_apply_name_item(rows,u"a",u"x",0)==fallback2);
    auto other=item(u"b",u"x",2);other->provision_text.reset();*rows={other};check(!resolve_apply_name_item(rows,u"a",u"x",2));
    other->provision_type=u"a";fails([&]{resolve_apply_name_item(rows,u"a",u"x",2);});
    other->provision_type.reset();*rows={exact,other};fails([&]{resolve_apply_name_item(rows,u"a",u"x",2);}); // A late fault is not hidden by a prior exact match.
    exact->name.reset();*rows={exact};check(resolve_apply_name_item(rows,u"a",u"x",2)==exact); // Name wrapping is the caller's responsibility.
}
