#include "platform/desktop/bitmap_mesh_frame.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace saga::core;
using namespace saga::desktop;
namespace {
void check_at(bool v,int line){if(!v){std::cerr<<"bitmap mesh frame line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Queries:MeshDepthHost {
    bool surface_is_null(MeshDepthSurface*)override{check(false);return false;}
    float low_depth_scaling()override{check(false);return 0;}
};
struct Fixture {
    saga::assets::Seb seb{0,4,{{0,1,std::int16_t{0}}},{{}}};
    PreparedAnimationResource animations;
    PixelImage image;std::shared_ptr<std::vector<RenderImage*>> images;
    RenderDrawResource resource;BitmapApplication app;BitmapTextures textures;SpriteMatrix world;
    Queries queries;MeshDepthState depth;SubmissionMaterial material;
    std::unique_ptr<BitmapBackend> backend;
    Fixture():animations(([&]{seb.records[0].value_count=10;seb.records[0].values={0,0,0,0,1,1,0,0,0,0};return std::vector<const saga::assets::Seb*>{&seb};})()),
        images(std::make_shared<std::vector<RenderImage*>>(1,&image)),resource{&animations.resource(),images}{
        image.width=image.height=1;app.characters=&resource;textures.bind(image,1,1,0);backend=std::make_unique<BitmapBackend>(app,app,textures,32,world);
    }
    RenderCommand command(std::int32_t d=0){RenderCommand c;c.values={-1,1,d,0,0,8,8,16,16,-1,-1,-1,-1,0,255,-1,0,-1,0};return c;}
    BitmapMeshFrame::Materials provider(){return [this](auto,auto,std::int32_t queue)->SubmissionMaterial&{material.queue.absolute=queue;return material;};}
};
}
int main()try{
    {Fixture f;BitmapMeshFrame frame(*f.backend,f.depth,f.queries,{24,false,[]{return true;}},f.provider());auto q=make_render_queue();insert_render_command(q,f.command(100));
        frame.begin_frame();frame.draw(q,0);frame.finish();check(frame.records().active_count==1&&f.backend->quads().size()==1);
        auto* row=frame.records().rows->at(0);auto* group=row->layer->groups->values.at(0);auto storage=group->geometry->positions;
        check(group->vertex_count==4&&group->index_count==6&&storage->at(0).z==f.backend->quads().at(0).points[0].z);
        check(f.backend->quads().at(0).native_uv.has_value());
        for(std::size_t i=0;i<4;++i)check(f.backend->quads().at(0).native_uv->at(i)==group->geometry->uv->at(i));
        check(group->geometry->uv->at(0)[1]==1-f.backend->quads().at(0).uv.bottom&&frame.uploaded().at(group).size()==1);
        frame.begin_frame();clear_render_queue(q);insert_render_command(q,f.command(150000));frame.draw(q,0);frame.finish();
        check(frame.cache().builders->size()==1&&frame.records().rows->at(0)==row&&row->layer->index==1);
        check(group->vertex_count==0&&group->index_count==0&&group->geometry->positions==storage&&frame.uploaded().at(group).size()==1);}
    // Full emitted batches cross the native index threshold, and reset retains currentGroup/uploaded old groups.
    {Fixture f;BitmapMeshFrame frame(*f.backend,f.depth,f.queries,{24,false,[]{return true;}},f.provider());auto q=make_render_queue();
        for(int i=0;i<9168;++i)insert_render_command(q,f.command());frame.begin_frame();frame.draw(q,0);frame.finish();
        auto* layer=frame.records().rows->at(0)->layer;auto* first=layer->groups->values.at(0);auto* second=layer->groups->values.at(1);
        check(layer->current_group==1&&first->index_count==55002&&second->index_count==6&&frame.uploaded().at(first).size()==9167);
        frame.begin_frame();clear_render_queue(q);insert_render_command(q,f.command());frame.draw(q,0);frame.finish();
        check(layer->groups->values.size()==2&&layer->current_group==1&&first->index_count==0&&second->index_count==6);
        check(frame.uploaded().at(first).size()==9167&&frame.uploaded().at(second).size()==1);}
    {Fixture f;BitmapMeshFrame frame(*f.backend,f.depth,f.queries,{24,true,[]{return false;}},f.provider());auto q=make_render_queue();insert_render_command(q,f.command());
        frame.begin_frame();frame.draw(q,0);frame.finish();check(frame.records().active_count==1);
        check(frame.cache().builders->at(&f.material)->depth.scale==1024&&frame.uploaded().begin()->second.empty());}
    {Fixture f;BitmapMeshFrame frame(*f.backend,f.depth,f.queries,{24,false,[]{return true;}},[&](auto,auto,auto)->SubmissionMaterial&{return f.material;});
        f.material.queue.absolute=-123;auto q=make_render_queue();insert_render_command(q,f.command());frame.begin_frame();fails([&]{frame.draw(q,0);});check(frame.cache().builders->empty());}
    std::cout<<"bitmap mesh frame integration ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
