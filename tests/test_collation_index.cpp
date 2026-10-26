#include "core/gameplay/collation_index.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool b){if(!b)std::abort();}
void word(std::string& s,std::uint32_t n){for(unsigned i=0;i<4;++i)s+=static_cast<char>((n>>(8*i))&255);}
int main(){
    CodePointIndexer index({10,20},{13,22},-1,-9);
    check(index.total_count()==5&&index.ranges()[1].index_start==3);
    for(int p=0;p<30;++p){auto expected=p>=10&&p<13?p-10:p>=20&&p<22?p-17:-1;check(index.to_index(p)==expected);}
    check(index.to_index(-1)==-1&&index.to_index(2147483647)==-1);
    check(CodePointIndexer({}, {3},7,9).to_index(0)==7);
    check(CodePointIndexer({20,0},{22,5},-1,0).to_index(2)==-1);
    check(CodePointIndexer({0,2},{5,7},-1,0).to_index(3)==3);
    CodePointIndexer reversed({5,10},{2,12},-1,0);
    check(reversed.total_count()==-1&&reversed.to_index(10)==-3);
    CodePointIndexer wrap({(-2147483647-1),0},{2147483647,2},-1,0);
    check(wrap.ranges()[0].count==-1&&wrap.total_count()==1&&wrap.to_index(0)==(-2147483647-1));
    try{CodePointIndexer bad({0}, {},0,0);check(false);}catch(const std::out_of_range&){}
    std::string core(1,3),tail(1,3);
    for(int i=0;i<5;++i){word(core,5);core.append("\xff\x02\x03\x04\x05",5);}
    word(tail,2);
    for(int i=0;i<2;++i){word(tail,42);word(tail,static_cast<std::uint32_t>(i));word(tail,0);tail+='\0';}
    tail.append(2,'\0');word(tail,0);
    auto tables=saga::assets::CollationTables::read(core,tail);
    check(collation_weight(tables,saga::assets::CollationTable::Level1,index,10)==255);
    check(collation_weight(tables,saga::assets::CollationTable::Category,index,21)==5);
    try{collation_weight(tables,saga::assets::CollationTable::Level2,index,0);check(false);}catch(const std::out_of_range&){}
    check(find_collation_tailoring(tables,42)==&tables.tailoring()[0]);
    check(!find_collation_tailoring(tables,65578));
}
