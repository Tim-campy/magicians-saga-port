#pragma once
#include "image_draw.hpp"
#include <array>
#include <optional>

namespace saga::core {
// Selected texture is captured by _drawBitmap before UV computation. Its virtual
// dimensions need not equal Image.width/height (an atlas page is one example).
class BitmapTexture {
public:
    virtual ~BitmapTexture()=default;
    virtual std::int32_t width()=0;
    virtual std::int32_t height()=0;
};
struct BitmapAtlasOwner {PixelImage* page=nullptr;};
struct BitmapAtlasRegion {BitmapAtlasOwner* owner=nullptr;std::int32_t u=0,v=0;bool active=false;};
class BitmapAlphaHost {
public:
    virtual ~BitmapAlphaHost()=default;
    virtual std::int32_t alpha_type(PixelImage&)=0;
    virtual void set_alpha_type(PixelImage&,std::int32_t)=0;
};
struct BitmapSource {
    PixelImage* page;
    BitmapDraw draw;
    std::array<std::int32_t,4> integer_key; // Grayscale key: before atlas offsets.
};
// _drawBitmap 0x1b66234..0x1b66338: standard resolution and active atlas handoff.
// CheckUse, grayscale image creation and Unity object liveness belong to caller.
BitmapSource prepare_bitmap_source(PixelImage&,BitmapDraw,const BitmapAtlasRegion*,BitmapAlphaHost&);
struct BitmapSamplingCache {
    PixelImage* image=nullptr;
    std::int32_t alpha_type=0;
    std::shared_ptr<std::vector<float>> crop=std::make_shared<std::vector<float>>(4,0.0F);
    std::shared_ptr<ImageBounds> uv=std::make_shared<ImageBounds>(); // Bottom-left texture coordinates.
    bool valid=false,dirty=true;
};
struct BitmapSamplingResult {bool same_material,recomputed;};
// _drawBitmap 0x1b66878..0x1b66b38, a slice, NOT the full drawing method.
// Changes draw.source_width/height only on cache misses, including invalid crops.
// The caller clears dirty and restores the original image identity after material
// emission; this function deliberately leaves that later state transition alone.
BitmapSamplingResult update_bitmap_sampling(BitmapSamplingCache&,PixelImage&,BitmapDraw&,
                                           ImageBounds& source_work,BitmapTexture&,BitmapAlphaHost&);
ImageBounds bitmap_destination(const BitmapDraw&,float expansion);

struct BitmapPoint {float x=0,y=0,z=0;};
using BitmapPoints=std::shared_ptr<std::vector<BitmapPoint>>;
struct BitmapMeshCoordinates {bool use_surface_height=false;}; // Y is inverted in both modes; this selects its origin.
struct BitmapSurface {std::int32_t height=0;};
struct BitmapGeometryState {
    std::int32_t mesh_index=0;
    std::shared_ptr<std::vector<BitmapMeshCoordinates*>> mesh_modes;
    BitmapSurface* surface=nullptr;
    bool begin_fill=false,call_draw_bitmap=false;
};
class BitmapGeometryHost {
public:
    virtual ~BitmapGeometryHost()=default;
    virtual bool render_target_alive()=0;
    virtual std::int32_t render_target_height()=0;
    virtual std::int32_t screen_height()=0;
    virtual void screen_transform(float& x,float& y)=0; // Graphics.screenMatrix_.TransPoints.
    virtual std::optional<std::array<float,2>> fill_point(float x,float y)=0;
    virtual void add_fill_point(float x,float y,float transformed_x,float transformed_y)=0;
};
// Graphics.GetTransRectGL overloads, 0x1b69ee8 / 0x1b68058. Existing output
// arrays keep their identity and length, even when longer/shorter than four.
BitmapPoints transform_bitmap_rectangle(BitmapGeometryState&,float x,float y,float width,float height,
                                       BitmapPoints,bool transform,const ImageBounds* fill_bounds,BitmapGeometryHost&);
BitmapPoints transform_bitmap_rectangle(BitmapGeometryState&,const ImageBounds*,BitmapPoints,
                                       const ImageBounds* fill_bounds,BitmapGeometryHost&);
} // namespace saga::core
