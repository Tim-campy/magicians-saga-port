#include "assets/culture_signs.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace saga::assets;
void check(bool b){if(!b){std::cerr<<"culture sign contract failure\n";std::exit(1);}}
template<class F>void fails(F f){try{f();check(false);}catch(const std::invalid_argument&){}}
int main(){
    {
        const std::string names_pool("aa\0bb\0cc\0",9);
        std::string names(12,'\0'),cultures(168,'\0');
        names[4]=3;names[8]=6;names[2]=2;names[6]=0;names[10]=1;
        cultures[2*56+0x20]=7;cultures[0x20]=8;cultures[56+0x20]=9;
        check(find_culture_number_index("aa",names,cultures,names_pool)==7);
        check(find_culture_number_index("bb",names,cultures,names_pool)==8);
        check(find_culture_number_index("cc",names,cultures,names_pool)==9);
        check(find_culture_number_index(std::string("bb\0junk",7),names,cultures,names_pool)==8);
        for(const auto* key:{"","AA","a","ab","z"})check(!find_culture_number_index(key,names,cultures,names_pool));
        fails([&]{find_culture_number_index("bb",names.substr(1),cultures,names_pool);});
        fails([&]{find_culture_number_index("bb",names,cultures.substr(1),names_pool);});
        fails([&]{find_culture_number_index("bb",names,cultures,{});});
        fails([&]{find_culture_number_index("bb",names,cultures,"aaaa");});
        names[7]=static_cast<char>(0x80);fails([&]{find_culture_number_index("bb",names,cultures,names_pool);});
    }
    std::string records(96,'\0'),pool(300,'x');
    pool[0]='\0';pool.replace(260,5,std::string("\xe2\x80\x8e+\0",5));pool.replace(270,2,std::string("-\0",2));
    const auto put=[&](std::size_t at,unsigned value){records[at]=static_cast<char>(value&255);records[at+1]=static_cast<char>(value>>8);};
    put(48+0x14,270);put(48+0x16,260);put(48+0x10,299);
    const auto signs=read_culture_signs(records,pool,1);
    check(signs.negative=="-" && signs.positive==std::string("\xe2\x80\x8e+",4));
    check(signs.positive.data()==pool.data()+260);
    check(read_culture_signs(records,pool,0).positive.empty());
    fails([&]{read_culture_signs(records,pool,2);});
    fails([&]{read_culture_signs(records.substr(1),pool,0);});
    fails([&]{read_culture_signs({},pool,0);});
    fails([&]{read_culture_signs(records,{},0);});
    put(48+0x14,300);fails([&]{read_culture_signs(records,pool,1);});
    put(48+0x14,299);fails([&]{read_culture_signs(records,pool,1);});
    put(48+0x14,270);put(48+0x16,65535);fails([&]{read_culture_signs(records,pool,1);});
}
