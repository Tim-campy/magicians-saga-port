#include "platform/desktop/bitmap_backend.hpp"
#include "core/gameplay/mesh_submission.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace saga::core;
using namespace saga::desktop;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"bitmap backend line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
saga::assets::Seb file(std::int16_t texture){
    saga::assets::Seb s{0,4,{{0,1,0}},{{}}};auto& r=s.records[0];r.value_count=10;
    r.values={0,texture,0,0,2,2,0,0,0,0};return s;
}
void insert(RenderQueue& queue,int resource,int clip,int depth,int x){
    RenderCommand c;c.values={-1,resource,depth,clip,0,x,4,-1,-1,-1,-1,-1,-1,0,255,-1,0,-1,0};insert_render_command(queue,c);
}
struct DepthHost:BitmapDepthHost {
    MeshDepthState state;MeshBuilderDepth builder{&state};bool enabled=true;
    DepthHost(){begin_mesh_depth_frame(state);configure_mesh_builder_depth(builder,24,false);}
    bool surface_is_null(MeshDepthSurface*)override{check(false);return true;}
    float low_depth_scaling()override{check(false);return 0;}
    MeshDepthState& mesh_depth_state()override{return state;}
    MeshBuilderDepth& depth_builder(std::size_t,std::uint32_t,std::int32_t)override{return builder;}
    bool depth_step_enabled()override{return enabled;}
};
struct QuadGroup:SubmissionGroup {const BitmapQuad* quad=nullptr;};
struct SubmissionHost:MeshSubmissionHost {
    std::vector<const BitmapQuad*> drawn;int clears=0;
    void clear_depth()override{++clears;}
    void set_material_pass(SubmissionMaterial& m)override{check(m.unused_frames==0);}
    void draw_mesh(SubmissionGroup& group,OffscreenMatrix)override{drawn.push_back(static_cast<QuadGroup&>(group).quad);}
    void after_mesh_draw(SubmissionMaterial&,SubmissionGroup&)override{}
    std::int32_t check_count()override{check(false);return 0;} // These supplied materials have no image-lifetime binding.
};
}
int main()try{
    auto a=file(2),b=file(7);std::vector<const saga::assets::Seb*> aa(4,nullptr),bb(6,nullptr);aa[3]=&a;bb[5]=&b;
    PreparedAnimationResource first(aa),second(bb);
    PixelImage image_a,image_b,image_global;image_a.width=8;image_a.height=8;image_b.width=16;image_b.height=4;image_b.render_queue=2;
    image_global.width=4;image_global.height=4;
    auto ia=std::make_shared<std::vector<RenderImage*>>(3,nullptr),ib=std::make_shared<std::vector<RenderImage*>>(8,nullptr),ig=std::make_shared<std::vector<RenderImage*>>(3,nullptr);
    ia->at(2)=&image_a;ib->at(7)=&image_b;ig->at(2)=&image_global;
    RenderDrawResource ar{&first.resource(),ia},br{&second.resource(),ib},gr{&first.resource(),ig};
    BitmapApplication app,global;*application_resource_slot(app,1)=&ar;*application_resource_slot(app,3)=&br;global.characters=&gr;
    check(!application_resource_slot(app,0)&&!application_resource_slot(app,6)&&get_application_resource(app,3)==&br);
    BitmapTextures textures;const auto key_a=textures.bind(image_a,8,8,0),key_b=textures.bind(image_b,16,4,3),key_g=textures.bind(image_global,4,4,0);
    check(key_a!=key_b&&textures.bind(image_a,8,8,0)==key_a);fails([&]{textures.bind(image_a,8,8,3);});
    PixelImage unknown;fails([&]{textures.key(unknown);});
    BitmapApplication* live_global=&global;
    SpriteMatrix world;BitmapBackend backend(app,[&]{return live_global;},textures,64,world);RenderQueue queue=make_render_queue();
    insert(queue,1,3,150007,3);insert(queue,3,5,300009,12);insert(queue,1,3,0,24);
    RenderDrawState state{&queue,0,&backend.matrix()};
    auto draw=[&]{draw_render_queue(state,&backend.graphics(),0,backend.resources());};draw();
    const auto initial=backend.quads();check(initial.size()==4);
    check(initial[0].texture==key_a&&initial[1].texture==key_b&&initial[2].texture==key_b&&initial[3].texture==key_a);
    check(initial[0].render_queue==2000&&initial[1].render_queue==2452&&initial[2].render_queue==3002&&initial[3].render_queue==2000);
    check(initial[0].depth.group==1&&initial[0].depth.index==7&&initial[1].depth.group==2&&initial[1].depth.index==9);
    check(initial[0].uv.right>initial[1].uv.right&&initial[0].uv.right==initial[3].uv.right);
    backend.clear_quads();draw();check(backend.quads().size()==4&&backend.quads()[3].uv.right==initial[0].uv.right);
    DepthHost native;backend.set_depth_host(&native);backend.clear_quads();draw();
    const auto& depth_quads=backend.quads();check(depth_quads.size()==4&&depth_quads[0].native_depth);
    const float first_z=749993.0F-native.builder.step;
    check(depth_quads[0].points[0].z==first_z&&depth_quads[0].points[3].z==first_z);
    check(depth_quads[2].points[0].z<depth_quads[1].points[0].z); // One decrement per material pass, not per vertex.
    // Feed actual native-Z bitmap passes through ready LayerMesh records. The caller supplies
    // material/builder ownership; this does not infer production batch construction from texture IDs.
    {std::array<QuadGroup,4> groups;std::array<SubmissionLayer,4> layers;
        std::array<SubmissionMaterial,3> materials;std::array<SubmissionRecord,4> records;
        auto rows=std::make_shared<std::vector<SubmissionRecord*>>();
        for(std::size_t i=0;i<4;++i){const auto material=i==3?0:i;
            groups[i].index_count=6;groups[i].quad=&depth_quads[i];materials[material].queue.absolute=depth_quads[i].render_queue;
            layers[i].index=depth_quads[i].depth.group;layers[i].groups=std::make_shared<SubmissionGroupList>();layers[i].groups->values={&groups[i]};
            init_submission_record(records[i],&materials[material],&layers[i]);rows->push_back(&records[i]);}
        sort_submissions(rows,0,3);MeshSubmissionState submission;
        submission.cursors=std::make_shared<std::vector<std::int32_t>>(1,0);submission.lengths=std::make_shared<std::vector<std::int32_t>>(1,4);
        submission.lists=std::make_shared<std::vector<SubmissionRecords>>(1,rows);submission.matrices=std::make_shared<std::vector<OffscreenMatrix>>(1);
        submission.used_materials=std::make_shared<std::vector<SubmissionMaterial*>>();SubmissionHost host;
        submit_mesh_streams(submission,1,host);
        check((host.drawn==std::vector<const BitmapQuad*>{&depth_quads[3],&depth_quads[0],&depth_quads[1],&depth_quads[2]}));
        check(host.clears==2&&submission.used_count==4&&submission.used_materials->at(0)==submission.used_materials->at(1));
        check(host.drawn[3]->native_depth&&host.drawn[3]->points[0].z==depth_quads[2].points[0].z);}
    check(native.state.layer==0);backend.set_depth_host(nullptr);
    // Existing global AppData override is distinct from the primary manager and its texture dimensions.
    clear_render_queue(queue);insert(queue,1,3,0,3);auto row=queue.buffers->at(0)->at(0);row->at(render_texture)=2;row->at(render_image_resource)=1;
    backend.clear_quads();draw();check(backend.quads().size()==1&&backend.quads()[0].texture==key_g);
    check(backend.quads()[0].uv.right>initial[0].uv.right);
    // Caller-supplied world transform affects actual vertices across frames.
    const auto x=backend.quads()[0].points[0].x;post_translate_matrix(world,5,0);backend.set_matrix(&world);
    backend.clear_quads();draw();check(backend.quads()[0].points[0].x==x+5);
    backend.set_matrix(nullptr);backend.clear_quads();draw();check(backend.quads()[0].points[0].x==x);
    // Resource fields are live; an absent primary manager skips before reading global override.
    app.characters=nullptr;backend.clear_quads();draw();check(backend.quads().empty());
    app.characters=&ar;global.characters=nullptr;fails(draw);
    BitmapApplication replacement;replacement.characters=&ar;live_global=&replacement;
    backend.clear_quads();draw();check(backend.quads()[0].texture==key_a);
    live_global=nullptr;fails(draw);
    std::cout<<"bitmap backend contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
