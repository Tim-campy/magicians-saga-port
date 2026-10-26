#include "assets/collation_tables.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace saga::assets;
void check(bool b){if(!b){std::cerr<<"collation table contract failure\n";std::exit(1);}}
void integer(std::string& s,std::uint32_t n){for(unsigned i=0;i<4;++i)s+=static_cast<char>((n>>(8*i))&255);}
template<class F>void fails(F f){try{f();check(false);}catch(const std::invalid_argument&){}}
std::string load(const char* path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("cannot read collation test input");return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
    std::string core(1,3),tail(1,3);
    for(unsigned i=0;i<5;++i){integer(core,i);core.append(i,static_cast<char>(i+128));}
    integer(tail,2);integer(tail,0xffffffffU);integer(tail,0x80000000U);integer(tail,0x7fffffffU);tail+=static_cast<char>(255);
    integer(tail,42);integer(tail,0);integer(tail,1);tail+='\0';tail+="AB";
    integer(tail,3);tail.append("\0\0\0\xd8\xff\xff",6);
    auto tables=CollationTables::read(core,tail);
    check(tables.table(CollationTable::Ignorable).empty());
    check(tables.table(CollationTable::Level3)==std::string(4,static_cast<char>(132)));
    check(tables.tailoring()[0].lcid==-1&&tables.tailoring()[0].index==(-2147483647-1));
    check(tables.tailoring()[0].count==2147483647&&tables.tailoring()[0].french_sort);
    check(tables.tailoring()[1].lcid==42&&!tables.tailoring()[1].french_sort);
    check(tables.tailoring_units()==std::u16string({0,0xd800,0xffff}));
    auto copied=tables;tables=CollationTables{};check(copied.table(CollationTable::Category).size()==1);
    for(std::size_t i=0;i<core.size();++i)fails([&]{CollationTables::read(core.substr(0,i),tail);});
    for(std::size_t i=0;i<tail.size();++i)fails([&]{CollationTables::read(core,tail.substr(0,i));});
    fails([&]{CollationTables::read(core+"x",tail);});fails([&]{CollationTables::read(core,tail+"x");});
    auto wrong=core;wrong[0]=2;fails([&]{CollationTables::read(wrong,tail);});
    wrong=tail;wrong[1]=static_cast<char>(255);wrong[4]=static_cast<char>(255);fails([&]{CollationTables::read(core,wrong);});
    if(argc>1){
        check(argc==3);auto real=CollationTables::read(load(argv[1]),load(argv[2]));
        check(real.tailoring().size()==33&&real.tailoring_units().size()==3142);
        const std::size_t sizes[]{16384,42160,42160,8704,9472};
        for(std::size_t i=0;i<5;++i)check(real.table(static_cast<CollationTable>(i)).size()==sizes[i]);
        std::cout<<"five core tables, 33 tailoring records, 3142 UTF-16 units\n";
    }
}
