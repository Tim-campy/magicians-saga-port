#include "render_colors.hpp"
#include <stdexcept>
#include <utility>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing render color input");return *p;}
template<class T>T& at(const std::shared_ptr<std::vector<T>>& p,std::int32_t i){return required(p.get()).at(static_cast<std::size_t>(i));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
void dirty(RenderColorState& s){required(s.cache_dirty)=true;}
void select_colors(SpriteRenderPass& p,RenderColors colors,RenderColorWork& work){
    const auto size=required(colors.get()).size();
    if(size==0)p.colors=work.empty_fallback;
    else if(size==1){p.colors=p.local;at(p.local,0)=at(colors,0);}
    else p.colors=std::move(colors);
    const auto n=required(p.colors.get()).size();
    auto found=work.outputs.find(n);
    if(found==work.outputs.end())found=work.outputs.emplace(n,std::make_shared<std::vector<RenderColor>>(n)).first;
    p.output=found->second;
}
void prepare_pass_cache(RenderColorWork& work){
    if(work.passes)return;
    work.passes=std::make_shared<std::vector<SpriteRenderPasses>>(5);
    for(std::int32_t n=0;n<5;++n){
        auto list=std::make_shared<std::vector<std::shared_ptr<SpriteRenderPass>>>(static_cast<std::size_t>(n));
        at(work.passes,n)=list;
        for(auto& p:*list)p=std::make_shared<SpriteRenderPass>();
    }
}
}
// Graphics.SetRenderMode(mode,source,destination), 0x1b5d96c.
void set_render_mode(RenderColorState& s,std::int32_t mode,std::int32_t source,std::int32_t destination){
    if(mode==1&&add(source,destination)==256)destination=sub(destination,1);
    s.mode=mode;s.source=source;s.destination=destination;
    s.replace=(mode==0&&source>254)||(mode==1&&source>254&&destination==0);dirty(s);
}
// Graphics.PushRenderMode(), 0x1b71c54; row storage uses standard containers.
void save_render_mode(RenderColorState& s){
    auto list=s.renders;
    if(s.render_count>=static_cast<std::int64_t>(required(list.get()).size()))list->push_back(std::make_shared<std::vector<std::int32_t>>(3));
    auto row=at(s.renders,s.render_count);at(row,0)=s.mode;at(row,1)=s.source;at(row,2)=s.destination;
    s.render_count=add(s.render_count,1);
}
// Graphics.PushRenderMode(mode,source,destination), 0x1b6cf70.
void push_render_mode(RenderColorState& s,std::int32_t m,std::int32_t a,std::int32_t b){save_render_mode(s);set_render_mode(s,m,a,b);}
// Graphics.PopRenderMode, 0x1b6d8d4.
void pop_render_mode(RenderColorState& s){
    const auto i=sub(s.render_count,1);if(i<0)return;auto row=at(s.renders,i);
    s.render_count=sub(s.render_count,1);
    const auto m=at(row,0),a=at(row,1),b=at(row,2);set_render_mode(s,m,a,b);
}
// Graphics.SetBlendMode(mode,packed), 0x1b71b10.
void set_blend_mode(RenderColorState& s,std::int32_t mode,std::int32_t packed){
    if(static_cast<std::uint32_t>(mode)>3U)throw std::invalid_argument("render blend mode outside 0..3");
    s.blend_mode=mode;s.blend_color=packed;const auto value=static_cast<std::uint32_t>(packed);
    s.blend={static_cast<float>((value>>16)&255U)/255.0F,static_cast<float>((value>>8)&255U)/255.0F,
             static_cast<float>(value&255U)/255.0F,static_cast<float>(value>>24)/255.0F};dirty(s);
}
// Graphics.PushBlendMode(), 0x1b72ea0.
void save_blend_mode(RenderColorState& s){
    auto list=s.blends;
    if(s.blend_count>=static_cast<std::int64_t>(required(list.get()).size()))list->push_back(std::make_shared<std::vector<std::int32_t>>(2));
    auto row=at(s.blends,s.blend_count);at(row,0)=s.blend_mode;at(row,1)=s.blend_color;s.blend_count=add(s.blend_count,1);
}
// Graphics.PushBlendMode(mode,packed), 0x1b72fd8.
void push_blend_mode(RenderColorState& s,std::int32_t m,std::int32_t c){save_blend_mode(s);set_blend_mode(s,m,c);}
// Graphics.PopBlendMode, 0x1b730dc.
void pop_blend_mode(RenderColorState& s){
    const auto i=sub(s.blend_count,1);if(i<0)return;auto row=at(s.blends,i);
    const auto m=at(row,0),c=at(row,1);set_blend_mode(s,m,c);s.blend_count=sub(s.blend_count,1);
}
// RenderPass.Init(Graphics,mode,source,destination,rgb,alpha,colors), 0x1b7ca94.
void initialize_render_pass(SpriteRenderPass& p,RenderColorState* owner,std::int32_t mode,std::int32_t source,
                            std::int32_t destination,std::int32_t rgb,std::int32_t alpha,RenderColors colors,RenderColorWork& work){
    p.owner=owner;p.mode=mode;p.source=source;p.destination=destination;
    required(colors.get());p.material=nullptr;p.restore_blend=false;p.alpha_test=-1;p.rgb_factor=rgb;p.alpha_factor=alpha;
    select_colors(p,std::move(colors),work);
}
// RenderPass.Init(Graphics,material,colors), 0x1b7c8ac; material stays opaque.
void initialize_material_pass(SpriteRenderPass& p,RenderColorState* owner,const void* material,RenderColors colors,RenderColorWork& work){
    p.owner=owner;p.mode=0;p.source=255;p.destination=0;p.material=material;p.restore_blend=false;p.alpha_test=-1;p.rgb_factor=255;p.alpha_factor=255;
    select_colors(p,std::move(colors),work);
}
// RenderPass.Copy, 0x1b7cc80.
void copy_render_pass(SpriteRenderPass& p,const SpriteRenderPass* source){
    const auto& q=required(source);p.owner=q.owner;p.mode=q.mode;p.source=q.source;p.destination=q.destination;
    p.colors=q.colors;p.material=q.material;p.alpha_test=q.alpha_test;p.restore_blend=q.restore_blend;
    p.rgb_factor=q.rgb_factor;p.alpha_factor=q.alpha_factor;p.output=q.output; // Private singleton work is not copied.
}
// Graphics.GetRenderPass(Image,Color[]), 0x1b62cf0.
SpriteRenderPasses get_render_passes(RenderColorState& s,bool image,RenderColors colors,RenderColorWork& work,RenderColorHost& host){
    prepare_pass_cache(work);auto alpha=image?host.image_alpha_type():-1;
    if(alpha==3&&s.is_gui&&!s.gl_render)alpha=-1;
    auto result=at(work.passes,1);
    if(s.forced_material){initialize_material_pass(required(at(result,0).get()),&s,s.forced_material,std::move(colors),work);return result;}
    const bool split=alpha==3&&(!s.is_gui||s.gl_render);
    if(split)result=at(work.passes,static_cast<std::int32_t>(required(result.get()).size()*2));
    std::int32_t count=1;
    if(!s.replace&&!s.figure_clip&&(s.mode==1||s.mode==2)){
        if(add(s.source,s.destination)==255){
            initialize_render_pass(required(at(result,0).get()),&s,s.mode,s.source,s.destination,255,s.source,colors,work);
        }else{
            count=(s.destination<255?1:0)+(s.source>=1?1:0);result=at(work.passes,split?count*2:count);count=0;
            if(s.destination<=254){initialize_render_pass(required(at(result,count).get()),&s,s.mode,0,s.destination,255,sub(255,s.destination),colors,work);++count;}
            if(s.source>=1){initialize_render_pass(required(at(result,count).get()),&s,s.mode,s.source,255,255,s.source,colors,work);++count;}
        }
    }else initialize_render_pass(required(at(result,0).get()),&s,-1,255,255,s.source,255,colors,work);
    if(alpha==3){
        if(!s.is_gui||s.gl_render){
            for(std::int32_t i=0;i<count;++i){auto& p=required(at(result,i).get());p.alpha_test=1;
                auto& duplicate=required(at(result,add(count,i)).get());copy_render_pass(duplicate,&p);duplicate.alpha_test=2;}
        }else required(at(result,0).get()).alpha_test=2;
    }
    return result;
}
// RenderPass._begin, 0x1b7ccdc; GUI operations are explicit host callbacks.
void begin_render_pass_state(SpriteRenderPass& p,RenderColorHost& host){
    auto& s=required(p.owner);
    if(host.render_gui_mode(s)){host.push_gui_matrix();host.load_gui_pixel_matrix();const float w=static_cast<float>(host.screen_width()),h=static_cast<float>(host.screen_height());host.gui_viewport(0,0,w,h);}
    if(p.material)return;
    if(p.mode!=-1)push_render_mode(s,p.mode,p.source,p.destination);
    if(p.source==0&&p.destination<=254){p.restore_blend=s.blend_mode==1;if(p.restore_blend)push_blend_mode(s,0,0);}
}
// RenderPass.Begin, 0x1b7cdb8.
RenderColors begin_render_pass(SpriteRenderPass& p,RenderColorHost& host){
    begin_render_pass_state(p,host);auto output=p.output;auto& values=required(output.get());
    for(std::size_t i=0;i<values.size();++i){
        const auto color=required(p.colors.get()).at(i);auto& out=values.at(i);
        out.a=(color.a*static_cast<float>(p.alpha_factor))/255.0F;
        out.r=(color.r*static_cast<float>(p.rgb_factor))/255.0F;out.g=(color.g*static_cast<float>(p.rgb_factor))/255.0F;out.b=(color.b*static_cast<float>(p.rgb_factor))/255.0F;
        auto& s=required(p.owner);
        if(s.blend_mode==2){out.r=out.r*(s.blend.r*s.blend.a);out.g=out.g*(s.blend.g*s.blend.a);out.b=out.b*(s.blend.b*s.blend.a);}
    }
    return output;
}
// RenderPass.End, 0x1b7d008.
void end_render_pass(SpriteRenderPass& p,RenderColorHost& host){
    auto& s=required(p.owner);if(host.render_gui_mode(s))host.pop_gui_matrix();if(p.material)return;
    if(p.mode!=-1)pop_render_mode(s);
    if(p.restore_blend)pop_blend_mode(s);
}
// Image._getAlphaType, 0x1b901e8: scalar slice; texture readback is not ported.
void ImageAlphaClassifier::add(float alpha){
    if(alpha<0.01F)transparent_=core::add(transparent_,1);
    else if(alpha<=0.999F)partial_=core::add(partial_,1); // ARM B.HI also skips unordered.
}
std::int32_t ImageAlphaClassifier::finish(std::int32_t count,std::int32_t previous)const{
    if(partial_>=1)return sub(count,partial_)==transparent_?2:3;
    return transparent_>=1?1:previous;
}
// ShaderBuilder.Init, 0x1b3e8b0: reached textured-sprite slice only.
SpriteShaderState sprite_shader_state(const RenderColorState& s,bool image,std::int32_t alpha,float a,RenderColors colors,std::int32_t test,bool gui){
    SpriteShaderState result;result.has_image=image;result.gui=gui;result.mode=s.mode;result.source=s.source;result.destination=s.destination;
    if(result.mode==1&&result.source>=255&&result.destination==0)result.mode=0;
    result.shape=s.figure_clip!=0;result.color_blend=!result.shape&&s.blend_mode==1;
    result.replace=!result.shape&&image&&a>=0.99F&&result.mode==0&&result.source>=255&&s.blend_mode==0;
    if(result.replace&&colors)for(const auto& c:*colors)if(c.r<1||c.g<1||c.b<1){result.replace=false;break;}
    result.alpha_type=image?alpha:-1;result.alpha_test=test;
    if(test==1||test==2)result.alpha_type=test;
    if(result.mode!=0)result.alpha_type=2;
    else if(a<0.01F){if(result.alpha_type==0)result.alpha_type=1;}
    else if(a<=0.99F&&result.alpha_type!=2)result.alpha_type=2;
    return result;
}
// ShaderBuilder.GetNameId, 0x1b3ed80.
std::uint32_t sprite_shader_key(const SpriteShaderState& s){
    const auto stencil=(s.stencil_write?0x40000U:0U)|(s.color_mask?0x20000U:0U)|(s.depth_only?0x80000U:0U);if(stencil)return stencil;
    std::uint32_t flags=(s.gui?1U:0U)|(s.has_image?16U:0U);
    if(s.shape)return flags|(s.invert_stencil?10U:2U);
    if(s.stencil)flags|=4U;
    if(s.color_blend)flags|=32U;
    if(s.replace)flags|=0x10000U;
    if(s.mode==1)flags|=add(s.source,s.destination)==255?0x2040U:(s.destination==255?0x4040U:0x8040U);
    else if(s.mode==2)flags|=add(s.source,s.destination)==255?0x2080U:(s.source>=1?0x4080U:0x8080U);
    else if(s.alpha_type==0)flags|=0x100U;
    if(s.gui)return flags;
    if(s.alpha_type==1)flags|=0x400U;else if(s.alpha_type==2)flags|=0x200U;
    if(s.alpha_test==1)flags|=0x800U;else if(s.alpha_test==2)flags|=0x1000U;return flags;
}
// Fixed-function states selected by ShaderBuilder.GetCode, 0x1b3eee8.
// Public SDL blending implements them; no shader source generator is copied.
SpritePixelPipeline sprite_pixel_pipeline(std::uint32_t key){
    if((key&0xe002fU)!=0||(key&16U)==0)throw std::invalid_argument("unbound sprite stencil/shape/add-color shader");
    SpritePixelPipeline p;p.modulate=(key&0x10000U)==0;
    if(key&0x40U)p.blend=(key&0x2000U)?SpriteBlend::alpha:((key&0x4000U)?SpriteBlend::add:SpriteBlend::attenuate);
    else if(key&0x80U)p.blend=(key&0x2000U)?SpriteBlend::subtract_alpha:((key&0x4000U)?SpriteBlend::subtract_add:SpriteBlend::attenuate);
    else p.blend=(key&0x100U)?SpriteBlend::replace:SpriteBlend::alpha;
    if(key&0x800U)p.alpha_test=SpriteAlphaTest::greater_099;else if(key&0x1000U)p.alpha_test=SpriteAlphaTest::at_most_099;else if(key&0x400U)p.alpha_test=SpriteAlphaTest::at_least_001;
    return p;
}
bool accepts_sprite_alpha(SpriteAlphaTest test,float a){
    switch(test){case SpriteAlphaTest::none:return true;case SpriteAlphaTest::at_least_001:return a>=0.01F;
        case SpriteAlphaTest::greater_099:return a>0.99F;case SpriteAlphaTest::at_most_099:return a<=0.99F;}return false;
}
} // namespace saga::core
