#include "core/gameplay/hashtable_enumerator.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"Hashtable enumeration contract failure\n";std::exit(1);}}
template<class F> void fails(F f){try{f();check(false);}catch(const CultureManagedException&) {}}
int main(){
    auto table=std::make_shared<HashtableState>();
    auto a=std::make_shared<int>(1),b=std::make_shared<int>(2),v=std::make_shared<int>(3);
    table->buckets=std::make_shared<std::vector<HashtableBucket>>(
        std::initializer_list<HashtableBucket>{{a,v,false},{b,v,true},{nullptr,v,false},{b,nullptr,false}});
    HashtableEnumerator e(table);
    fails([&]{e.key();});fails([&]{e.value();});fails([&]{e.entry();});fails([&]{e.current();});
    check(e.next()&&e.key()==b&&!e.value());
    check(std::get<HashtableEntry>(e.current()).key==b);
    auto cloned=e.clone();check(cloned->key()==b);
    check(e.next()&&e.key()==a&&e.value()==v);
    check(cloned->key()==b&&cloned->next()&&cloned->key()==a);
    check(!e.next()&&!e.next());fails([&]{e.key();});
    e.reset();check(e.next()&&e.key()==b);
    ++table->version;
    check(e.key()==b&&!e.value()&&e.entry().key==b);
    fails([&]{e.next();});fails([&]{e.reset();});check(e.key()==b);
    fails([&]{cloned->next();});
    for(int mode:{1,2,0,3,-1}){
        HashtableEnumerator cursor(table,mode);check(cursor.next());
        const auto result=cursor.current();
        if(mode==1)check(std::get<AsyncValue>(result)==b);
        else if(mode==2)check(!std::get<AsyncValue>(result));
        else check(std::get<HashtableEntry>(result).key==b);
    }
    // Buckets are fetched from the table on each advance, not snapshotted.
    HashtableEnumerator live(table);check(live.next());
    table->buckets=std::make_shared<std::vector<HashtableBucket>>(3);
    (*table->buckets)[2]={a,v,false};check(live.next()&&live.key()==a);
    table->buckets=nullptr;fails([&]{live.next();});check(live.key()==a);
    fails([&]{live.reset();});fails([&]{live.key();});
    table->buckets=std::make_shared<std::vector<HashtableBucket>>();
    live.reset();check(!live.next());
    fails([&]{HashtableEnumerator missing(nullptr);});
    table->buckets=nullptr;fails([&]{HashtableEnumerator missing(table);});
    // Exhaustion holds cached references until Reset; getters still reject them.
    table->buckets=std::make_shared<std::vector<HashtableBucket>>(1);
    auto retained=std::make_shared<int>(5);std::weak_ptr<int> weak=retained;
    (*table->buckets)[0]={a,retained,false};HashtableEnumerator lifetime(table);
    check(lifetime.next());retained.reset();(*table->buckets)[0].value.reset();
    check(!lifetime.next()&&!weak.expired());lifetime.reset();check(weak.expired());
}
