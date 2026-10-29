#pragma once
#include "render_draw.hpp"
#include "assets/opt_grid.hpp"

namespace saga::core {
struct ImageFrameBox {std::int32_t u,v,width,height;};
struct PixelImage:RenderImage {
    std::int32_t render_queue=0; // Image +0xd8; relative graphics/material queue offset.
    std::int32_t width=0,height=0,standard_width=0,standard_height=0;
    std::int32_t columns=0,rows=0,passes=0; // Image +0x98/+0x9c/+0xa0.
    std::int16_t step_x=0,step_y=0; // +0xc0/+0xc2.
    std::shared_ptr<std::vector<PixelImage*>> sources; // +0xb8.
    std::shared_ptr<std::vector<ImageFrameBox>> frames; // Decoded +0xa8/+0xb0, absent for regular OPT.
    std::int32_t frame_mark=0; // +0xc4.
};
struct BitmapDraw {float x,y,width,height,u,v,source_width,source_height;};
struct ImageRect {float x,y,width,height;std::int32_t u,v;float source_width,source_height;};
struct ImageBounds {float left,top,right,bottom;};
struct ImageCullRect {float x,y,width,height;}; // RectF input, distinct from RectangleF bounds work.
struct ImageDrawStatistics {
    bool enabled=false,debug=false,clip_warning_shown=false;
    std::int32_t pieces=0,texels=0,image_texels=0,frame=0;
};
struct ImageDrawState {
    std::int32_t flip=0,local_matrix_index=-1,local_coordinate_count=0;
    ImageCullRect culling{};bool culling_disabled=false;
    std::shared_ptr<std::vector<float>> clip; // Graphics.imageClip_ +0xe8, x/y/width/height; NaN disables an axis.
    std::shared_ptr<ImageBounds> bounds; // Shared Graphics.drawImageRect_ work, caller supplied.
    ImageDrawStatistics* statistics=nullptr; // Effective shared Config draw accounting/debug fields.
};
class ImageDrawHost {
public:
    virtual ~ImageDrawHost()=default;
    virtual bool culling_overlap(const ImageCullRect& culling,const ImageRect& request)=0;
    virtual void null_image_debug()=0;
    virtual bool begin_sample()=0;
    virtual void end_sample(bool)=0;
    virtual void check_use(PixelImage&)=0;
    virtual void clipping_unavailable()=0;
    virtual void push_rotation(float angle,float x,float y)=0;
    virtual void push_scale(float x,float y,float center_x,float center_y)=0;
    virtual void push_translation(float x,float y)=0;
    virtual void pop_matrix()=0;
    virtual void bitmap(PixelImage&,const BitmapDraw&)=0; // Graphics._drawBitmap, explicit pixel/graphics backend.
    virtual SpriteArray piece(PixelImage&,std::int32_t column,std::int32_t row,std::int32_t pass)=0;
    virtual SpriteArray frame_piece(PixelImage&,std::int32_t frame,std::int32_t pass)=0;
    virtual void use_frame(PixelImage&,std::int32_t)=0; // Original GetOptimizeSeb result is discarded.
};
// Preparation from the existing regular OPT reader; no FF parser, dimension repair or atlas synthesis.
class PreparedOptImage:public PixelImage {
public:
    explicit PreparedOptImage(const assets::OptGrid&);
    SpriteArray piece(std::int32_t column,std::int32_t row,std::int32_t pass,SpriteArray shared_result)const;
private:
    assets::OptGrid grid_; // Keep compact parsed cells/pieces; no padded per-pass matrix.
};
// Reached wrappers, preserving binary32 source conversion and null-image debug behavior.
void draw_image_rect(ImageDrawState&,PixelImage*,float x,float y,std::int32_t u,std::int32_t v,std::int32_t width,std::int32_t height,ImageDrawHost&); // 0x1b64c90.
void draw_scaled_image_rect(ImageDrawState&,PixelImage*,float x,float y,float width,float height,std::int32_t u,std::int32_t v,std::int32_t source_width,std::int32_t source_height,ImageDrawHost&); // 0x1b64dc0.
void draw_image_internal(ImageDrawState&,PixelImage*,ImageRect,ImageDrawHost&); // 0x1b637bc; u/v remain int32 until native conversion points.
} // namespace saga::core
