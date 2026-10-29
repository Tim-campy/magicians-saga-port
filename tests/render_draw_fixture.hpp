#pragma once
#include "core/gameplay/render_draw.hpp"
#include <functional>
#include <string>
#include <tuple>

// Graphics, images and matrix operations are recorded inputs/outputs, not a pixel backend.
struct RenderDrawServices:saga::core::RenderDrawHost,saga::core::RenderGraphics,
                          saga::core::RenderDepth,saga::core::RenderMatrix {
    using Resource=saga::core::RenderDrawResource;
    using Clip=saga::core::SharedAnimationClip;
    saga::core::RenderQueue queue=saga::core::make_render_queue();
    saga::core::RenderDrawState state{&queue,0,this};
    saga::core::RenderImage first_image,second_image;
    std::array<Clip,128> clips{};
    saga::core::AnimationResource animations{std::make_shared<std::vector<Clip*>>()};
    Resource catalog{&animations,std::make_shared<std::vector<saga::core::RenderImage*>>()};
    Resource global{&animations,std::make_shared<std::vector<saga::core::RenderImage*>>()};
    saga::core::SpriteArray supplied=std::make_shared<saga::core::SpriteValues>(saga::core::SpriteValues{0,0,3,4,10,20,-2,-5,0,0,0,77});
    saga::core::SebStaticWork work;
    std::vector<std::string> calls;std::function<void(const std::string&)> hook;
    std::function<Resource*(std::int32_t)> resource_provider,global_provider;
    std::vector<std::pair<std::int32_t,std::int32_t>> depths,missing;
    std::vector<std::array<std::int32_t,4>> invalid;
    std::vector<std::tuple<std::int32_t,std::int32_t,std::int32_t>> render_modes;
    std::vector<std::pair<std::int32_t,std::int32_t>> blends;
    std::vector<saga::core::RenderImageDraw> draws;
    std::vector<std::int32_t> reversals;std::vector<saga::core::RenderMatrix*> pushed_matrices;
    std::vector<std::array<float,9>> matrices;std::vector<std::pair<float,float>> translations;
    std::vector<std::array<float,3>> rotations;std::vector<float> tangent_angles;
    int render_pops=0,blend_pops=0,matrix_pops=0,resets=0;
    bool missing_mesh=false;float tangent=0.25F;
    RenderDrawServices(){
        for(auto& clip:clips)animations.clips->push_back(&clip);
        *catalog.images={&first_image,&second_image,nullptr,&second_image};
        *global.images={&second_image,&first_image};
    }
    void step(const std::string& call){calls.push_back(call);if(hook)hook(call);}
    Resource* resource(std::int32_t id)override{step("resource");return resource_provider?resource_provider(id):&catalog;}
    Resource* global_resource(std::int32_t id)override{step("global");return global_provider?global_provider(id):&global;}
    saga::core::SpriteArray sprite(Clip& clip,std::int32_t frame,std::int32_t layer)override{
        step("sprite");return clip.sprites?saga::core::seb_sprite(*clip.sprites,work,frame,layer):supplied;
    }
    void invalid_image_override(std::int32_t r,std::int32_t t,std::int32_t c,std::int32_t l)override{invalid.push_back({r,t,c,l});step("invalid");}
    void missing_image(std::int32_t r,std::int32_t t)override{missing.emplace_back(r,t);step("missing");}
    float transform_tangent(float angle)override{tangent_angles.push_back(angle);step("tangent");return tangent;}
    saga::core::RenderDepth* mesh_manager()override{step("mesh");return missing_mesh?nullptr:this;}
    void set_depth(std::int32_t index,std::int32_t relative)override{depths.emplace_back(group,index);if(relative!=0)throw std::invalid_argument("unexpected relative mesh depth");step("depth");}
    void push_render_mode(std::int32_t m,std::int32_t a,std::int32_t i)override{render_modes.emplace_back(m,a,i);step("render+");}
    void pop_render_mode()override{++render_pops;step("render-");}
    void push_blend_mode(std::int32_t m,std::int32_t c)override{blends.emplace_back(m,c);step("blend+");}
    void pop_blend_mode()override{++blend_pops;step("blend-");}
    void push_matrix(saga::core::RenderMatrix& m)override{pushed_matrices.push_back(&m);step("matrix+");}
    void pop_matrix()override{++matrix_pops;step("matrix-");}
    void draw_image(const saga::core::RenderImageDraw& draw)override{draws.push_back(draw);reversals.push_back(reverse);step("draw");}
    void set_values(std::array<float,9> values)override{matrices.push_back(values);step("values");}
    void pre_translate(float x,float y)override{translations.emplace_back(x,y);step("translate");}
    void set_rotation(float angle,float x,float y)override{rotations.push_back({angle,x,y});step("rotate");}
    void reset()override{++resets;step("reset");}
    saga::core::EventInstruction add(std::int32_t depth=123,std::int32_t type=0){
        saga::core::RenderCommand c;c.type=type;c.values={-1,1,depth,0,0,50,60,-1,-1,-1,-1,-1,-1,0,255,-1,0,-1,0};
        saga::core::insert_render_command(queue,c);
        return queue.buffers->at(static_cast<std::size_t>(type))->at(static_cast<std::size_t>(queue.counts->at(static_cast<std::size_t>(type))-1));
    }
    void draw(std::int32_t type=0){saga::core::draw_render_queue(state,this,type,*this);}
};
