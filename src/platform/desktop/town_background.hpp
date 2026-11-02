#pragma once
#include "gameplay_catalog.hpp"
#include "archive_image_set.hpp"
#include "bitmap_backend.hpp"
#include "core/gameplay/select_form_pass.hpp"
#include "core/gameplay/select_camera.hpp"
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
};
class TownBackground final:private core::SelectBackgroundHost,private core::SelectActiveAreaHost,private core::SelectCameraHost {
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
};
} // namespace saga::desktop
