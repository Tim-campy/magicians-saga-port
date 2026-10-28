#include "bitmap_geometry.hpp"
#include "entity_visual.hpp"
#include <cmath>
#include <utility>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing bitmap geometry input");return *p;}
float positive_origin(float x){return std::isnan(x)?x:(x>0.0F?x:0.0F);} // ARM FMAX(x,+0), NaN propagates.
}
BitmapSource prepare_bitmap_source(PixelImage& image,BitmapDraw draw,const BitmapAtlasRegion* atlas,BitmapAlphaHost& host){
    if(image.passes==0||image.frames){
        if(image.standard_width>=1){const float scale=static_cast<float>(image.width)/static_cast<float>(image.standard_width);draw.u=draw.u*scale;draw.source_width=draw.source_width*scale;}
        if(image.standard_height>=1){const float scale=static_cast<float>(image.height)/static_cast<float>(image.standard_height);draw.v=draw.v*scale;draw.source_height=draw.source_height*scale;}
    }
    BitmapSource result{&image,draw,{factory_cell_coordinate(draw.u+0.1F,1),factory_cell_coordinate(draw.v+0.1F,1),
                                    factory_cell_coordinate(draw.source_width+0.1F,1),factory_cell_coordinate(draw.source_height+0.1F,1)}};
    if(atlas&&atlas->active){
        auto* page=required(atlas->owner).page;
        const auto u=atlas->u,v=atlas->v;
        const auto alpha=host.alpha_type(image);
        result.page=&required(page);
        result.draw.u=result.draw.u+static_cast<float>(u);
        result.draw.v=result.draw.v+static_cast<float>(v);
        host.set_alpha_type(*page,alpha);
    }
    return result;
}
BitmapSamplingResult update_bitmap_sampling(BitmapSamplingCache& cache,PixelImage& image,BitmapDraw& draw,
                                           ImageBounds& work,BitmapTexture& texture,BitmapAlphaHost& host){
    const float end_u=draw.u+draw.source_width,end_v=draw.v+draw.source_height;
    bool changed=cache.dirty||cache.image!=&image;
    bool same=false;
    if(!changed){same=cache.alpha_type==host.alpha_type(image);changed=!same;}
    cache.image=&image;cache.alpha_type=host.alpha_type(image);
    if(changed)cache.dirty=true;
    auto& key=required(cache.crop.get());
    if(!changed&&key.at(0)==draw.u&&key.at(1)==draw.v&&key.at(2)==draw.source_width&&key.at(3)==draw.source_height)
        return {same,false};
    key.at(0)=draw.u;key.at(1)=draw.v;key.at(2)=draw.source_width;key.at(3)=draw.source_height;
    const float left=positive_origin(draw.u),top=positive_origin(draw.v);
    float width=draw.u<0.0F?end_u:draw.source_width;
    float height=draw.v<0.0F?end_v:draw.source_height;
    const float image_width=static_cast<float>(image.width),image_height=static_cast<float>(image.height);
    if(left+width>image_width)width=image_width-left;
    if(top+height>image_height)height=image_height-top;
    draw.source_width=width;draw.source_height=height;
    cache.valid=width>0.0F&&height>0.0F;
    if(!cache.valid)return {same,true};
    work={left+0.005F,top+0.005F,(left+width)+(-0.01F),(top+height)+(-0.01F)};
    const float crop_width=work.right-work.left;
    const auto width1=texture.width();
    const float crop_height=work.bottom-work.top;
    const auto height1=texture.height();
    const float crop_left=work.left;
    const auto width2=texture.width();
    const auto height2=texture.height();
    const float fresh_height=work.bottom-work.top,crop_top=work.top;
    const auto height3=texture.height();
    auto& uv=required(cache.uv.get());
    const float normalized_width=crop_width/static_cast<float>(width1),normalized_height=crop_height/static_cast<float>(height1);
    const float u=crop_left/static_cast<float>(width2);
    const float v=(static_cast<float>(height2)-fresh_height-crop_top)/static_cast<float>(height3);
    uv={u,v,u+normalized_width,v+normalized_height};
    return {same,true};
}
ImageBounds bitmap_destination(const BitmapDraw& draw,float expansion){
    const float half=expansion*0.5F;
    return {draw.x-half,draw.y-half,(draw.x+draw.width)+half,(draw.y+draw.height)+half};
}
BitmapPoints transform_bitmap_rectangle(BitmapGeometryState& state,float x,float y,float width,float height,
                                       BitmapPoints points,bool transform,const ImageBounds* fill_bounds,BitmapGeometryHost& host){
    if(!points)points=std::make_shared<std::vector<BitmapPoint>>(4);
    // Sequential stores precede bounds checks for the next corner, as in native.
    points->at(0).x=x;points->at(0).y=y;
    const float right=x+width;
    points->at(1).x=right;points->at(1).y=y;
    const float bottom=y+height;
    points->at(2).x=right;points->at(2).y=bottom;
    points->at(3).x=x;points->at(3).y=bottom;
    auto* mode=required(state.mesh_modes.get()).at(static_cast<std::size_t>(state.mesh_index));
    float surface_height=0.0F;
    if(required(mode).use_surface_height){
        if(state.surface)surface_height=static_cast<float>(state.surface->height);
        else if(host.render_target_alive())surface_height=static_cast<float>(host.render_target_height());
        else surface_height=static_cast<float>(host.screen_height());
    }
    for(std::size_t i=0;i<points->size();++i){
        auto& p=points->at(i);p.z=0.0F;float fill_x=p.x,fill_y=p.y;
        if(fill_bounds){
            fill_x=fill_bounds->left;fill_y=fill_bounds->top;
            if(i==1||i==2)fill_x=fill_x+(fill_bounds->right-fill_bounds->left);
            if((i&~std::size_t{1})==2)fill_y=fill_y+(fill_bounds->bottom-fill_bounds->top);
        }
        if(transform)host.screen_transform(p.x,p.y);
        p.y=surface_height-p.y;
        if(state.begin_fill&&state.call_draw_bitmap){
            const auto replacement=host.fill_point(fill_x,fill_y);
            if(replacement){p.x=(*replacement)[0];p.y=(*replacement)[1];}
            else host.add_fill_point(fill_x,fill_y,p.x,p.y);
        }
    }
    return points;
}
BitmapPoints transform_bitmap_rectangle(BitmapGeometryState& state,const ImageBounds* rectangle,BitmapPoints points,
                                       const ImageBounds* fill_bounds,BitmapGeometryHost& host){
    const auto& r=required(rectangle);
    return transform_bitmap_rectangle(state,r.left,r.top,r.right-r.left,r.bottom-r.top,std::move(points),true,fill_bounds,host);
}
} // namespace saga::core
