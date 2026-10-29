#include "core/gameplay/effect_frame.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>

using namespace saga::core;
namespace {
void check(bool value){if(!value)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Actor {
    ComponentSlots entity{35};EffectComponent effect;ContactPosition position;
    Actor(){entity.add(26,&effect);entity.add(0,&position);}
};
struct Host final:EffectInitializationWorld,EffectFrameHost {
    EffectSystemState state;EntitySubset group{[](ComponentSlots* e){return e&&e->contains(26)&&!e->contains(4);},[](const ComponentSlots&){return 0;}};
    std::vector<ComponentSlots*> destroyed;std::function<void(ComponentSlots&)> on_parent,on_destroy;
    bool dead=false,null_subset=false,fail_subset=false;
    Host(){initialize_effects(state,this);}
    EntitySubset* effect_subset(const EventIntegers& all,const EventIntegers& none)override{
        check(all==EventIntegers{26}&&none==EventIntegers{4});if(fail_subset)throw std::runtime_error("subset");return null_subset?nullptr:&group;
    }
    bool effect_parent_destroyed(ComponentSlots& e)override{if(on_parent)on_parent(e);return dead;}
    void destroy_effect_entity(ComponentSlots& e)override{destroyed.push_back(&e);if(on_destroy)on_destroy(e);}
};
}
int main(){
    const auto low=std::numeric_limits<std::int32_t>::min(),high=std::numeric_limits<std::int32_t>::max();
    {Host h;auto old=h.state.expired;h.state.expired->add(nullptr);initialize_effects(h.state,&h);check(h.state.expired!=old&&h.state.expired->size()==0);
     old=h.state.expired;h.fail_subset=true;fails([&]{initialize_effects(h.state,&h);});check(h.state.expired==old&&h.state.subset==&h.group);
     h.fail_subset=false;h.null_subset=true;initialize_effects(h.state,&h);check(!h.state.subset);fails([&]{update_effects(h.state,h);});
     fails([&]{initialize_effects(h.state,nullptr);});}
    {Host h;const auto version=h.state.expired->version();update_effects(h.state,h);check(h.state.expired->version()==version);}
    for(std::int32_t type=-1;type<=9;++type){
        Host h;Actor a;a.effect.type=type;a.effect.frame=4;a.effect.max_frame=5;a.position.y=7;h.group.add(&a.entity);update_effects(h.state,h);
        const auto frame=type==0?0:((type==1||(type>=4&&type<=7))?5:4);
        check(a.effect.frame==frame&&a.position.y==(type==1?8.0F:7.0F));
    }
    {Actor a;a.effect.depth=true;a.effect.frame=high;update_normal_effect(&a.entity);check(a.effect.frame==high);
     a.effect.depth=false;a.effect.max_frame=0;update_normal_effect(&a.entity);check(a.effect.frame==low);
     a.effect.frame=high;a.effect.max_frame=-1;update_normal_effect(&a.entity);check(a.effect.frame==0);
     a.effect.frame=-5;a.effect.max_frame=3;update_normal_effect(&a.entity);check(a.effect.frame==-1);
     a.effect.frame=high;update_level_up_effect(&a.entity);check(a.effect.frame==low);update_time_effect(nullptr);
     fails([&]{update_normal_effect(nullptr);});a.entity.remove(26);fails([&]{update_damage_number_effect(&a.entity);});check(a.position.y==1);}
    {Host h;Actor a,b,parent;a.effect.parent=&parent.entity;b.effect.parent=&parent.entity;a.effect.type=7;b.effect.type=1;
     h.dead=true;h.group.add(&a.entity);h.group.add(&b.entity);h.on_destroy=[&](ComponentSlots& e){check(a.effect.frame==1&&b.effect.frame==1&&b.position.y==1);h.group.remove(&e,26,e.get(26));};
     update_effects(h.state,h);check(h.destroyed==std::vector<ComponentSlots*>({&a.entity,&b.entity})&&h.group.size()==0&&h.state.expired->size()==0);}
    {Host h;Actor a,parent;a.effect.type=7;a.effect.parent=&parent.entity;h.dead=true;h.group.add(&a.entity);
     h.on_parent=[&](ComponentSlots&){a.effect.type=1;};update_effects(h.state,h);check(a.position.y==1&&a.effect.frame==1);}
    {Host h;Actor a,b,parent;a.effect.parent=&parent.entity;b.effect.parent=&parent.entity;h.dead=true;h.group.add(&a.entity);h.group.add(&b.entity);
     b.entity.change(26,nullptr);fails([&]{update_effects(h.state,h);});check(h.state.expired->size()==1&&h.destroyed.empty());
     b.entity.change(26,&b.effect);h.on_destroy=[](ComponentSlots&){throw std::runtime_error("destroy");};fails([&]{update_effects(h.state,h);});
     check(h.state.expired->size()==3&&h.destroyed.size()==1);}
    {Host h;Actor a,parent;a.effect.parent=&parent.entity;h.dead=true;h.group.add(&a.entity);auto old=h.state.expired;
     h.on_destroy=[&](ComponentSlots&){h.state.expired=std::make_shared<EmitterExpiryQueue>();h.state.expired->add(nullptr);};
     update_effects(h.state,h);check(old->size()==1&&h.state.expired!=old&&h.state.expired->size()==0);}
    {Host h;Actor a,parent;a.effect.parent=&parent.entity;h.dead=true;h.group.add(&a.entity);
     h.on_destroy=[&](ComponentSlots&){h.state.expired->add(nullptr);};fails([&]{update_effects(h.state,h);});check(h.state.expired->size()==2);}
}
