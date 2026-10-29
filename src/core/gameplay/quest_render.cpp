#include "quest_render.hpp"
#include <exception>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing quest render input");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t subtract(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t divide(std::int32_t a,std::int32_t b){
    if(!b)return 0;
    if(a==std::numeric_limits<std::int32_t>::min()&&b==-1)return a;
    return a/b;
}
QuestRenderApplication& application(QuestRenderForm& f){return required(f.application);}
void cleanup(QuestRenderForm& f,std::int32_t width,std::int32_t height,QuestRenderHost& host){
    auto& viewport=host.render_viewport();viewport.width=width;viewport.height=height;
    clear_render_queue(required(application(f).drawing.queue));
}
}
bool static_quest_render(QuestRenderForm& f,QuestRenderSurface* surface,QuestRenderHost& host){
    return f.draw_in_background||required(surface).empty||host.active(f);
}
void update_static_quest_render(QuestCameraForm& f){f.update_static_render=true;}
void draw_quest_offscreen(QuestRenderForm& f,RenderGraphics* graphics,QuestRenderHost& host){
    auto* image=required(f.surface).image;
    const auto viewport=host.render_viewport();
    const auto width=quest_game_width(&f.camera,host),height=quest_game_height(&f.camera,host);
    RenderImageDraw draw;draw.image=image;draw.u=viewport.x;draw.v=viewport.y;
    draw.source_width=width;draw.source_height=height;
    draw.x=static_cast<float>(viewport.x);draw.y=static_cast<float>(viewport.y);
    draw.width=static_cast<float>(viewport.width);draw.height=static_cast<float>(viewport.height);draw.scaled=true;
    required(graphics).draw_image(draw);
}
bool dynamic_quest_render(QuestRenderForm& f,RenderGraphics* graphics,QuestRenderSurface* surface,QuestRenderHost& host){
    auto& manager=required(f.manager);
    if(!manager.capturing){
        if(manager.paused)return false;
        if(!static_quest_render(f,surface,host))return false;
    }
    auto& g=required(graphics);host.set_clip_enable(g,false);
    auto* resource=application(f).quest_resource;
    if(resource){
        host.push_origin(g,0,0,false);
        // Reread resQuest after PushOrigin, as the original does.
        auto images=required(application(f).quest_resource).images;
        auto& image=required(dynamic_cast<PixelImage*>(required(images.get()).at(90)));
        const auto image_width=image.width,image_height=image.height;
        const auto width=quest_game_width(&f.camera,host);
        const auto height=divide(multiply(divide(multiply(width,100),image_width),image_height),100);
        const auto bottom=quest_game_height(&f.camera,host);
        RenderImageDraw draw;draw.image=&image;draw.source_width=image_width;draw.source_height=image_height;
        draw.x=0;draw.y=static_cast<float>(subtract(bottom,height));
        draw.width=static_cast<float>(width);draw.height=static_cast<float>(height);draw.scaled=true;
        g.draw_image(draw);host.pop_origin(g);
    }
    g.pop_blend_mode(); // The enclosing offscreen renderer owns the matching push.
    auto& offscreen=required(surface);offscreen.set_statics_transform(0,0,0,1,1);
    if(f.camera.update_static_render){f.camera.update_static_render=false;offscreen.render_statics();}
    const auto viewport=host.render_viewport();
    std::exception_ptr failure;
    try{
        const auto width=quest_game_width(&f.camera,host);host.render_viewport().width=width;
        const auto height=quest_game_height(&f.camera,host);host.render_viewport().height=height;
        auto& app=application(f);draw_render_queue(app.drawing,graphics,0,required(app.host));
        required(f.world).dynamic_draw(g);
    }catch(...){failure=std::current_exception();}
    // LSDA includes viewport queries in this scope; background/static setup remains outside it.
    cleanup(f,viewport.width,viewport.height,host);
    if(failure)std::rethrow_exception(failure);
    return true;
}
} // namespace saga::core
