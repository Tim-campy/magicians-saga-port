#include "mission_profile.hpp"
#include "mission_spatial.hpp"
#include "core/animation/seb_sprite.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
template<class T>T& required(T* value){if(!value)throw std::invalid_argument("missing mission drawing input");return *value;}
template<class T>T& field(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
struct GaugeOutputSink final:RenderGraphics {
    [[noreturn]]static void unexpected(){throw std::logic_error("unexpected headless gauge graphics operation");}
    RenderDepth* mesh_manager()override{unexpected();}
    void push_render_mode(std::int32_t,std::int32_t,std::int32_t)override{unexpected();}
    void pop_render_mode()override{unexpected();}
    void push_blend_mode(std::int32_t,std::int32_t)override{unexpected();}
    void pop_blend_mode()override{unexpected();}
    void push_matrix(RenderMatrix&)override{unexpected();}
    void pop_matrix()override{unexpected();}
    void draw_image(const RenderImageDraw&)override{} // Intentional headless pixel sink; native gauge queries still execute.
};
}

void MissionProfile::draw_parameter_gauge_request(const ParameterGaugeRequest& request,BitmapBackend* backend){
    auto& camera=mission->gauge_camera_host();
    if(backend){backend->draw_parameter_gauge(request,camera,&gauge_scratch_);return;}
    struct Host final:GaugeDrawingHost {
        MissionProfile& owner;QuestCameraHost& camera;
        Host(MissionProfile& p,QuestCameraHost& c):owner(p),camera(c){}
        PixelImage* gauge_image(std::int32_t id)override{return owner.gauge_image(id);}
        std::int32_t gauge_view_width()override{return quest_game_width(camera.current_camera_form(),camera);}
        std::int32_t gauge_view_height()override{return quest_game_height(camera.current_camera_form(),camera);}
        std::shared_ptr<std::vector<float>> gauge_alpha(PixelImage& image,std::int32_t x,std::int32_t y,std::int32_t w,std::int32_t h,std::shared_ptr<std::vector<float>> alpha)override{
            return owner.gauge_alpha(image,x,y,w,h,std::move(alpha));
        }
    } host(*this,camera);
    GaugeOutputSink output;core::draw_parameter_gauge(gauge_scratch_,&output,request,host);
}

std::unique_ptr<MissionProfile::TextBitmap> MissionProfile::make_text_bitmap(const std::u16string& contents,std::int32_t request,std::uint32_t color){
    if(!events_||!events_->font_provider()||!events_->font_scenario())return nullptr;
    const auto& scenario=*events_->font_scenario();const std::vector<std::int32_t> presets{12,24,30};
    TextFontState font{0,0,true,0,1,scenario.requested_scale};
    set_text_font_size(font,request,true,&presets);update_text_font_scale(font,scenario.requested_scale,&presets);adjust_text_font_size(font,scenario.layout_scale);
    const auto pixels=text_gui_pixel_size(font.adjusted_size,scenario.application_ratio);
    auto raster=events_->font_provider()->rasterize_run(contents,pixels,scenario.load_flags,scenario.kerning_mode);
    auto measured=contents;std::replace(measured.begin(),measured.end(),u' ',u'.');
    auto result=std::make_unique<TextBitmap>();
    result->width=text_run_logical_width(events_->font_provider()->run_width(measured,pixels,scenario.load_flags,scenario.kerning_mode),scenario.application_ratio);
    result->ascent=raster.ascent;result->left=raster.left;result->top=raster.top;
    result->font_height=font.size;result->scale=100.0F/scenario.application_ratio;result->ready.pixels=std::move(raster.pixels);
    if(!result->ready.pixels.rgba.empty()){
        auto& ready=result->ready;ready.image=std::make_unique<PixelImage>();auto& image=*ready.image;
        image.width=image.standard_width=ready.pixels.width;image.height=image.standard_height=ready.pixels.height;
        ImageAlphaClassifier alpha;
        for(std::size_t i=0;i<ready.pixels.rgba.size();i+=4){
            ready.pixels.rgba[i]=static_cast<std::uint8_t>((color>>16)&255U);
            ready.pixels.rgba[i+1]=static_cast<std::uint8_t>((color>>8)&255U);ready.pixels.rgba[i+2]=static_cast<std::uint8_t>(color&255U);
            ready.pixels.rgba[i+3]=static_cast<std::uint8_t>((static_cast<std::uint32_t>(ready.pixels.rgba[i+3])*(color>>24)+127U)/255U);
            alpha.add(static_cast<float>(ready.pixels.rgba[i+3])/255.0F);
        }
        ready.alpha_type=alpha.finish(image.width*image.height);textures.bind(ready);
    }
    return result;
}

