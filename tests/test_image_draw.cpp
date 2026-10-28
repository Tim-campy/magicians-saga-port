#include "image_draw_fixture.hpp"
#include <cstdlib>
#include <cmath>
#include <iostream>
using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"image drawing contract line "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
saga::assets::OptGrid hat(){return {30,30,1,2,{{0,1},{1,1}},{{{-1,4,4,20,0,19,15}},{{-1,4,5,0,0,20,15}}}};}
int main(){
    {ImageDrawServices h;h.draw(nullptr);check(h.calls.empty());h.stats.debug=true;h.draw(nullptr);check(h.calls==std::vector<std::string>{"null"});}
    {ImageDrawServices h;PixelImage image;h.draw(&image,3,4,10,20);check(h.calls==std::vector<std::string>{"begin","use","bitmap","end"});
     const auto d=h.bitmaps[0].second;check(d.x==100&&d.y==200&&d.width==10&&d.height==20&&d.u==3&&d.v==4&&d.source_width==10&&d.source_height==20);}
    {ImageDrawServices h;PreparedOptImage image(hat());h.draw(&image,0,30);const auto d=h.bitmaps[0].second;
     check(h.pieces==std::vector<std::array<int,3>>{{0,1,0}}&&d.x==104&&d.y==205&&d.width==20&&d.height==15&&d.u==0&&d.v==0&&d.source_width==20);}
    {ImageDrawServices h;PreparedOptImage image(hat());h.draw(&image,5,6,8,5);const auto d=h.bitmaps[0].second;
     check(d.x==100&&d.y==200&&d.width==8&&d.height==5&&d.u==21&&d.v==2&&d.source_width==8&&d.source_height==5);}
    {ImageDrawServices h;PreparedOptImage image(hat());h.draw(&image,0,0,60,60);check(h.bitmaps.size()==1);const auto d=h.bitmaps[0].second;
     check(d.x==104&&d.y==204&&d.width==19&&d.height==15);}
    {ImageDrawServices h;saga::assets::OptGrid grid{112,128,1,1,{{0,1}},{{{-1,226,127,0,0,398,393}}}};
     PreparedOptImage image(grid);draw_image_rect(h.state,&image,0,0,0,0,624,640,h);const auto d=h.bitmaps[0].second;
     check(image.step_x==112&&image.step_y==128&&d.x==226&&d.y==127&&d.width==398&&d.height==393&&d.source_width==398&&d.source_height==393);}
    {ImageDrawServices h;PreparedOptImage image(hat());draw_scaled_image_rect(h.state,&image,100,200,300,300,0,0,30,30,h);
     const auto d=h.bitmaps[0].second;check(d.x==140&&d.y==240&&d.width==190&&d.height==150&&d.u==20&&d.source_width==19);}
    {ImageDrawServices h;auto grid=hat();grid.rows=1;grid.cells.resize(1);PreparedOptImage image(grid);image.standard_width=60;image.standard_height=60;
     h.draw(&image,0,0,60,60);const auto d=h.bitmaps[0].second;check(d.x==108&&d.y==208&&d.width==38&&d.height==30&&d.source_width==19);}
    {ImageDrawServices h;PixelImage image;image.passes=1;image.columns=16777218;image.rows=1;image.step_x=1;image.step_y=1;
     h.draw(&image,16777217,0,1,1);check(h.pieces[0][0]==16777217);}
    {ImageDrawServices h;PixelImage image;h.state.culling_disabled=false;h.overlap=false;h.draw(&image);check(h.calls==std::vector<std::string>{"cull"});
     h.state.local_matrix_index=0;h.draw(&image);check(h.bitmaps.size()==1);}
    {ImageDrawServices h;PixelImage image;h.state.clip->at(0)=105;h.state.clip->at(2)=10;h.draw(&image);const auto d=h.bitmaps[0].second;
     check(d.x==105&&d.width==10&&d.u==5&&d.source_width==10);}
    {ImageDrawServices h;PixelImage image;h.state.clip->at(0)=105;h.state.clip->at(2)=10;h.state.flip=1;h.draw(&image);const auto d=h.bitmaps[0].second;
     check(d.x==105&&d.width==10&&d.u==15&&d.source_width==10&&h.scales[0]==std::array<float,4>{-1,1,5,15}&&h.pops==1);}
    {ImageDrawServices h;PixelImage image;h.state.clip->at(1)=205;h.state.clip->at(3)=10;h.state.flip=2;h.draw(&image);const auto d=h.bitmaps[0].second;
     check(d.y==205&&d.height==10&&d.v==15&&d.source_height==10);}
    {ImageDrawServices h;PixelImage image;h.state.clip->at(0)=200;h.state.clip->at(2)=10;h.draw(&image);check(h.bitmaps.empty()&&h.ends==1&&h.pops==0);}
    {ImageDrawServices h;PixelImage image;h.state.clip->resize(2);h.draw(&image);check(h.bitmaps.size()==1);}
    {ImageDrawServices h;PixelImage image;h.state.clip->at(0)=100;h.state.clip->at(2)=10;
     draw_scaled_image_rect(h.state,&image,100,200,std::numeric_limits<float>::quiet_NaN(),30,0,0,30,30,h);
     check(h.bitmaps.size()==1&&std::isnan(h.bitmaps[0].second.width)&&std::isnan(h.bitmaps[0].second.source_width));}
    for(int flip:{4,5}){ImageDrawServices h;PixelImage image;h.state.flip=flip;h.draw(&image);check(h.rotations[0]==std::array<float,3>{-90,15,15}&&h.pops==1);}
    {ImageDrawServices h;PreparedOptImage image(hat());h.state.local_matrix_index=0;h.draw(&image);const auto d=h.bitmaps[0].second;
     check(h.translations[0]==std::make_pair(4.0F,4.0F)&&d.x==100&&d.y==200&&d.width==19&&h.pops==1);}
    {ImageDrawServices h;PreparedOptImage image(hat());h.state.flip=1;h.state.local_matrix_index=0;
     h.hook=[](const std::string& s){if(s=="bitmap")throw std::runtime_error("bitmap");};fails([&]{h.draw(&image);});check(h.pops==2&&h.ends==1);}
    {ImageDrawServices h;PreparedOptImage image(hat());h.state.flip=1;h.state.local_matrix_index=0;
     h.hook=[](const std::string& s){if(s=="translate")throw std::runtime_error("translation");};fails([&]{h.draw(&image);});check(h.pops==1&&h.ends==1);}
    {ImageDrawServices h;PixelImage image;h.state.flip=1;h.hook=[](const std::string& s){if(s=="scale")throw std::runtime_error("scale");};
     fails([&]{h.draw(&image);});check(h.pops==0&&h.ends==1);}
    {ImageDrawServices h;PixelImage image;h.hook=[](const std::string& s){if(s=="end")throw std::runtime_error("profiler");};fails([&]{h.draw(&image);});check(h.ends==1);}
    {ImageDrawServices h;PreparedOptImage image(hat());image.width=39;image.height=15;h.stats.enabled=true;h.stats.frame=7;
     h.hook=[&](const std::string& s){if(s=="bitmap"){h.scratch->at(5)=9;h.scratch->at(6)=8;}};h.draw(&image);
     check(h.stats.pieces==1&&h.stats.texels==72&&h.stats.image_texels==585&&image.frame_mark==7&&h.bitmaps[0].second.width==19);
     h.draw(&image);check(h.stats.image_texels==585&&h.stats.pieces==2);}
    {ImageDrawServices h;auto grid=hat();grid.pieces[0].values[0]=0;PreparedOptImage image(grid);image.sources=std::make_shared<std::vector<PixelImage*>>(1,nullptr);
     h.state.local_matrix_index=0;fails([&]{h.draw(&image);});check(h.bitmaps.empty()&&h.pops==1&&h.ends==1);}
    {ImageDrawServices h;PixelImage image;image.passes=1;image.frames=std::make_shared<std::vector<ImageFrameBox>>(std::vector<ImageFrameBox>{{0,0,30,30}});
     h.frame_record=std::make_shared<SpriteValues>(SpriteValues{-1,4,4,20,0,19,15});h.state.clip->at(0)=105;h.state.clip->at(2)=10;h.stats.debug=true;
     h.draw(&image);const auto d=h.bitmaps[0].second;check(h.warnings==1&&d.x==104&&d.width==19);h.draw(&image);check(h.warnings==1&&h.bitmaps.size()==2);
     h.draw(&image,1,0);check(h.bitmaps.size()==2);}
    {auto grid=hat();grid.cells[0].piece_count=0;PreparedOptImage image(grid);auto scratch=std::make_shared<SpriteValues>(7,99);
     check(!image.piece(0,0,0,scratch)&&scratch->at(0)==99);check(image.piece(0,1,0,scratch)==scratch&&scratch->at(5)==20);}
}
