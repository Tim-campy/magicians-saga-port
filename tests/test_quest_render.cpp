#include "core/gameplay/quest_render.hpp"
#include "core/gameplay/offscreen_transform.hpp"
#include "render_draw_fixture.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace saga::core;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"quest render contract line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct View:QuestViewportView {std::int32_t width=480,height=720;std::function<void(bool)> hook;
    std::int32_t game_width()override{if(hook)hook(true);return width;}
    std::int32_t game_height()override{if(hook)hook(false);return height;}
};
struct Scene:QuestRenderHost,QuestRenderSurface,QuestRenderWorld,OffscreenTransformHost {
    OffscreenTransforms transforms{1,std::make_shared<std::vector<OffscreenMatrix>>(2),{}, {}};
    RenderDrawServices drawing;StageQuestForm stage{150,0};View view;
    QuestCameraForm camera{stage,nullptr,&view};QuestRenderManager manager;
    QuestRenderApplication app{drawing.state,&drawing,nullptr};
    QuestRenderForm form{camera,&manager,&app,this,this,false};
    QuestRenderViewport viewport{20,30,460,690};
    PixelImage background;RenderDrawResource background_resource;
    bool is_active=true;int active_calls=0;
    std::vector<std::string> calls;std::function<void(const std::string&)> hook;
    Scene(){
        background.width=333;background.height=111;
        background_resource.images=std::make_shared<std::vector<RenderImage*>>(91,nullptr);
        background_resource.images->at(90)=&background;
        drawing.hook=[this](const auto& c){step(c);};
    }
    void step(const std::string& c){calls.push_back(c);if(hook)hook(c);}
    std::int32_t quest_view_x()override{return viewport.x;}
    std::int32_t quest_view_y()override{return viewport.y;}
    EventInstruction camera_saved_integers()override{throw std::logic_error("render must not read saves");}
    QuestRenderViewport& render_viewport()override{return viewport;}
    bool active(QuestRenderForm&)override{++active_calls;step("active");return is_active;}
    void set_clip_enable(RenderGraphics& g,bool enable)override{check(&g==&drawing&&!enable);step("clip-off");}
    void push_origin(RenderGraphics&,float x,float y,bool option)override{check(x==0&&y==0&&!option);step("origin+");}
    void pop_origin(RenderGraphics&)override{step("origin-");}
    void set_statics_transform(float x,float y,float depth,float sx,float sy)override{
        check(x==0&&y==0&&depth==0&&sx==1&&sy==1);
        set_static_offscreen_transform(transforms,x,y,depth,sx,sy,*this);step("static-transform");
    }
    OffscreenMatrix translation_scale(float x,float y,float z,float sx,float sy,float sz)override{
        OffscreenMatrix m;m.values[0]=sx;m.values[5]=sy;m.values[10]=sz;m.values[15]=1;
        m.values[12]=x;m.values[13]=y;m.values[14]=z;return m;
    }
    OffscreenMatrices allocate_statics(std::int32_t count)override{
        check(count>=0);return std::make_shared<std::vector<OffscreenMatrix>>(static_cast<std::size_t>(count));
    }
    void render_statics()override{check(!camera.update_static_render);step("static-render");}
    void dynamic_draw(RenderGraphics& g)override{check(&g==&drawing);step("world");}
    bool render(){return dynamic_quest_render(form,&drawing,this,*this);}
};
}
int main()try{
    // QuestForm's real static-transform reset propagates through the recovered surface kernel.
    {Scene s;const auto entries=s.transforms.statics;entries->at(1).values.fill(17);
        s.transforms.effective_density=3;check(s.render());check(s.transforms.statics==entries);
        OffscreenMatrix identity;identity.values[0]=identity.values[5]=identity.values[10]=identity.values[15]=1;
        check(s.transforms.static_base.values==identity.values);
        for(const auto& m:*entries)check(m.values==identity.values);}
    // Capture bypasses pause and activity; background and empty bypass activity only.
    for(int flags=0;flags<32;++flags){Scene s;s.manager.capturing=(flags&1)!=0;s.manager.paused=(flags&2)!=0;
        s.form.draw_in_background=(flags&4)!=0;s.empty=(flags&8)!=0;s.is_active=(flags&16)!=0;
        const bool expected=s.manager.capturing||(!s.manager.paused&&(s.form.draw_in_background||s.empty||s.is_active));
        check(s.render()==expected);check(s.drawing.blend_pops==(expected?1:0));
        check(s.active_calls==(!s.manager.capturing&&!s.manager.paused&&!s.form.draw_in_background&&!s.empty?1:0));
    }
    {Scene s;s.form.draw_in_background=true;check(static_quest_render(s.form,nullptr,s));
        s.form.draw_in_background=false;fails([&]{static_quest_render(s.form,nullptr,s);});
        s.manager.paused=true;check(!dynamic_quest_render(s.form,nullptr,nullptr,s));
        s.form.manager=nullptr;fails([&]{s.render();});}
    // Actual AppData consumer runs inside logical (unscaled) viewport, followed by World.
    {Scene s;auto row=s.drawing.add(150007);s.drawing.add(1,1);auto buffer=s.drawing.queue.buffers;
        update_static_quest_render(s.camera);
        s.hook=[&](const auto& c){if(c=="mesh"||c=="world")check(s.viewport.width==306&&s.viewport.height==460);};
        check(s.render());check(s.app.drawing.phase==1&&s.drawing.draws.size()==1);
        check((s.drawing.depths==std::vector<std::pair<int,int>>({{1,7}})));
        check(s.viewport.width==460&&s.viewport.height==690&&!s.camera.update_static_render);
        check(*s.drawing.queue.counts==EventIntegers({0,0})&&s.drawing.queue.buffers==buffer);
        check(buffer->at(0)->at(0)==row); // Clearing retains reusable rows.
        const auto& calls=s.calls;
        check(std::find(calls.begin(),calls.end(),"static-render")<std::find(calls.begin(),calls.end(),"mesh"));
        check(calls.back()=="world");}
    // Background uses image 90 and integer operations at each native division, not a float ratio.
    {Scene s;s.app.quest_resource=&s.background_resource;check(s.render());
        const auto& d=s.drawing.draws.at(0);check(d.image==&s.background&&d.scaled);
        check(d.x==0&&d.y==359&&d.width==306&&d.height==101);
        check(d.u==0&&d.v==0&&d.source_width==333&&d.source_height==111);
        check(s.calls.at(2)=="origin+"&&s.calls.at(4)=="origin-"&&s.calls.at(5)=="blend-");}
    {Scene s;s.app.quest_resource=&s.background_resource;s.background.width=0;check(s.render());
        check(s.drawing.draws.at(0).height==0&&s.drawing.draws.at(0).y==460);}
    {Scene s;s.app.quest_resource=&s.background_resource;s.background.width=1;s.background.height=std::numeric_limits<int>::max();
        check(s.render());check(s.drawing.draws.at(0).height==-306);}
    // DrawOffscreen copies viewport before querying width/height, with no implicit matrix or camera translation.
    {Scene s;RenderImage image;s.image=&image;
        s.view.hook=[&](bool width){if(width){s.viewport.width=999;s.viewport.height=888;}};
        draw_quest_offscreen(s.form,&s.drawing,s);
        const auto& d=s.drawing.draws.at(0);check(d.image==&image&&d.scaled);
        check(d.u==20&&d.v==30&&d.source_width==306&&d.source_height==460);
        check(d.x==20&&d.y==30&&d.width==460&&d.height==690);
        fails([&]{draw_quest_offscreen(s.form,nullptr,s);});}
    // Draw and world failures both restore dimensions and clear both queues.
    for(const auto* failure:{"draw","world"}){Scene s;s.drawing.add();s.drawing.add(0,1);
        s.hook=[&](const auto& c){if(c==failure){s.viewport.x=99;throw std::runtime_error(failure);}};
        fails([&]{s.render();});check(s.viewport.width==460&&s.viewport.height==690&&s.viewport.x==99);
        check(*s.drawing.queue.counts==EventIntegers({0,0}));
    }
    // Cleanup uses fresh AppData, rather than the instance captured for drawing.
    {Scene s;s.drawing.add();RenderDrawServices replacement;replacement.add(0,1);
        QuestRenderApplication app{replacement.state,&replacement,nullptr};
        s.hook=[&](const auto& c){if(c=="world")s.form.application=&app;};
        check(s.render());check(s.drawing.queue.counts->at(0)==1&&replacement.queue.counts->at(1)==0);}
    // Background/static setup is outside the cleanup scope; viewport queries are inside it.
    {Scene s;s.drawing.add();update_static_quest_render(s.camera);
        s.hook=[](const auto& c){if(c=="static-render")throw std::runtime_error("static");};
        fails([&]{s.render();});check(!s.camera.update_static_render&&s.drawing.queue.counts->at(0)==1);}
    for(bool fail_width:{true,false}){Scene s;s.drawing.add();
        s.view.hook=[&](bool width){if(width==fail_width)throw std::runtime_error("viewport");};
        fails([&]{s.render();});check(s.viewport.width==460&&s.viewport.height==690&&s.drawing.queue.counts->at(0)==0);}
    {Scene s;s.app.quest_resource=&s.background_resource;s.drawing.add();
        s.hook=[&](const auto& c){if(c=="origin+")s.app.quest_resource=nullptr;};
        fails([&]{s.render();});check(s.drawing.queue.counts->at(0)==1&&s.drawing.blend_pops==0);}
    // Cleanup failure supersedes a drawing failure, after restoring dimensions.
    {Scene s;s.drawing.add();s.hook=[&](const auto& c){if(c=="draw"){s.form.application=nullptr;throw std::runtime_error("draw");}};
        try{s.render();check(false);}catch(const std::invalid_argument&){check(s.viewport.width==460&&s.viewport.height==690);}}
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
