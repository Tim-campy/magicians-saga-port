#pragma once
#include "quest_camera_fixture.hpp"
#include "cell_fixture.hpp"
#include "core/gameplay/render_frame.hpp"
#include <tuple>

// Synthetic SEB bounds are explicit inputs; this does not pretend to decode sprites.
struct RenderServices final:RenderInitializationWorld,RenderFrameHost {
    HumanFixture& t;CameraServices& camera;CellServices& cells;
    EntitySubset visible{[](ComponentSlots*){return true;},[this](const ComponentSlots& e){return static_cast<std::int32_t>(t.f.entity_id(const_cast<ComponentSlots&>(e)));}};
    RenderQueue output=make_render_queue();RenderQueue* application=&output;RenderSystemState state;
    std::array<SharedAnimationClip,128> clocks{};
    AnimationResource resource{std::make_shared<std::vector<SharedAnimationClip*>>()};ApplicationAnimationResources catalog;
    EventInstruction bounds=std::make_shared<EventIntegers>(EventIntegers{0,0,3,4,10,20,0,0,0,0,0,0});
    std::int32_t layer_count=2,extra_depth_rate=100;
    std::vector<ComponentSlots*> errors;std::vector<std::tuple<SharedAnimationClip*,std::int32_t,std::int32_t>> queries;
    std::function<void(const std::string&)> hook;std::vector<std::string> calls;
    std::function<AnimationResource*(std::int32_t)> resource_provider;
    bool missing_cursor=false;
    RenderServices(HumanFixture& fixture,CameraServices& view,CellServices& index):t(fixture),camera(view),cells(index){
        for(auto& clock:clocks){clock.frame=11;clock.max_frame=20;resource.clips->push_back(&clock);}
        catalog.characters=&resource;catalog.common=&resource;catalog.enemies=&resource;catalog.equipment=&resource;
        catalog.quest=&resource;catalog.residents=&resource;catalog.selection=&resource;catalog.chips=&resource;
        catalog.bullets=&resource;catalog.effects=&resource;catalog.hats=&resource;catalog.weapons=&resource;catalog.gauges=&resource;
        cells.ordered_visible=&visible;initialize_render(state,this);
    }
    void step(const std::string& s){calls.push_back(s);if(hook)hook(s);}
    MapComponent* render_map()override{step("map");return cells.map_result;}
    struct Cursor final:RenderCursor {
        EntitySubset::Cursor cursor;explicit Cursor(EntitySubset& group):cursor(group.cursor()){}
        std::optional<ComponentSlots*> next()override{return cursor.next();}
    };
    std::unique_ptr<RenderCursor> visible_render_cursor()override{step("cursor");return missing_cursor?nullptr:std::make_unique<Cursor>(visible);}
    AnimationResource* render_resource(RenderQueue&,std::int32_t id)override{step("resource");return resource_provider?resource_provider(id):get_animation_resource(catalog,id);}
    EventInstruction render_sprite(SharedAnimationClip& clip,std::int32_t frame,std::int32_t layer)override{
        queries.emplace_back(&clip,frame,layer);step("sprite");return bounds;
    }
    std::int32_t render_layer_count(SharedAnimationClip&)override{step("layers");return layer_count;}
    std::int32_t render_extra_depth_rate()override{step("depth-rate");return extra_depth_rate;}
    RenderQueue* current_render_queue()override{step("application");return application;}
    void render_error(ComponentSlots& e,const std::exception&)override{errors.push_back(&e);step("error");}
    void update(){update_render(state,camera,*this);}
    ComponentSlots& object(ProjectilePoint p={10,4,6}){
        return create_z_object(p,1,2,t.f.ids,static_cast<EffectFactoryHost&>(t.f));
    }
    const EventIntegers& row(std::size_t i=0,std::size_t type=0)const{return *output.buffers->at(type)->at(i);}
};
