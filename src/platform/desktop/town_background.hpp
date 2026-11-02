#pragma once
#include "gameplay_catalog.hpp"
#include "archive_image_set.hpp"
#include "bitmap_backend.hpp"
#include "core/gameplay/select_form_pass.hpp"
#include "core/gameplay/select_camera.hpp"
#include "core/gameplay/game_random.hpp"
#include <ostream>

namespace saga::desktop {
struct TownBackgroundContext {
    // Provisional desktop viewport/scale; AppData cctor supplies VIEW_X/Y=0/53.
    std::int32_t width=600,height=640,view_x=0,view_y=53;
    float view_scale=100.0F;
    std::int32_t form_scale=100,camera_request_x=0,camera_request_y=-9999;
    BitmapCamera projection{0,1500100,-1};
    std::int32_t depth_bits=24;
    bool depth_step=true;
    bool scenery=false;
    std::int32_t app_seed=42,lib_seed=42; // Separate provisional streams at the cloud-init boundary.
};
class TownBackground final:private core::SelectBackgroundHost,private core::SelectActiveAreaHost,private core::SelectCameraHost,private core::SelectSceneryHost,private core::SelectFloorHost {
public:
    TownBackground(ResourceArchive& tables,const GameArchiveEntry& selected,ResourceProfile,std::int32_t capacity,TownBackgroundContext);
    void draw(BitmapBackend&);
    void report(std::ostream&)const;
    BitmapApplication& application(){return application_;}
    BitmapTextures& textures(){return textures_;}
    const TownBackgroundContext& context()const{return context_;}
private:
    struct Draw {std::int32_t clip,frame;float x,y;};
    core::SelectCameraState camera_;
    TownBackgroundContext context_;
    std::int32_t camera_x_=0,camera_y_=0;
    core::SelectSceneryState scenery_{std::make_shared<std::vector<core::EventInstruction>>(),nullptr,nullptr,camera_.left_extent,camera_.right_extent};
    struct Random final:core::RandomSource {
        explicit Random(std::int32_t seed):source(seed){}
        core::LegacySystemRandom source;std::uint64_t draws=0;
        std::int32_t next()override{++draws;return source.next();}
    };
    Random app_random_,lib_random_;
    core::AreaTable areas_;
    std::vector<std::optional<core::AreaProgressState>> progress_;
    std::shared_ptr<core::CameraAreas> camera_areas_;
    core::EventInstruction save_integers_;
    ResourceArchive archive_;
    ArchiveResourceManager manager_;
    ArchiveImageSet images_;
    BitmapApplication application_;
    BitmapTextures textures_;
    ReadyArchiveImage solid_;
    BitmapBackend* backend_=nullptr;
    std::uint32_t color_=0;
    std::array<std::int32_t,2> fill_{};
    std::vector<Draw> draws_;
    struct SceneryDraw {std::int32_t clip,frame;float x,y;std::int32_t layer,anchor,reverse;};
    std::vector<SceneryDraw> scenery_draws_;
    struct FloorDraw {std::int32_t frame,x,y;};
    std::vector<FloorDraw> floor_draws_;
    core::SebStaticWork floor_work_;
    std::int32_t floor_layer_=0;
    std::size_t quads_=0;
    std::shared_ptr<core::EventIntegers> normal_save_integers()override{return save_integers_;}
    std::shared_ptr<core::CameraAreas> camera_areas()override{return camera_areas_;}
    std::int32_t view_width()override;
    std::int32_t view_height()override;
    std::int32_t game_width()override;
    std::int32_t game_height()override;
    std::array<float,3> background_pixel()override;
    void background_color(std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    std::int32_t background_width()override{return game_width();}
    std::int32_t background_height()override{return game_height();}
    void fill_background(std::int32_t,std::int32_t)override;
    core::SelectBackgroundResource background_resource()override{return &manager_.drawing();}
    std::int32_t background_view_height()override{return view_height();}
    std::int32_t background_clip()override{return 0;} // RESSELECT_SEB_BG; cctor 0x11192b0.
    void draw_background(core::SelectBackgroundResource,std::int32_t,std::int32_t,float,float)override;
    std::int32_t raw_view_height()override{return view_height();}
    std::int32_t cloud_right_edge()override{return 490+240;} // Init +0x160; AppData.VIEW_W.
    std::int32_t scenery_clip(std::int32_t)override;
    std::int32_t app_random(std::int32_t bound)override{return core::app_random_bounded(app_random_,bound);}
    std::int32_t lib_random(std::int32_t bound)override{return core::lib_random_bounded(lib_random_,bound);}
    std::int32_t lib_random(std::int32_t low,std::int32_t high)override{return core::random_inclusive(lib_random_,low,high);}
    core::SelectBackgroundResource scenery_resource()override{return &manager_.drawing();}
    std::array<std::int32_t,4> scenery_bounds(std::int32_t,std::int32_t)override;
    void scenery_reverse(std::int32_t value)override{backend_->graphics().reverse=value;}
    void scenery_draw(core::SelectBackgroundResource,std::int32_t,std::int32_t,float,float,std::int32_t)override;
    void scenery_draw_indexed(core::SelectBackgroundResource,std::int32_t,std::int32_t,float,float,std::int32_t,std::int32_t)override;
    std::int32_t floor_view_height()override{return view_height();}
    core::SebSpriteState* floor_sprite()override;
    core::RenderImage* floor_image()override{return manager_.drawing().images->at(3);} // RESSELECT_IMG_SELECT_FLOOR.
    void floor_draw(core::SebSpriteState*,core::RenderImage*,std::int32_t,std::int32_t,std::int32_t)override;
    void floor_depth_layer(std::int32_t value)override{floor_layer_=value;backend_->set_depth_layer(value);}
    core::SelectBackgroundResource gate_resource()override{return &manager_.drawing();}
    std::int32_t gate_clip()override{return 3;} // RESSELECT_SEB_GATE, AppData static+0x170.
    void gate_draw(core::SelectBackgroundResource,std::int32_t,std::int32_t,std::int32_t,float,float)override;
};
} // namespace saga::desktop
