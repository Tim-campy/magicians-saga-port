#include "core/gameplay/bitmap_geometry.hpp"
#include <cstdlib>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
using namespace saga::core;
void check_at(bool condition,int line){if(!condition){std::cerr<<"bitmap geometry contract line "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Services:BitmapTexture,BitmapAlphaHost,BitmapGeometryHost {
    PixelImage image,page;BitmapSamplingCache cache;ImageBounds work;
    int alpha=2,stored=-1,texture_width=200,texture_height=100;
    std::vector<std::string> calls;std::function<void(const std::string&)> hook;
    BitmapMeshCoordinates coordinates{true};BitmapSurface surface{100};BitmapGeometryState state;
    bool target=false;float tx=0,ty=0;std::vector<std::array<float,4>> additions;
    std::vector<std::array<float,2>> lookups;
    std::optional<std::array<float,2>> replacement;
    Services(){image.width=100;image.height=50;state.surface=&surface;state.mesh_modes=std::make_shared<std::vector<BitmapMeshCoordinates*>>(1,&coordinates);}
    void call(const std::string& s){calls.push_back(s);if(hook)hook(s);}
    int width()override{call("width");return texture_width;}
    int height()override{call("height");return texture_height;}
    int alpha_type(PixelImage&)override{call("alpha");return alpha;}
    void set_alpha_type(PixelImage& i,int value)override{check(&i==&page);stored=value;call("set-alpha");}
    bool render_target_alive()override{call("alive");return target;}
    int render_target_height()override{call("target");return 80;}
    int screen_height()override{call("screen");return 60;}
    void screen_transform(float& x,float& y)override{x=x+tx;y=y+ty;call("transform");}
    std::optional<std::array<float,2>> fill_point(float x,float y)override{lookups.push_back({x,y});call("fill");return replacement;}
    void add_fill_point(float x,float y,float a,float b)override{additions.push_back({x,y,a,b});call("add");}
    BitmapSamplingResult sample(BitmapDraw& d){return update_bitmap_sampling(cache,image,d,work,*this,*this);}
};
int main(){
    {Services h;h.image.standard_width=200;h.image.standard_height=100;
     const auto source=prepare_bitmap_source(h.image,{1,2,3,4,10,20,30,40},nullptr,h);
     check(source.page==&h.image&&source.draw.u==5&&source.draw.v==10&&source.draw.source_width==15&&source.integer_key==std::array<int,4>{5,10,15,20});
     h.image.passes=1;const auto opt=prepare_bitmap_source(h.image,{1,2,3,4,10,20,30,40},nullptr,h);check(opt.draw.u==10&&opt.draw.source_width==30);}
    {Services h;BitmapAtlasOwner owner{&h.page};BitmapAtlasRegion atlas{&owner,20,-5,true};
     const auto source=prepare_bitmap_source(h.image,{1,2,3,4,1.95F,-0.15F,4,5},&atlas,h);
     check(source.page==&h.page&&source.draw.u==21.95F&&source.draw.v==-5.15F&&source.integer_key==std::array<int,4>{2,0,4,5});
     check(h.stored==2&&h.calls==std::vector<std::string>{"alpha","set-alpha"});
     owner.page=nullptr;fails([&]{prepare_bitmap_source(h.image,source.draw,&atlas,h);});check(h.calls.back()=="alpha");
     atlas.owner=nullptr;const auto count=h.calls.size();fails([&]{prepare_bitmap_source(h.image,source.draw,&atlas,h);});check(h.calls.size()==count);}
    {Services h;const auto inf=std::numeric_limits<float>::infinity();
     const auto result=prepare_bitmap_source(h.image,{0,0,1,1,inf,-inf,inf,0},nullptr,h);
     check(result.integer_key[0]==std::numeric_limits<int>::min()&&result.integer_key[1]==std::numeric_limits<int>::min());}
    {Services h;BitmapDraw d{10,20,80,40,-2,-3,12,13};const auto r=h.sample(d);
     check(r.recomputed&&!r.same_material&&d.x==10&&d.y==20&&d.width==80&&d.height==40&&d.source_width==10&&d.source_height==10);
     check(h.work.left==0.005F&&h.work.top==0.005F&&h.work.right==9.99F&&h.work.bottom==9.99F);
     check(h.cache.valid&&h.cache.dirty&&h.cache.uv->left==0.005F/200.0F);
     check(h.calls==std::vector<std::string>{"alpha","width","height","width","height","height"});
     h.cache.dirty=false;h.calls.clear();d.source_width=12;d.source_height=13;
     const auto hit=h.sample(d);check(hit.same_material&&!hit.recomputed&&d.source_width==12&&d.source_height==13);
     check(h.calls==std::vector<std::string>{"alpha","alpha"}); // The caller's unclipped extents survive a hit.
     h.alpha=3;h.calls.clear();check(h.sample(d).recomputed&&h.cache.alpha_type==3&&h.cache.dirty);}
    {Services h;BitmapDraw d{0,0,20,20,99,49,8,8};h.sample(d);check(d.source_width==1&&d.source_height==1&&h.cache.valid);
     d.u=100;h.cache.dirty=false;h.calls.clear();h.sample(d);check(!h.cache.valid&&d.source_width==0&&h.calls==std::vector<std::string>{"alpha","alpha"});}
    {Services h;BitmapDraw d{0,0,1,1,0,0,0.01F,0.01F};h.sample(d);
     check(h.cache.valid&&h.cache.uv->right<h.cache.uv->left&&h.cache.uv->bottom<h.cache.uv->top);}
    {Services h;BitmapDraw d{0,0,1,1,std::numeric_limits<float>::quiet_NaN(),0,1,1};h.sample(d);
     check(h.cache.valid&&std::isnan(h.cache.uv->left)&&std::isnan(h.cache.crop->at(0)));h.cache.dirty=false;check(h.sample(d).recomputed);
     d.u=0;d.source_width=std::numeric_limits<float>::quiet_NaN();h.sample(d);check(!h.cache.valid);}
    {Services h;h.cache.crop->resize(2);BitmapDraw d{0,0,1,1,2,3,4,5};fails([&]{h.sample(d);});
     check(h.cache.image==&h.image&&h.cache.crop->at(0)==2&&h.cache.crop->at(1)==3);}
    {Services h;BitmapDraw d{0,0,1,1,10,20,30,10};int query=0;
     h.hook=[&](const std::string& s){if(s=="width")h.texture_width=++query==1?100:200;};h.sample(d);
     check(h.cache.uv->left==10.005F/200.0F);
     check(h.cache.uv->right==h.cache.uv->left+(39.99F-10.005F)/100.0F);}
    {const auto r=bitmap_destination({10,20,-4,8,0,0,1,1},3);check(r.left==8.5F&&r.top==18.5F&&r.right==7.5F&&r.bottom==29.5F);}
    {Services h;h.tx=3;h.ty=4;auto points=transform_bitmap_rectangle(h.state,1,2,10,20,nullptr,true,nullptr,h);
     check(points->size()==4&&points->at(0).x==4&&points->at(0).y==94&&points->at(2).x==14&&points->at(2).y==74&&points->at(0).z==0);
     auto identity=points;ImageBounds r{1,2,11,22};points=transform_bitmap_rectangle(h.state,&r,points,nullptr,h);check(points==identity);}
    {Services h;h.state.surface=nullptr;h.target=true;auto p=transform_bitmap_rectangle(h.state,1,2,3,4,nullptr,false,nullptr,h);
     check(p->at(0).y==78&&h.calls==std::vector<std::string>{"alive","target"});
     h.target=false;h.calls.clear();p=transform_bitmap_rectangle(h.state,1,2,3,4,p,false,nullptr,h);check(p->at(0).y==58&&h.calls.back()=="screen");
     h.coordinates.use_surface_height=false;h.calls.clear();p=transform_bitmap_rectangle(h.state,1,2,3,4,p,false,nullptr,h);check(p->at(0).y==-2&&h.calls.empty());}
    {Services h;auto short_points=std::make_shared<std::vector<BitmapPoint>>(2);fails([&]{transform_bitmap_rectangle(h.state,1,2,3,4,short_points,true,nullptr,h);});
     check(short_points->at(0).x==1&&short_points->at(1).x==4&&h.calls.empty());
     h.state.mesh_modes.reset();auto p=std::make_shared<std::vector<BitmapPoint>>(4,BitmapPoint{7,8,9});fails([&]{transform_bitmap_rectangle(h.state,1,2,3,4,p,true,nullptr,h);});
     check(p->at(3).x==1&&p->at(3).y==6&&p->at(3).z==9);}
    {Services h;h.tx=3;h.state.begin_fill=true;h.state.call_draw_bitmap=true;ImageBounds fill{10,20,30,40};
     auto p=std::make_shared<std::vector<BitmapPoint>>(5,BitmapPoint{7,8,9});
     transform_bitmap_rectangle(h.state,1,2,3,4,p,true,&fill,h);
     check(h.lookups==std::vector<std::array<float,2>>{{10,20},{30,20},{30,40},{10,40},{10,20}});
     check(h.additions[0]==std::array<float,4>{10,20,4,98}&&p->at(4).x==10&&p->at(4).y==92&&p->at(4).z==0);
     h.replacement=std::array<float,2>{55,66};h.additions.clear();transform_bitmap_rectangle(h.state,1,2,3,4,p,true,nullptr,h);
     check(p->at(3).x==55&&p->at(3).y==66&&h.additions.empty());}
    std::cout<<"bitmap geometry contract ok\n";
}
