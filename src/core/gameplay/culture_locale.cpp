#include "culture_locale.hpp"
#include <cstdint>

namespace saga::core {
namespace {
std::string_view c_string(std::string_view value) {
    return value.substr(0,value.find('\0'));
}
}
// Native locale supplier 0x1056464, including precedence of present empty values.
std::string select_native_locale(CultureLocaleHost& host) {
    auto platform=host.platform_locale();
    if(!c_string(platform).empty())return std::string(c_string(platform));
    auto candidate=host.environment("LC_ALL");
    if(!candidate)candidate=host.environment("LANG");
    if(!candidate)candidate=host.process_locale();
    if(!candidate)return {};
    auto value=c_string(*candidate);
    if(value=="C.UTF-8")value="en-us";
    if(value=="C" || value.find(' ')!=value.npos || value.find('/')!=value.npos)return {};
    return std::string(value);
}
// Native CultureInfo locale bridge 0x1036e10. Culture lookup/fallback is separate.
std::string normalize_culture_locale(std::string_view value) {
    value=c_string(value);
    value=value.substr(0,value.find('.'));
    value=value.substr(0,value.find('@'));
    std::string result(value);
    const auto underscore=result.find('_');
    if(underscore!=result.npos)result[underscore]='-';
    for(auto& c:result)if(c>='A' && c<='Z')c=static_cast<char>(c+('a'-'A'));
    return result;
}
// Native helpers 0xfe91a0/0xfe91a8/0xfe923c: malformed high-surrogate pairs
// consume both code units, rather than retrying the second one.
std::string encode_culture_lookup_name(std::u16string_view name) {
    name=name.substr(0,name.find(u'\0'));
    std::string result;
    for(std::size_t i=0;i<name.size();) {
        std::uint32_t scalar=name[i++];
        if(scalar>=0xd800 && scalar<=0xdbff) {
            if(i==name.size())scalar=0xfffd;
            else {
                const std::uint32_t next=name[i++];
                scalar=next>=0xdc00 && next<=0xdfff
                    ? 0x10000+((scalar-0xd800)<<10)+(next-0xdc00) : 0xfffd;
            }
        } else if(scalar>=0xdc00 && scalar<=0xdfff)scalar=0xfffd;
        if(scalar<0x80)result.push_back(static_cast<char>(scalar));
        else if(scalar<0x800) {
            result.push_back(static_cast<char>(0xc0|(scalar>>6)));
            result.push_back(static_cast<char>(0x80|(scalar&0x3f)));
        } else if(scalar<0x10000) {
            result.push_back(static_cast<char>(0xe0|(scalar>>12)));
            result.push_back(static_cast<char>(0x80|((scalar>>6)&0x3f)));
            result.push_back(static_cast<char>(0x80|(scalar&0x3f)));
        } else {
            result.push_back(static_cast<char>(0xf0|(scalar>>18)));
            result.push_back(static_cast<char>(0x80|((scalar>>12)&0x3f)));
            result.push_back(static_cast<char>(0x80|((scalar>>6)&0x3f)));
            result.push_back(static_cast<char>(0x80|(scalar&0x3f)));
        }
    }
    return result;
}
// CultureInfo.ConstructLocaleFromName 0x1e27ab8. Caller rejects null names.
bool construct_culture_from_name(std::u16string_view name,CultureNameLookup& lookup) {
    if(lookup.construct(name))return true;
    if(name.size()<3)return false;
    auto end=name.size()-1;
    while(end>0) {
        // LastIndexOf starts one position before the current candidate's end.
        const auto dash=name.rfind(u'-',end-1);
        if(dash==name.npos || dash==0)return false;
        end=dash;
        if(lookup.construct(name.substr(0,end)))return true;
    }
    return false;
}
}
