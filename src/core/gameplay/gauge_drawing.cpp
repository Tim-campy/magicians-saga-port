#include "gauge_drawing.hpp"
#include "ecs_combat.hpp"
#include "entity_visual.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing gauge input");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t mul(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
}
GaugePoint anchor_gauge(std::int32_t x,std::int32_t y,std::int32_t w,std::int32_t h,std::int32_t anchor){
    const auto dx=(anchor&2)!=0?w/2:(anchor&4)!=0?w:0;
    const auto dy=(anchor&32)!=0?h/2:(anchor&64)!=0?h:0;
    return {sub(x,dx),sub(y,dy)};
}
std::int32_t gauge_image_offset(GaugeDrawState& state,PixelImage* input,GaugeDrawingHost& host){
    auto& image=required(input);
    const auto width=std::min(image.width,30),y=add(image.height/2,image.height/4);
    state.alpha=host.gauge_alpha(image,0,y,width,2,state.alpha);
    auto& alpha=required(state.alpha.get());
    std::size_t i=0;while(i<alpha.size()&&alpha[i]==0.0F)++i;
    return static_cast<std::int32_t>(i);
}
void draw_gauge(GaugeDrawState& state,RenderGraphics* graphics,std::int32_t x,std::int32_t y,
                std::int32_t id,std::int32_t rate,std::int32_t scale,GaugeDrawingHost& host){
    auto* image=host.gauge_image(id);
    const auto offset=gauge_image_offset(state,image,host);
    const auto width=required(image).width,height=image->height/2;
    const auto inset=mul(offset,scale)/100;
    const float fraction=static_cast<float>(std::clamp(rate,0,100))/100.0F;
    const float fill=fraction*static_cast<float>(sub(image->width,mul(inset,2)));
    const auto fill_width=factory_cell_coordinate(fill,1);
    const auto background_width=mul(width,scale)/100,product=mul(fill_width,scale);
    const auto output_width=product<=99&&rate>=1?1:product/100;
    auto& g=required(graphics);
    RenderImageDraw draw;draw.image=image;draw.scaled=true;draw.x=static_cast<float>(x);draw.y=static_cast<float>(y);
    draw.width=static_cast<float>(background_width);draw.height=static_cast<float>(height);draw.source_width=width;draw.source_height=height;
    g.draw_image(draw);
    draw.x=static_cast<float>(add(x,inset));draw.width=static_cast<float>(output_width);
    draw.u=inset;draw.v=height;draw.source_width=fill_width;
    g.draw_image(draw);
}
void draw_parameter_gauge(GaugeDrawState& state,RenderGraphics* graphics,const ParameterGaugeRequest& r,GaugeDrawingHost& host){
    auto& e=required(r.entity);
    if(!e.contains(23))return;
    auto& links=required(dynamic_cast<CombatParameterLinks*>(e.get(23)));
    if(!required(links.values).count(r.parameter))return;
    auto* parameter=resolve_combat_parameter(e,r.parameter);
    if(!r.force){const auto& p=required(parameter);if(add(p.value,p.extra)<1)return;}
    auto& image=required(host.gauge_image(r.image));const auto width=image.width,height=image.height/2;
    const auto view_width=host.gauge_view_width(),view_height=host.gauge_view_height();
    const auto p=anchor_gauge(r.x,r.y,width,height,r.anchor);
    // CheckHitRect includes touching edges and wraps each endpoint addition.
    if(add(p.x,width)<0||view_width<p.x||view_height<p.y||add(p.y,height)<0)return;
    const auto& captured=required(parameter);
    const auto scale=easing_zero_amplitude(50,150,1000,add(captured.maximum,captured.extra_maximum));
    const auto rate=damage_parameter_rate(required(parameter));
    draw_gauge(state,graphics,p.x,p.y,r.image,rate,scale,host);
}
void draw_enemy_gauges(EntitySubset& group,EnemyGaugeDrawHost& host){
    auto cursor=group.cursor();
    while(const auto entity=cursor.next()){
        auto& e=required(*entity);const auto& data=required(host.gauge_enemy_record(e));
        if((data.flags&1)!=0||host.gauge_enemy_defeats(data)<6)continue;
        auto* raw=e.get(0);auto* position=dynamic_cast<ContactPosition*>(raw);
        if(raw&&!position)throw std::invalid_argument("invalid gauge position type");
        const auto p=host.gauge_screen_position(position);
        const auto image=host.enemy_gauge_palette(data.attribute);
        host.parameter_gauge({&e,p.x,add(p.y,3),image,0,2,false});
    }
}
} // namespace saga::core
