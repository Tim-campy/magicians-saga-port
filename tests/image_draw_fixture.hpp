#pragma once
#include "core/gameplay/image_draw.hpp"
#include <functional>
#include <limits>
#include <string>

struct ImageDrawServices:saga::core::ImageDrawHost {
    using Image=saga::core::PixelImage;
    saga::core::ImageDrawStatistics stats;
    saga::core::ImageDrawState state;
    saga::core::SpriteArray scratch=std::make_shared<saga::core::SpriteValues>(7,0),frame_record;
    std::vector<std::string> calls;std::function<void(const std::string&)> hook;
    std::vector<std::pair<Image*,saga::core::BitmapDraw>> bitmaps;
    std::vector<std::array<std::int32_t,3>> pieces;
    std::vector<std::array<float,4>> scales;
    std::vector<std::array<float,3>> rotations;
    std::vector<std::pair<float,float>> translations;
    int pops=0,ends=0,warnings=0;bool overlap=true,profile=true;
    ImageDrawServices(){
        const auto nan=std::numeric_limits<float>::quiet_NaN();
        state.clip=std::make_shared<std::vector<float>>(std::vector<float>{nan,nan,nan,nan});
        state.bounds=std::make_shared<saga::core::ImageBounds>();state.statistics=&stats;state.culling_disabled=true;
    }
    void step(const std::string& s){calls.push_back(s);if(hook)hook(s);}
    bool culling_overlap(const saga::core::ImageCullRect&,const saga::core::ImageRect&)override{step("cull");return overlap;}
    void null_image_debug()override{step("null");}
    bool begin_sample()override{step("begin");return profile;}
    void end_sample(bool value)override{if(value!=profile)throw std::invalid_argument("incorrect profiler token");++ends;step("end");}
    void check_use(Image&)override{step("use");}
    void clipping_unavailable()override{++warnings;step("warning");}
    void push_rotation(float a,float x,float y)override{rotations.push_back({a,x,y});step("rotate");}
    void push_scale(float x,float y,float cx,float cy)override{scales.push_back({x,y,cx,cy});step("scale");}
    void push_translation(float x,float y)override{translations.emplace_back(x,y);step("translate");}
    void pop_matrix()override{++pops;step("pop");}
    void bitmap(Image& image,const saga::core::BitmapDraw& draw)override{bitmaps.emplace_back(&image,draw);step("bitmap");}
    saga::core::SpriteArray piece(Image& image,std::int32_t column,std::int32_t row,std::int32_t pass)override{
        pieces.push_back({column,row,pass});step("piece");
        auto* prepared=dynamic_cast<saga::core::PreparedOptImage*>(&image);return prepared?prepared->piece(column,row,pass,scratch):nullptr;
    }
    saga::core::SpriteArray frame_piece(Image&,std::int32_t frame,std::int32_t pass)override{pieces.push_back({frame,-1,pass});step("frame-piece");return frame_record;}
    void use_frame(Image&,std::int32_t)override{step("frame");}
    void draw(Image* image,std::int32_t u=0,std::int32_t v=0,std::int32_t width=30,std::int32_t height=30){
        saga::core::draw_image_rect(state,image,100,200,u,v,width,height,*this);
    }
};
