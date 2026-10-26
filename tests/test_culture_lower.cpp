#include "core/gameplay/culture_lower.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"culture lowercase contract failure\n";std::exit(1);}}
int main(int argc,char** argv){
    constexpr std::array<std::pair<unsigned,unsigned>,9> ranges{{{0xc0,0x556},{0x10a0,0x10c5},
        {0x1e00,0x1ffc},{0x2160,0x216f},{0x24b6,0x24cf},{0x2c00,0x2c2e},
        {0x2c60,0x2ce2},{0xa640,0xa696},{0xa722,0xa78b}}};
    std::array<std::string,9> data;CultureLowerTables tables;
    std::array<char16_t,65536> expected;
    for(unsigned c=0;c<65536;++c)expected[c]=static_cast<char16_t>(c);
    for(unsigned c=0x41;c<=0x5a;++c)expected[c]=static_cast<char16_t>(c+32);
    for(unsigned c=0xff21;c<=0xff3a;++c)expected[c]=static_cast<char16_t>(c+32);
    expected[0x2132]=0x214e;expected[0x2183]=0x2184;
    for(std::size_t i=0;i<ranges.size();++i){
        for(unsigned c=ranges[i].first;c<=ranges[i].second;++c){
            const auto output=static_cast<unsigned>((c*17U+i*257U)&65535U);
            data[i].push_back(static_cast<char>(output&255));data[i].push_back(static_cast<char>(output>>8));
            expected[c]=static_cast<char16_t>(output);
        }
        tables[i]=data[i];
    }
    for(unsigned c=0;c<65536;++c)check(lower_invariant_unit(static_cast<char16_t>(c),tables)==expected[c]);
    std::u16string input{u'A',0,0xd801,0xdc00,0xd800,u'Z',0x2132,0x212a};
    auto result=lower_invariant_text(input,tables);check(result.size()==input.size());
    for(std::size_t i=0;i<input.size();++i)check(result[i]==expected[input[i]]);
    check(lower_invariant_text({},{}).empty());
    for(std::size_t i=0;i<ranges.size();++i){
        auto invalid=tables;invalid[i]=invalid[i].substr(1);
        try{lower_invariant_unit(static_cast<char16_t>(ranges[i].first),invalid);check(false);}catch(const std::invalid_argument&){}
    }
    // Optional local integration: nine extracted arrays, never part of the repository.
    if(argc!=1){
        check(argc==10);
        for(std::size_t i=0;i<9;++i){std::ifstream file(argv[i+1],std::ios::binary);check(bool(file));
            data[i]=std::string(std::istreambuf_iterator<char>(file),{});tables[i]=data[i];}
        std::size_t changed=0;
        for(unsigned c=0;c<65536;++c)if(lower_invariant_unit(static_cast<char16_t>(c),tables)!=c)++changed;
        check(lower_invariant_text(u"EN-US",tables)==u"en-us");
        check(lower_invariant_unit(0x00c0,tables)==0x00e0);
        check(lower_invariant_unit(0x10a0,tables)==0x2d00);
        check(lower_invariant_unit(0x212a,tables)==0x212a);
        std::cout<<"local BMP sweep: "<<changed<<" changed code units\n";
    }
}
