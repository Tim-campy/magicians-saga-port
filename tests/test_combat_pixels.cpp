#include "combat_scene_fixture.hpp"
#include "platform/desktop/bitmap_mesh_frame.hpp"
#include "platform/desktop/gl_bitmap.hpp"
#include <SDL3/SDL_main.h>
#include <filesystem>
#include <set>

using namespace saga::desktop;
namespace {
OffscreenMatrix identity(){OffscreenMatrix m;m.values[0]=m.values[5]=m.values[10]=m.values[15]=1;return m;}
struct Queries:MeshDepthHost {
    bool surface_is_null(MeshDepthSurface*)override{throw std::logic_error("low-depth query not reached");}
    float low_depth_scaling()override{throw std::logic_error("low-depth query not reached");}
};
struct Quit {~Quit(){SDL_Quit();}};
void save(const DecodedImage& pixels,const std::filesystem::path& path){
    std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> surface(SDL_CreateSurfaceFrom(pixels.width,pixels.height,SDL_PIXELFORMAT_RGBA32,
        const_cast<std::uint8_t*>(pixels.rgba.data()),pixels.width*4),SDL_DestroySurface);
    check(surface&&SDL_SaveBMP(surface.get(),path.string().c_str()));
}
}
int main(int argc,char** argv)try{
    check(SDL_Init(SDL_INIT_VIDEO));Quit quit;configure_gl_bitmap_context();
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Live combat pixels",480,640,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);check(window!=nullptr);
    DecodedImage texture{64,64,std::vector<std::uint8_t>(64*64*4,255)};
    for(std::size_t i=0;i<texture.rgba.size();i+=4){texture.rgba[i]=50;texture.rgba[i+1]=200;texture.rgba[i+2]=80;}
    GlBitmapRenderer gpu(*window,480,640,[&](std::size_t key){check(key==0);return GlBitmapTexture{&texture,GlBitmapWrap::clamp,GlBitmapWrap::clamp};});
    // Explicit synthetic two-layer SEBs and image slots, never a replacement for real production catalogs.
    saga::assets::Seb seb{0,1000,{{0,2,std::int16_t{0}},{2,2,std::int16_t{0}}},std::vector<saga::assets::SebRecord>(4)};
    for(std::size_t i=0;i<seb.records.size();++i){auto& r=seb.records[i];r.value_count=10;r.values={static_cast<std::int16_t>(i%2==0?0:1000),0,3,4,10,20,0,0,0,0};}
    PreparedAnimationResource animations(std::vector<const saga::assets::Seb*>(128,&seb));PixelImage image;image.width=image.height=64;
    auto images=std::make_shared<std::vector<RenderImage*>>(1024,&image);RenderDrawResource resource{&animations.resource(),images};
    BitmapApplication app;for(int id:{1,2,3,4,7,8,11,18,19,20,21,22,23})*application_resource_slot(app,id)=&resource;
    BitmapTextures textures;textures.bind(image,64,64,0);SpriteMatrix world; // Explicit identity; the tested camera already projects visible entities.
    BitmapBackend backend(app,app,textures,640,world);MeshDepthState depth;Queries queries;
    // Fixture identity policy is explicit. Production MaterialManager canonicalization remains separate.
    std::map<std::tuple<std::size_t,std::uint32_t,std::int32_t>,SubmissionMaterial> materials;
    BitmapMeshFrame mesh(backend,depth,queries,{24,false,[]{return true;}},[&](auto t,auto k,auto q)->SubmissionMaterial&{
        auto& value=materials[{t,k,q}];value.queue.absolute=q;return value;
    });
    GlMeshBindings bindings;GlMeshSubmissionHost sink(gpu,bindings,[](auto&,auto&){},[]()->std::int32_t{throw std::logic_error("fixture has no image counter");});
    const auto unit=identity();auto vp=unit;vp.values[0]=2.0F/480;vp.values[5]=2.0F/640;vp.values[10]=2.0F/1500100;vp.values[12]=vp.values[13]=vp.values[14]=-1;
    MeshSubmissionState submission;submission.cursors=std::make_shared<std::vector<std::int32_t>>(1,0);submission.lengths=std::make_shared<std::vector<std::int32_t>>(1,0);
    submission.lists=std::make_shared<std::vector<SubmissionRecords>>(1,mesh.records().rows);submission.matrices=std::make_shared<std::vector<OffscreenMatrix>>(1,unit);
    submission.used_materials=std::make_shared<std::vector<SubmissionMaterial*>>();
    std::size_t total_quads=0,peak=0,visible_frames=0;std::set<std::uint64_t> captures;
    for(bool physical:{false,true})run_combat_scene(physical,[&](RenderServices& render,EmitterServices&,HumanFixture&,int frame){
        mesh.begin_frame();mesh.draw(render.output,0);mesh.draw(render.output,1);mesh.finish();bind_bitmap_mesh_frame(bindings,mesh);
        const auto count=backend.quads().size();total_quads+=count;peak=std::max(peak,count);
        std::int32_t indices=0;for(std::int32_t i=0;i<mesh.records().active_count;++i)indices+=submission_index_count(*mesh.records().rows->at(static_cast<std::size_t>(i))->layer);
        check(static_cast<std::size_t>(indices)==count*6);
        for(const auto& q:backend.quads())check(q.native_depth);
        submission.cursors->at(0)=0;submission.lengths->at(0)=mesh.records().active_count;
        gpu.begin_frame(GlBitmapClear::color_and_depth,{0,0,0,0},vp);submit_mesh_streams(submission,1,sink);
        if(frame%10==0){auto pixels=gpu.read_pixels();std::uint64_t hash=14695981039346656037ULL;bool visible=false;
            for(std::size_t i=0;i<pixels.rgba.size();++i){hash=(hash^pixels.rgba[i])*1099511628211ULL;if(i%4==3&&pixels.rgba[i])visible=true;}
            if(visible)++visible_frames;captures.insert(hash);
            if(argc==2&&(frame==0||frame==10||frame==30)){auto path=std::filesystem::path(argv[1]);std::filesystem::create_directories(path);save(pixels,path/(std::string(physical?"physical-":"magic-")+std::to_string(frame)+".bmp"));}
        }
    });
    check(total_quads>0&&peak>1&&visible_frames>0&&captures.size()>1);
    std::cout<<"combat GPU: 240 frames, "<<total_quads<<" quads, peak "<<peak<<", "<<captures.size()<<" distinct sampled frames\n";return 0;
}catch(const std::exception& e){std::cerr<<"combat GPU: "<<e.what()<<'\n';return 1;}
