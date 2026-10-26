#include "async_local_values.hpp"
namespace saga::core {
void ContextCultureSlot::set(std::shared_ptr<CultureInstance> value){
    set_async_local_value(shared_from_this(),std::move(value),true,execution_);
}
void ContextCultureSlot::changed(AsyncValue,AsyncValue current,bool){
    async_local_set_current_culture(std::static_pointer_cast<CultureInstance>(current),thread_);
}
// ExecutionContext.SetLocalValue 0x1eca504: decoded identity lookup/update contract.
void set_async_local_value(std::shared_ptr<AsyncLocalKey> key,AsyncValue value,bool notify,AsyncExecutionHost& host){
    auto context=host.mutable_execution_context();
    if(!context)throw CultureManagedException("null mutable execution context");
    bool found=false;AsyncValue previous;
    if(context->values){
        if(!key)throw CultureManagedException("null AsyncLocal dictionary key");
        for(const auto& entry:*context->values)if(entry.key==key){found=true;previous=entry.value;break;}
    }
    if(previous==value)return;
    auto values=context->values?std::make_shared<AsyncLocalValues>(*context->values):std::make_shared<AsyncLocalValues>();
    context->values=values; // The original installs the copy before its indexer assignment.
    if(!key)throw CultureManagedException("null AsyncLocal dictionary key");
    if(found){for(auto& entry:*values)if(entry.key==key){entry.value=value;break;}}
    else values->push_back({key,value});
    if(!notify)return;
    if(!found){
        auto list=std::make_shared<AsyncLocalNotifications>();
        if(context->notifications)list->keys=context->notifications->keys;
        context->notifications=list; // List constructor resets its version before Add.
        ++list->version;
        list->keys.push_back(key);
    }
    key->changed(previous,value,false);
}
}
