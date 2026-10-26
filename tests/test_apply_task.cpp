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
int main(){
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
