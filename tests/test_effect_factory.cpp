#include "core/gameplay/effect_factory.hpp"
#include "core/ecs/dynamic_subsets.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:EffectFactoryHost,ComponentObserver {
    ComponentEvents events;ComponentSlots entity{35,&events};std::int32_t id=-9;
    EntityRegistry registry{*this};DynamicSubsets groups{registry,[&](const ComponentSlots&){return id;}};
    EffectComponent effect;ContactPosition position;ProjectileSpeed speed;EffectSprite sprite;EffectDepth depth;GarbageLifetime garbage;
    CellComponent cell;EntityImage image;EntityPlayback playback;std::vector<int> calls;std::function<void(int)> after_add;
    int duration=8;bool fail_resource=false,fail_image=false;
    void added(ComponentSlots& e,std::int32_t i,Component* c)override{calls.push_back(i);groups.added(e,i,c);if(after_add)after_add(i);}
    void removed(ComponentSlots& e,std::int32_t i,Component* c)override{groups.removed(e,i,c);}
    void changed(ComponentSlots& e,std::int32_t i,Component* a,Component* b)override{groups.changed(e,i,a,b);}
    bool registered_id(std::int32_t key)override{return registry.contains(key);}
    ComponentSlots* create(std::int32_t type,std::int32_t key)override{check(type==0);calls.push_back(-2);id=key;return &entity;}
    void register_entity(ComponentSlots& e,bool temporary)override{check(&e==&entity&&!temporary);calls.push_back(-3);EntityRegistration r{id,&e,&events};registry.add(&r,false);}
    std::int32_t clip_max_frame(std::int32_t resource,std::int32_t clip)override{check(resource==20&&clip==7);calls.push_back(-1);if(fail_resource)throw std::runtime_error("resource");return duration;}
    EffectComponent& acquire_effect()override{return effect;}
    ContactPosition& acquire_position()override{return position;}
    ProjectileSpeed& acquire_speed()override{return speed;}
    EffectSprite& acquire_sprite()override{return sprite;}
    EffectDepth& acquire_depth()override{return depth;}
    GarbageLifetime& acquire_garbage()override{return garbage;}
    CellComponent& acquire_cell()override{return cell;}
    EntityImage& acquire_image()override{if(fail_image)throw std::runtime_error("image");return image;}
    EntityPlayback& acquire_playback()override{return playback;}
};
EffectRequest request(){return {{-47.9F,2,48.9F},101,9,20,7,true,false,0,3,nullptr,4,true};}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
int main(){
    {Host h;EntityIdState ids;auto filter=std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{26,0,1});auto& group=h.groups.get(filter);
        auto& cells=h.groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{5}));
        auto& animated_images=h.groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{7,10}));
        auto r=request();r.parent=&h.entity;create_effect(r,ids,h);
        check(h.calls==std::vector<int>{-1,-2,-3,26,0,1,2,5,4,22,7,10});
        check(ids.counter==1&&h.registry.find(0)==&h.entity&&group.size()==1);
        check(h.effect.type==101&&h.effect.value==9&&h.effect.depth&&h.effect.frame==3&&h.effect.max_frame==7&&h.effect.parent==&h.entity);
        check(h.position.x==-47.9F&&!h.position.parent&&h.speed.z==0&&h.cell.xi==-1&&h.cell.yi==2);
        check(cells.size()==1&&h.entity.get(5)==&h.cell&&!h.cell.parent&&h.cell.children&&h.cell.children->empty());
        check(h.sprite.animation.resource==20&&h.sprite.animation.clip==7&&h.sprite.animation.frame==3&&h.sprite.layers==std::vector<std::int32_t>{-1});
        check(h.depth.value==0&&h.depth.extra==40&&h.garbage.frames==7);
        check(animated_images.size()==1&&h.entity.get(7)==&h.image&&h.entity.get(10)==&h.playback);
        check(*h.image.arrays.textures==std::vector<std::int32_t>{4}&&*h.image.arrays.resources==std::vector<std::int32_t>{20});
        check(*h.image.arrays.u==std::vector<std::int32_t>{-1}&&*h.image.arrays.v==*h.image.arrays.u&&*h.image.arrays.width==*h.image.arrays.u&&*h.image.arrays.height==*h.image.arrays.u);
        check(h.playback.state.rate==1&&h.playback.state.return_clip==-1);}
    {Host h;EntityIdState ids;auto r=request();r.loop=true;r.depth=false;r.image=-1;r.animate=false;
        create_effect(r,ids,h);check(h.calls==std::vector<int>{-2,-3,26,0,1,2}&&h.effect.max_frame==0);}
    {Host h;EntityIdState ids;h.duration=0;create_visual_effect({},7,-1,false,ids,h);check(h.effect.max_frame==-1&&h.garbage.frames==-1&&h.effect.type==0&&h.effect.frame==0);}
    {Host h;EntityIdState ids;h.fail_resource=true;fails([&]{create_effect(request(),ids,h);});check(ids.counter==0&&h.calls==std::vector<int>{-1});}
    {Host h;EntityIdState ids;h.fail_image=true;fails([&]{create_effect(request(),ids,h);});
        check(ids.counter==1&&h.entity.contains(22)&&!h.entity.contains(10));}
    {Host h;EntityIdState ids;auto r=request();r.max_frame=11;r.position.x=std::numeric_limits<float>::infinity();r.position.z=std::numeric_limits<float>::quiet_NaN();
        h.after_add=[&](int slot){if(slot==26){r.resource=999;r.frame=88;}};
        create_effect(r,ids,h);check(h.cell.xi==std::numeric_limits<int>::min()/24&&h.cell.yi==0&&h.sprite.animation.resource==20&&h.sprite.animation.frame==3);}
}
