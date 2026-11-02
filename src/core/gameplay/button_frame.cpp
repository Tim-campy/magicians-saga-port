#include "button_frame.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing entity-button data");return *p;}
EntityButton& button(ComponentSlots& e){return required(dynamic_cast<EntityButton*>(e.get(17)));}
ContactPosition& position(ComponentSlots& e){return required(dynamic_cast<ContactPosition*>(e.get(0)));}
EntitySprite& sprite(ComponentSlots& e){return required(dynamic_cast<EntitySprite*>(e.get(2)));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t scaled(std::int32_t factor,std::int32_t value){return multiply(factor,value)/100;}
void require_context(const void* p){if(!p)throw std::invalid_argument("missing button drawing context");}
}
ButtonDrawMetrics ButtonDrawingHost::button_draw_metrics(){throw std::logic_error("unbound button drawing metrics");}
ButtonDrawMetrics ButtonDrawingHost::button_touch_metrics(){throw std::logic_error("unbound button touch metrics");}
void ButtonDrawingHost::button_black_color(){throw std::logic_error("unbound button color");}
ButtonApplicationContext ButtonDrawingHost::capture_button_application(){throw std::logic_error("unbound button application");}
ButtonResourceContext ButtonDrawingHost::button_resource(ButtonApplicationContext,std::int32_t){throw std::logic_error("unbound button resource");}
ButtonSebContext ButtonDrawingHost::button_seb(ButtonResourceContext,std::int32_t){throw std::logic_error("unbound button Seb");}
std::array<std::int32_t,4> ButtonDrawingHost::button_bounds(ButtonSebContext,std::int32_t){throw std::logic_error("unbound button bounds");}
void ButtonDrawingHost::button_draw_sprite(ButtonResourceContext,std::int32_t,std::int32_t,float,float){throw std::logic_error("unbound button sprite");}
void ButtonDrawingHost::button_draw_scaled_sprite(ButtonResourceContext,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t){throw std::logic_error("unbound scaled button sprite");}
void ButtonDrawingHost::button_draw_seb_ex(ButtonResourceContext,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t){throw std::logic_error("unbound button SebEx");}
void ButtonDrawingHost::button_render_mode(std::int32_t,std::int32_t,std::int32_t){throw std::logic_error("unbound button render mode");}
void ButtonDrawingHost::button_reset_render_mode(){throw std::logic_error("unbound button render reset");}
void ButtonDrawingHost::button_draw_text(ButtonText,float,float,std::int32_t){throw std::logic_error("unbound button text");}
void ButtonDrawingHost::button_add_touch(std::int32_t,InputRectangle,std::int32_t){throw std::logic_error("unbound rectangular button touch");}
void ButtonDrawingHost::button_add_touch_seb(std::int32_t,std::int32_t,std::int32_t,ButtonSebContext,std::int32_t,std::int32_t,ButtonTouchOption){throw std::logic_error("unbound Seb button touch");}
void ButtonDrawingHost::button_add_touch_delay(std::int32_t,std::int32_t,std::int32_t,ButtonSebContext,std::int32_t,std::int32_t,ButtonTouchOption){throw std::logic_error("unbound delayed button touch");}
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
void draw_buttons(ButtonFrameState& state,bool dynamic,ButtonDrawingHost& host){
    const auto metrics=host.button_draw_metrics();
    const auto view_scale=factory_cell_coordinate(metrics.view_scale,1);
    host.button_black_color();
    auto cursor=required(state.subset).cursor();
    const auto factor=dynamic?scaled(metrics.quest_scale,view_scale):100;
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);if(button(e).dynamic!=dynamic||e.contains(21))continue;
        auto* captured=dynamic_cast<EntityButton*>(e.get(17));
        const auto app=host.capture_button_application();
        const auto resource=sprite(e).animation.resource;require_context(app);
        const auto manager=host.button_resource(app,resource);require_context(manager);
        const auto seb=host.button_seb(manager,sprite(e).animation.clip);
        const auto bounds_frame=sprite(e).animation.frame;require_context(seb);
        const auto bounds=host.button_bounds(seb,bounds_frame);
        const auto px=position(e).x;auto& b=required(captured);
        const auto x=add(add(factory_cell_coordinate(px,1),bounds[0]),b.offset_x);
        const auto py=position(e).y;
        const auto y=add(add(factory_cell_coordinate(py,1),bounds[1]),b.offset_y);
        if(b.scale==100){
            if(b.hover)host.button_render_mode(1,255,255);
            const auto clip=sprite(e).animation.clip,frame=sprite(e).animation.frame;
            host.button_draw_sprite(manager,clip,frame,static_cast<float>(x),static_cast<float>(y));
            if(b.hover)host.button_reset_render_mode(); // Fresh hover read, no exceptional cleanup.
        }else{
            const auto clip=sprite(e).animation.clip,frame=sprite(e).animation.frame;
            host.button_draw_scaled_sprite(manager,clip,frame,x,y,b.scale);
        }
        const ButtonText text=b.text;
        if(text){
            const auto tx=add(add(x,bounds[2]/2),b.text_offset_x),ty=add(add(y,bounds[3]/2),b.text_offset_y);
            host.button_draw_text(text,static_cast<float>(tx),static_cast<float>(ty),34);
        }
        const auto tx=scaled(factor,x),ty=scaled(factor,y);
        if(dynamic){
            const auto id=b.id,frame=sprite(e).animation.frame,value=b.value;
            host.button_add_touch_delay(id,tx,ty,seb,frame,value,nullptr);
        }else{
            const auto scale=b.scale,id=b.id,value=b.value;
            host.button_add_touch(id,{tx,ty,scaled(scale,scaled(factor,bounds[2])),scaled(scale,scaled(factor,bounds[3]))},value);
        }
    }
}
void draw_button(const ButtonDrawRequest& input,ButtonDrawingHost& host){
    const auto args=input;
    const auto app=host.capture_button_application();require_context(app);
    const auto manager=host.button_resource(app,args.resource);require_context(manager);
    const auto seb=host.button_seb(manager,args.clip);require_context(seb);
    const auto bounds=host.button_bounds(seb,args.frame);
    const auto x=sub(args.x,(args.anchor&2)!=0?bounds[2]/2:(args.anchor&4)!=0?bounds[2]:0);
    const auto y=sub(args.y,(args.anchor&32)!=0?bounds[3]/2:(args.anchor&64)!=0?bounds[3]:0);
    host.button_draw_seb_ex(manager,args.clip,args.frame,x,y,100,17);
    if(args.dynamic){
        const auto metrics=host.button_touch_metrics();
        const float product_x=metrics.view_scale*static_cast<float>(multiply(metrics.quest_scale,x));
        const float product_y=metrics.view_scale*static_cast<float>(multiply(metrics.quest_scale,y));
        const auto tx=factory_cell_coordinate(product_x/10000.0F,1),ty=factory_cell_coordinate(product_y/10000.0F,1);
        host.button_add_touch_delay(args.id,tx,ty,seb,args.frame,args.value,args.option);
    }else host.button_add_touch_seb(args.id,x,y,seb,args.frame,args.value,args.option);
}
}
