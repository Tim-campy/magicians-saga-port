#include "platform/desktop/text_font_backend.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace saga::desktop;
void check(bool value){static int assertion=0;++assertion;if(!value){std::cerr<<"font backend assertion "<<assertion<<'\n';std::abort();}}
template<class F>void fails(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught);}
int main(int argc,char** argv){
    check(argc==2);DesktopTextFont font(std::filesystem::u8path(argv[1]));
    check(font.run_width(u"",16,0,-1)==0);
    const auto a=font.run_width(u"A",64,0,-1),v=font.run_width(u"V",64,0,-1);
    check(a>0&&v>0);check(font.run_width(u"AV",64,0,-1)==a+v);
    check(font.run_width(u"AV",64,0,0)==a+v); // This GPOS-only fixture has no legacy FT_Get_Kerning pair.
    const auto non_ascii=font.run_width(u"caf\u00e9",16,0,-1);check(non_ascii>0&&std::isfinite(non_ascii));
    const auto first=font.run_width(u"whole run",12,0,0);check(first==font.run_width(u"whole run",12,0,0));
    check(font.run_width(u"whole run",24,0,0)>first);
    fails([&]{font.run_width(u"x",0,0,0);});fails([&]{font.run_width(u"x",16,0,3);});
    fails([&]{font.run_width(u"x",16,0,2);}); // Unscaled kerning has font units, not pixel advances.
    fails([&]{font.run_width(u"x",16,1,0);}); // FT_LOAD_NO_SCALE changes the selected contract.
    fails([&]{font.run_width(u"\U0010ffff",16,0,0);}); // No invented fallback face/glyph.
    fails([&]{font.run_width(std::u16string(1,static_cast<char16_t>(0xdc00)),16,0,0);});
    std::cout<<"public font run backend contract ok\n";
}
