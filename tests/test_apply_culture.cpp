#include "core/gameplay/apply_culture.hpp"
#include "assets/culture_signs.hpp"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"apply culture contract failure\n";std::exit(1);}}
ApplyNumberSigns signs(std::string positive,std::string negative="-") {
    std::string record(48,'\0'),pool=negative+'\0'+positive+'\0';
    record[0x16]=static_cast<char>(negative.size()+1);
    return read_apply_number_signs(record,pool,0);
}
struct Host:ApplyGiftHost {
    ApplyNumberSigns value;
    ApplyNumberSigns current_number_signs() override {return value;}
    bool starts_special_gift(const std::u16string&) override {return false;}
    std::shared_ptr<ApplyCatalog> catalog() override {return std::make_shared<ApplyCatalog>();}
};
int main(int argc,char** argv){
    auto value=signs("\xe2\x80\x8e+");
    check(value.positive==u"\u200e+" && value.negative==u"-");
    check(parse_apply_integer(u"\u200e+2147483647",value)==2147483647);
    check(parse_apply_integer(u"-2147483648",value)==(-2147483647-1));
    try{parse_apply_integer(u"+1",value);check(false);}catch(const ApplyNumberError& e){check(e.kind==ApplyNumberFailure::Format);}
    check(signs("\xd8\x9c+").positive==u"\u061c+");
    check(signs("\xe2\x80\x8e+\xe2\x80\x8e").positive==u"\u200e+\u200e");
    auto astral=signs("\xf0\x9f\x98\x80");
    check(astral.positive==u"\U0001f600" && parse_apply_integer(u"\U0001f60012",astral)==12);
    check(signs("\xc2\x80\xdf\xbf\xe0\xa0\x80\xef\xbf\xbf\xf4\x8f\xbf\xbf").positive==u"\u0080\u07ff\u0800\uffff\U0010ffff");
    check(signs("").positive.empty());
    for(const auto* bad:{"\x80","\xc0\xaf","\xe0\x80\xaf","\xed\xa0\x80","\xf4\x90\x80\x80","\xf5\x80\x80\x80","\xe2\x80","\xc2x"}) {
        try{signs(bad);check(false);}catch(const std::invalid_argument&){}
    }
    Host host;host.value=value;ApplyTaskState state{};
    initialize_apply_gift(state,u"crystal_\u200e+42",0,true,host);
    check(state.provision_value==42 && state.provision_text==u"\u200e+42");
    initialize_apply_gift(state,u"crystal_+42",0,true,host);
    check(state.provision_value==0); // Original Init catches this format failure.
    // Optional local-only integration run; CI uses the synthetic cases above.
    if(argc!=1) {
        check(argc==3 || argc==5);
        const auto read=[](const char* path){
            std::ifstream file(path,std::ios::binary);check(file.good());
            return std::string(std::istreambuf_iterator<char>(file),{});
        };
        const auto records=read(argv[1]),pool=read(argv[2]);
        check(records.size()==339*48);
        for(std::size_t i=0;i<339;++i) {
            const auto decoded=read_apply_number_signs(records,pool,i);
            check(parse_apply_integer(decoded.positive+u"123",decoded)==123);
            check(parse_apply_integer(decoded.negative+u"123",decoded)==-123);
        }
        std::cout<<"339 extracted records decoded and parsed\n";
        if(argc==5) {
            const auto names=read(argv[3]),cultures=read(argv[4]);
            check(names.size()==339*4 && cultures.size()==339*56);
            const auto u16=[](const std::string& data,std::size_t at){return static_cast<std::size_t>(static_cast<unsigned char>(data.at(at)))+256U*static_cast<unsigned char>(data.at(at+1));};
            for(std::size_t i=0;i<339;++i) {
                const auto offset=u16(names,i*4),culture=u16(names,i*4+2);
                check(offset<pool.size() && culture<339);
                const auto end=pool.find('\0',offset);check(end!=pool.npos);
                const auto number=saga::assets::find_culture_number_index(pool.substr(offset,end-offset),names,cultures,pool);
                check(number && *number==u16(cultures,culture*56+0x20));
                const auto decoded=read_apply_number_signs(records,pool,*number);
                check(parse_apply_integer(decoded.positive+u"123",decoded)==123);
                check(parse_apply_integer(decoded.negative+u"123",decoded)==-123);
            }
            std::cout<<"339 culture names resolved to matching number records\n";
        }
    }
}
