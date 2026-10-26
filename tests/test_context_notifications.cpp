#include "core/gameplay/context_notifications.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
using namespace saga::core;
void check(bool v){if(!v){std::cerr<<"context notification contract failure\n";std::exit(1);}}
struct Failure:ContextNotificationFailureHost{int calls=0;std::exception_ptr last;void fail_fast(std::exception_ptr e)override{++calls;last=e;}};
struct Key:AsyncLocalKey{
    int id;std::vector<int>& order;std::function<void()> effect;AsyncValue before,after;
    Key(int i,std::vector<int>& o):id(i),order(o){}
    void changed(AsyncValue a,AsyncValue b,bool context_changed)override{
        check(context_changed);before=a;after=b;order.push_back(id);if(effect)effect();
    }
};
auto state(){auto s=std::make_shared<AsyncExecutionState>();s->values=std::make_shared<AsyncLocalValues>();s->notifications=std::make_shared<AsyncLocalNotifications>();return s;}
template<class F>void managed_failure(F f){try{f();check(false);}catch(const CultureManagedException&) {}}
int main(){
    std::vector<int> order;auto a=std::make_shared<Key>(1,order),b=std::make_shared<Key>(2,order),c=std::make_shared<Key>(3,order);
    auto x=std::make_shared<int>(1),y=std::make_shared<int>(2);Failure failure;
    auto before=state(),after=state();before->notifications->keys={a,b};after->notifications->keys={b,c,a};
    *before->values={{a,x},{b,x}};*after->values={{a,y},{b,x},{c,y}};
    notify_async_context_changed(before,after,failure);check((order==std::vector<int>{1,3})&&failure.calls==0);
    check(a->before==x&&a->after==y&&!c->before&&c->after==y);
    order.clear();before->notifications->keys.clear();*before->values={{a,nullptr}};after->notifications->keys={a};
    notify_async_context_changed(before,after,failure);check(order.empty()); // Existing silent/null key skipped.
    before->notifications=after->notifications;*before->values={{a,x}};
    notify_async_context_changed(before,after,failure);check((order==std::vector<int>{1}));
    order.clear();notify_async_context_changed(after,nullptr,failure);check((order==std::vector<int>{1})&&!a->after);
    order.clear();notify_async_context_changed(nullptr,after,failure);check((order==std::vector<int>{1})&&!a->before);
    // Old-list pointer stays captured; new list and dictionary values are read after callbacks.
    order.clear();before=state();after=state();before->notifications->keys={a,b};*before->values={{a,x},{b,x}};
    *after->values={{a,y},{b,x},{c,y}};a->effect=[&]{after->notifications=std::make_shared<AsyncLocalNotifications>();after->notifications->keys={c};(*after->values)[1].value=y;before->notifications=nullptr;};
    notify_async_context_changed(before,after,failure);check((order==std::vector<int>{1,2,3}));a->effect={};
    // List version failure in phase one propagates and retains prior callback effects.
    before=state();after=state();before->notifications->keys={a};*before->values={{a,x}};
    a->effect=[&]{++before->notifications->version;};managed_failure([&]{notify_async_context_changed(before,after,failure);});check(failure.calls==0);a->effect={};
    // Same failure in phase two is routed to FailFast, with the original exception.
    after->notifications->keys={a};*after->values={{a,y}};a->effect=[&]{++after->notifications->version;};
    notify_async_context_changed(nullptr,after,failure);check(failure.calls==1&&failure.last);a->effect={};
    managed_failure([&]{std::rethrow_exception(failure.last);});
    a->effect=[] {throw CultureManagedError("managed object outside System.Exception");};
    try{notify_async_context_changed(nullptr,after,failure);check(false);}catch(const CultureManagedError&){}
    check(failure.calls==1);a->effect=[] {throw std::logic_error("host bug");};
    try{notify_async_context_changed(nullptr,after,failure);check(false);}catch(const std::logic_error&){}
    check(failure.calls==1);
}
