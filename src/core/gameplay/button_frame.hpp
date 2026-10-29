#pragma once
#include "input_frame.hpp"
#include "entity_visual.hpp"
#include "quest_camera.hpp"
#include <string>

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
}
