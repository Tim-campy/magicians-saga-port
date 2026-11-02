#include "town_background.hpp"
#include "decoded_image.hpp"
#include "core/gameplay/render_frame.hpp"
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
void json_text(std::ostream& out,const FormText& text){
    if(!text){out<<"null";return;}
    out<<'"';constexpr char hex[]="0123456789abcdef";
    for(const auto raw:*text){const auto c=static_cast<unsigned char>(raw);
        if(c=='"'||c=='\\')out<<'\\'<<raw;else if(c<32)out<<"\\u00"<<hex[c>>4]<<hex[c&15];else out<<raw;}
    out<<'"';
}
}
TownBackground::DrawingArchive::DrawingArchive(const GameArchiveEntry& entry,ResourceProfile profile)
    :archive(read_game_archive(entry.path),std::move(profile)),manager(archive),images(manager,decode_png){}
TownBackground::TownBackground(ResourceArchive& tables,const GameArchiveEntry& selected,ResourceProfile profile,std::int32_t capacity,TownBackgroundContext context,EventScript buildings,std::shared_ptr<std::vector<std::uint8_t>> enabled,const GameArchiveEntry* background,const GameArchiveEntry* common)
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
    if(context_.buildings){
        if(!context_.scenery||!buildings||!enabled)throw std::invalid_argument("town buildings require scenery and native cache/permissions");
        building_cache_=std::move(buildings);building_enabled_=std::move(enabled);
        const auto text=[&](const char* name){const auto data=tables.data(name);
            if(!data)throw std::invalid_argument(std::string("town buildings require ")+name);
            return std::string(reinterpret_cast<const char*>(data->bytes.data()),data->bytes.size());};
        building_records_=load_building_table(text("Building.txt"),false,parse_event_decimal);
        building_views_.resize(building_records_.size());
        for(std::size_t i=0;i<building_records_.size();++i)if(building_records_[i]){
            const auto& row=*building_records_[i];
            building_views_[i]=std::make_shared<TownBuildingView>(TownBuildingView{row.image,row.type,std::make_shared<const std::string>(row.name)});
        }
        auto rewards=load_king_reward_table(text("KingReward.txt"),false,parse_event_decimal);
        king_rewards_->resize(rewards.size());
        for(std::size_t i=0;i<rewards.size();++i)if(rewards[i]){
            reset_king_reward_for_new_game(*rewards[i]);king_rewards_->at(i)=std::make_shared<KingRewardDefinition>(std::move(*rewards[i]));
        }
        characters_=load_character_progression_table(text("Chara.txt"),false,parse_event_decimal);
        character_states_.resize(characters_.size());
        for(std::size_t i=0;i<characters_.size();++i)if(characters_[i]){
            character_states_[i]=std::make_shared<CharacterNewGameState>();reset_character_for_new_game(*character_states_[i],*characters_[i]);
        }
        image_clocks_=archive_.image_animation_infos();
    }
    if(context_.areas){
        if(context_.automatic_images)throw std::invalid_argument("automatic town render clocks are unbound; explicit --automatic-images 0 is supported");
        if(!context_.buildings||!background||!common)throw std::invalid_argument("town areas require buildings, bg0 and com2 resources; regenerate --town-resources");
        area_views_->resize(areas_.size());area_marks_=std::make_shared<EventIntegers>(areas_.size(),0); // Init +0x200.
        for(std::size_t i=0;i<areas_.size();++i)if(areas_[i])area_views_->at(i)=std::make_shared<SelectAreaDrawingRecord>(SelectAreaDrawingRecord{areas_[i]->image,progress_[i]->discovery_state,std::make_shared<const std::string>(areas_[i]->name)});
        area_background_=std::make_unique<DrawingArchive>(*background,archive_.selected_profile());
        common_=std::make_unique<DrawingArchive>(*common,archive_.selected_profile());
        textures_.bind(area_background_->images);textures_.bind(common_->images);
    }
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
    backend_=&backend;draws_.clear();scenery_draws_.clear();floor_draws_.clear();floor_font_=0;
    fonts_.clear();font_requests_.clear();chip_draws_.clear();labels_.clear();touches_.clear();notices_->clear();king_queries_=0;
    area_draws_.clear();area_labels_.clear();area_touches_.clear();area_color_requests_=0;
    const auto before=backend.quads().size();
    try{
        const auto origin=draw_select_background({camera_x_,camera_y_,camera_.left_extent,camera_.right_extent,context_.view_y,&context_.view_x},*this);
        if(context_.scenery){
            draw_select_decorations(scenery_,0,origin,*this);
            draw_select_terrain(scenery_,origin,*this);
            draw_select_floor(scenery_,origin,*this);
            if(context_.buildings)draw_select_town_buildings({town_frame_,selected_chip_,context_.top_form,notices_},origin,*this);
            if(context_.areas)draw_select_town_areas({town_frame_,selected_area_,form_state_,context_.top_form,area_marks_,notices_},origin,*this);
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
std::shared_ptr<std::vector<EventObject>> TownBackground::town_occupants(){
    // GetLivingSubMemberArray 0x116a20c: indexed address projection, later rows win.
    auto result=std::make_shared<std::vector<EventObject>>(static_cast<std::size_t>(town_count()));
    for(std::size_t i=0;i<characters_.size();++i)if(characters_[i]&&(characters_[i]->flags&4)&&character_states_[i]&&character_states_[i]->living_address!=-1)
        result->at(static_cast<std::size_t>(character_states_[i]->living_address))=character_states_[i];
    return result;
}
std::int32_t TownBackground::town_clip(std::int32_t field){
    switch(field){case 0x168:return 1;case 0x188:return 9;case 0x190:return 11;case 0x1a8:return 17;}
    throw std::invalid_argument("unsupported town building selector");
}
void TownBackground::town_draw_chip(SelectBackgroundResource resource,std::int32_t clip,std::int32_t frame,std::int32_t image,std::int32_t x,std::int32_t y){
    chip_draws_.push_back({clip,frame,image,x,y});
    backend_->draw_seb_image(static_cast<RenderDrawResource*>(resource),clip,frame,x,y,100,17,image_clocks_.get(),image);
}
void TownBackground::town_push_font(std::int32_t value){fonts_.push_back(value);font_requests_.push_back({1,value});}
void TownBackground::town_pop_font(){
    if(fonts_.empty())throw std::logic_error("unbalanced town font scope");
    font_requests_.push_back({-1,fonts_.back()});fonts_.pop_back();
}
void TownBackground::town_label(FormText text,std::int32_t x,std::int32_t y){
    // SAGA_STUB(town-building-text): retain native anchor34/font request; glyph output is unbound.
    labels_.push_back({std::move(text),x,y});
}
void TownBackground::town_touch(std::int32_t slot,bool selected,std::int32_t x,std::int32_t y){
    // SAGA_STUB(town-building-touch): retain native requests; interactive hit registration is unbound.
    touches_.push_back({slot,selected?1:0,x,y});
}
void TownBackground::area_draw(std::int32_t resource,std::int32_t clip,std::optional<std::int32_t> frame,std::int32_t x,std::int32_t y){
    area_draws_.push_back({resource,clip,frame,x,y});
    auto* manager=resource==0?&area_background_->manager.drawing():resource==11?&manager_.drawing():nullptr;
    if(!manager)throw std::invalid_argument("unsupported town area resource");
    if(frame)backend_->draw_seb_frame(manager,clip,*frame,static_cast<float>(x),static_cast<float>(y));
    else{
        // Seb.PushThisFrameRender 0x1bb3790 returns false without mutation when
        // the explicit AutoAnimation gate is off. Its true branch remains unbound.
        backend_->draw_seb(resource,clip,std::nullopt,x,y);
    }
}
std::int32_t TownBackground::area_clip(std::int32_t field){
    switch(field){case 0x18c:return 10;case 0x194:return 12;case 0x188:return 9;case 0x1a8:return 17;}
    throw std::invalid_argument("unsupported town area selector");
}
std::int32_t TownBackground::area_clip_frames(std::int32_t resource,std::int32_t clip){
    auto& animations=resource==2?common_->manager.animations():resource==11?manager_.animations():throw std::invalid_argument("unsupported town area animation resource");
    return animations.clips->at(static_cast<std::size_t>(clip))->max_frame;
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
        out<<"],\"floor\":{\"font\":"<<floor_font_<<",\"strips\":[";
        for(std::size_t i=0;i<floor_draws_.size();++i){if(i)out<<',';const auto& d=floor_draws_[i];out<<'['<<d.frame<<','<<d.x<<','<<d.y<<']';}
        out<<"]}}";
    }
    if(context_.buildings){
        out<<",\"buildings_draw\":{\"frame\":"<<town_frame_<<",\"selected\":"<<selected_chip_
           <<",\"provisional_context\":true,\"top_form\":"<<(context_.top_form?"true":"false")
           <<",\"high_score\":"<<context_.high_score<<",\"building_slots\":"<<building_records_.size()
           <<",\"character_slots\":"<<characters_.size()<<",\"king_reward_slots\":"<<king_rewards_->size()<<",\"king_queries\":"<<king_queries_
           <<",\"font_requests\":[";
        for(std::size_t i=0;i<font_requests_.size();++i){if(i)out<<',';out<<'['<<font_requests_[i][0]<<','<<font_requests_[i][1]<<']';}
        out<<"],\"chips\":[";
        for(std::size_t i=0;i<chip_draws_.size();++i){if(i)out<<',';const auto& d=chip_draws_[i];out<<'['<<d.clip<<','<<d.frame<<','<<d.image<<','<<d.x<<','<<d.y<<']';}
        out<<"],\"notices\":[";
        for(std::size_t i=0;i<notices_->size();++i){if(i)out<<',';out<<'[';const auto& row=*notices_->at(i);for(std::size_t j=0;j<row.size();++j){if(j)out<<',';out<<row[j];}out<<']';}
        out<<"],\"labels\":[";
        for(std::size_t i=0;i<labels_.size();++i){if(i)out<<',';const auto& label=labels_[i];out<<"{\"text\":";
            json_text(out,label.text);
            out<<",\"x\":"<<label.x<<",\"y\":"<<label.y<<",\"anchor\":34,\"font\":7}";}
        out<<"],\"touches\":[";
        for(std::size_t i=0;i<touches_.size();++i){if(i)out<<',';const auto& row=touches_[i];out<<'['<<row[0]<<','<<row[1]<<','<<row[2]<<','<<row[3]<<']';}
        out<<"]}";
    }
    if(context_.areas){
        out<<",\"areas_draw\":{\"selected\":"<<selected_area_<<",\"form_state\":"<<form_state_<<",\"font\":10,\"automatic_images\":false,\"provisional_clock_gate\":true,\"color_source\":944,\"color_requests\":"<<area_color_requests_<<",\"marks\":[";
        for(std::size_t i=0;i<area_marks_->size();++i){if(i)out<<',';out<<area_marks_->at(i);}out<<"],\"draws\":[";
        for(std::size_t i=0;i<area_draws_.size();++i){if(i)out<<',';const auto& d=area_draws_[i];out<<'['<<d.resource<<','<<d.clip<<',';
            if(d.frame)out<<*d.frame;else out<<"null";out<<','<<d.x<<','<<d.y<<']';}out<<"],\"labels\":[";
        for(std::size_t i=0;i<area_labels_.size();++i){if(i)out<<',';const auto& d=area_labels_[i];out<<"{\"text\":";json_text(out,d.text);out<<",\"x\":"<<d.x<<",\"y\":"<<d.y<<",\"anchor\":34,\"font\":10}";}
        out<<"],\"touches\":[";
        for(std::size_t i=0;i<area_touches_.size();++i){if(i)out<<',';out<<'[';const auto& d=area_touches_[i];for(std::size_t j=0;j<d.size();++j){if(j)out<<',';out<<d[j];}out<<']';}out<<"]}";
    }
    out<<'}';
}
} // namespace saga::desktop
