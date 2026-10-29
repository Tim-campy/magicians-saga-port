#include "platform/desktop/archive_image_set.hpp"
#include "jar_fixture.hpp"
#include "render_draw_fixture.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace saga::desktop;
using namespace saga::core;
using saga::test::Bytes;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"archive image set line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
Bytes text(const std::string& s){return {s.begin(),s.end()};}
Bytes clip(){Bytes out;for(const int v:{2,4,1,0,0,2,0,0,2,1,0,0,0,0,1,0,0,7,0,0,2,1,0,0,0,0}){
    out.push_back(static_cast<std::uint8_t>(v>>8));out.push_back(static_cast<std::uint8_t>(v));}return out;}
DecodedImage decode(const Bytes& bytes){check(bytes==Bytes{42});return {2,1,{255,0,0,0,0,255,0,255}};}
}
int main()try{
    // Shared desktop alpha projection preserves top-down ordering, suffix and partial writes.
    auto alpha=std::make_shared<std::vector<float>>(4,0.5F);
    const DecodedImage sample{2,2,{0,0,0,0,0,0,0,255,0,0,0,128,0,0,0,64}};
    auto same=decoded_image_alpha(sample,0,0,2,1,alpha);
    check(same==alpha&&alpha->at(0)==0&&alpha->at(1)==1&&alpha->at(2)==0.5F);
    alpha=std::make_shared<std::vector<float>>(1,-1.0F);
    fails([&]{decoded_image_alpha(sample,0,1,2,1,alpha);});check(alpha->at(0)==128.0F/255.0F);
    fails([&]{decoded_image_alpha(sample,1,1,2,1,alpha);});check(alpha->at(0)==128.0F/255.0F);
    const auto fixture=saga::test::jar_fixture({{"img.inf",text("2\tshared.png,bin,20x30,q+1\n7\tshared.png,bin,a1,q-2")},
        {"seb.inf",text("clip.seb")},{"clip.seb",clip()},{"shared.png",{42}}});
    ResourceArchive archive(fixture,{"English.lproj",0});ArchiveResourceManager manager(archive);
    PixelImage old;old.standard_width=20;old.standard_height=30;old.render_queue=1;manager.bind_image(2,&old);
    auto bindings=manager.drawing().images;
    {
        int decoded=0;ArchiveImageSet images(manager,[&](const auto& b){++decoded;return decode(b);});
        check(decoded==2&&images.ready_count()==2);const auto& a=images.at(2);const auto& b=images.at(7);
        check(a.image.get()!=b.image.get()&&a.image->width==2&&a.image->height==1);
        check(a.image->standard_width==20&&a.image->standard_height==30&&a.image->render_queue==1);
        check(b.image->standard_width==0&&b.image->render_queue==-2&&a.alpha_type==1&&b.alpha_type==1);
        check(bindings->at(2)==a.image.get()&&bindings->at(7)==b.image.get()&&!bindings->at(3));
        fails([&]{images.at(0);});fails([&]{images.at(-1);});
        RenderDrawServices renderer;renderer.resource_provider=[&](auto){return &manager.drawing();};
        renderer.add()->at(render_layer)=0;renderer.add()->at(render_layer)=1;renderer.draw();
        check(renderer.draws.size()==2&&renderer.draws[0].image==a.image.get()&&renderer.draws[1].image==b.image.get());
        // A later host binding survives destruction; old borrowed array slots are cleared even after replacement.
        manager.bind_image(2,&old);manager.drawing().images=std::make_shared<std::vector<RenderImage*>>(8,nullptr);
    }
    check(bindings->at(2)==&old&&!bindings->at(7));manager.drawing().images=bindings;
    // Decode failure, malformed output and the total budget all leave previous bindings intact.
    fails([&]{ArchiveImageSet images(manager,[](const auto&)->DecodedImage{throw std::runtime_error("decoder");});});
    check(bindings->at(2)==&old&&!bindings->at(7));
    int calls=0;fails([&]{ArchiveImageSet images(manager,[&](const auto& b){if(++calls==2)throw std::runtime_error("second");return decode(b);});});
    check(calls==2&&bindings->at(2)==&old&&!bindings->at(7));
    for(auto bad:{DecodedImage{0,1,{}},DecodedImage{1,1,{1}},DecodedImage{std::numeric_limits<int>::max(),2,{}}}){
        fails([&]{ArchiveImageSet images(manager,[&](const auto&){return bad;});});check(bindings->at(2)==&old);
    }
    fails([&]{ArchiveImageSet images(manager,decode,15);});check(bindings->at(2)==&old&&!bindings->at(7));
    // Constructor validates the final publication array after decoder callbacks.
    fails([&]{ArchiveImageSet images(manager,[&](const auto& b){manager.drawing().images.reset();return decode(b);});});
    check(bindings->at(2)==&old);manager.drawing().images=bindings;
    // Regular OPT objects are prepared through the existing reader/consumer.
    const Bytes opt{1,1,1,1,1,255,255,0,0,0,0,0,0,0,0,0,1,0,1};
    ResourceArchive optimized(saga::test::jar_fixture({{"img.inf",text("image.png,bin")},{"seb.inf",{}},{"image.png",{42}},{"image.opt",opt}}),{"English.lproj",0});
    ArchiveResourceManager opt_manager(optimized);ArchiveImageSet opt_images(opt_manager,decode);
    auto* prepared=dynamic_cast<PreparedOptImage*>(opt_images.at(0).image.get());check(prepared&&prepared->passes==1&&prepared->columns==1);
    std::cout<<"archive image set contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
