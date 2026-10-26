#include "core/gameplay/hashtable_rehash.hpp"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <limits>
using namespace saga::core;
void check(bool v){if(!v){std::cerr<<"Hashtable rehash contract failure\n";std::exit(1);}}
int main(){
    auto hash=hashtable_hash(0xffffffffU,7);check(hash.seed==0x7fffffffU&&hash.step==4);
    hash=hashtable_hash(0x80000000U,7);check(hash.seed==0&&hash.step==1);
    auto table=std::make_shared<HashtableState>();
    auto a=std::make_shared<int>(1),b=std::make_shared<int>(2),c=std::make_shared<int>(3);
    table->buckets=std::make_shared<std::vector<HashtableBucket>>(5);
    (*table->buckets)[0]={a,b,false,0x80000000U};
    (*table->buckets)[1]={a,c,true,0x80000000U};
    (*table->buckets)[2]={b,nullptr,false,0};
    (*table->buckets)[4]={c,a,false,0};
    table->count=3;table->occupancy=99;table->load_factor=0.72F;table->version=0xffffffffU;
    auto old=table->buckets;HashtableEnumerator cursor(table);check(cursor.next()&&cursor.key()==c);
    rehash_hashtable(*table,7);
    check(table->buckets!=old&&table->count==3&&table->occupancy==2);
    check(table->version==0&&table->load_size==5&&!table->writer_in_progress);
    check((*table->buckets)[0].key==a&&(*table->buckets)[1].key==b&&(*table->buckets)[2].key==c);
    check((*table->buckets)[0].hash_collision==0x80000000U&&(*table->buckets)[1].hash_collision==0x80000000U);
    check((*table->buckets)[2].hash_collision==0&&!(*table->buckets)[1].value);
    check((*old)[1].tombstone&&old->size()==5&&cursor.key()==c);
    try{cursor.next();check(false);}catch(const CultureManagedException&){}
    HashtableEnumerator current(table);check(current.next()&&current.key()==c);
    check(current.next()&&current.key()==b);check(current.next()&&current.key()==a);check(!current.next());
    rehash_hashtable(*table,7);check(table->version==1&&table->occupancy==2);
    // Signed conversion follows ARM64 FCVTZS, including the explicit +infinity override.
    for(float factor:{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::quiet_NaN(),1.0e20F,-0.5F}){
        table->load_factor=factor;rehash_hashtable(*table,7);
        if(std::isnan(factor))check(table->load_size==0);
        else if(std::isinf(factor))check(table->load_size==std::numeric_limits<std::int32_t>::min());
        else if(factor>0)check(table->load_size==std::numeric_limits<std::int32_t>::max());
        else check(table->load_size==-3);
    }
    HashtableState missing;missing.occupancy=8;missing.version=4;
    try{rehash_hashtable(missing,7);check(false);}catch(const CultureManagedException&){}
    check(missing.occupancy==0&&missing.version==4&&!missing.writer_in_progress);
}
