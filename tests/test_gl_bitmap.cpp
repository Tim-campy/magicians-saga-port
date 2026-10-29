#include "platform/desktop/gl_bitmap.hpp"
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace saga::core;
using namespace saga::desktop;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"GL bitmap line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
OffscreenMatrix identity(){OffscreenMatrix m;m.values[0]=m.values[5]=m.values[10]=m.values[15]=1;return m;}
BitmapQuad quad(std::size_t texture,float z,std::uint32_t key=0x110){
    BitmapQuad q;q.texture=texture;q.shader_key=key;q.pipeline=sprite_pixel_pipeline(key);q.native_depth=true;
    q.points={{{-1,-1,z},{1,-1,z},{1,1,z},{-1,1,z}}};q.uv={0,1,1,0};return q;
}
std::array<int,4> pixel(const DecodedImage& image,int x=16,int y=16){
    const auto at=(static_cast<std::size_t>(y)*static_cast<std::size_t>(image.width)+static_cast<std::size_t>(x))*4;
    return {image.rgba.at(at),image.rgba.at(at+1),image.rgba.at(at+2),image.rgba.at(at+3)};
}
void save(const DecodedImage& image,const std::filesystem::path& path){
    std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> surface(SDL_CreateSurfaceFrom(image.width,image.height,SDL_PIXELFORMAT_RGBA32,const_cast<std::uint8_t*>(image.rgba.data()),image.width*4),SDL_DestroySurface);
    check(surface&&SDL_SaveBMP(surface.get(),path.string().c_str()));
}
struct Depth:BitmapDepthHost {
    MeshDepthState state;MeshBuilderDepth builder{&state};
    Depth(){begin_mesh_depth_frame(state);configure_mesh_builder_depth(builder,24,false);}
    bool surface_is_null(MeshDepthSurface*)override{check(false);return false;}
    float low_depth_scaling()override{check(false);return 0;}
    MeshDepthState& mesh_depth_state()override{return state;}
    MeshBuilderDepth& depth_builder(std::size_t,std::uint32_t,std::int32_t)override{return builder;}
    bool depth_step_enabled()override{return true;}
};
struct Quit {~Quit(){SDL_Quit();}};
}
int main(int argc,char** argv)try{
    check(SDL_Init(SDL_INIT_VIDEO));Quit quit;configure_gl_bitmap_context();
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Native depth contract",32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    check(window!=nullptr);
    std::array<DecodedImage,5> textures{{{1,1,{255,0,0,255}},{1,1,{0,0,255,255}},{1,1,{255,255,255,0}},
        {2,2,{255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}},{2,1,{255,0,0,252,255,0,0,253}}}};
    GlBitmapRenderer renderer(*window,32,32,[&](std::size_t key){return GlBitmapTexture{&textures.at(key),GlBitmapWrap::clamp,GlBitmapWrap::clamp};});
    std::cout<<"GL driver: "<<renderer.driver()<<'\n';const auto unit=identity();
    auto draw=[&](const BitmapQuad& q){renderer.set_material(q.shader_key);renderer.draw(q,unit);};
    auto begin=[&]{renderer.begin_frame(GlBitmapClear::color_and_depth,{0,0,0,0},unit);};
    fails([&]{renderer.read_pixels();});
    // Z, not emission order: the near red rectangle survives a later far blue rectangle.
    begin();draw(quad(0,-0.5F));draw(quad(1,0.5F));check((pixel(renderer.read_pixels())==std::array<int,4>{255,0,0,255}));
    begin();draw(quad(1,0.5F));draw(quad(0,-0.5F));check((pixel(renderer.read_pixels())==std::array<int,4>{255,0,0,255}));
    begin();draw(quad(0,0));draw(quad(1,0));check((pixel(renderer.read_pixels())==std::array<int,4>{0,0,255,255})); // LessEqual.
    begin();draw(quad(0,-2));draw(quad(0,2));check((pixel(renderer.read_pixels())==std::array<int,4>{0,0,0,0}));
    // Frame clear policy is explicit: the shipped Offscreen camera clears depth only.
    begin();draw(quad(0,-0.5F));renderer.begin_frame(GlBitmapClear::preserve,{0,1,0,1},unit);draw(quad(1,0.5F));check(pixel(renderer.read_pixels())[0]==255);
    renderer.begin_frame(GlBitmapClear::depth,{0,1,0,1},unit);check(pixel(renderer.read_pixels())[0]==255);
    draw(quad(1,0.5F));check(pixel(renderer.read_pixels())[2]==255);
    // Transparent disables ZWrite. True alpha discard must also prevent depth writes.
    begin();draw(quad(0,-0.5F,0x210));draw(quad(1,0.5F));check(pixel(renderer.read_pixels())[2]==255);
    begin();draw(quad(2,-0.5F,0x410));draw(quad(1,0.5F));check(pixel(renderer.read_pixels())[2]==255);
    begin();draw(quad(4,-0.5F,0xd10));draw(quad(1,0.5F));auto split=renderer.read_pixels();
    check(pixel(split,8)[2]==255&&pixel(split,24)[0]==255); // 252/255 rejected; 253/255 accepted by >0.99.
    begin();draw(quad(4,0,0x1510));split=renderer.read_pixels();check(pixel(split,8)[0]==255&&pixel(split,24)[3]==0); // <=0.99.
    // Source rows and native bottom-left UVs retain the established image orientation.
    begin();draw(quad(3,0));auto oriented=renderer.read_pixels();
    check((pixel(oriented,4,4)==std::array<int,4>{255,0,0,255}));check(pixel(oriented,27,4)[1]==255);
    check(pixel(oriented,4,27)[2]==255&&pixel(oriented,27,27)[0]==255);
    // Depth-only clear preserves color, overrides a preceding transparent write mask,
    // and is restricted to the active viewport (OpenGL's scissor, not viewport alone).
    begin();draw(quad(0,-0.5F));renderer.set_material(0x210);renderer.set_viewport(0,0,16,32);renderer.clear_depth();
    check(pixel(renderer.read_pixels(),8)[0]==255);renderer.set_viewport(0,0,32,32);draw(quad(1,0.5F));auto cleared=renderer.read_pixels();
    check(pixel(cleared,8)[2]==255&&pixel(cleared,24)[0]==255);
    // A supplied model matrix changes actual coverage; it is not dropped at submission.
    begin();auto model=unit;model.values[0]=0.5F;model.values[12]=0.5F;renderer.set_material(0x110);renderer.draw(quad(0,0),model);
    auto moved=renderer.read_pixels();check(pixel(moved,4)[3]==0&&pixel(moved,24)[0]==255);
    // Every reached blend equation is checked against the public RGBA8 blend contract.
    const std::array<std::uint32_t,6> keys{0x110,0x210,0x4250,0x2090,0x4090,0x50};
    for(auto key:keys){renderer.begin_frame(GlBitmapClear::color_and_depth,{0.2F,0.4F,0.6F,0.5F},unit);const auto background=pixel(renderer.read_pixels());
        auto q=quad(0,0,key);q.color={0.5F,1,1,0.5F};draw(q);const auto out=pixel(renderer.read_pixels());
        const double a=0.5,c=0.5,d=background[0]/255.0,b=background[3]/255.0;double rgb=0,alpha=0;
        switch(q.pipeline.blend){case SpriteBlend::replace:rgb=c;alpha=a;break;case SpriteBlend::alpha:rgb=c*a+d*(1-a);alpha=a+b*(1-a);break;
            case SpriteBlend::add:rgb=c*a+d;alpha=a*a+b;break;case SpriteBlend::subtract_alpha:rgb=d-c*a;alpha=b*(1-a)-a;break;
            case SpriteBlend::subtract_add:rgb=d-c*a;alpha=b-a*a;break;case SpriteBlend::attenuate:rgb=d*(1-a);alpha=a+b*(1-a);break;}
        auto expected=[](double v){return static_cast<int>(std::clamp(v,0.0,1.0)*255.0+0.5);};
        check(std::abs(out[0]-expected(rgb))<=1&&std::abs(out[3]-expected(alpha))<=1);}
    // Concrete AppData -> bitmap emitter -> native vertex Z -> ready-stream merge -> GPU pixels.
    saga::assets::Seb seb{0,4,{{0,1,std::int16_t{0}}},{{}}};seb.records[0].value_count=10;seb.records[0].values={0,0,0,0,1,1,0,0,0,0};
    PreparedAnimationResource animations({&seb});PixelImage red,blue;red.width=red.height=blue.width=blue.height=1;
    auto red_images=std::make_shared<std::vector<RenderImage*>>(1,&red),blue_images=std::make_shared<std::vector<RenderImage*>>(1,&blue);
    RenderDrawResource red_resource{&animations.resource(),red_images},blue_resource{&animations.resource(),blue_images};
    BitmapApplication app,global;app.characters=&red_resource;app.enemies=&blue_resource;BitmapTextures bindings;
    check(bindings.bind(red,1,1,0)==0&&bindings.bind(blue,1,1,0)==1);SpriteMatrix world;BitmapBackend backend(app,global,bindings,32,world);Depth depth;backend.set_depth_host(&depth);
    RenderQueue queue=make_render_queue();for(int resource:{1,3}){RenderCommand c;c.values={-1,resource,resource==1?100:0,0,0,8,8,16,16,-1,-1,-1,-1,0,255,-1,0,-1,0};insert_render_command(queue,c);}
    RenderDrawState drawing{&queue,0,&backend.matrix()};draw_render_queue(drawing,&backend.graphics(),0,backend.resources());const auto& quads=backend.quads();
    check(quads.size()==2&&quads[0].native_depth&&quads[0].points[0].z<quads[1].points[0].z);
    std::array<SubmissionMaterial,2> materials;std::array<SubmissionGroup,2> groups;std::array<SubmissionLayer,2> layers;std::array<SubmissionRecord,2> records;GlMeshBindings gpu_bindings;
    auto rows=std::make_shared<std::vector<SubmissionRecord*>>();
    for(std::size_t i=0;i<2;++i){gpu_bindings.materials[&materials[i]]=quads[i].shader_key;materials[i].queue.absolute=quads[i].render_queue;
        groups[i].index_count=6;gpu_bindings.groups[&groups[i]]={&quads[i],1};layers[i].index=quads[i].depth.group;
        layers[i].groups=std::make_shared<SubmissionGroupList>();layers[i].groups->values={&groups[i]};init_submission_record(records[i],&materials[i],&layers[i]);rows->push_back(&records[i]);}
    sort_submissions(rows,0,1);MeshSubmissionState submission;submission.cursors=std::make_shared<std::vector<std::int32_t>>(1,0);submission.lengths=std::make_shared<std::vector<std::int32_t>>(1,2);
    submission.lists=std::make_shared<std::vector<SubmissionRecords>>(1,rows);submission.matrices=std::make_shared<std::vector<OffscreenMatrix>>(1,unit);submission.used_materials=std::make_shared<std::vector<SubmissionMaterial*>>();
    GlMeshSubmissionHost host(renderer,gpu_bindings,[](auto&,auto&){},[]()->std::int32_t{throw std::logic_error("no image check expected");});
    OffscreenMatrix vp=unit;vp.values[0]=vp.values[5]=2.0F/32;vp.values[10]=2.0F/1500100;vp.values[12]=vp.values[13]=vp.values[14]=-1;
    renderer.begin_frame(GlBitmapClear::color_and_depth,{0,0,0,0},vp);submit_mesh_streams(submission,1,host);auto overlap=renderer.read_pixels();check(pixel(overlap)[0]==255&&pixel(overlap)[2]==0);
    layers[1].index=1;init_submission_record(records[1],&materials[1],&layers[1]);sort_submissions(rows,0,1);submission.cursors->at(0)=0;
    renderer.begin_frame(GlBitmapClear::color_and_depth,{0,0,0,0},vp);submit_mesh_streams(submission,1,host);auto layer=renderer.read_pixels();check(pixel(layer)[2]==255&&pixel(layer)[0]==0);
    if(argc==2){const std::filesystem::path path(argv[1]);std::filesystem::create_directories(path);save(overlap,path/"same-layer-depth.bmp");save(layer,path/"next-layer-clear.bmp");save(oriented,path/"texture-orientation.bmp");save(cleared,path/"viewport-clear.bmp");}
    // Unsupported input is rejected, rather than silently flattening or guessing a material.
    auto invalid=quad(0,0);invalid.native_depth=false;renderer.set_material(invalid.shader_key);fails([&]{renderer.draw(invalid,unit);});
    fails([&]{renderer.set_material(0);});fails([&]{renderer.set_viewport(0,0,33,32);});
    SubmissionMaterial foreign_material;SubmissionGroup foreign_group;
    fails([&]{host.set_material_pass(foreign_material);});fails([&]{host.draw_mesh(foreign_group,unit);});
    renderer.present();std::cout<<"GL bitmap depth pixels ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
