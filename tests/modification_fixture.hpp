#pragma once
#include "emitter_fixture.hpp"
#include "core/gameplay/modification_frame.hpp"

struct ModificationServices final:ModificationInitializationWorld,ModificationFrameHost {
    HumanFixture& t;ModificationSystemState state;
    std::vector<ComponentSlots*> destroyed;std::vector<Component*> returned;
    std::function<void()> on_subset;std::function<void(ComponentSlots&)> on_destroy;
    std::function<void(Component*)> on_return;bool null_subset=false;
    explicit ModificationServices(HumanFixture& value,bool isolate=false):t(value){
        if(isolate)for(auto& n:t.f.nodes)if(n.entity.contains(15))n.entity.remove(15);
        initialize_modifications(state,this);
    }
    EntitySubset* modification_subset(const EventIntegers& all)override{check(all==EventIntegers{15});if(on_subset)on_subset();
        return null_subset?nullptr:&t.f.groups.get(std::make_shared<ComponentGroupFilter>(all));}
    void destroy_modified_entity(ComponentSlots& e)override{destroyed.push_back(&e);if(on_destroy)on_destroy(e);
        destroy_quest_entity(t.f.entity_id(e),true,static_cast<Fixture&>(t.f));}
    void return_modification(Component* p)override{returned.push_back(p);if(on_return)on_return(p);}
    ComponentSlots& make(std::int32_t type,std::int32_t max_frame,bool loop=false,std::int32_t frame=0,bool destroy=false){
        auto& e=create_entity(t.f.ids,0,false,static_cast<Fixture&>(t.f));
        AnimationModificationRequest r;r.type=type;r.max_frame=max_frame;r.loop=loop;r.frame=frame;r.destroy=destroy;
        add_animation_modification(e,r,static_cast<StageRecoveryHost&>(t.f));return e;
    }
    void update(){update_modifications(state,*this);}
};
