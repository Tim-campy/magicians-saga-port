#include "hashtable_write.hpp"
#include "hashtable_rehash.hpp"
#include <limits>
namespace saga::core {
// String.GetHashCode 0x1ced638 -> GetLegacyNonRandomizedHashCode 0x1ced63c.
std::uint32_t hashtable_string_hash(std::u16string_view text) {
    std::uint32_t first=5381,second=5381;
    for(std::size_t i=0;i<text.size()&&text[i]!=0;){
        first=(first*33U)^static_cast<std::uint32_t>(text[i++]);
        if(i==text.size()||text[i]==0)break;
        second=(second*33U)^static_cast<std::uint32_t>(text[i++]);
    }
    return first+second*0x5d588b65U;
}
namespace {
std::int32_t signed_bits(std::uint32_t bits){
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::shared_ptr<std::vector<HashtableBucket>> buckets(HashtableState& state){
    if(!state.buckets)throw CultureManagedException("null Hashtable buckets");
    return state.buckets;
}
HashtableBucket& slot(std::vector<HashtableBucket>& array,std::uint32_t index){
    if(index>=array.size())throw CultureManagedException("Hashtable bucket outside array");
    return array[index];
}
std::uint32_t length(const std::vector<HashtableBucket>& array){
    if(array.size()<2||array.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::invalid_argument("Hashtable capacity outside supported range");
    return static_cast<std::uint32_t>(array.size());
}
void commit(HashtableState& state,std::uint32_t index,AsyncValue key,AsyncValue value,std::uint32_t hash,bool fresh){
    state.writer_in_progress=true;
    auto array=buckets(state);auto& target=slot(*array,index);
    if(fresh){target.key=key;target.tombstone=false;target.value=value;target.hash_collision|=hash;
        state.count=signed_bits(static_cast<std::uint32_t>(state.count)+1U);}
    else target.value=value;
    ++state.version;state.writer_in_progress=false;
}
class StringKeys : public HashtableKeyHost {
public:
    std::uint32_t hash(AsyncValue key)override{return hashtable_string_hash(*std::static_pointer_cast<std::u16string>(key));}
    bool equals(AsyncValue a,AsyncValue b)override{return *std::static_pointer_cast<std::u16string>(a)==*std::static_pointer_cast<std::u16string>(b);}
    void duplicate(AsyncValue,AsyncValue)override{} // String formatting has no key callbacks.
};
}
// Hashtable.Insert 0x1e3c678, base KeyEquals 0x1e3d7b8.
void insert_hashtable(HashtableState& state,AsyncValue key,AsyncValue value,bool add,
                      const std::vector<std::int32_t>& primes,HashtableKeyHost& host){
    if(!key)throw CultureManagedException("null Hashtable key");
    if(state.count>=state.load_size)expand_hashtable(state,primes);
    else if(state.count>=101&&signed_bits(state.occupancy)>state.load_size)
        rehash_hashtable(state,static_cast<std::int32_t>(length(*buckets(state))));
    const auto hash_length=length(*buckets(state));
    const auto raw_hash=host.hash(key);
    const auto hash=hashtable_hash(raw_hash,hash_length);
    auto array=buckets(state);auto size=length(*array);auto index=hash.seed%size;
    std::int32_t free=-1;std::uint32_t tries=0;
    for(;;){
        const auto current=slot(*array,index);
        if(free<0&&current.tombstone&&(current.hash_collision&0x80000000U))free=static_cast<std::int32_t>(index);
        if((!current.key&&!current.tombstone)||(current.tombstone&&!(current.hash_collision&0x80000000U))){
            commit(state,free<0?index:static_cast<std::uint32_t>(free),key,value,hash.seed,true);return;
        }
        if((current.hash_collision&0x7fffffffU)==hash.seed&&!current.tombstone&&
           (current.key==key||host.equals(current.key,key))){
            if(add){auto live=buckets(state);host.duplicate(slot(*live,index).key,key);throw CultureManagedException("duplicate Hashtable key");}
            commit(state,index,key,value,hash.seed,false);return;
        }
        array=buckets(state);
        if(free<0){auto& collision=slot(*array,index).hash_collision;
            if(!(collision&0x80000000U)){collision|=0x80000000U;++state.occupancy;}}
        size=length(*array);
        index=static_cast<std::uint32_t>((static_cast<std::uint64_t>(index)+hash.step)%size);
        ++tries;
        if(tries>=size)break;
    }
    if(free<0)throw CultureManagedException("Hashtable insertion failed");
    commit(state,static_cast<std::uint32_t>(free),key,value,hash.seed,true);
}
StringCallDataStore::StringCallDataStore(std::shared_ptr<HashtableState> state,std::shared_ptr<const std::vector<std::int32_t>> primes)
    :state_(std::move(state)),primes_(std::move(primes)){
    if(!state_||!primes_)throw std::invalid_argument("StringCallDataStore requires state and prime data");
}
std::int32_t StringCallDataStore::count(){return state_->count;}
std::shared_ptr<CallDataEnumerator> StringCallDataStore::enumerate(){return std::make_shared<HashtableEnumerator>(state_);}
void StringCallDataStore::set(AsyncValue key,AsyncValue value){StringKeys host;insert_hashtable(*state_,key,value,false,*primes_,host);}
}
