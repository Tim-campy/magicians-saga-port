#include "core/gameplay/execution_context.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"execution context contract failure\n";std::exit(1);}}
struct Copies:ExecutionContextCopyHost {
    std::vector<int> calls;AsyncExecutionState* mutate=nullptr;bool fail=false,null_result=false;
    AsyncValue logical=std::make_shared<int>(10),illogical=std::make_shared<int>(20);
    AsyncValue clone_logical_call(AsyncValue value)override{
        check(bool(value));calls.push_back(1);
        if(mutate){mutate->synchronization=nullptr;mutate->flags=0xffffffffU;mutate->illogical_call=illogical;}
        return null_result?nullptr:logical;
    }
    AsyncValue clone_illogical_call(AsyncValue value)override{
        check(bool(value));calls.push_back(2);
        if(fail)throw CultureManagedException("illogical clone");
        if(mutate)mutate->values=std::make_shared<AsyncLocalValues>();
        return null_result?nullptr:illogical;
    }
};
struct Key:AsyncLocalKey {
    int calls=0;
    void changed(AsyncValue,AsyncValue,bool changed_context)override{check(!changed_context);++calls;}
};
int main(){
    {Copies h;ThreadExecutionState t;t.belongs_to_outer_scope=true;
        auto value=mutable_execution_context(t,h);check(value==t.context && !t.belongs_to_outer_scope && h.calls.empty());
        value->flags=7;check(mutable_execution_context(t,h)==value && value->flags==7);}
    {Copies h;AsyncExecutionState source;source.synchronization=std::make_shared<int>(1);source.synchronization_no_flow=std::make_shared<int>(2);
        source.logical_call=std::make_shared<int>(3);source.illogical_call=std::make_shared<int>(4);
        source.values=std::make_shared<AsyncLocalValues>();source.notifications=std::make_shared<AsyncLocalNotifications>();source.flags=7;
        auto copy=copy_mutable_execution_context(source,h);
        check((h.calls==std::vector<int>{1,2}) && copy->flags==2);
        check(copy->synchronization==source.synchronization && copy->synchronization_no_flow==source.synchronization_no_flow);
        check(copy->logical_call==h.logical && copy->illogical_call==h.illogical);
        check(copy->values==source.values && copy->notifications==source.notifications);
        auto original_sync=source.synchronization;h.mutate=&source;source.illogical_call=nullptr;source.flags=0;
        copy=copy_mutable_execution_context(source,h);
        check(copy->synchronization==original_sync && !source.synchronization && copy->flags==2 && copy->values==source.values);
        h.mutate=nullptr;h.null_result=true;copy=copy_mutable_execution_context(source,h);check(!copy->logical_call && !copy->illogical_call);}
    {Copies h;ThreadExecutionState t;t.context=std::make_shared<AsyncExecutionState>();auto original=t.context;
        original->logical_call=std::make_shared<int>(1);original->illogical_call=std::make_shared<int>(2);
        t.belongs_to_outer_scope=true;h.fail=true;
        try{mutable_execution_context(t,h);check(false);}catch(const CultureManagedException&){}
        check(t.context==original && t.belongs_to_outer_scope);
        h.fail=false;auto copy=mutable_execution_context(t,h);check(copy!=original && t.context==copy && !t.belongs_to_outer_scope);}
    {Copies h;ThreadExecutionState t;OwnedAsyncExecutionHost host(t,h);auto key=std::make_shared<Key>();
        auto a=std::make_shared<int>(1),b=std::make_shared<int>(2);
        set_async_local_value(key,a,true,host);auto captured=t.context;auto values=captured->values;
        t.belongs_to_outer_scope=true;set_async_local_value(key,b,true,host);
        check(t.context!=captured && t.context->values!=values && (*values)[0].value==a);
        check((*t.context->values)[0].value==b && t.context->notifications==captured->notifications && key->calls==2);
        // Even unchanged values obtain a mutable context before identity comparison.
        captured=t.context;t.belongs_to_outer_scope=true;set_async_local_value(key,b,true,host);
        check(t.context!=captured && t.context->values==captured->values && key->calls==2);}
}
