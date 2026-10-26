#include "thread_culture_set.hpp"
#include "culture_setters.hpp"
namespace saga::core {
// Thread.InitializeCultureValues 0x1ecd198: first store survives a later UI failure.
void initialize_thread_cultures(ThreadCultureWriteHost& thread,CurrentCultureState& state,CurrentCultureHost& current){
    thread.write_thread_culture(construct_current_culture(state,current));
    // ConstructCurrentUICulture 0x1e24dd0 delegates to ConstructCurrentCulture.
    thread.write_thread_ui_culture(construct_current_culture(state,current));
}
// Thread.set_CurrentCulture 0x1ecd314. No local exception handler or read-only conversion.
void set_thread_culture(std::shared_ptr<CultureInstance> value,ThreadCultureWriteHost& thread,
    CurrentCultureState& state,CurrentCultureHost& current){
    if(!value)throw CultureMutationError(CultureMutationFailure::NullValue);
    if(!thread.thread_culture() && !thread.thread_ui_culture())initialize_thread_cultures(thread,state,current);
    if(thread.disable_async_culture_flow()){
        thread.write_thread_culture(std::move(value));return;
    }
    auto slot=thread.async_culture_slot();
    if(!slot){
        auto candidate=thread.make_async_culture_slot();
        thread.publish_async_culture_slot(std::move(candidate));
        slot=thread.async_culture_slot();
        if(!slot)throw CultureManagedException("null published AsyncLocal culture slot");
    }
    slot->set(std::move(value));
}
// Thread.AsyncLocalSetCurrentCulture 0x1ecc804 ignores previous/context-changed fields.
void async_local_set_current_culture(std::shared_ptr<CultureInstance> value,ThreadCultureWriteHost& thread){
    thread.write_thread_culture(std::move(value));
}
}
