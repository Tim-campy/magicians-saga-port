#include "platform/desktop/resource_archive.hpp"
#include "jar_fixture.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace saga::desktop;
using saga::test::Bytes;
using saga::test::jar_fixture;
namespace {
void check(bool value){if(!value)std::abort();}
template<class F>void fails(F op){bool caught=false;try{op();}catch(const std::exception&){caught=true;}check(caught);}
Bytes text(const std::string& s){return {s.begin(),s.end()};}
Bytes clip(){
    Bytes out;
    for(const int value:{1,4,1,0, 0,7,0,0,1,1,0,0,0,0}){
        out.push_back(static_cast<std::uint8_t>(value>>8));out.push_back(static_cast<std::uint8_t>(value));
    }
    return out;
}
}
int main(){
    const Bytes png{137,80,78,71,13,10,26,10}; // Binding test; real PNG decode has separate pixel tests.
    const Bytes opt{1,1,1,1,1, 255,255, 0,0,0,0,0,0,0,0,0,1,0,1};
    auto bytes=jar_fixture({{"img.inf",text("7\troot.png,bin\n2\tfirst.png\n2\tlast.png")},
        {"English.lproj/img.inf",text("\xef\xbb\xbf 7\t../choice.gif,bin,40x40\r\n")},
        {"seb.inf",text("3\tclip.seb\n")},{"clip.seb",clip()},
        {"drawable-hdpi/English.lproj/choice.png",png},{"English.lproj/choice.opt",opt}});
    ResourceArchive archive(bytes,{"English.lproj",1});bytes.clear();
    check(archive.images().member=="English.lproj/img.inf"&&archive.images().slots.size()==8&&!archive.images().slots[0]);
    check(archive.animations().slots.size()==4&&!archive.animations().slots[0]);
    const auto selected=archive.sprite(3,0,0);
    check(selected.texture==7&&selected.standard_width==40&&selected.standard_height==40);
    check(selected.png==png&&selected.image_member=="drawable-hdpi/English.lproj/choice.png");
    check(selected.animation_member=="clip.seb"&&selected.animation.header_value==4&&selected.opt&&selected.opt->pieces.size()==1);
    fails([&]{archive.sprite(0,0,0);});fails([&]{archive.sprite(-1,0,0);});
    auto source=[&](std::string image,std::string animation){return jar_fixture({{"img.inf",text(image)},{"seb.inf",text(animation)},{"clip.seb",clip()},{"choice.png",png}});};
    {ResourceArchive plain(source("2\tfirst.png\n7\tchoice.png,bin\n2\tlast.png", "clip.seb\n"),{"English.lproj",0});
     check(plain.images().slots[2]=="last.png"&&plain.animations().slots[0]=="clip.seb");
     check(!plain.sprite(0,0,0).opt);}
    {ResourceArchive unknown(source("7\tchoice.png,bin,q+1", "clip.seb"),{"English.lproj",0});fails([&]{unknown.sprite(0,0,0);});}
    {ResourceArchive unknown(source("7\tchoice.png,bin", "clip.seb,u"),{"English.lproj",0});fails([&]{unknown.sprite(0,0,0);});}
    fails([&]{ResourceArchive missing(jar_fixture({{"img.inf",{}}}),{"English.lproj",0});});
    fails([&]{ResourceArchive bad(source("7\tchoice.png", "clip.seb"),{"non-ascii-\xc3\xa9",0});});
    std::cout<<"resource archive contract ok\n";
}
