#include "core/gameplay/hashtable_capacity.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
using namespace saga::core;
void check(bool v){if(!v){std::cerr<<"Hashtable capacity contract failure\n";std::exit(1);}}
template<class F>void fails(F f){try{f();check(false);}catch(const CultureManagedException&) {}}
int main(int argc,char** argv){
    const std::vector<std::int32_t> primes{3,7,11,23}; // Synthetic policy fixture.
    check(!hashtable_is_prime(0)&&hashtable_is_prime(1)&&hashtable_is_prime(2));
    check(hashtable_is_prime(-1)&&!hashtable_is_prime(-2)&&!hashtable_is_prime(9));
    check(hashtable_get_prime(4,primes)==7&&hashtable_get_prime(7,primes)==7);
    check(hashtable_get_prime(4,{11,7})==11); // Stored order, not minimum/sort.
    check(hashtable_get_prime(607,{})==613); // Prime 607 fails the secondary-step filter.
    check(hashtable_get_prime(0x7ffffffe,{})==0x7ffffffe);
    check(hashtable_get_prime(0x7fffffff,{})==0x7fffffff);
    fails([&]{hashtable_get_prime(-1,primes);});
    check(hashtable_expand_prime(3,primes)==7);
    check(hashtable_expand_prime(0x40000000,primes)==0x7feffffd);
    check(hashtable_expand_prime(0x7feffffc,primes)==0x7feffffd);
    fails([&]{hashtable_expand_prime(0x7feffffd,primes);});
    auto table=make_hashtable(0,1.0F,primes);
    check(table->buckets->size()==3&&table->load_factor==0.72F&&table->load_size==2);
    check(table->count==0&&table->version==0&&table->occupancy==0&&!table->writer_in_progress);
    auto key=std::make_shared<int>(1);(*table->buckets)[1]={key,nullptr,false,1};table->count=1;
    HashtableEnumerator cursor(table);check(cursor.next());
    expand_hashtable(*table,primes);
    check(table->buckets->size()==7&&table->count==1&&table->version==1&&table->load_size==5);
    check((*table->buckets)[1].key==key);fails([&]{cursor.next();});
    table=make_hashtable(3,1.0F,primes);check(table->buckets->size()==7&&table->load_size==5);
    table=make_hashtable(2,1.0F,primes);check(table->buckets->size()==3&&table->load_size==2);
    table=make_hashtable(0,0.1F,primes);check(table->load_size==0);
    fails([&]{make_hashtable(-1,1.0F,primes);});
    for(float factor:{0.0F,1.1F,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
        fails([&]{make_hashtable(0,factor,primes);});
    fails([&]{make_hashtable(std::numeric_limits<std::int32_t>::max(),1.0F,primes);});
    if(argc==2){
        // Optional local-only 72-entry initializer; no original table is embedded.
        std::ifstream file(argv[1],std::ios::binary);check(bool(file));
        std::vector<std::int32_t> original;
        for(int i=0;i<72;++i){unsigned char b[4];file.read(reinterpret_cast<char*>(b),4);check(bool(file));
            const auto n=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8U)|(std::uint32_t(b[2])<<16U)|(std::uint32_t(b[3])<<24U);
            check(n<=0x7fffffffU);original.push_back(static_cast<std::int32_t>(n));}
        check(file.peek()==std::char_traits<char>::eof());
        for(std::size_t i=0;i<original.size();++i){
            check(hashtable_is_prime(original[i]));check(hashtable_get_prime(original[i],original)==original[i]);
            if(i)check(original[i]>original[i-1]&&hashtable_get_prime(original[i-1]+1,original)==original[i]);}
        check(make_hashtable(3,1.0F,original)->buckets->size()==7);
        std::cout<<"72 local prime entries checked\n";
    }
}
