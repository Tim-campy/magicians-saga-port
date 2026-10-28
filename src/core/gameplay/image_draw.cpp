#include "image_draw.hpp"
#include "entity_visual.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing image drawing input");return *p;}
template<class T>T& at(const std::shared_ptr<std::vector<T>>& p,std::int32_t i){return required(p.get()).at(static_cast<std::size_t>(i));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t mul(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t divide(std::int32_t a,std::int32_t b){if(!b)return 0;if(a==std::numeric_limits<std::int32_t>::min()&&b==-1)return a;return a/b;}
std::int32_t integer(float v){return factory_cell_coordinate(v,1);}
void accounting(ImageDrawState& s,PixelImage* image,const SpriteArray& p){
    auto& stats=required(s.statistics);
    if(stats.enabled){stats.pieces=add(stats.pieces,1);stats.texels=add(stats.texels,mul(at(p,5),at(p,6)));
        auto& source=required(image);if(source.frame_mark!=stats.frame)stats.image_texels=add(stats.image_texels,mul(source.width,source.height));
    }
    required(image).frame_mark=stats.frame;
}
bool clip_axis(float clip,float size,float& position,float& extent,std::int32_t& source,float& source_extent,bool reverse){
    const float original_position=position,original_extent=extent,original_source=static_cast<float>(source),original_source_extent=source_extent;
    const float left=clip>position?clip:position;
    const float clip_end=clip+size,request_end=position+extent;
    const float right=clip_end<request_end?clip_end:request_end;
    float start=left-original_position;start=start/original_extent;start=start*original_source_extent;start=start+original_source;
    float end=right-original_position;end=end/original_extent;end=end*original_source_extent;end=end+original_source;
    source=integer(start);source_extent=end-start;
    if(reverse){float adjusted=original_source+original_source_extent;adjusted=adjusted+original_source;
        const float stop=static_cast<float>(source)+source_extent;source=integer(adjusted-stop);}
    position=left;extent=right-left;
    // ARM FCMP/B.LS accepts unordered values here; do not change to !(value>0).
    return !(extent<=0.0F||source_extent<=0.0F);
}
void emit_piece(ImageDrawState& s,PixelImage& image,const SpriteArray& p,ImageRect request,
                std::int32_t dx,std::int32_t dy,float scale_x,float scale_y,ImageDrawHost& host){
    PixelImage* source=&image;
    if(at(p,0)!=-1){source=at(image.sources,at(p,0));if(source)host.check_use(*source);}
    auto bounds=s.bounds;auto& r=required(bounds.get());r={request.x,request.y,request.x+request.width,request.y+request.height};
    float u=static_cast<float>(dx),v=static_cast<float>(dy),sw=request.source_width,sh=request.source_height;
    if(dx<0){sw=sw+u;r.left=r.left-scale_x*u;u=0;}
    float end=sw+u;const float pw=static_cast<float>(at(p,5));
    if(end>pw){r.right=r.right-scale_x*(end-pw);sw=pw-u;}
    if(dy<0){sh=sh+v;r.top=r.top-scale_y*v;v=0;}
    end=sh+v;const float ph=static_cast<float>(at(p,6));
    if(end>ph){r.bottom=r.bottom-scale_y*(end-ph);sh=ph-v;}
    const bool local=s.local_matrix_index>=0||s.local_coordinate_count>0;
    if(local)host.push_translation(r.left-request.x,r.top-request.y);
    auto emit=[&]{if(source){BitmapDraw draw{local?request.x:r.left,local?request.y:r.top,r.right-r.left,r.bottom-r.top,
                    u+static_cast<float>(at(p,3)),v+static_cast<float>(at(p,4)),sw,sh};host.bitmap(*source,draw);}};
    if(local){try{emit();}catch(...){host.pop_matrix();throw;}host.pop_matrix();}else emit();
    accounting(s,source,p); // Even a null external source reaches native mark/accounting guards.
}
void render(ImageDrawState& s,PixelImage& image,ImageRect request,std::int32_t u,std::int32_t v,ImageDrawHost& host){
    if(image.passes==0){host.bitmap(image,{request.x,request.y,request.width,request.height,static_cast<float>(u),static_cast<float>(v),request.source_width,request.source_height});return;}
    if(image.frames){
        const float scale_x=request.width/request.source_width,scale_y=request.height/request.source_height;
        auto frames=image.frames;
        for(std::size_t i=0;i<frames->size();++i){const auto& box=frames->at(i);
            if(box.u!=u||box.v!=v||static_cast<float>(box.width)!=request.source_width||static_cast<float>(box.height)!=request.source_height)continue;
            const auto frame=static_cast<std::int32_t>(i);host.use_frame(image,frame);
            for(std::int32_t pass=0;pass<image.passes;pass=add(pass,1)){
                auto p=host.frame_piece(image,frame,pass);if(p)emit_piece(s,image,p,request,sub(0,at(p,1)),sub(0,at(p,2)),scale_x,scale_y,host);
            }
            break;
        }
        return;
    }
    if(image.standard_width>=1){const auto size=mul(image.columns,image.step_x);
        const float scale=static_cast<float>(size)/static_cast<float>(image.standard_width);
        u=integer(scale*static_cast<float>(u));request.source_width=static_cast<float>(integer(request.source_width*scale));}
    if(image.standard_height>=1){const auto size=mul(image.rows,image.step_y);
        const float scale=static_cast<float>(size)/static_cast<float>(image.standard_height);
        v=integer(scale*static_cast<float>(v));request.source_height=static_cast<float>(integer(request.source_height*scale));}
    const auto row=divide(v,image.step_y);if(row<0)return;
    const auto column=divide(u,image.step_x);if(column<0||row>=image.rows||column>=image.columns||image.passes<1)return;
    const float scale_x=request.width/request.source_width,scale_y=request.height/request.source_height;
    for(std::int32_t pass=0;pass<image.passes;pass=add(pass,1)){
        auto p=host.piece(image,column,row,pass);
        if(p){const auto dx=sub(sub(u,mul(column,image.step_x)),at(p,1)),dy=sub(sub(v,mul(row,image.step_y)),at(p,2));
            emit_piece(s,image,p,request,dx,dy,scale_x,scale_y,host);}
    }
}
}
PreparedOptImage::PreparedOptImage(const assets::OptGrid& grid):grid_(grid){
    columns=grid.columns;rows=grid.rows;step_x=grid.cell_width;step_y=grid.cell_height;passes=1;
    for(const auto& cell:grid.cells)passes=std::max(passes,static_cast<std::int32_t>(cell.piece_count));
}
SpriteArray PreparedOptImage::piece(std::int32_t column,std::int32_t row,std::int32_t pass,SpriteArray result)const{
    const auto index=add(mul(add(mul(columns,row),column),passes),pass);
    if(index<0||index>=mul(mul(columns,rows),passes))throw std::out_of_range("OPT piece slot outside image");
    const auto cell=static_cast<std::size_t>(index/passes),inside=static_cast<std::size_t>(index%passes);
    const auto& c=grid_.cells.at(cell);if(inside>=c.piece_count)return nullptr;
    const auto& p=grid_.pieces.at(static_cast<std::size_t>(c.first_piece)+inside).values;
    for(std::int32_t i=0;i<7;++i)at(result,i)=p.at(static_cast<std::size_t>(i));return result;
}
void draw_image_internal(ImageDrawState& s,PixelImage* image,ImageRect request,ImageDrawHost& host){
    if(!image)return;
    if(!s.culling_disabled&&s.local_matrix_index<0&&s.local_coordinate_count<=0&&!host.culling_overlap(s.culling,request))return;
    const bool sample=host.begin_sample();
    try{
        host.check_use(*image);
        const bool horizontal=!std::isnan(at(s.clip,0))&&!std::isnan(at(s.clip,2));
        const bool vertical=!std::isnan(at(s.clip,1))&&!std::isnan(at(s.clip,3));
        auto u=request.u,v=request.v;
        bool visible=true;
        if((horizontal||vertical)&&image->passes!=0&&image->frames){auto& stats=required(s.statistics);
            if(!stats.clip_warning_shown&&stats.debug){host.clipping_unavailable();stats.clip_warning_shown=true;}}
        else{
            if(horizontal)visible=clip_axis(at(s.clip,0),at(s.clip,2),request.x,request.width,u,request.source_width,(static_cast<std::uint32_t>(s.flip)&1U)!=0);
            if(visible&&vertical)visible=clip_axis(at(s.clip,1),at(s.clip,3),request.y,request.height,v,request.source_height,(static_cast<std::uint32_t>(s.flip)&2U)!=0);
        }
        if(visible){
            if(s.flip!=0){const float x=request.source_width*0.5F,y=request.source_height*0.5F;
                if(s.flip==4||s.flip==5)host.push_rotation(-90.0F,x,y);
                else host.push_scale((static_cast<std::uint32_t>(s.flip)&1U)?-1.0F:1.0F,(static_cast<std::uint32_t>(s.flip)&2U)?-1.0F:1.0F,x,y);
            }
            try{render(s,*image,request,u,v,host);}catch(...){if(s.flip!=0)host.pop_matrix();throw;}
            if(s.flip!=0)host.pop_matrix();
        }
    }catch(...){host.end_sample(sample);throw;}
    host.end_sample(sample);
}
void draw_image_rect(ImageDrawState& s,PixelImage* image,float x,float y,std::int32_t u,std::int32_t v,std::int32_t width,std::int32_t height,ImageDrawHost& host){
    if(!image){if(required(s.statistics).debug)host.null_image_debug();return;}
    draw_image_internal(s,image,{x,y,static_cast<float>(width),static_cast<float>(height),u,v,static_cast<float>(width),static_cast<float>(height)},host);
}
void draw_scaled_image_rect(ImageDrawState& s,PixelImage* image,float x,float y,float width,float height,std::int32_t u,std::int32_t v,std::int32_t source_width,std::int32_t source_height,ImageDrawHost& host){
    if(!image){if(required(s.statistics).debug)host.null_image_debug();return;}
    draw_image_internal(s,image,{x,y,width,height,u,v,static_cast<float>(source_width),static_cast<float>(source_height)},host);
}
} // namespace saga::core