void MissionProfile::output_text(DebugText source,float x,float y,std::int32_t anchor,BitmapBackend* backend){
    const auto font=drawing_fonts_.back();std::u16string contents=source?*source:std::u16string{};
    const auto translated=drawing_translations_.find(contents);
    if(translated!=drawing_translations_.end())contents=translated->second;
    // SAGA_STUB(mission-drawing-translation): exact selected text-table entries only;
    // concatenated Japanese/Hindi/regex translation and alternate fonts remain external.
    const bool unsupported=std::any_of(contents.begin(),contents.end(),[](char16_t c){return c>127;});
    const auto key=std::make_tuple(contents,font,drawing_color_);TextBitmap* raster=nullptr;
    if(source&&!unsupported){
        auto it=drawing_bitmaps_.find(key);
        if(it==drawing_bitmaps_.end())it=drawing_bitmaps_.emplace(key,make_text_bitmap(contents,font,drawing_color_)).first;
        raster=it->second.get();
    }
    drawing_texts_.push_back({source?*source:std::u16string{},x,y,font,anchor,drawing_color_,raster!=nullptr});
    if(!raster||!backend||!raster->ready.image)return;
    if(anchor&2)x=x-raster->width*0.5F;else if(anchor&4)x=x-raster->width;
    if(anchor&32)y=y-static_cast<float>(raster->font_height)*0.5F;else if(anchor&64)y=y-static_cast<float>(raster->font_height);
    // Same explicit public face-ascender geometry policy as mission notices.
    x=x+static_cast<float>(raster->left)*raster->scale;y=y+(raster->ascent+static_cast<float>(raster->top))*raster->scale;
    backend->draw_bitmap(*raster->ready.image,x,y,static_cast<float>(raster->ready.pixels.width)*raster->scale,static_cast<float>(raster->ready.pixels.height)*raster->scale);
}

PixelImage& MissionProfile::solid_color(std::uint32_t color){
    if(const auto found=solid_colors_.find(color);found!=solid_colors_.end())return *found->second->image;
    auto ready=std::make_unique<ReadyArchiveImage>();ready->pixels={1,1,{static_cast<std::uint8_t>(color>>16),static_cast<std::uint8_t>(color>>8),static_cast<std::uint8_t>(color),static_cast<std::uint8_t>(color>>24)}};
    ready->image=std::make_unique<PixelImage>();ready->image->width=ready->image->height=1;
    ready->image->standard_width=ready->image->standard_height=1;ready->alpha_type=0;
    auto& image=*ready->image;textures.bind(*ready);solid_colors_.emplace(color,std::move(ready));return image;
}

