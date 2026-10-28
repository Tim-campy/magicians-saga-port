#include "core/gameplay/animation_resources.hpp"
#include "core/gameplay/effect_factory.hpp"
#include "core/ecs/dynamic_subsets.hpp"
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace saga::core;
namespace {
std::int32_t integer(const std::filesystem::path& argument,std::int32_t maximum) {
    const auto text=argument.string();std::int32_t value=0;
    const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||value<0||value>maximum)
        throw std::invalid_argument("numeric argument outside scenario limits");
    return value;
}
saga::assets::Seb read_seb(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if(!input)throw std::runtime_error("cannot open SEB file");
    const auto size=input.tellg();
    if(size<0||static_cast<std::uint64_t>(size)>saga::assets::SebLimits{}.max_bytes)
        throw std::runtime_error("SEB input exceeds byte limit");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));input.seekg(0);
    if(!bytes.empty()&&!input.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("cannot read complete SEB file");
    return saga::assets::parse_seb(bytes.data(),bytes.size());
}
struct Scenario final:EffectFactoryHost,AnimationInitializationHost,ComponentObserver {
    ComponentEvents events;ComponentSlots entity{35,&events};std::int32_t id=-1;
    EntityRegistry registry{*this};DynamicSubsets groups{registry,[&](const ComponentSlots&){return id;}};
    ApplicationAnimationResources app;EntityIdState ids;
    EffectComponent effect;ContactPosition position;ProjectileSpeed speed;EntitySprite sprite;
    EntityDepth depth;CellComponent cell;EntityImage image;EntityPlayback playback;GarbageLifetime garbage;
    void added(ComponentSlots& e,std::int32_t i,Component* c)override{groups.added(e,i,c);}
    void removed(ComponentSlots& e,std::int32_t i,Component* c)override{groups.removed(e,i,c);}
    void changed(ComponentSlots& e,std::int32_t i,Component* a,Component* b)override{groups.changed(e,i,a,b);}
    bool registered_id(std::int32_t key)override{return registry.contains(key);}
    ComponentSlots* create(std::int32_t type,std::int32_t key)override{
        if(type!=0||ids.counter!=1)throw std::invalid_argument("scenario supports one effect entity");
        id=key;return &entity;
    }
    void register_entity(ComponentSlots& e,bool temporary)override{EntityRegistration r{id,&e,&events};registry.add(&r,temporary);}
    EntitySubset* subset(const std::vector<std::int32_t>& all)override{return &groups.get(std::make_shared<ComponentGroupFilter>(all));}
    AnimationResource* resource(std::int32_t index)override{return get_animation_resource(app,index);}
    std::int32_t clip_max_frame(std::int32_t r,std::int32_t clip)override{return animation_clip_max_frame(app,r,clip);}
    EffectComponent& acquire_effect()override{return effect;}
    ContactPosition& acquire_position()override{return position;}
    ProjectileSpeed& acquire_speed()override{return speed;}
    EntitySprite& acquire_sprite()override{return sprite;}
    EntityDepth& acquire_depth()override{return depth;}
    CellComponent& acquire_cell()override{return cell;}
    EntityImage& acquire_image()override{return image;}
    EntityPlayback& acquire_playback()override{return playback;}
    GarbageLifetime& acquire_garbage()override{return garbage;}
};
}
#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
#else
int main(int argc,char** argv) {
#endif
    if(argc!=6) {
        std::cerr<<"Usage: animation_trace <selected-seb> <clip-id> <passes> <application-update:0|1> <automatic:0|1>\n";
        return 2;
    }
    try {
        const auto clip=integer(argv[2],65535),passes=integer(argv[3],100000);
        const SharedAnimationGates gates{integer(argv[5],1)!=0,integer(argv[4],1)!=0};
        const auto file=read_seb(argv[1]);
        PreparedAnimationResource loaded(index_resource_catalog({clip}),{&file});
        Scenario scenario;scenario.app.effects=&loaded.resource();
        AnimationSystemState state;initialize_animation(state,&scenario);
        create_visual_effect({},clip,-1,false,scenario.ids,scenario);
        auto* clock=loaded.resource().clips->at(static_cast<std::size_t>(clip));
        std::cout<<"{\"scenario\":\"effect_animation\",\"resource\":20,\"clip\":"<<clip
                 <<",\"max_frame\":"<<clock->max_frame<<",\"effect_lifetime\":"<<scenario.garbage.frames
                 <<",\"entity_id\":"<<scenario.id<<",\"group_size\":"<<state.subset->size()<<"}\n";
        for(std::int32_t pass=0;pass<=passes;++pass) {
            if(pass!=0)update_animations(state,gates);
            std::cout<<"{\"pass\":"<<pass<<",\"entity_frame\":"<<scenario.sprite.animation.frame
                     <<",\"shared_frame\":"<<clock->frame<<"}\n";
        }
        return std::cout?0:1;
    } catch(const std::exception& error) {
        std::cerr<<"animation_trace: "<<error.what()<<'\n';return 1;
    }
}
