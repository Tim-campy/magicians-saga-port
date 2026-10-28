#include "render_fixture.hpp"
#include "render_draw_fixture.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

struct Setup {
    HumanFixture t;
    CellServices cells{[this](const auto& all){return &t.f.groups.get(std::make_shared<ComponentGroupFilter>(all));}};
    CameraServices camera{t,cells.map};RenderServices render{t,camera,cells};
    Setup(){cells.init();camera.init();}
};
int main()try{
    {Setup s;auto& r=s.render;auto& e=r.object();s.camera.form.x=3;s.camera.form.y=4;r.visible.add(&e);r.update();
     check(r.errors.empty()&&r.output.counts->at(0)==2&&r.queries.size()==2);
     check(r.row()==EventIntegers({-1,1,220,2,0,1,0,10,20,3,4,10,20,0,255,-1,0,-1,0}));
     check(r.row(1).at(render_depth)==221&&field<EntityDepth>(e,4).value==220);
     auto first=r.output.buffers->at(0)->at(0);r.update();check(r.output.counts->at(0)==4);clear_render_queue(r.output);r.update();
     check(r.output.counts->at(0)==2&&r.output.buffers->at(0)->at(0)==first);}
    {Setup s;auto& r=s.render;saga::assets::SebRecord a;a.value_count=10;a.values={0,0,7,8,82,20,-41,0,0,0};
     auto b=a;b.values[0]=1;b.values[4]=84;saga::assets::Seb file{0,2,{{0,2,std::int16_t{0}}},{a,b}};
     PreparedAnimationResource loaded(index_resource_catalog({2}),{&file});
     r.resource_provider=[&](std::int32_t id){return id==1?&loaded.resource():nullptr;};auto& e=r.object();r.visible.add(&e);r.update();
     check(r.errors.empty()&&r.output.counts->at(0)==1&&r.row().at(render_u)==7&&r.row().at(render_width)==82);
     field<EntitySprite>(e,2).animation.frame=1;clear_render_queue(r.output);r.update();
     check(r.errors.empty()&&r.row().at(render_width)==84&&r.row().at(render_frame)==1);
     RenderDrawServices drawing;drawing.catalog.animations=&loaded.resource();drawing.state.queue=&r.output;drawing.draw();
     check(drawing.draws.size()==1&&drawing.draws[0].width==84&&drawing.draws[0].x==-37&&drawing.draws[0].y==4);
     check(drawing.draws[0].image==&drawing.first_image&&!drawing.draws[0].scaled);}
    {Setup s;auto& r=s.render;auto& e=r.object();e.remove(5);field<EntityDepth>(e,4).value=17;field<EntitySprite>(e,2).layers=std::make_shared<EventIntegers>(EventIntegers{0});r.visible.add(&e);r.update();
     check(r.row().at(render_depth)==17&&r.row().at(render_alpha)==255&&r.errors.empty());}
    {Setup s;auto& r=s.render;auto& hidden=r.object();Component invisible;hidden.add(21,&invisible);hidden.remove(0);
     auto& no_position=r.object();no_position.remove(0);no_position.remove(2);r.visible.add(&hidden);r.visible.add(&no_position);r.update();
     check(r.output.counts->at(0)==0&&r.errors.empty()&&r.queries.empty());}
    {Setup s;auto& r=s.render;auto& e=r.object();Component sync;e.add(13,&sync);field<EntitySprite>(e,2).animation.frame=8;r.visible.add(&e);r.update();
     check(r.row().at(render_frame)==11&&std::get<1>(r.queries.at(0))==11);}
    {Setup s;auto& r=s.render;auto& e=r.object();field<EntitySprite>(e,2).layers=std::make_shared<EventIntegers>(EventIntegers{1,-1,0});r.visible.add(&e);r.update();
     check(r.output.counts->at(0)==2&&std::get<2>(r.queries.at(0))==1&&std::get<2>(r.queries.at(1))==0);
     check(r.row().at(render_layer)==0&&r.row(1).at(render_layer)==1);}
    {Setup s;auto& r=s.render;auto& e=r.object();auto& sprite=field<EntitySprite>(e,2);
     sprite.layers=std::make_shared<EventIntegers>(EventIntegers{0,1});auto original=sprite.layers;r.visible.add(&e);
     r.hook=[&](const std::string& step){if(step=="sprite"){sprite.layers=std::make_shared<EventIntegers>(EventIntegers{99});original->at(1)=3;}};
     r.update();check(r.errors.empty()&&r.output.counts->at(0)==2&&std::get<2>(r.queries.at(1))==3);}
    {Setup s;auto& r=s.render;auto& e=r.object();r.layer_count=33;r.visible.add(&e);r.update();
     check(r.errors==std::vector<ComponentSlots*>{&e}&&r.output.counts->at(0)==0&&r.state.layers->at(31)==31);}
    {Setup s;auto& r=s.render;auto& e=r.object();auto old=r.state.layers;
     r.hook=[&](const std::string& step){if(step=="layers")r.state.layers=std::make_shared<EventIntegers>(32,99);};
     r.visible.add(&e);r.update();check(r.errors.empty()&&old->at(1)==0&&r.state.layers->at(1)==1);}
    {Setup s;auto& r=s.render;auto& e=r.object();add_image_component(e,{9,-1,5,6,12,14},static_cast<EffectFactoryHost&>(s.t.f));r.visible.add(&e);r.update();
     check(r.row().at(render_texture)==9&&r.row().at(render_image_resource)==-1&&r.row().at(render_u)==5&&r.row().at(render_v)==6);
     check(r.row().at(render_width)==12&&r.row().at(render_source_width)==12&&r.row().at(render_height)==14&&r.errors.empty());}
    {Setup s;auto& r=s.render;auto& e=r.object();add_image_component(e,{9,-2,5,6,12,14},static_cast<EffectFactoryHost&>(s.t.f));field<EntityImage>(e,7).arrays.u.reset();r.visible.add(&e);r.update();
     check(r.output.counts->at(0)==0&&r.errors.empty());} // Resource -2 skips before crop arrays.
    {Setup s;auto& r=s.render;auto& e=r.object();add_image_component(e,{-2,-1,5,6,12,14},static_cast<EffectFactoryHost&>(s.t.f));field<EntityImage>(e,7).arrays.u.reset();r.visible.add(&e);r.update();
     check(r.output.counts->at(0)==0&&r.errors==std::vector<ComponentSlots*>{&e});} // Texture -2 is checked after crop arrays.
    {Setup s;auto& r=s.render;auto& e=r.object();add_image_component(e,{9,-1,-1,-1,-1,-1},static_cast<EffectFactoryHost&>(s.t.f));r.visible.add(&e);r.update();
     check(r.row().at(render_u)==3&&r.row(1).at(render_u)==3&&r.row(1).at(render_source_width)==10);}
    {Setup s;auto& r=s.render;auto& e=r.object();auto& n=s.t.f.node(e);n.modification.type=9;n.modification.angle=30;
     n.modification.x=1;n.modification.y=2;n.modification.scale_x=2;n.modification.scale_y=0.5F;n.modification.anchor=34;n.modification.alpha=100;
     n.color.values={-1,256,128,3000};e.add(15,&n.modification);e.add(6,&n.color);r.visible.add(&e);r.update();
     check(r.errors.empty()&&r.row().at(render_x)==0&&r.row().at(render_y)==11&&r.row(1).at(render_x)==-4&&r.row(1).at(render_y)==18);
     check(r.row().at(render_width)==20&&r.row().at(render_height)==10&&r.row().at(render_alpha)==1100&&r.row().at(render_skew)==30);
     check(static_cast<std::uint32_t>(r.row().at(render_color))==0xff00ff80U);
     n.modification.alpha=255;clear_render_queue(r.output);r.update();check(r.row().at(render_alpha)==3000);}
    {Setup s;auto& r=s.render;auto& a=r.object({20,0,0});auto& b=r.object({10,0,0});auto& broken=r.object();broken.remove(2);auto& skipped=r.object();
     for(auto* e:{&a,&b,&broken,&skipped})r.visible.add(e);r.update();check(r.errors==std::vector<ComponentSlots*>{&broken}&&r.output.counts->at(0)==4);
     check(r.row().at(render_depth)==210&&r.row(3).at(render_depth)==221);}
    {Setup s;auto& r=s.render;auto& e=r.object();r.visible.add(&e);r.bounds=std::make_shared<EventIntegers>(5,0);r.update();
     check(r.errors.size()==1&&r.output.counts->at(0)==0&&field<EntityDepth>(e,4).value==220);}
    {Setup s;auto& r=s.render;auto& e=r.object();field<EntitySprite>(e,2).layers=std::make_shared<EventIntegers>(EventIntegers{0});r.visible.add(&e);
     r.hook=[&](const std::string& step){if(step=="sprite"){field<EntitySprite>(e,2).animation.resource=3;field<EntitySprite>(e,2).animation.clip=7;
         field<EntityDepth>(e,4).value=77;field<ContactPosition>(e,0).x=999;}};
     r.update();check(r.row().at(render_resource)==3&&r.row().at(render_clip)==7&&r.row().at(render_depth)==77&&r.row().at(render_x)==4);}
    {Setup s;auto& r=s.render;auto& a=r.object();auto& b=r.object();r.visible.add(&a);field<EntitySprite>(a,2).layers=std::make_shared<EventIntegers>(EventIntegers{0});
     r.hook=[&](const std::string& step){if(step=="sprite")r.visible.add(&b);};r.update();
     check(r.output.counts->at(0)==1&&r.errors==std::vector<ComponentSlots*>{&a});}
    {Setup s;auto& r=s.render;auto& e=r.object();r.visible.add(&e);auto cursor=r.visible.cursor();r.visible.clear_entities();
     fails([&]{cursor.next();});check(r.visible.size()==0);auto empty=r.visible.cursor();r.visible.clear_entities();fails([&]{empty.next();});
     r.visible.add(&e);check(r.visible.cursor().next()==&e);}
    {Setup s;auto& r=s.render;auto old=r.state.layers;r.cells.map_result=nullptr;initialize_render(r.state,&r);
     check(!r.state.map&&r.state.layers!=old&&r.state.layers->size()==32);auto current=r.state.layers;
     fails([&]{initialize_render(r.state,nullptr);});check(r.state.layers==current);
     r.missing_cursor=true;r.update();check(r.errors.empty());s.camera.current=nullptr;fails([&]{r.update();});check(r.errors.empty());}
    check(static_cast<std::uint32_t>(pack_render_color(256,-2,128))==0xffff0080U);
    check(render_isometric_x(10,6)==4&&render_isometric_y(10,6)==8);
    check(std::isinf(render_isometric_y(std::numeric_limits<float>::max(),std::numeric_limits<float>::max())));
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