void MissionProfile::load_debug_data(const std::filesystem::path& path){
    if(std::filesystem::file_size(path)>64*1024)throw std::length_error("debug metadata labels exceed64KiB");
    std::ifstream input(path);input.imbue(std::locale::classic());std::string tag;std::int32_t version=0,count=0;
    if(!(input>>tag>>version)||tag!="mission_debug"||version!=1)throw std::invalid_argument("invalid debug metadata label version");
    if(!(input>>tag>>count)||tag!="parameter_ids"||count!=13)throw std::invalid_argument("invalid debug parameter IDs");
    auto ids=std::make_shared<EventIntegers>(13);for(auto& id:*ids)if(!(input>>id))throw std::invalid_argument("short debug parameter IDs");
    const auto names=[&](const char* label,std::int32_t expected){
        if(!(input>>tag>>count)||tag!=label||count!=expected)throw std::invalid_argument("invalid debug name array");
        auto output=std::make_shared<std::vector<DebugText>>();output->reserve(static_cast<std::size_t>(count));
        for(std::int32_t i=0;i<count;++i){std::string text;if(!(input>>std::quoted(text)))throw std::invalid_argument("short debug name array");output->push_back(std::make_shared<const std::u16string>(decode_mission_text(text)));}
        return output;
    };
    auto parameters=names("parameter_names",13),humans=names("human_states",7),enemies=names("enemy_states",6),texts=names("texts",7);
    input>>std::ws;if(!input.eof())throw std::invalid_argument("trailing debug metadata labels");
    debug_parameter_ids_=std::move(ids);debug_parameter_names_=std::move(parameters);debug_human_states_=std::move(humans);debug_enemy_states_=std::move(enemies);debug_texts_=std::move(texts);
}

