#include "platform/desktop/culture_data.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::desktop;
void check(bool value){if(!value){std::cerr<<"culture data contract failure\n";std::exit(1);}}
int main(int argc,char** argv){
    const auto parsed=parse_neutral_cultures(R"({"entries":[{"lcid":2147483647,"name":"a\u00e9\ud801\udc00"},{"name":"b","lcid":-2147483648}],"schema":1})");
    check(parsed.size()==2 && parsed[0].name==u"a\u00e9\U00010400" && parsed[1].lcid==(-2147483647-1));
    check(parse_neutral_cultures(R"({"schema":1,"entries":[]})").empty());
    for(auto bad:{std::string("{\"schema\":1,\"entries\":[{\"name\":\"\xff\",\"lcid\":1}]}"),
                  std::string("{\"schema\":1,\"entries\":[{\"name\":\"\n\",\"lcid\":1}]}")}){
        try{parse_neutral_cultures(bad);check(false);}catch(const std::invalid_argument&){}
    }
    for(auto bad:{R"({"schema":1,"schema":1,"entries":[]})",R"({"schema":2,"entries":[]})",
        R"({"schema":1,"entries":[{"name":"a","lcid":1},{"name":"\u0061","lcid":2}]})",
        R"({"schema":1,"entries":[{"name":"a","lcid":2147483648}]})",
        R"({"schema":1,"entries":[{"name":"a","lcid":-2147483649}]})",
        R"({"schema":1,"entries":[{"name":"a","lcid":01}]})",
        R"({"schema":1,"entries":[{"name":"a","lcid":1.0}]})",
        R"({"schema":1,"entries":[{"name":"a","lcid":1,}]})",
        R"({"schema":1,"entries":[{"name":"a","lcid":1,"unknown":0}]})",
        R"({"schema":1,"entries":[{"name":"\uXYZ0","lcid":1}]})",
        R"({"schema":1,"entries":[{"name":"\q","lcid":1}]})",
        R"({"schema":1,"entries":[]} trailing)",R"({"entries":[]})"}){
        try{parse_neutral_cultures(bad);check(false);}catch(const std::invalid_argument&){}
    }
    if(argc==1)return 0;
    try{
        check(argc==4);auto data=CultureData::load(argv[1]);CultureDataHost host(data);
        auto culture=host.specific(saga::core::decode_culture_utf8(argv[2]));
        auto format=host.get_format(*culture);
        const auto value=saga::core::parse_apply_integer(saga::core::decode_culture_utf8(argv[3]),{format->positive,format->negative});
        check(host.get_format(*culture)==format);
        auto invariant=host.specific(u"");check(invariant==host.invariant_culture());
        check(host.get_format(*invariant)==host.invariant_number_format());
        std::cout<<culture->identity.lcid<<' '<<value<<'\n';
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
