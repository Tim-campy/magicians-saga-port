#include "platform/desktop/archive_resource_manager.hpp"
#include "core/gameplay/quest_render.hpp"
#include "jar_fixture.hpp"
#include "render_draw_fixture.hpp"
#include <cstdlib>
#include <iostream>

using namespace saga::desktop;
using namespace saga::core;
using saga::test::Bytes;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"archive manager contract line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F op){bool caught=false;try{op();}catch(const std::exception&){caught=true;}check(caught);}
Bytes text(const std::string& s){return {s.begin(),s.end()};}
Bytes clip(){Bytes out;for(const int v:{1,4,1,0,0,7,0,0,1,1,0,0,0,0}){
    out.push_back(static_cast<std::uint8_t>(v>>8));out.push_back(static_cast<std::uint8_t>(v));}return out;}
struct Mission:QuestRenderHost,QuestRenderWorld,QuestRenderSurface,QuestViewportView {
    QuestRenderViewport viewport{0,0,480,640};int world_draws=0;
    QuestRenderViewport& render_viewport()override{return viewport;}
    bool active(QuestRenderForm&)override{return true;}
    std::int32_t quest_view_x()override{return viewport.x;}
    std::int32_t quest_view_y()override{return viewport.y;}
    EventInstruction camera_saved_integers()override{throw std::logic_error("render must not read saves");}
    std::int32_t game_width()override{return 480;}
    std::int32_t game_height()override{return 640;}
    void set_clip_enable(RenderGraphics&,bool enable)override{check(!enable);}
    void push_origin(RenderGraphics&,float,float,bool)override{check(false);}
    void pop_origin(RenderGraphics&)override{check(false);}
    void set_statics_transform(float,float,float,float,float)override{}
    void render_statics()override{check(false);}
    void dynamic_draw(RenderGraphics&)override{check(viewport.width==320&&viewport.height==426);++world_draws;}
};
}
int main()try{
    const Bytes png{137,80,78,71,13,10,26,10}; // Image decoding is a public backend service.
    const auto bytes=saga::test::jar_fixture({
        {"img.inf",text("7\tchoice.png,bin,q+1,40x40\n9\tother.png,a1")},
        {"seb.inf",text("3\tclip.seb,v,c2\n5\tclip.seb,u,c3\n6\tCLIP.SEB")},
        {"clip.seb",clip()},{"choice.png",png}});
    ResourceArchive archive(bytes,{"English.lproj",0});
    const auto* selected=&archive.animation(3);check(selected==&archive.animation(3));
    const auto* source=&archive.image(7);check(source==&archive.image(7)&&source->png==png);
    fails([&]{archive.image(9);}); // An unrelated unsupported image does not block SEB readiness.
    ArchiveResourceManager manager(archive);
    auto& clips=*manager.animations().clips;check(clips.size()==7&&!clips[0]&&!clips[4]);
    auto& a=*clips[3];auto& b=*clips[5];auto& c=*clips[6];
    check(&a!=&b&&a.max_frame==4&&b.max_frame==4&&a.frame==0&&b.frame==0);
    check(a.sprites->groups==b.sprites->groups&&a.sprites->frames==b.sprites->frames);
    check(a.sprites->packed==b.sprites->packed&&a.sprites->extended==b.sprites->extended);
    check(a.sprites->work!=b.sprites->work&&a.sprites->cache!=b.sprites->cache);
    check(a.sprites->cache->size()==2&&b.sprites->cache->size()==3);
    check(a.sprites->reverse==1&&b.sprites->reverse==2&&!a.sprites->flip&&!b.sprites->flip);
    // Filename comparison precedes archive lowercasing: case variants are fresh loads.
    check(a.sprites->groups!=c.sprites->groups&&c.sprites->reverse==0&&c.sprites->cache==nullptr);
    a.frame=2;check(b.frame==0);advance_shared_animation(b,{false,true});check(a.frame==2&&b.frame==1);
    // Constructor projection shares packed records, not mutable evaluation work or clocks.
    a.sprites->flip=true;auto copy=clone_seb_sprites(*a.sprites);
    check(copy.groups==a.sprites->groups&&copy.work!=a.sprites->work&&!copy.cache&&copy.reverse==0&&!copy.flip);
    auto saved=copy.packed->at(0);copy.packed->at(0)^=1;check(copy.packed->at(0)==a.sprites->packed->at(0));copy.packed->at(0)=saved;
    // Stable complete arrays feed the existing consumer at original catalog IDs.
    check(manager.drawing().animations==&manager.animations()&&manager.drawing().images->size()==10);
    PixelImage image;image.width=1;image.height=1;image.standard_width=40;image.standard_height=40;image.render_queue=1;
    manager.bind_image(7,&image);check(manager.drawing().images->at(7)==&image);
    PixelImage wrong;fails([&]{manager.bind_image(7,&wrong);});check(manager.drawing().images->at(7)==&image);
    fails([&]{manager.bind_image(0,&image);});fails([&]{manager.bind_image(-1,&image);});
    RenderDrawServices renderer;renderer.resource_provider=[&](auto){return &manager.drawing();};
    auto row=renderer.add();row->at(render_clip)=5;renderer.draw();
    check(renderer.draws.size()==1&&renderer.draws.at(0).image==&image&&renderer.reversals.at(0)==1);
    // The same ready manager now feeds the actual QuestForm callback/viewport/cleanup path.
    Mission mission;StageQuestForm stage{150,0};QuestCameraForm camera{stage,nullptr,&mission};QuestRenderManager gates;
    QuestRenderApplication app{renderer.state,&renderer,nullptr};
    QuestRenderForm form{camera,&gates,&app,&mission,&mission,false};
    check(dynamic_quest_render(form,&renderer,&mission,mission));
    check(mission.world_draws==1&&renderer.draws.size()==2&&renderer.draws.back().image==&image);
    check(mission.viewport.width==480&&mission.viewport.height==640&&renderer.queue.counts->at(0)==0);
    auto retained=b.sprites;auto retained_cache=b.sprites->cache;auto retained_clip=&b;
    manager.bind_image(7,nullptr);check(manager.drawing().images->at(7)==nullptr);
    manager.bind_image(7,&image);check(clips[5]==retained_clip&&clips[5]->sprites==retained&&retained->cache==retained_cache);
    // A fresh manager has independent clocks, packed-record storage and readiness.
    ArchiveResourceManager second(archive);check(second.animations().clips->at(3)!=clips[3]);
    check(second.animations().clips->at(3)->sprites->groups!=a.sprites->groups&&!second.drawing().images->at(7));
    std::cout<<"archive resource manager contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
