#include "core/gameplay/hashtable_write.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
using namespace saga::core;
void check(bool v){if(!v){std::cerr<<"Hashtable write contract failure\n";std::exit(1);}}
template<class F>void fails(F f){try{f();check(false);}catch(const CultureManagedException&) {}}
struct Keys:HashtableKeyHost{
    std::function<void()> on_hash,on_equals;int hashes=0,comparisons=0,duplicates=0;std::uint32_t raw=0;
    std::uint32_t hash(AsyncValue)override{++hashes;if(on_hash)on_hash();return raw;}
    bool equals(AsyncValue a,AsyncValue b)override{++comparisons;if(on_equals)on_equals();return *std::static_pointer_cast<int>(a)==*std::static_pointer_cast<int>(b);}
    void duplicate(AsyncValue,AsyncValue)override{++duplicates;}
};
struct ContextHost:CallContextHost{
    std::shared_ptr<const std::vector<std::int32_t>> primes;
    std::shared_ptr<CallDataStore> make_store()override{return std::make_shared<StringCallDataStore>(make_hashtable(0,1.0F,*primes),primes);}
    std::optional<std::u16string> string_key(AsyncValue v)override{return v?std::optional<std::u16string>(*std::static_pointer_cast<std::u16string>(v)):std::nullopt;}
    AsyncValue clone_correlation(AsyncValue)override{throw CultureManagedException("unused clone");}
};
int main(){
    check(hashtable_string_hash(u"")==371857150U);
    check(hashtable_string_hash(u"a")==372029373U);
    check(hashtable_string_hash(std::u16string_view(u"ab\0tail",7))==hashtable_string_hash(u"ab"));
    auto primes=std::make_shared<const std::vector<std::int32_t>>(std::initializer_list<std::int32_t>{3,7,17,37});
    auto a=std::make_shared<int>(1),b=std::make_shared<int>(2),c=std::make_shared<int>(3);
    {Keys h;auto t=make_hashtable(0,1.0F,*primes);
     fails([&]{insert_hashtable(*t,nullptr,a,false,*primes,h);});check(h.hashes==0&&t->version==0);
     insert_hashtable(*t,a,b,false,*primes,h);insert_hashtable(*t,b,c,false,*primes,h);
     check(t->count==2&&t->occupancy==1&&t->version==2);
     HashtableEnumerator e(t);check(e.next());
     insert_hashtable(*t,a,b,false,*primes,h); // Expand even for same-value overwrite.
     check(t->buckets->size()==7&&t->count==2&&t->version==4);fails([&]{e.next();});
     auto equivalent=std::make_shared<int>(1);
     insert_hashtable(*t,equivalent,c,false,*primes,h);check(t->count==2&&(*t->buckets)[0].key==a&&(*t->buckets)[0].value==c);
     const auto version=t->version;fails([&]{insert_hashtable(*t,a,b,true,*primes,h);});check(h.duplicates==1&&t->version==version);
     // A removed slot cannot end the duplicate search.
     (*t->buckets)[0].tombstone=true;(*t->buckets)[0].hash_collision=0x80000000U;--t->count;
     insert_hashtable(*t,b,a,false,*primes,h);check((*t->buckets)[0].tombstone&&t->count==1);
     insert_hashtable(*t,c,nullptr,false,*primes,h);check(!(*t->buckets)[0].tombstone&&(*t->buckets)[0].key==c&&t->count==2);}
    {Keys h;auto t=make_hashtable(3,1.0F,*primes);insert_hashtable(*t,a,b,false,*primes,h);
     h.on_equals=[] {throw CultureManagedException("equality failed");};const auto version=t->version;
     fails([&]{insert_hashtable(*t,b,c,false,*primes,h);});check(t->version==version&&!t->writer_in_progress&&t->count==1);}
    {Keys h;auto t=make_hashtable(0,1.0F,*primes);
     h.raw=1;h.on_hash=[&]{t->buckets=std::make_shared<std::vector<HashtableBucket>>(7);(*t->buckets)[1]={a,b,false,1};t->count=1;};
     insert_hashtable(*t,b,c,false,*primes,h);check(t->count==2&&(*t->buckets)[3].key==b);}
    {Keys h;auto t=make_hashtable(3,1.0F,*primes);
     (*t->buckets)[0]={a,b,false,1};(*t->buckets)[1]={b,a,false,0};t->count=2;
     h.on_equals=[] {throw CultureManagedException("late equality failure");};
     fails([&]{insert_hashtable(*t,c,a,false,*primes,h);});
     check(t->version==0&&t->count==2&&t->occupancy==1&&(*t->buckets)[0].hash_collision==0x80000001U);}
    {ContextHost host;host.primes=primes;IllogicalCallContext source;source.data=host.make_store();
     auto first=std::make_shared<std::u16string>(u"ab");auto second=std::make_shared<std::u16string>(u"ab\0tail",7);
     source.data->set(first,a);source.data->set(second,b);source.data->set(std::make_shared<std::u16string>(u"third"),c);
     check(source.data->count()==3);auto copy=copy_illogical_call_context(source,host);check(copy->data!=source.data&&copy->data->count()==3);
     source.data->set(first,c);auto entries=copy->data->enumerate();int seen=0;
     while(entries->next()){auto key=std::static_pointer_cast<std::u16string>(entries->key());
         if(*key==*first){check(entries->value()==a);++seen;}
         if(*key==*second){check(entries->value()==b);++seen;}}
     check(seen==2);}
}
