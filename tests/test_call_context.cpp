#include "core/gameplay/call_context.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
using namespace saga::core;
void check(bool value) { if (!value) { std::cerr << "call context contract failure\n"; std::exit(1); } }
struct Entry { AsyncValue key, value; };
struct Enumerator : CallDataEnumerator {
    std::vector<Entry> items; std::size_t index=0; std::vector<int>* trace=nullptr;
    bool next() override { return index++ < items.size(); }
    AsyncValue key() override { trace->push_back(1); return items[index-1].key; }
    AsyncValue value() override { trace->push_back(2); return items[index-1].value; }
};
struct Store : CallDataStore {
    std::vector<Entry> items; std::vector<int>* trace=nullptr;
    std::function<void()> on_count,on_enumerate;
    std::int32_t count() override { if(on_count)on_count(); return static_cast<std::int32_t>(items.size()); }
    std::shared_ptr<CallDataEnumerator> enumerate() override {
        if(on_enumerate)on_enumerate();
        auto e=std::make_shared<Enumerator>();e->items=items;e->trace=trace;return e;
    }
    void set(AsyncValue key,AsyncValue value) override { trace->push_back(5);items.push_back({key,value}); }
};
struct Host : CallContextHost {
    std::vector<int> trace; int allocations=0,clones=0; bool fail=false;
    AsyncValue cloned=std::make_shared<int>(9);
    std::shared_ptr<CallDataStore> make_store() override {
        trace.push_back(0);++allocations;auto s=std::make_shared<Store>();s->trace=&trace;return s;
    }
    std::optional<std::u16string> string_key(AsyncValue v) override {
        trace.push_back(3);if(!v)return std::nullopt;return *std::static_pointer_cast<std::u16string>(v);
    }
    AsyncValue clone_correlation(AsyncValue) override {
        trace.push_back(4);++clones;if(fail)throw CultureManagedException("clone failed");return cloned;
    }
    std::shared_ptr<Store> source(std::u16string key,AsyncValue value) {
        auto s=std::make_shared<Store>();s->trace=&trace;
        s->items.push_back({std::make_shared<std::u16string>(key),value});return s;
    }
};
int main() {
    const std::u16string special=u"System.Diagnostics.Trace.CorrelationManagerSlot";
    auto value=std::make_shared<int>(7);
    { Host h;IllogicalCallContext source;source.host_context=value;
      auto copy=copy_illogical_call_context(source,h);check(copy->host_context==value&&!copy->data&&h.allocations==0);
      source.data=h.make_store();h.allocations=0;copy=copy_illogical_call_context(source,h);check(!copy->data&&h.allocations==0);
      source.data=h.source(special,value);h.trace.clear();copy=copy_illogical_call_context(source,h);
      auto target=std::static_pointer_cast<Store>(copy->data);
      check(target!=source.data&&target->items[0].value==value&&h.clones==0);
      check((h.trace==std::vector<int>{0,1,2,3,5})); }
    { Host h;LogicalCallContext source;source.remoting=std::make_shared<CallRemotingData>();source.remoting->logical_call_id=value;
      source.security=std::make_shared<CallSecurityData>();source.security->principal=value;
      source.host_context=value;source.send_headers=value;source.receive_headers=value;
      auto copy=copy_logical_call_context(source,h);
      check(copy->remoting!=source.remoting&&copy->remoting->logical_call_id==value);
      check(copy->security!=source.security&&copy->security->principal==value);
      check(copy->host_context==value&&!copy->send_headers&&!copy->receive_headers);
      source.data=h.source(special,value);source.correlation_manager=true;h.trace.clear();
      copy=copy_logical_call_context(source,h);
      check(std::static_pointer_cast<Store>(copy->data)->items[0].value==h.cloned);
      check((h.trace==std::vector<int>{1,3,0,2,4,5}));
      h.cloned=nullptr;copy=copy_logical_call_context(source,h);
      check(!std::static_pointer_cast<Store>(copy->data)->items[0].value);
      h.fail=true;try{copy_logical_call_context(source,h);check(false);}catch(const CultureManagedException&){}
      check(std::static_pointer_cast<Store>(source.data)->items[0].value==value); }
    { Host h;LogicalCallContext source;auto data=h.source(special,value);source.data=data;
      data->on_enumerate=[&]{source.correlation_manager=true;};
      auto copy=copy_logical_call_context(source,h);
      check(!copy->correlation_manager&&h.clones==1); // Late source flag controls the branch.
      auto replacement=h.source(u"ordinary",value);data->on_count=[&]{source.data=replacement;};
      copy=copy_logical_call_context(source,h);
      check(*std::static_pointer_cast<std::u16string>(std::static_pointer_cast<Store>(copy->data)->items[0].key)==u"ordinary"); }
    { Host h;LogicalCallContext source;source.correlation_manager=true;source.data=h.source(special,nullptr);
      try{copy_logical_call_context(source,h);check(false);}catch(const CultureManagedException&){}
      check(h.allocations==1&&h.clones==0);
      source.data=h.source(u"ordinary",nullptr);auto copy=copy_logical_call_context(source,h);
      check(!std::static_pointer_cast<Store>(copy->data)->items[0].value); }
    { Host h;TypedCallContextCopyHost copies(h);ThreadExecutionState thread;
      thread.context=std::make_shared<AsyncExecutionState>();auto previous=thread.context;
      auto logical=std::make_shared<LogicalCallContext>();logical->correlation_manager=true;
      logical->data=h.source(special,value);previous->logical_call=logical;
      previous->illogical_call=std::make_shared<IllogicalCallContext>();
      thread.belongs_to_outer_scope=true;OwnedAsyncExecutionHost execution(thread,copies);
      auto current=execution.mutable_execution_context();
      check(current!=previous&&current->logical_call!=previous->logical_call);
      check(current->illogical_call!=previous->illogical_call&&h.clones==1);
      auto cloned=std::static_pointer_cast<LogicalCallContext>(current->logical_call);
      check(std::static_pointer_cast<Store>(cloned->data)->items[0].value==h.cloned); }

}
