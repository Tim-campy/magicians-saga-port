#include "culture_locale.hpp"

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
