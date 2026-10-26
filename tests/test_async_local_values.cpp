#include "core/gameplay/async_local_values.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"async local value contract failure\n";std::exit(1);}}
struct Host:AsyncExecutionHost {
    int calls=0;std::shared_ptr<AsyncExecutionState> state=std::make_shared<AsyncExecutionState>();
    std::shared_ptr<AsyncExecutionState> mutable_execution_context()override{++calls;return state;}
};
struct Key:AsyncLocalKey {
    Host& host;int calls=0;bool fail=false,reenter=false;AsyncValue old_value,new_value;
    std::shared_ptr<AsyncLocalKey> nested;AsyncValue nested_value;
    explicit Key(Host& h):host(h){}
    void changed(AsyncValue previous,AsyncValue current,bool context_changed)override{
        ++calls;check(!context_changed);old_value=previous;new_value=current;
        bool stored=false;for(auto& entry:*host.state->values)if(entry.key.get()==this)stored=entry.value==current;
        check(stored);
        if(reenter)set_async_local_value(nested,nested_value,true,host);
        if(fail)throw CultureManagedException("notification");
    }
};
int main(){
    Host h;auto key=std::make_shared<Key>(h);auto a=std::make_shared<int>(7),b=std::make_shared<int>(7);
    set_async_local_value(key,nullptr,true,h);
    check(h.calls==1 && !h.state->values && !h.state->notifications && key->calls==0);
    set_async_local_value(key,a,true,h);auto first=h.state->values;auto first_list=h.state->notifications;
    check(key->calls==1 && !key->old_value && key->new_value==a && first_list->keys.size()==1 && first_list->version==1);
    set_async_local_value(key,a,true,h);check(h.state->values==first && key->calls==1);
    set_async_local_value(key,b,true,h);
    check(h.state->values!=first && (*first)[0].value==a && key->old_value==a && key->new_value==b && key->calls==2);
    check(h.state->notifications==first_list);
    auto other=std::make_shared<Key>(h);set_async_local_value(other,a,false,h);
    check(other->calls==0 && h.state->notifications==first_list);
    // Registration depends on a previously absent dictionary key, not on list membership.
    set_async_local_value(other,b,true,h);check(other->calls==1 && h.state->notifications==first_list);
    auto third=std::make_shared<Key>(h);set_async_local_value(third,a,true,h);
    check(h.state->notifications!=first_list && first_list->keys.size()==1);
    check(h.state->notifications->keys.size()==2 && h.state->notifications->keys[1]==third && h.state->notifications->version==1);
    third->fail=true;
    try{set_async_local_value(third,b,true,h);check(false);}catch(const CultureManagedException&){}
    check(third->new_value==b && h.state->values->back().value==b);
    key->reenter=true;key->nested=other;key->nested_value=a;
    set_async_local_value(key,nullptr,true,h);check(key->new_value==nullptr && other->new_value==a);
    check(h.state->values->size()==3); // Null clears the value, not the dictionary entry.
    {Host empty;set_async_local_value(nullptr,nullptr,true,empty);check(!empty.state->values);
        try{set_async_local_value(nullptr,a,false,empty);check(false);}catch(const CultureManagedException&){}
        check(empty.state->values && empty.state->values->empty());
        auto before=empty.state->values;
        try{set_async_local_value(nullptr,nullptr,false,empty);check(false);}catch(const CultureManagedException&){}
        check(empty.state->values==before);}
    {Host missing;missing.state.reset();try{set_async_local_value(key,a,true,missing);check(false);}catch(const CultureManagedException&){}check(missing.calls==1);}
}
