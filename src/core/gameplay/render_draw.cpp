#include "render_draw.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing render drawing input");return *p;}
template<class T>T& at(const std::shared_ptr<std::vector<T>>& p,std::int32_t i){return required(p.get()).at(static_cast<std::size_t>(i));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t negate(std::int32_t a){return word(0U-static_cast<std::uint32_t>(a));}
bool in_range(std::int32_t value,std::int32_t first,std::uint32_t last_offset){return static_cast<std::uint32_t>(value)-static_cast<std::uint32_t>(first)<=last_offset;}
bool allows_texture_override(std::int32_t resource){return resource>=0&&resource<=23&&(0x00fc008eU&(1U<<static_cast<unsigned>(resource)))!=0;}
std::int32_t dimension(const EventInstruction& row,const SpriteArray& sprite,std::int32_t field,std::int32_t fallback){
    const auto value=at(row,field);return value==-1?at(sprite,fallback):value;
}
}
void draw_render_queue(RenderDrawState& state,RenderGraphics* target,std::int32_t type,RenderDrawHost& host){
    state.phase=add(state.phase,1)%20;
    auto& graphics=required(target);auto* mesh=graphics.mesh_manager();
    std::int32_t index=0;
    while(index<at(required(state.queue).counts,type)){
        auto row=at(at(required(state.queue).buffers,type),index);
        const auto depth=std::max(at(row,render_depth),0);auto& manager=required(mesh);manager.group=depth/150000;
        manager.set_depth(depth%150000,0);
        const auto texture=at(row,render_texture),resource_id=at(row,render_resource);
        auto* resource=host.resource(resource_id);
        if(resource){
            auto clips=required(resource->animations).clips;
            auto* clip=at(clips,at(row,render_clip));
            auto sprite=host.sprite(required(clip),at(row,render_frame),at(row,render_layer));
            if(sprite&&at(sprite,1)!=-1){
                if(at(sprite,8)==1)graphics.reverse=at(sprite,9)==1?3:1;
                else if(at(sprite,9)==1)graphics.reverse=2;
                RenderImage* image=nullptr;
                if(texture!=-1&&allows_texture_override(resource_id)){
                    if(at(row,render_image_resource)!=-1)resource=host.global_resource(at(row,render_image_resource));
                    if(texture>=0&&texture<static_cast<std::int32_t>(required(required(resource).images.get()).size()))
                        image=at(required(resource).images,texture);
                    else host.invalid_image_override(resource_id,texture,at(row,render_clip),at(row,render_layer));
                }else image=at(resource->images,at(sprite,1));
                if(!image)host.missing_image(resource_id,at(sprite,1));
                const auto width=dimension(row,sprite,render_width,4),height=dimension(row,sprite,render_height,5);
                const auto u=dimension(row,sprite,render_u,2),v=dimension(row,sprite,render_v,3);
                const auto source_width=dimension(row,sprite,render_source_width,4),source_height=dimension(row,sprite,render_source_height,5);
                const auto alpha=at(row,render_alpha);
                if(alpha!=255){
                    const auto mode=alpha/1000,remainder=alpha%1000;
                    if(in_range(alpha,-999,1998))graphics.push_render_mode(mode,remainder,0);
                    else if(in_range(alpha,1000,999)||in_range(alpha,2000,999))graphics.push_render_mode(mode,remainder,255-remainder);
                    else if(in_range(alpha,3000,999))graphics.push_render_mode(1,255,remainder);
                }
                static_cast<void>(at(sprite,11)); // Native validates the unused sprite Color slot here.
                if(at(row,render_color)!=-1)graphics.push_blend_mode(2,at(row,render_color));
                const auto angle=at(row,render_angle);
                if(angle!=0){
                    if(at(row,render_skew)!=0){
                        const float tangent=host.transform_tangent(static_cast<float>(angle));
                        // Ordinary 3x3 identity initializer, metadata field24286/default_data172368.
                        std::array<float,9> values{1,0,0,0,1,0,0,0,1};values[1]=tangent;
                        required(state.matrix).set_values(values);
                        const float offset=tangent*static_cast<float>(negate(source_width));
                        required(state.matrix).pre_translate(0.0F,offset*0.5F);
                    }else{
                        auto& matrix=required(state.matrix);
                        const auto x=negate(at(sprite,6)),y=negate(at(sprite,7));
                        matrix.set_rotation(static_cast<float>(angle),static_cast<float>(x),static_cast<float>(y));
                    }
                    graphics.push_matrix(required(state.matrix));
                }
                RenderImageDraw draw;draw.image=image;draw.u=u;draw.v=v;draw.source_width=source_width;draw.source_height=source_height;
                draw.scaled=width!=source_width||height!=source_height;
                draw.x=static_cast<float>(add(at(row,render_x),at(sprite,6)));draw.y=static_cast<float>(add(at(row,render_y),at(sprite,7)));
                draw.width=static_cast<float>(width);draw.height=static_cast<float>(height);graphics.draw_image(draw);
                if(at(row,render_angle)!=0){required(state.matrix).reset();graphics.pop_matrix();}
                if(at(row,render_color)!=-1)graphics.pop_blend_mode();
                if(at(row,render_alpha)!=255)graphics.pop_render_mode();
                if(at(sprite,8)==1||at(sprite,9)==1)graphics.reverse=0;
            }
        }
        index=add(index,1);
    }
}
} // namespace saga::core
