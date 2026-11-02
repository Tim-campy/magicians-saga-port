#include "core/gameplay/effect_factory.hpp"
#include "core/gameplay/animation_pass.hpp"
#include "core/gameplay/combat_damage.hpp"
#include "core/ecs/dynamic_subsets.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
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
struct LabelHost final:DamageLabelHost {
    std::array<std::array<std::int32_t,5>,3> palettes{{{{80,81,82,51,34}},{{35,36,37,51,34}},{{38,39,40,51,34}}}};
    std::vector<std::string> calls;std::vector<std::array<std::int32_t,6>> labels;
    std::vector<std::pair<DamageNumberPalette,std::int32_t>> lookups;
    std::function<void(std::size_t)> callback;std::size_t fail_at=0;
    void step(std::string name){calls.push_back(std::move(name));if(callback)callback(calls.size());if(calls.size()==fail_at)throw std::runtime_error("label callback");}
    std::int32_t damage_number_clip(DamageNumberPalette palette,std::int32_t attribute)override{
        lookups.emplace_back(palette,attribute);step(palette==DamageNumberPalette::normal?"normal":palette==DamageNumberPalette::good?"good":"critical");
        return palettes.at(static_cast<std::size_t>(palette)).at(static_cast<std::size_t>(attribute));
    }
    void create_damage_label(const DamageLabelRequest& r)override{
        step(r.type==1?"number":r.value==1?"guard":"critical label");labels.push_back({r.type,r.value,r.resource,r.clip,r.height,r.max_frame});
    }
};
void label_contracts(){
    for(bool human:{false,true})for(bool critical:{false,true})for(std::int32_t affinity=-1;affinity<=4;++affinity)for(std::int32_t attribute=0;attribute<5;++attribute){
        ComponentSlots attacker(35);Component marker;if(human)attacker.add(14,&marker);LabelHost host;DamageNumbers numbers;
        numbers.amount=17;numbers.affinity=affinity;numbers.critical=critical;produce_damage_labels(&attacker,attribute,numbers,host);
        std::vector<std::string> expected{"normal"};std::vector<std::array<std::int32_t,6>> labels;
        auto palette=DamageNumberPalette::normal;
        if(human){
            if(affinity==0){expected.push_back("good");palette=DamageNumberPalette::good;}
            else if(affinity>=1&&affinity<=3){expected.push_back("normal");if(affinity==2){expected.push_back("guard");labels.push_back({7,1,0,0,30,20});}}
            if(critical){expected.push_back("critical");expected.push_back("critical label");palette=DamageNumberPalette::critical;labels.push_back({7,2,0,0,42,20});}
        }
        expected.push_back("number");labels.push_back({1,17,2,host.palettes[static_cast<std::size_t>(palette)][static_cast<std::size_t>(attribute)],20,30});
        check(host.calls==expected&&host.labels==labels);for(const auto& lookup:host.lookups)check(lookup.second==attribute);
    }
    const std::vector<std::string> complete{"normal","normal","guard","critical","critical label","number"};
    for(std::size_t failure=1;failure<=complete.size();++failure){ComponentSlots attacker(35);Component marker;attacker.add(14,&marker);LabelHost host;DamageNumbers n;
        n.amount=7;n.affinity=2;n.critical=true;host.fail_at=failure;fails([&]{produce_damage_labels(&attacker,0,n,host);});
        check(host.calls==std::vector<std::string>(complete.begin(),complete.begin()+static_cast<std::ptrdiff_t>(failure)));
        check(host.labels.size()==(failure<=3?0U:failure<=5?1U:2U));
    }
    for(bool initially_human:{false,true}){ComponentSlots attacker(35);Component marker;if(initially_human)attacker.add(14,&marker);LabelHost host;DamageNumbers n;
        n.affinity=0;host.callback=[&](std::size_t call){if(call==1){if(initially_human)attacker.remove(14);else attacker.add(14,&marker);}};
        produce_damage_labels(&attacker,1,n,host);check(host.calls==(initially_human?std::vector<std::string>{"normal","number"}:std::vector<std::string>{"normal","good","number"}));
        check(host.labels.back()[3]==(initially_human?81:36));
    }
    // The native amount/affinity/critical locals and the Human branch survive later output callbacks.
    {ComponentSlots attacker(35);Component marker;attacker.add(14,&marker);LabelHost host;DamageNumbers n;n.amount=-7;n.affinity=2;n.critical=true;
     host.callback=[&](std::size_t call){if(call==3){attacker.remove(14);n.amount=999;n.affinity=0;n.critical=false;}};
     produce_damage_labels(&attacker,2,n,host);check(host.calls==complete&&host.labels.back()==std::array<std::int32_t,6>{1,-7,2,40,20,30});}
    // Each palette read is fresh, including the redundant normal lookup for neutral/guard affinities.
    {ComponentSlots attacker(35);Component marker;attacker.add(14,&marker);LabelHost host;DamageNumbers n;n.affinity=1;
     host.callback=[&](std::size_t call){if(call==2)host.palettes[0][0]=999;};produce_damage_labels(&attacker,0,n,host);check(host.labels.back()[3]==999);}
    for(auto attribute:{-1,5,std::numeric_limits<std::int32_t>::max()}){LabelHost host;DamageNumbers n;
     fails([&]{produce_damage_labels(nullptr,attribute,n,host);});check(host.calls==std::vector<std::string>{"normal"}&&host.labels.empty());}
    {LabelHost host;DamageNumbers n;fails([&]{produce_damage_labels(nullptr,0,n,host);});check(host.calls==std::vector<std::string>{"normal"});}
    for(auto amount:{std::numeric_limits<std::int32_t>::min(),0,std::numeric_limits<std::int32_t>::max()}){
     ComponentSlots attacker(35);LabelHost host;DamageNumbers n;n.amount=amount;n.affinity=std::numeric_limits<std::int32_t>::min();n.critical=true;
     produce_damage_labels(&attacker,4,n,host);check(host.labels==std::vector<std::array<std::int32_t,6>>{{1,amount,2,34,20,30}}&&host.calls==std::vector<std::string>{"normal","number"});}
}
int main(){
    label_contracts();
    {Host h;EntityIdState ids;auto filter=std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{26,0,1});auto& group=h.groups.get(filter);
        auto& cells=h.groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{5}));
        auto& animated_images=h.groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{7,10}));
        auto r=request();r.parent=&h.entity;create_effect(r,ids,h);
        check(h.calls==std::vector<int>{-1,-2,-3,26,0,1,2,5,4,22,7,10});
        check(ids.counter==1&&h.registry.find(0)==&h.entity&&group.size()==1);
        check(h.effect.type==101&&h.effect.value==9&&h.effect.depth&&h.effect.frame==3&&h.effect.max_frame==7&&h.effect.parent==&h.entity);
        check(h.position.x==-47.9F&&!h.position.parent&&h.speed.z==0&&h.cell.xi==-1&&h.cell.yi==2);
        check(cells.size()==1&&h.entity.get(5)==&h.cell&&!h.cell.parent&&h.cell.children&&h.cell.children->empty());
        check(h.sprite.animation.resource==20&&h.sprite.animation.clip==7&&h.sprite.animation.frame==3&&*h.sprite.layers==std::vector<std::int32_t>{-1});
        check(h.depth.value==0&&h.depth.extra==40&&h.garbage.frames==7);
        check(animated_images.size()==1&&h.entity.get(7)==&h.image&&h.entity.get(10)==&h.playback);
        check(*h.image.arrays.textures==std::vector<std::int32_t>{4}&&*h.image.arrays.resources==std::vector<std::int32_t>{20});
        check(*h.image.arrays.u==std::vector<std::int32_t>{-1}&&*h.image.arrays.v==*h.image.arrays.u&&*h.image.arrays.width==*h.image.arrays.u&&*h.image.arrays.height==*h.image.arrays.u);
        check(h.playback.state.rate==1&&h.playback.state.return_clip==-1);
        auto& animated=h.groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{2,10}));
        SharedAnimationClip clip{0,8};AnimationResource resource{std::make_shared<std::vector<SharedAnimationClip*>>(8,nullptr)};
        resource.clips->at(7)=&clip;AnimationSystemState state{&animated,std::make_shared<std::vector<AnimationResource*>>(24,nullptr)};
        state.resources->at(20)=&resource;
        for(int i=0;i<5;++i)update_animations(state,{false,true});
        check(h.sprite.animation.frame==0&&clip.frame==5&&h.garbage.frames==7);}
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
