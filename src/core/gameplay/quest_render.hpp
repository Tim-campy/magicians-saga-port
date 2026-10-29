#pragma once
#include "quest_camera.hpp"
#include "image_draw.hpp"

namespace saga::core {
struct QuestRenderManager {bool capturing=false,paused=false;};
struct QuestRenderViewport {std::int32_t x=0,y=0,width=0,height=0;}; // QuestForm VIEW_X/Y/W/H.
class QuestRenderWorld {
public:
    virtual ~QuestRenderWorld()=default;
    virtual void dynamic_draw(RenderGraphics&)=0;
};
class QuestRenderSurface {
public:
    virtual ~QuestRenderSurface()=default;
    RenderImage* image=nullptr;bool empty=false; // Offscreen +0x38/+0x40.
    virtual void set_statics_transform(float x,float y,float angle,float scale_x,float scale_y)=0;
    virtual void render_statics()=0;
};
struct QuestRenderApplication {
    RenderDrawState drawing;
    RenderDrawHost* host=nullptr;
    RenderDrawResource* quest_resource=nullptr; // AppData.resQuest_, distinct from resource selector calls.
};
struct QuestRenderForm {
    QuestCameraForm& camera;
    QuestRenderManager* manager=nullptr;
    QuestRenderApplication* application=nullptr;
    QuestRenderWorld* world=nullptr; // Bind the same World as camera.world through its render interface.
    QuestRenderSurface* surface=nullptr; // DrawOffscreen uses this field, not DynamicRender's argument.
    bool draw_in_background=false;
};
class QuestRenderHost:public QuestFormCameraHost {
public:
    virtual QuestRenderViewport& render_viewport()=0;
    virtual bool active(QuestRenderForm&)=0; // Existing MyFormBase.Active contract; no guessed stack state.
    virtual void set_clip_enable(RenderGraphics&,bool)=0;
    virtual void push_origin(RenderGraphics&,float x,float y,bool option)=0;
    virtual void pop_origin(RenderGraphics&)=0;
};
// QuestForm offscreen callbacks. Graphics/world/viewport services remain portable interfaces.
bool dynamic_quest_render(QuestRenderForm&,RenderGraphics*,QuestRenderSurface*,QuestRenderHost&); // 0x11df6e4.
bool static_quest_render(QuestRenderForm&,QuestRenderSurface*,QuestRenderHost&); // 0x11dfa1c; gate only.
void draw_quest_offscreen(QuestRenderForm&,RenderGraphics*,QuestRenderHost&); // 0x11df004.
void update_static_quest_render(QuestCameraForm&); // 0x11df46c.
} // namespace saga::core
