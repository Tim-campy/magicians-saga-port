#include "button_frame.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing entity-button data");return *p;}
EntityButton& button(ComponentSlots& e){return required(dynamic_cast<EntityButton*>(e.get(17)));}
ContactPosition& position(ComponentSlots& e){return required(dynamic_cast<ContactPosition*>(e.get(0)));}
}
ComponentSlots& add_button(ComponentSlots& e,const ButtonProperties& input,ButtonFactoryHost& host){
    const auto args=input; // Native scalar/reference arguments are captured before the pool callback.
    auto& b=host.acquire_button();b.id=args.id;b.value=args.value;b.dynamic=args.dynamic;b.auto_destroy=args.auto_destroy;
    b.scale=args.scale;b.text=args.text;b.text_offset_x=args.text_offset_x;b.text_offset_y=args.text_offset_y;
    b.parent=args.parent;b.offset_x=args.offset_x;b.offset_y=args.offset_y;
    return e.add(17,&b); // Native AddButton does not reset a pooled component's hover flag.
}
ComponentSlots& create_button(const ButtonCreation& input,EntityIdState& ids,ButtonFactoryHost& host){
    const auto args=input; // Destruction/factory callbacks cannot replace the caller's argument values.
    if(args.unique_value){
        auto cursor=required(host.factory_buttons()).cursor();ComponentSlots* previous=nullptr;
        while(const auto entry=cursor.next()){
            auto& e=required(*entry);if(button(e).value==args.properties.value){previous=&e;break;}
        }
        if(previous)host.destroy_button(*previous,true); // Only the first value match; group ID is ignored.
    }
    auto& e=create_entity(ids,0,false,host);auto& p=host.acquire_position();
    p.x=static_cast<float>(args.x);p.y=static_cast<float>(args.y);p.z=0;
    p.offset_x=0;p.offset_y=0;p.offset_z=0;p.parent=nullptr;e.add(0,&p);
    auto& sprite=host.acquire_sprite();sprite.animation={args.resource,args.clip,args.frame};
    sprite.layers=std::make_shared<std::vector<std::int32_t>>(1,-1);e.add(2,&sprite);
    return add_button(e,args.properties,host);
}
void initialize_buttons(ButtonFrameState& state,ButtonInitializationWorld* world,ButtonFrameHost& host){
    auto& target=required(world);state.subset=target.button_subset({17,0,2});
    target.subscribe_button_touch([&state,&host](const InputTouch* e){button_touch(state,e,host);});
}
void update_buttons(ButtonFrameState& state,ButtonFrameHost& host){
    {
        auto cursor=required(state.subset).cursor();
        while(const auto entry=cursor.next()){
            auto& e=required(*entry);auto* parent=button(e).parent;
            if(!parent||host.button_parent_destroyed(*parent)||!parent->contains(0))continue;
            auto* own=dynamic_cast<ContactPosition*>(e.get(0));
            if(e.get(0)&&!own)throw std::invalid_argument("incorrect button Position type");
            const auto xx=position(*parent).x,xz=position(*parent).z;
            const auto x=host.project_button_x(xx,xz);required(own).x=x;
            const auto yx=position(*parent).x,yz=position(*parent).z;
            const auto projected=host.project_button_y(yx,yz);
            const auto y=projected-position(*parent).y;own->y=y;
            auto& form=required(host.button_camera_form());const auto camera_x=form.x,camera_y=form.y;
            const auto shifted_x=own->x-static_cast<float>(camera_x),shifted_y=y-static_cast<float>(camera_y);
            own->x=shifted_x;own->y=shifted_y;
            if(!button(e).dynamic){
                const auto unscaled_x=own->x;
                const auto scale=required(host.button_camera_form()).state.scale;
                const float product_x=unscaled_x*static_cast<float>(scale),product_y=own->y*static_cast<float>(scale);
                own->x=product_x/100.0F;own->y=product_y/100.0F;
            }
        }
    }
    // Public Where(autoDestroy).ToArray: finish selection before any destruction callback.
    // autoDestroy is the complete native predicate, including buttons with a live/null parent.
    std::vector<ComponentSlots*> removed;
    {auto cursor=required(state.subset).cursor();while(const auto entry=cursor.next()){
        auto& e=required(*entry);if(button(e).auto_destroy)removed.push_back(&e);
    }}
    for(auto* e:removed)host.destroy_button(required(e),true);
}
void button_touch(ButtonFrameState& state,const InputTouch* event,ButtonFrameHost& host){
    const auto& touch=required(event);const auto control=required(touch.component);
    const auto id=control.group,value=control.value;
    auto cursor=required(state.subset).cursor();ComponentSlots* selected=nullptr;
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);if(button(e).id==id&&button(e).value==value){selected=&e;break;}
    }
    if(selected)button(*selected).hover=touch.type==13;
    host.send_button_event(event,selected); // Always send, including a null match.
}
}
