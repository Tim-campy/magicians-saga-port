#include "core/ecs/filter_identity.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
int main(){
    std::function<void()> silent=[]{},fail=[]{throw std::runtime_error("diagnostic");};
    std::vector<std::int32_t> values{0,1,31,32,-1};
    check(component_types_hash(&values,silent)==4); // 1+2+2^31+1+2^31 wraps.
    check(component_types_hash(nullptr,fail)==0);
    std::vector<std::int32_t> a{0,1},b{1,0},empty;
    FilterIdentity x,y;x.lists.all=&a;y.lists.all=&b;
    check(x.hash(silent)==3&&y.hash(silent)==3&&!x.equals(&y,silent));
    y.lists.all=&a;check(x.equals(&y,silent)); // Setters do not reset the cached hash.
    check(!x.equals(nullptr,fail));
    FilterIdentity null,blank;blank.lists.all=&empty;check(null.hash(silent)==0&&blank.hash(silent)==0&&!null.equals(&blank,silent));
    a[0]=2;FilterIdentity fresh;fresh.lists.all=&a;
    check(x.hash(fail)==3&&fresh.hash(silent)==6&&!x.equals(&fresh,silent));
    std::vector<std::int32_t> one{0},long_list(33,0);
    FilterIdentity partial;partial.lists.all=&one;partial.lists.any=&long_list;
    bool failed=false;try{partial.hash(fail);}catch(const std::runtime_error&){failed=true;}
    check(failed&&partial.hash(fail)==1); // A failed OR hash leaves the AND cache published.
    int logs=0;std::function<void()> log=[&]{++logs;};
    FilterIdentity large;large.lists.all=&long_list;check(large.hash(log)==33&&logs==1);check(large.hash(log)==33&&logs==1);
    // Cached -1 remains the recomputation sentinel; all 32 bits set.
    std::vector<std::int32_t> all_bits;for(int i=0;i<32;++i)all_bits.push_back(i);
    FilterIdentity sentinel;sentinel.lists.all=&all_bits;check(sentinel.hash(silent)==-1);
    FilterIdentity reentrant;reentrant.lists.all=&all_bits;reentrant.lists.any=&long_list;
    std::function<void()> nested=[&]{reentrant.lists.all=&one;reentrant.lists.any=nullptr;check(reentrant.hash(silent)==1);};
    check(reentrant.hash(nested)==-34); // Captured AND=-1 survives the nested cache write.
    all_bits[0]=1;check(sentinel.hash(silent)==0);
}
