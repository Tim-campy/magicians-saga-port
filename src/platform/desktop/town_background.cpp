#include "town_background.hpp"
#include "decoded_image.hpp"
#include "core/gameplay/render_frame.hpp"
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace saga::desktop {
using namespace core;
TownBackground::TownBackground(ResourceArchive& tables,const GameArchiveEntry& selected,ResourceProfile profile,std::int32_t capacity,TownBackgroundContext context)
    :camera_{false,0,0,camera_x_,camera_y_,0,0},context_(context),
     app_random_(context.app_seed),lib_random_(context.lib_seed),
     camera_areas_(std::make_shared<CameraAreas>()),save_integers_(std::make_shared<EventIntegers>(20)),
     archive_(read_game_archive(selected.path),std::move(profile)),manager_(archive_),images_(manager_,decode_png){
    if(context.width<1||context.height<2||context.width>8192||context.height>8192||static_cast<std::uint64_t>(context.width)*context.height>16U*1024U*1024U)
        throw std::invalid_argument("invalid town viewport");
    if(context.view_scale!=100.0F||context.form_scale!=100)throw std::invalid_argument("town background projection requires scale100");
    if(selected.kind!="resource"||selected.id!=11||selected.name!="select")throw std::invalid_argument("town background requires native select resource11");
    const auto table=tables.data("Area.txt");if(!table)throw std::invalid_argument("town background requires Area.txt");
    areas_=load_area_table(std::string_view(reinterpret_cast<const char*>(table->bytes.data()),table->bytes.size()),false,parse_event_decimal);
    progress_.resize(areas_.size());camera_areas_->resize(areas_.size());
    for(std::size_t i=0;i<areas_.size();++i)if(areas_[i]){
        progress_[i].emplace();reset_area_for_new_game(*progress_[i],*areas_[i]);
        // Read-only projection for this drawing subsequence; no town events run here.
        camera_areas_->at(i)=std::make_shared<CameraAreaRecord>(CameraAreaRecord{progress_[i]->discovery_state});
    }
    // Only slot19 is consumed by SelectActiveAreaHost; this is not a full save array.
    save_integers_->at(19)=capacity;
    set_select_active_x(camera_,*this);
    set_select_camera_position(camera_,context.camera_request_x,context.camera_request_y,*this);
    application_.selection=&manager_.drawing();textures_.bind(images_);
    solid_.pixels={1,1,{0,0,0,255}};solid_.image=std::make_unique<PixelImage>();
    solid_.image->width=solid_.image->height=solid_.image->standard_width=solid_.image->standard_height=1;
    textures_.bind(solid_);
    if(context_.scenery){initialize_select_scenery_clouds(scenery_,*this);initialize_select_decorations(scenery_,*this);}
}
std::int32_t TownBackground::view_width(){return scaled_view_dimension(context_.width,context_.view_scale);}
std::int32_t TownBackground::view_height(){return scaled_view_dimension(context_.height,context_.view_scale);}
std::int32_t TownBackground::game_width(){return scaled_select_dimension(view_width(),context_.form_scale);}
std::int32_t TownBackground::game_height(){return scaled_select_dimension(view_height(),context_.form_scale);}
std::array<float,3> TownBackground::background_pixel(){
    // RESSELECT_IMG_BG=9, cctor 0x11192c8. Public decoded RGBA8 sampling
    // supplies the GetPixel service (native GetPixel flips Unity's bottom-up Y).
    // SAGA_STUB(town-pixel-colorspace): original texture color-space/atlas policy is unverified.
    const auto& pixels=images_.at(9).pixels;
    if(pixels.rgba.size()<4)throw std::invalid_argument("missing town background pixel");
    return {pixels.rgba[0]/255.0F,pixels.rgba[1]/255.0F,pixels.rgba[2]/255.0F};
}
void TownBackground::background_color(std::int32_t r,std::int32_t g,std::int32_t b,std::int32_t a){
    if(a!=255)throw std::logic_error("town prefix requires opaque Color alpha");
    // MyFormBase.Color -> Graphics.GetColorOfRGB RGBA clamps every channel.
    // With the native literal alpha255, reuse the existing RGB packing contract.
    color_=static_cast<std::uint32_t>(pack_render_color(r,g,b));
    solid_.pixels.rgba={static_cast<std::uint8_t>(color_>>16),static_cast<std::uint8_t>(color_>>8),static_cast<std::uint8_t>(color_),255};
}
void TownBackground::fill_background(std::int32_t width,std::int32_t height){
    fill_={width,height};backend_->draw_bitmap(*solid_.image,0,0,static_cast<float>(width),static_cast<float>(height));
}
void TownBackground::draw_background(SelectBackgroundResource resource,std::int32_t clip,std::int32_t frame,float x,float y){
    draws_.push_back({clip,frame,x,y});backend_->draw_seb_frame(static_cast<RenderDrawResource*>(resource),clip,frame,x,y);
}
void TownBackground::draw(BitmapBackend& backend){
    backend_=&backend;draws_.clear();scenery_draws_.clear();floor_draws_.clear();floor_layer_=0;const auto before=backend.quads().size();
    try{
        const auto origin=draw_select_background({camera_x_,camera_y_,camera_.left_extent,camera_.right_extent,context_.view_y,&context_.view_x},*this);
        if(context_.scenery){
            draw_select_decorations(scenery_,0,origin,*this);
            draw_select_terrain(scenery_,origin,*this);
            draw_select_floor(scenery_,origin,*this);
        }
    }
    catch(...){backend_=nullptr;throw;}
    quads_=backend.quads().size()-before;backend_=nullptr;
}
std::int32_t TownBackground::scenery_clip(std::int32_t field){
    // AppData..cctor: BG0, GROUND4, CLOUD14, BIRD22 and LEAF23.
    switch(field){case 0x164:return 0;case 0x174:return 4;case 0x19c:return 14;case 0x1bc:return 22;case 0x1c0:return 23;}
    throw std::invalid_argument("unsupported town scenery selector");
}
std::array<std::int32_t,4> TownBackground::scenery_bounds(std::int32_t clip,std::int32_t frame){
    auto* clock=manager_.animations().clips->at(static_cast<std::size_t>(clip));
    if(!clock||!clock->sprites)throw std::invalid_argument("missing town scenery bounds");
    return seb_bounding_rect(*clock->sprites,frame);
}
void TownBackground::scenery_draw(SelectBackgroundResource resource,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t anchor){
    scenery_draws_.push_back({clip,frame,x,y,-1,anchor,backend_->graphics().reverse});
    if(anchor)throw std::logic_error("unexpected terrain anchor");
    backend_->draw_seb_frame(static_cast<RenderDrawResource*>(resource),clip,frame,x,y);
}
void TownBackground::scenery_draw_indexed(SelectBackgroundResource resource,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t layer,std::int32_t anchor){
    scenery_draws_.push_back({clip,frame,x,y,layer,anchor,backend_->graphics().reverse});
    backend_->draw_seb_anchor_frame(static_cast<RenderDrawResource*>(resource),clip,frame,x,y,anchor,layer);
}
SebSpriteState* TownBackground::floor_sprite(){
    auto* clock=manager_.animations().clips->at(2);return clock?clock->sprites:nullptr; // RESSELECT_SEB_FLOOR.
}
void TownBackground::floor_draw(SebSpriteState* sprite,RenderImage* image,std::int32_t frame,std::int32_t x,std::int32_t y){
    floor_draws_.push_back({frame,x,y});
    draw_fixed_image_seb(&backend_->graphics(),x,y,sprite,image,frame,floor_work_);
}
void TownBackground::gate_draw(SelectBackgroundResource resource,std::int32_t clip,std::int32_t frame,std::int32_t layer,float x,float y){
    scenery_draws_.push_back({clip,frame,x,y,layer,0,backend_->graphics().reverse});
    backend_->draw_seb_frame(static_cast<RenderDrawResource*>(resource),clip,frame,x,y,layer);
}
void TownBackground::report(std::ostream& out)const{
    out<<std::setprecision(std::numeric_limits<float>::max_digits10)
       <<",\"background\":{\"resource\":11,\"image\":9,\"clip\":0,\"provisional_context\":true,\"viewport\":["<<context_.width<<','<<context_.height
       <<"],\"view_scale\":"<<context_.view_scale<<",\"form_scale\":"<<context_.form_scale<<",\"view_origin\":["<<context_.view_x<<','<<context_.view_y
       <<"],\"projection\":["<<context_.projection.near_clip<<','<<context_.projection.far_clip<<','<<context_.projection.z
       <<"],\"depth_bits\":"<<context_.depth_bits<<",\"depth_step\":"<<(context_.depth_step?"true":"false")
       <<",\"camera\":["<<camera_x_<<','<<camera_y_<<"],\"extents\":["<<camera_.left_extent<<','<<camera_.right_extent<<"],\"area_discovery\":[";
    for(std::size_t i=0;i<progress_.size();++i){if(i)out<<',';if(progress_[i])out<<progress_[i]->discovery_state;else out<<"null";}
    out<<"],\"fill_color\":"<<color_<<",\"fill_size\":["<<fill_[0]<<','<<fill_[1]<<"],\"quads\":"<<quads_<<",\"draws\":[";
    for(std::size_t i=0;i<draws_.size();++i){if(i)out<<',';const auto& d=draws_[i];out<<'['<<d.clip<<','<<d.frame<<','<<d.x<<','<<d.y<<']';}
    out<<']';
    if(context_.scenery){
        out<<",\"scenery\":{\"provisional_rng\":true,\"app_seed\":"<<context_.app_seed<<",\"lib_seed\":"<<context_.lib_seed
           <<",\"app_draws\":"<<app_random_.draws<<",\"lib_draws\":"<<lib_random_.draws;
        const auto rows=[&](const char* name,const EventScript& values){out<<",\""<<name<<"\":[";
            for(std::size_t i=0;i<values->size();++i){if(i)out<<',';out<<'[';const auto& row=*values->at(i);
                for(std::size_t j=0;j<row.size();++j){if(j)out<<',';out<<row[j];}out<<']';}out<<']';};
        rows("clouds",scenery_.clouds);rows("birds",scenery_.birds);rows("leaves",scenery_.leaves);
        out<<",\"draws\":[";
        for(std::size_t i=0;i<scenery_draws_.size();++i){if(i)out<<',';const auto& d=scenery_draws_[i];
            out<<'['<<d.clip<<','<<d.frame<<','<<d.x<<','<<d.y<<','<<d.layer<<','<<d.anchor<<','<<d.reverse<<']';}
        out<<"],\"floor\":{\"depth_layer\":"<<floor_layer_<<",\"strips\":[";
        for(std::size_t i=0;i<floor_draws_.size();++i){if(i)out<<',';const auto& d=floor_draws_[i];out<<'['<<d.frame<<','<<d.x<<','<<d.y<<']';}
        out<<"]}}";
    }
    out<<'}';
}
} // namespace saga::desktop
