#include "core/gameplay/culture_locale.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"culture locale contract failure\n";std::exit(1);}}
struct Host:CultureLocaleHost {
    std::string platform;
    std::optional<std::string> all,language,process;
    std::vector<std::string> calls;
    std::string platform_locale() override {calls.push_back("platform");return platform;}
    std::optional<std::string> environment(std::string_view name) override {calls.emplace_back(name);return name=="LC_ALL"?all:language;}
    std::optional<std::string> process_locale() override {calls.push_back("process");return process;}
};
int main(){
    {Host h;h.platform="pt-BR";h.all="en_US";check(select_native_locale(h)=="pt-BR");check(h.calls==std::vector<std::string>{"platform"});}
    {Host h;h.all="";h.language="ja_JP";check(select_native_locale(h).empty());check((h.calls==std::vector<std::string>{"platform","LC_ALL"}));}
    {Host h;h.all="C";h.language="ja_JP";check(select_native_locale(h).empty());check(h.calls.size()==2);}
    {Host h;h.language="C.UTF-8";check(select_native_locale(h)=="en-us");check((h.calls==std::vector<std::string>{"platform","LC_ALL","LANG"}));}
    {Host h;h.process="EN_US.UTF-8@calendar";check(normalize_culture_locale(select_native_locale(h))=="en-us");check((h.calls==std::vector<std::string>{"platform","LC_ALL","LANG","process"}));}
    {Host h;check(select_native_locale(h).empty());check(h.calls.size()==4);}
    for(const auto* value:{"en US","en/US","C"}){Host h;h.all=value;check(select_native_locale(h).empty());check(h.calls.size()==2);}
    // Validation and C.UTF-8 replacement apply only to the fallback source.
    {Host h;h.platform="C.UTF-8";check(select_native_locale(h)=="C.UTF-8");check(normalize_culture_locale(select_native_locale(h))=="c");}
    {Host h;h.platform="a/b c";check(select_native_locale(h)=="a/b c");}
    {Host h;h.all=std::string("en_US\0ignored",13);check(select_native_locale(h)=="en_US");}
    check(normalize_culture_locale("SR_Latn_RS.UTF-8@latin")=="sr-latn_rs");
    check(normalize_culture_locale("DE_DE@euro.UTF-8")=="de-de");
    check(normalize_culture_locale(".UTF-8").empty());
    check(normalize_culture_locale("@variant").empty());
    check(normalize_culture_locale("PT-br")=="pt-br");
    check(normalize_culture_locale("C")=="c");
    check(normalize_culture_locale("POSIX")=="posix");
    check(normalize_culture_locale(std::string("EN\0US",5))=="en");
}
