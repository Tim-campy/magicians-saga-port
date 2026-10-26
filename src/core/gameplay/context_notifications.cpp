#include "context_notifications.hpp"
namespace saga::core {
namespace {
struct Value { bool exists=false; AsyncValue value; };
Value lookup(const std::shared_ptr<AsyncExecutionState>& context,const std::shared_ptr<AsyncLocalKey>& key){
    if(!context||!context->values)return {};
    if(!key)throw CultureManagedException("null async local key");
    for(const auto& entry:*context->values)if(entry.key==key)return {true,entry.value};
    return {};
}
template<class F>void visit(const std::shared_ptr<AsyncLocalNotifications>& list,F callback){
    const auto version=list->version;
    std::size_t index=0;
    for(;;){
        if(version!=list->version)throw CultureManagedException("async notification list changed");
        if(index>=list->keys.size())return;
        auto key=list->keys[index++];callback(key);
    }
}
void notify(const std::shared_ptr<AsyncLocalKey>& key,AsyncValue before,AsyncValue after){
    if(before==after)return;
    if(!key)throw CultureManagedException("null async notification key");
    key->changed(before,after,true);
}
}
// ExecutionContext.OnAsyncLocalContextChanged 0x1ec9fb8.
void notify_async_context_changed(std::shared_ptr<AsyncExecutionState> previous,
                                  std::shared_ptr<AsyncExecutionState> current,
                                  ContextNotificationFailureHost& failure){
    const auto old_list=previous?previous->notifications:nullptr;
    if(old_list)visit(old_list,[&](const auto& key){
        const auto before=lookup(previous,key);const auto after=lookup(current,key);
        notify(key,before.value,after.value);
    }); // This phase propagates failures; Dispose is a no-op in the shipped specialization.
    const auto new_list=current?current->notifications:nullptr;
    if(!new_list||new_list==old_list)return;
    try{
        visit(new_list,[&](const auto& key){
            const auto before=lookup(previous,key);
            if(before.exists)return; // A stored null also suppresses this second notification.
            const auto after=lookup(current,key);notify(key,before.value,after.value);
        });
    }catch(const CultureManagedException&){failure.fail_fast(std::current_exception());}
}
}
