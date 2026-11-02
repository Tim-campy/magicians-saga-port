#pragma once
#include "input_frame.hpp"
#include "entity_visual.hpp"
#include "quest_camera.hpp"
#include <string>
#include <memory>

namespace saga::core {
struct EntityButton final:Component {
    std::int32_t id=0,value=0;bool dynamic=false,auto_destroy=false;
    std::int32_t scale=0;std::shared_ptr<std::u16string> text;
    std::int32_t text_offset_x=0,text_offset_y=0;ComponentSlots* parent=nullptr;
    std::int32_t offset_x=0,offset_y=0;bool hover=false;
};
struct ButtonProperties {
    std::int32_t id,value;bool dynamic,auto_destroy;std::int32_t scale;
    std::shared_ptr<std::u16string> text;std::int32_t text_offset_x,text_offset_y;
    ComponentSlots* parent;std::int32_t offset_x,offset_y;
};
struct ButtonCreation {
    std::int32_t x,y,resource,clip,frame;ButtonProperties properties;bool unique_value;
};
class ButtonFactoryHost:public EntityCreationHost {
public:
    virtual EntityButton& acquire_button()=0;
    virtual ContactPosition& acquire_position()=0;
    virtual EntitySprite& acquire_sprite()=0;
    virtual EntitySubset* factory_buttons()=0; // Supplied world's And17, not the frame subset.
    virtual void destroy_button(ComponentSlots&,bool)=0;
};
ComponentSlots& add_button(ComponentSlots&,const ButtonProperties&,ButtonFactoryHost&); // Entity.AddButton 0x112cc0c.
ComponentSlots& create_button(const ButtonCreation&,EntityIdState&,ButtonFactoryHost&); // World.CreateButton 0x11343d0.
struct ButtonFrameState {EntitySubset* subset=nullptr;}; // ButtonSystem +0x20.
class ButtonFrameHost {
public:
    virtual ~ButtonFrameHost()=default;
    virtual bool button_parent_destroyed(ComponentSlots&)=0;
    virtual float project_button_x(float x,float z)=0; // Current AppData.TransormIsometricViewX.
    virtual float project_button_y(float x,float z)=0;
    virtual QuestCameraForm* button_camera_form()=0; // Current global AppData.questForm.
    virtual void destroy_button(ComponentSlots&,bool remove_registration)=0;
    virtual void send_button_event(const InputTouch*,ComponentSlots*)=0; // Current global world event11.
};
class ButtonInitializationWorld {
public:
    using Receiver=std::function<void(const InputTouch*)>;
    virtual ~ButtonInitializationWorld()=default;
    virtual EntitySubset* button_subset(const EventIntegers&)=0;
    virtual void subscribe_button_touch(Receiver)=0; // Event9.
};
void initialize_buttons(ButtonFrameState&,ButtonInitializationWorld*,ButtonFrameHost&); // 0x114277c.
void update_buttons(ButtonFrameState&,ButtonFrameHost&); // 0x11428a0; pass0 gate external.
void button_touch(ButtonFrameState&,const InputTouch*,ButtonFrameHost&); // 0x11437b8.
// Borrowed identities; the host retains AppData, ResourceManager, SEB and TouchOption lifetimes.
using ButtonApplicationContext=const void*;
using ButtonResourceContext=void*;
using ButtonSebContext=void*;
using ButtonTouchOption=const void*;
using ButtonText=std::shared_ptr<const std::u16string>;
struct ButtonDrawMetrics {float view_scale=0;std::int32_t quest_scale=0;};
class ButtonDrawingHost {
public:
    virtual ~ButtonDrawingHost()=default;
    // GameView.GetInstance 0x10f2abc -> SurfaceBase.scaleRatio_ +0xc4, then current QuestForm.scale +0x1dc.
    virtual ButtonDrawMetrics button_draw_metrics();
    // Fresh QuestForm.scale, then BaseSystem.view_ +0x18 -> SurfaceBase.scaleRatio_ +0xc4.
    virtual ButtonDrawMetrics button_touch_metrics();
    virtual void button_black_color();
    virtual ButtonApplicationContext capture_button_application();
    virtual ButtonResourceContext button_resource(ButtonApplicationContext,std::int32_t resource);
    virtual ButtonSebContext button_seb(ButtonResourceContext,std::int32_t clip);
    virtual std::array<std::int32_t,4> button_bounds(ButtonSebContext,std::int32_t frame); // Left/top/width/height; signed int16 widened to int32.
    virtual void button_draw_sprite(ButtonResourceContext,std::int32_t clip,std::int32_t frame,float x,float y);
    virtual void button_draw_scaled_sprite(ButtonResourceContext,std::int32_t clip,std::int32_t frame,std::int32_t x,std::int32_t y,std::int32_t scale);
    virtual void button_draw_seb_ex(ButtonResourceContext,std::int32_t clip,std::int32_t frame,std::int32_t x,std::int32_t y,std::int32_t scale,std::int32_t anchor);
    virtual void button_render_mode(std::int32_t mode,std::int32_t source,std::int32_t destination);
    virtual void button_reset_render_mode();
    virtual void button_draw_text(ButtonText,float x,float y,std::int32_t anchor);
    virtual void button_add_touch(std::int32_t id,InputRectangle,std::int32_t value);
    virtual void button_add_touch_seb(std::int32_t id,std::int32_t x,std::int32_t y,ButtonSebContext,std::int32_t frame,std::int32_t value,ButtonTouchOption);
    virtual void button_add_touch_delay(std::int32_t id,std::int32_t x,std::int32_t y,ButtonSebContext,std::int32_t frame,std::int32_t value,ButtonTouchOption);
};
// ButtonSystem.Draw 0x1142dac / DrawDynamic 0x11437a8 -> DrawButtons 0x1142db4.
// The caller owns enabled and selects dynamic=false/true. The live predicate precedes Invisible21.
void draw_buttons(ButtonFrameState&,bool dynamic,ButtonDrawingHost&);
struct ButtonDrawRequest {
    std::int32_t id=35,x=0,y=0,resource=0,clip=0,frame=0,value=0,anchor=17;
    bool dynamic=false;ButtonTouchOption option=nullptr;
};
// Full DrawButton 0x1143988; default id35 also covers its forwarding overload 0x1143938.
void draw_button(const ButtonDrawRequest&,ButtonDrawingHost&);
}
