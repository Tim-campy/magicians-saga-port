#include "core/gameplay/collation_index.hpp"
#include <cstdlib>
#include <stdexcept>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace saga::core;
void check(bool b){if(!b)std::abort();}
void word(std::string& s,std::uint32_t n){for(unsigned i=0;i<4;++i)s+=static_cast<char>((n>>(8*i))&255);}
std::string load(const char* path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("missing collation fixture");return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
    std::string configs="SCI1";word(configs,7);
    for(int i=0;i<7;++i){word(configs,1);word(configs,0xffffffffU);word(configs,0xffffffffU);word(configs,10);word(configs,12);}
    auto decoded=read_collation_indexers(configs);check(decoded.size()==7&&decoded[6].to_index(11)==1);
    for(std::size_t n=0;n<configs.size();++n){try{read_collation_indexers(configs.substr(0,n));check(false);}catch(const std::invalid_argument&){}}
    try{read_collation_indexers(configs+"x");check(false);}catch(const std::invalid_argument&){}
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
    if(argc>1){
        check(argc==4);auto actual=read_collation_indexers(load(argv[1]));
        auto original=saga::assets::CollationTables::read(load(argv[2]),load(argv[3]));
        // Every UTF-16 unit maps inside the relevant weight table, including default slot 0.
        for(std::size_t i=1;i<5;++i){
            check(actual[i].total_count()==static_cast<std::int32_t>(original.table(static_cast<saga::assets::CollationTable>(i)).size()));
            for(std::int32_t point=0;point<=65535;++point)
                collation_weight(original,static_cast<saga::assets::CollationTable>(i),actual[i],point);
        }
        check(actual[0].total_count()==static_cast<std::int32_t>(original.table(saga::assets::CollationTable::Ignorable).size()));
        std::cout<<"seven indexers; all 65536 UTF-16 units bounded in each category/weight table\n";
    }
}
