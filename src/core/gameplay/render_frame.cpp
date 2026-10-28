#include "render_frame.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing render frame input");return *p;}
template<class T>T* optional(ComponentSlots& e,std::int32_t slot){
    auto* raw=e.get(slot);auto* value=dynamic_cast<T*>(raw);
    if(raw&&!value)throw std::invalid_argument("incorrect render component type");return value;
}
template<class T>T& field(ComponentSlots& e,std::int32_t slot){return required(optional<T>(e,slot));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t subtract(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t convert(float value){return factory_cell_coordinate(value,1);}
std::int32_t override_id(const ImageArray& array,std::int32_t layer){
    auto& values=required(array.get());
    if(values.size()==1)return values.at(0);
    if(layer>=static_cast<std::int32_t>(values.size()))return -1;
    return values.at(static_cast<std::size_t>(layer));
}
std::int32_t override_rect(const ImageArray& array,std::int32_t layer,std::int32_t original){
    auto& values=required(array.get());
    if(values.size()==1&&values.at(0)!=-1)return values.at(0);
    if(layer>=static_cast<std::int32_t>(values.size()))return original;
    const auto value=values.at(static_cast<std::size_t>(layer));return value==-1?original:value;
}
}
void initialize_render(RenderSystemState& state,RenderInitializationWorld* world){
    state.map=required(world).render_map();state.layers=std::make_shared<EventIntegers>(32,0);
}
float render_isometric_x(float x,float z){return x-z;}
float render_isometric_y(float x,float z){const float sum=x+z;return sum*0.5F;}
std::int32_t pack_render_color(std::int32_t r,std::int32_t g,std::int32_t b){
    return word(0xff000000U|(static_cast<std::uint32_t>(std::clamp(r,0,255))<<16U)|
                (static_cast<std::uint32_t>(std::clamp(g,0,255))<<8U)|static_cast<std::uint32_t>(std::clamp(b,0,255)));
}
void update_render(RenderSystemState& state,QuestCameraHost& viewport,RenderFrameHost& host){
    auto& form=required(viewport.current_camera_form());const auto camera_x=form.x,camera_y=form.y;
    quest_game_width(&form,viewport);quest_game_height(viewport.current_camera_form(),viewport); // Discarded, outside loop catch.
    ComponentSlots* current=nullptr;
    try{
        auto cursor=host.visible_render_cursor();
        while(const auto entry=required(cursor.get()).next()){
            current=*entry;auto& e=required(current);if(e.contains(21)||!e.contains(0))continue;
            auto& position=field<ContactPosition>(e,0);required(host.current_render_queue());
            float projected=render_isometric_x(position.x,position.z);auto x=convert(projected);
            required(host.current_render_queue());projected=render_isometric_y(position.x,position.z);projected=projected-position.y;
            auto y=convert(projected);
            if(e.contains(5)){
                field<CellComponent>(e,5);field<CellComponent>(e,5);
                const float overlay=e.contains(18)||e.contains(19)?9999999.0F:0.0F;
                const auto extra=field<EntityDepth>(e,4).extra,rate=host.render_extra_depth_rate();
                auto& destination=field<EntityDepth>(e,4);
                const auto scaled=word(static_cast<std::uint32_t>(rate)*static_cast<std::uint32_t>(extra))/100;
                float depth=position.y+position.x;depth=position.z+depth;depth=depth+static_cast<float>(scaled);depth=overlay+depth;
                destination.value=convert(depth);
            }
            auto* application=host.current_render_queue();const auto resource_id=field<EntitySprite>(e,2).animation.resource;
            auto* resource=host.render_resource(required(application),resource_id);auto clips=required(resource).clips;
            const auto clip_id=field<EntitySprite>(e,2).animation.clip;
            auto* clip=required(clips.get()).at(static_cast<std::size_t>(clip_id));
            const auto frame=e.contains(13)?required(clip).frame:field<EntitySprite>(e,2).animation.frame;
            auto original_layers=field<EntitySprite>(e,2).layers;auto& listed=required(original_layers.get());
            auto count=static_cast<std::int32_t>(listed.size());auto selected=original_layers;
            if(count==1&&listed.at(0)==-1){
                count=host.render_layer_count(required(clip));
                if(count>0){auto captured=state.layers;for(std::int32_t i=0;i<count;++i)required(captured.get()).at(static_cast<std::size_t>(i))=i;}
                selected=state.layers;
            }
            auto* image=optional<EntityImage>(e,7);
            for(std::int32_t i=0;i<count;++i){
                const auto layer=required(selected.get()).at(static_cast<std::size_t>(i));if(layer==-1)continue;
                auto sprite=host.render_sprite(required(clip),frame,layer);auto& bounds=required(sprite.get());
                auto u=bounds.at(2),v=bounds.at(3),width=bounds.at(4),height=bounds.at(5);
                std::int32_t texture=-1,image_resource=-1;
                if(image){
                    texture=override_id(image->arrays.textures,layer);image_resource=override_id(image->arrays.resources,layer);
                    if(image_resource==-2)continue;
                    u=override_rect(image->arrays.u,layer,u);v=override_rect(image->arrays.v,layer,v);
                    width=override_rect(image->arrays.width,layer,width);height=override_rect(image->arrays.height,layer,height);
                    if(texture==-2)continue;
                }
                auto* color=optional<DefeatColor>(e,6);auto alpha=color?color->values[3]:255;
                std::int32_t scaled_width=width,scaled_height=height,angle=0,skew=0;
                if(e.contains(15)){
                    auto& mod=field<ProjectileModification>(e,15);
                    scaled_width=convert(mod.scale_x*static_cast<float>(width));scaled_height=convert(mod.scale_y*static_cast<float>(height));
                    angle=mod.angle;x=add(x,convert(mod.x));
                    const auto width_change=subtract(width,scaled_width),height_change=subtract(height,scaled_height);
                    if((static_cast<std::uint32_t>(mod.anchor)&2U)!=0)x=add(x,width_change/2);
                    y=add(add(y,convert(mod.y)),height_change);
                    if((static_cast<std::uint32_t>(mod.anchor)&32U)!=0)y=subtract(y,height_change/2);
                    if(mod.alpha<=254)alpha=add(mod.alpha,1000);
                    if(mod.type==9)skew=mod.angle;
                }
                auto* output=host.current_render_queue();const auto res=field<EntitySprite>(e,2).animation.resource;
                const auto depth=field<EntityDepth>(e,4).value,output_clip=field<EntitySprite>(e,2).animation.clip;
                color=optional<DefeatColor>(e,6);const auto rgb=color?pack_render_color(color->values[0],color->values[1],color->values[2]):-1;
                RenderCommand command;command.values={texture,res,add(depth,layer),output_clip,frame,subtract(x,camera_x),subtract(y,camera_y),
                    scaled_width,scaled_height,u,v,width,height,layer,alpha,rgb,angle,image_resource,skew};
                insert_render_command(required(output),command);
            }
        }
    }catch(const std::exception& error){if(current)host.render_error(*current,error);}
    sort_render_queue(required(host.current_render_queue())); // Outside the loop catch, even after a failed entity.
}
}