struct MissionProfile::DrawingHost final:ButtonDrawingHost,DebugDrawingHost {
    MissionProfile& p;BitmapBackend* backend;bool dynamic;
    DrawingHost(MissionProfile& owner,BitmapBackend* output,bool pass):p(owner),backend(output),dynamic(pass){}
    std::int32_t scale(){return required(p.mission->gauge_camera_host().current_camera_form()).state.scale;}
    ButtonDrawMetrics button_draw_metrics()override{const auto instance=p.instance_view_scale_;return {instance,scale()};}
    ButtonDrawMetrics button_touch_metrics()override{const auto quest=scale();return {p.base_view_scale_,quest};}
    void button_black_color()override{p.drawing_color_=0xff000000U;}
    ButtonApplicationContext capture_button_application()override{return &p.application;}
    ButtonResourceContext button_resource(ButtonApplicationContext app,std::int32_t resource)override{
        if(app!=&p.application)throw std::invalid_argument("foreign button application");return get_application_resource(p.application,resource);
    }
    ButtonSebContext button_seb(ButtonResourceContext context,std::int32_t clip)override{
        auto& resource=required(static_cast<RenderDrawResource*>(context));return required(required(resource.animations).clips.get()).at(static_cast<std::size_t>(clip));
    }
    std::array<std::int32_t,4> button_bounds(ButtonSebContext context,std::int32_t frame)override{return seb_bounding_rect(required(required(static_cast<SharedAnimationClip*>(context)).sprites),frame);}
    std::int32_t resource_id(RenderDrawResource* resource){for(const auto id:{1,2,3,4,7,8,11,18,19,20,21,22,23})if(get_application_resource(p.application,id)==resource)return id;return -1;}
    void image(ButtonResourceContext context,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t size){
        p.button_draws_.push_back({resource_id(static_cast<RenderDrawResource*>(context)),clip,frame,size,x,y,dynamic});
    }
    void button_draw_sprite(ButtonResourceContext context,std::int32_t clip,std::int32_t frame,float x,float y)override{
        image(context,clip,frame,x,y,100);if(backend)backend->draw_seb_frame(static_cast<RenderDrawResource*>(context),clip,frame,x,y);
    }
    void button_draw_scaled_sprite(ButtonResourceContext context,std::int32_t clip,std::int32_t frame,std::int32_t x,std::int32_t y,std::int32_t size)override{
        image(context,clip,frame,static_cast<float>(x),static_cast<float>(y),size);if(backend)backend->draw_scaled_seb(static_cast<RenderDrawResource*>(context),clip,frame,x,y,size);
    }
    void button_draw_seb_ex(ButtonResourceContext context,std::int32_t clip,std::int32_t frame,std::int32_t x,std::int32_t y,std::int32_t size,std::int32_t anchor)override{
        // ResourceManagerExtension.DrawSebEx resolves -1 through this captured manager's clock.
        if(frame==-1)frame=required(static_cast<SharedAnimationClip*>(button_seb(context,clip))).frame;
        image(context,clip,frame,static_cast<float>(x),static_cast<float>(y),size);
        if(backend){ImageAnimationGroup* clocks=nullptr;for(const auto& owner:p.owners)if(&owner->manager.drawing()==context){clocks=owner->image_clocks.get();break;}
            backend->draw_seb_image(static_cast<RenderDrawResource*>(context),clip,frame,x,y,size,anchor,clocks);}
    }
    void button_render_mode(std::int32_t mode,std::int32_t source,std::int32_t destination)override{if(backend)backend->set_render_mode(mode,source,destination);}
    void button_reset_render_mode()override{button_render_mode(0,255,0);}
    void button_draw_text(ButtonText text,float x,float y,std::int32_t anchor)override{p.output_text(std::move(text),x,y,anchor,backend);}
    void button_add_touch(std::int32_t id,InputRectangle rect,std::int32_t value)override{p.button_touches_.push_back({id,value,rect,false});}
    void button_add_touch_seb(std::int32_t id,std::int32_t x,std::int32_t y,ButtonSebContext seb,std::int32_t frame,std::int32_t value,ButtonTouchOption)override{
        const auto rect=button_bounds(seb,frame);button_add_touch(id,{add(x,rect[0]),add(y,rect[1]),rect[2],rect[3]},value);
    }
    void button_add_touch_delay(std::int32_t id,std::int32_t x,std::int32_t y,ButtonSebContext seb,std::int32_t frame,std::int32_t value,ButtonTouchOption option)override{
        // SAGA_STUB(mission-entity-button-surface): retain deferred registration arguments;
        // native deferred Surface/SEB hit testing is not replaced with an immediate rectangle.
        p.button_touches_.push_back({id,value,{x,y,0,0},true,seb,frame,option});
    }
    bool debug_controls()override{return p.quest_frame_&&p.quest_frame_->debug_controls;}
    StageGeometry* debug_map()override{return p.spatial?static_cast<StageGeometry*>(&p.spatial->map()):p.geometry?&*p.geometry:nullptr;}
    std::int32_t debug_game_width()override{return p.view.width;}
    std::int32_t debug_game_height()override{return p.view.height;}
    void debug_validate_quest_form()override{required(p.mission->gauge_camera_host().current_camera_form());}
    EntitySubset* debug_subset(const EventIntegers& slots)override{return p.entity_world->subset(slots);}
    GaugePoint debug_project(const ContactPosition* position)override{return p.mission->gauge_screen_position(position);}
    GaugePoint debug_project(float x,float y,float z)override{ContactPosition position;position.x=x;position.y=y;position.z=z;return debug_project(&position);}
    StageQuest* debug_quest()override{return p.humans?p.humans->quest().get():nullptr;}
    DebugSystemContext debug_member_system()override{return &p.system_states_[6];}
    ComponentSlots* debug_hero(DebugSystemContext context)override{if(context!=&p.system_states_[6])throw std::invalid_argument("foreign member context");return member_hero(&p.entity_world->player_world());}
    std::int32_t debug_total_parameter(DebugSystemContext context,ComponentSlots& e,std::int32_t key)override{if(context!=&p.system_states_[6])throw std::invalid_argument("foreign member context");return required(p.humans.get()).debug_parameter(e,key);}
    DebugSystemContext debug_human_system()override{return &p.system_states_[4];}
    void human(DebugSystemContext context){if(context!=&p.system_states_[4])throw std::invalid_argument("foreign human context");}
    std::int32_t debug_attack_interval(DebugSystemContext context,ComponentSlots& e)override{human(context);return required(p.humans.get()).debug_interval(e);}
    std::int32_t debug_burst_interval(DebugSystemContext context,ComponentSlots& e)override{human(context);return required(p.humans.get()).debug_burst_wait(e);}
    std::int32_t debug_burst_count(DebugSystemContext context,ComponentSlots& e)override{human(context);return required(p.humans.get()).debug_burst_count(e);}
    std::int32_t debug_distance(DebugSystemContext context,ComponentSlots& e)override{human(context);return required(p.humans.get()).debug_distance(e);}
    DebugText debug_equipment_name(ComponentSlots& e,EquipmentSlot slot)override{return std::make_shared<const std::u16string>(decode_mission_text(required(p.humans->debug_equipment(e,slot)).name));}
    std::int32_t debug_equipment_level(ComponentSlots& e,EquipmentSlot slot)override{return required(p.humans.get()).debug_equipment_level(e,slot);}
    DebugText debug_character_name(ComponentSlots& e)override{
        const auto id=field<HumanRecordLink>(e,14).character_id;const auto found=p.initial_character_records_.find(id);
        if(found==p.initial_character_records_.end()||!found->second.name)throw std::logic_error("mutable character name requires native new-game/save context");
        return std::make_shared<const std::u16string>(decode_mission_text(*found->second.name));
    }
    const EnemyRecord* debug_enemy_record(ComponentSlots& e)override{const auto id=field<EnemyRecordLink>(e,24).enemy_id;const auto& row=required(p.catalog->enemies.get()).at(static_cast<std::size_t>(id));return row?&*row:nullptr;}
    EventInstruction debug_parameter_ids()override{return p.debug_parameter_ids_;}
    DebugNames debug_parameter_names()override{return p.debug_parameter_names_;}
    DebugNames debug_human_state_names()override{return p.debug_human_states_;}
    DebugNames debug_enemy_state_names()override{return p.debug_enemy_states_;}
    DebugText debug_text(DebugTextKey key)override{return required(p.debug_texts_.get()).at(static_cast<std::size_t>(key));}
    std::int32_t debug_palette(DebugPalette palette)override{return palette==DebugPalette::white?-1:word(0xffff003cU);}
    void debug_set_color(std::int32_t color)override{p.drawing_color_=static_cast<std::uint32_t>(color);}
    void debug_push_font(std::int32_t size)override{p.drawing_fonts_.push_back(size);}
    void debug_pop_font()override{if(p.drawing_fonts_.size()<2)throw std::logic_error("empty drawing font scope");p.drawing_fonts_.pop_back();}
    void debug_render_mode(std::int32_t mode,std::int32_t source,std::int32_t destination)override{button_render_mode(mode,source,destination);}
    void debug_reset_render_mode()override{button_reset_render_mode();}
    void debug_draw_text(DebugText text,float x,float y,std::int32_t anchor)override{p.output_text(std::move(text),x,y,anchor,backend);}
    void debug_draw_line(float x1,float y1,float x2,float y2,float width)override{
        p.drawing_primitives_.push_back({0,{x1,y1,x2,y2,width,0,0},p.drawing_color_});if(backend)backend->draw_line(p.solid_color(p.drawing_color_),x1,y1,x2,y2,width);
    }
    void debug_fill_rect(float x,float y,float width,float height)override{
        p.drawing_primitives_.push_back({1,{x,y,width,height,0,0,0},p.drawing_color_});if(backend)backend->draw_bitmap(p.solid_color(p.drawing_color_),x,y,width,height);
    }
    void debug_draw_arc(float x,float y,float width,float height,float start,float sweep,float line_width)override{
        p.drawing_primitives_.push_back({2,{x,y,width,height,start,sweep,line_width},p.drawing_color_});if(!backend)return;
        // SAGA_STUB(mission-debug-primitive-geometry): explicit public polyline policy,
        // 64 segments per complete turn; original rasterizer/pixel matching requires the oracle.
        const auto segments=std::max(1,static_cast<int>(std::ceil(std::abs(sweep)*64.0F/360.0F)));
        if(segments>4096)throw std::length_error("debug arc exceeds provider segment budget");
        const auto point=[&](float degrees){const float radians=degrees*3.14159265358979323846F/180.0F;return std::array<float,2>{x+width*0.5F+std::cos(radians)*width*0.5F,y+height*0.5F+std::sin(radians)*height*0.5F};};
        auto previous=point(start);for(int i=1;i<=segments;++i){const auto next=point(start+sweep*static_cast<float>(i)/static_cast<float>(segments));backend->draw_line(p.solid_color(p.drawing_color_),previous[0],previous[1],next[0],next[1],line_width);previous=next;}
    }
};

