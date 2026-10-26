#include "core/gameplay/culture_locale.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <stdexcept>
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
struct Lookup:CultureNameLookup {
    std::vector<std::u16string> calls;
    std::optional<std::u16string> accepted;
    bool construct(std::u16string_view name) override {calls.emplace_back(name);return accepted && name==*accepted;}
};
struct IdentityHost:CultureIdentityHost {
    int calls=0;
    bool result=true,fail=false;
    std::optional<std::u16string> territory;
    bool populate(CultureIdentity& state,std::int32_t lcid) override {
        ++calls;check(state.lcid==lcid);state.territory=territory;
        if(fail)throw std::runtime_error("population failed");
        return result;
    }
};
int main(){
    {CultureIdentity c{0x7f,false,std::nullopt};IdentityHost h;check(!is_neutral_culture(c,h));check(!c.constructed && h.calls==0);}
    {CultureIdentity c{1,false,std::nullopt};IdentityHost h;h.result=false;check(is_neutral_culture(c,h));check(c.constructed && h.calls==1);check(is_neutral_culture(c,h) && h.calls==1);}
    {CultureIdentity c{1,false,std::nullopt};IdentityHost h;h.territory=u"";check(!is_neutral_culture(c,h));check(c.constructed);}
    {CultureIdentity c{1,true,u"XX"};IdentityHost h;check(!is_neutral_culture(c,h));check(h.calls==0);}
    {CultureIdentity c{1,false,std::nullopt};IdentityHost h;h.fail=true;h.territory=u"YY";try{is_neutral_culture(c,h);check(false);}catch(const std::runtime_error&){}check(!c.constructed && c.territory==u"YY" && h.calls==1);}
    check(encode_culture_lookup_name(u"PT-br")=="PT-br");
    check(encode_culture_lookup_name(std::u16string(u"ab\0cd",5))=="ab");
    check(encode_culture_lookup_name(u"\u007f\u0080\u07ff\u0800\uffff\U00010000\U0010ffff")==std::string("\x7f\xc2\x80\xdf\xbf\xe0\xa0\x80\xef\xbf\xbf\xf0\x90\x80\x80\xf4\x8f\xbf\xbf"));
    const std::string replacement="\xef\xbf\xbd";
    check(encode_culture_lookup_name(std::u16string{0xd800})==replacement);
    check(encode_culture_lookup_name(std::u16string{0xdc00,u'x'})==replacement+"x");
    check(encode_culture_lookup_name(std::u16string{0xd800,u'x',u'y'})==replacement+"y");
    check(encode_culture_lookup_name(std::u16string{0xd800,0xd800,0xdc00})==replacement+replacement);
    check(encode_culture_lookup_name(std::u16string{0xd800,0,u'x'})==replacement);
    {Lookup l;l.accepted=u"zh-Hant";check(construct_culture_from_name(u"zh-Hant-XX",l));check((l.calls==std::vector<std::u16string>{u"zh-Hant-XX",u"zh-Hant"}));}
    {Lookup l;check(!construct_culture_from_name(u"a-b-c",l));check((l.calls==std::vector<std::u16string>{u"a-b-c",u"a-b",u"a"}));}
    {Lookup l;l.accepted=u"x-y";check(construct_culture_from_name(u"x-y",l));check(l.calls.size()==1);}
    // A trailing hyphen is skipped by the first backward search.
    {Lookup l;check(!construct_culture_from_name(u"ab-cd-",l));check((l.calls==std::vector<std::u16string>{u"ab-cd-",u"ab"}));}
    {Lookup l;check(!construct_culture_from_name(u"a--b",l));check((l.calls==std::vector<std::u16string>{u"a--b",u"a-",u"a"}));}
    for(const auto* name:{u"",u"a",u"ab",u"ab-",u"-ab",u"abc",u"a_b"}){Lookup l;check(!construct_culture_from_name(name,l));check(l.calls.size()==1);}
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