void MissionProfile::draw_entity_buttons(bool dynamic,BitmapBackend* backend){
    if(!mission||!system_states_[26].enabled)return;DrawingHost host(*this,backend,dynamic);mission->draw_entity_buttons(dynamic,host);
}
void MissionProfile::draw_debug_graphics(BitmapBackend* backend){if(!mission)return;DrawingHost host(*this,backend,true);mission->draw_debug(host);}
InputRectangle MissionProfile::draw_widget_button(const ButtonDrawRequest& request,BitmapBackend* backend){
    DrawingHost host(*this,backend,false);const auto before=button_touches_.size();core::draw_button(request,host);
    if(button_touches_.size()!=before+1)throw std::logic_error("movement widget requires one native registration");return button_touches_.back().bounds;
}
bool MissionProfile::has_static_buttons()const{
    if(!entity_world)return false;auto cursor=entity_world->subset({17,0,2})->cursor();while(const auto entry=cursor.next())if(!field<EntityButton>(required(*entry),17).dynamic)return true;return false;
}
void MissionProfile::write_drawing_report(std::ostream& out){
    out<<",\"button_view_scales\":{\"provisional\":true,\"instance\":"<<instance_view_scale_<<",\"base\":"<<base_view_scale_<<"},\"debug_requested_flags\":"<<debug_requested_flags_;
    out<<",\"button_image_draws\":[";bool first=true;for(const auto& r:button_draws_){if(!first)out<<',';first=false;out<<"{\"resource\":"<<r.resource<<",\"clip\":"<<r.clip<<",\"frame\":"<<r.frame<<",\"scale\":"<<r.scale<<",\"x\":"<<r.x<<",\"y\":"<<r.y<<",\"dynamic\":"<<(r.dynamic?"true":"false")<<'}';}out<<']';
    out<<",\"button_touch_requests\":[";first=true;for(const auto& r:button_touches_){if(!first)out<<',';first=false;out<<"{\"group\":"<<r.group<<",\"value\":"<<r.value<<",\"x\":"<<r.bounds.x<<",\"y\":"<<r.bounds.y<<",\"width\":"<<r.bounds.width<<",\"height\":"<<r.bounds.height<<",\"delayed\":"<<(r.delayed?"true":"false")<<'}';}out<<']';
    out<<",\"drawing_text_requests\":[";first=true;for(const auto& r:drawing_texts_){if(!first)out<<',';first=false;out<<"{\"units\":"<<r.source.size()<<",\"x\":"<<r.x<<",\"y\":"<<r.y<<",\"font\":"<<r.font<<",\"anchor\":"<<r.anchor<<",\"color\":"<<r.color<<",\"bound\":"<<(r.bound?"true":"false")<<'}';}out<<']';
    out<<",\"debug_primitive_requests\":[";first=true;for(const auto& r:drawing_primitives_){if(!first)out<<',';first=false;out<<"{\"type\":"<<r.type<<",\"color\":"<<r.color<<",\"values\":[";for(std::size_t i=0;i<r.values.size();++i){if(i)out<<',';out<<r.values[i];}out<<"]}";}out<<']';
}
}
