#include "core/gameplay/animation_pass.hpp"
#include "core/gameplay/contact_collision.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <cstdlib>
#include <limits>

using namespace saga::core;
void check(bool value) { if(!value)std::abort(); }
template<class E=std::exception,class F> void fails(F f) {
    bool caught=false;try{f();}catch(const E&){caught=true;}check(caught);
}
struct Fixture final:PlaybackPool {
    std::vector<std::int32_t> slots{2,10};SubsetFilter filter{&slots};
    EntitySubset group{filter,[](const ComponentSlots&){return 1;}};
    SubsetMembership membership{{&group}};ComponentSlots entity{35,&membership};
    EntitySprite sprite;EntityPlayback playback;Component tag;ContactDirection direction;
    SharedAnimationClip clip{0,4};
    AnimationResource resource{std::make_shared<std::vector<SharedAnimationClip*>>(1,&clip)};
    AnimationSystemState state{&group,std::make_shared<std::vector<AnimationResource*>>(1,&resource)};
    EntityPlayback& acquire_playback()override{return playback;}
    Fixture(){sprite.animation={0,0,0};entity.add(2,&sprite);add_animation_component(entity,1,-1,*this);}
    void run(){update_animations(state,{false,true});}
};
struct InitHost final:AnimationInitializationHost {
    Fixture f;AnimationSystemState* observing=nullptr;
    std::vector<std::vector<std::int32_t>> filters;std::vector<int> indices;
    bool fail_subset=false,null_subset=false;int fail_index=-1;
    std::function<void(int)> query;
    EntitySubset* subset(const std::vector<std::int32_t>& all)override{
        filters.push_back(all);if(fail_subset)throw std::runtime_error("subset");return null_subset?nullptr:&f.group;
    }
    AnimationResource* resource(std::int32_t i)override{
        check(observing&&observing->subset==(null_subset?nullptr:&f.group)&&observing->resources);
        indices.push_back(i);if(query)query(i);if(i==fail_index)throw std::runtime_error("resource");
        return i==2?nullptr:&f.resource; // Null managers are valid entries.
    }
};
int main(){
    constexpr auto hi=std::numeric_limits<std::int32_t>::max(),lo=std::numeric_limits<std::int32_t>::min();
    { SharedAnimationClip c{5,4};advance_shared_animation(c,{true,true});check(c.frame==5);
      advance_shared_animation(c,{false,false});check(c.frame==5);
      advance_shared_animation(c,{false,true});check(c.frame==2);
      c={hi,3};advance_shared_animation(c,{false,true});check(c.frame==-2);
      c={hi,-1};advance_shared_animation(c,{false,true});check(c.frame==0);
      c={-3,0};advance_shared_animation(c,{false,true});check(c.frame==-2);
    }
    { Fixture f;bool enabled=true;
      GameplayUpdate system(19,enabled,[&](std::int32_t){f.run();});std::vector<UpdateSystem*> systems{&system};
      UpdateSchedule schedule{&systems,nullptr};update_systems(schedule,2);
      check(f.clip.frame==1&&f.sprite.animation.frame==1&&f.group.size()==1);
      enabled=false;update_systems(schedule,-1);check(f.clip.frame==1&&f.sprite.animation.frame==1);
      enabled=true;update_systems(schedule,-1);check(f.clip.frame==2&&f.sprite.animation.frame==2);
      update_animations(f.state,{true,true});check(f.clip.frame==2&&f.sprite.animation.frame==3);
      update_animations(f.state,{false,false});check(f.clip.frame==2&&f.sprite.animation.frame==0);
    }
    { Fixture f;f.sprite.animation.frame=3;f.playback.state={2,12};f.direction.direction=3;f.direction.locked=true;
      f.entity.add(14,&f.tag).add(12,&f.direction);f.run();
      check(f.sprite.animation.clip==15&&f.sprite.animation.frame==0&&f.playback.state.return_clip==-1);
      // New clip 15 is outside this resource: Update must not look it up after Restore.
    }
    { Fixture f;f.sprite.animation.frame=3;f.playback.state.return_clip=12;f.entity.add(14,&f.tag);
      fails<MissingAnimationComponent>([&]{f.run();});
      check(f.sprite.animation.clip==12&&f.sprite.animation.frame==0&&f.playback.state.return_clip==12&&f.clip.frame==1);
    }
    { Fixture f;f.entity.add(16,&f.tag).add(12,&f.direction);f.direction.direction=3;
      change_animation(live_animation(f.entity),8,false);check(f.sprite.animation.clip==11);
      cut_in_animation(live_animation(f.entity),20,8);check(f.sprite.animation.clip==20);
      restore_animation(live_animation(f.entity));check(f.sprite.animation.clip==11);
      f.entity.remove(16);f.entity.add(24,&f.tag);
      cut_in_animation(live_animation(f.entity),20,8);check(f.sprite.animation.clip==23);
      restore_animation(live_animation(f.entity));check(f.sprite.animation.clip==8);
    }
    for(int mode=0;mode<4;++mode){Fixture f;f.sprite.animation.frame=7;
      if(mode==0)f.sprite.animation.resource=-1;if(mode==1)f.sprite.animation.clip=-1;
      if(mode==2)f.sprite.animation.resource=1;if(mode==3)f.state.resources->at(0)=nullptr;
      f.run();check(f.sprite.animation.frame==7&&f.clip.frame==(mode==3?0:1));
    }
    { Fixture f;f.sprite.animation.clip=1;fails<std::out_of_range>([&]{f.run();});
      check(f.clip.frame==1&&f.sprite.animation.frame==0);
    }
    { Fixture f;f.resource.clips->at(0)=nullptr;fails<std::invalid_argument>([&]{f.run();});check(f.sprite.animation.frame==0); }
    { Fixture f;AnimationResource bad;f.state.resources->push_back(&bad);
      fails<std::invalid_argument>([&]{f.run();});check(f.clip.frame==1&&f.sprite.animation.frame==0);
    }
    { Fixture f;f.entity.change(10,nullptr); // Change forwards notifications without recomputing membership.
      check(f.group.size()==1);fails<MissingAnimationComponent>([&]{f.run();});check(f.clip.frame==1&&f.sprite.animation.frame==0);
    }
    { Fixture f;f.state.subset=nullptr;fails<std::invalid_argument>([&]{f.run();});check(f.clip.frame==1); }
    { Fixture f;f.state.resources.reset();fails<std::invalid_argument>([&]{f.run();});check(f.sprite.animation.frame==0); }
    { Fixture f;f.clip.max_frame=0;f.sprite.animation.frame=3;f.playback.state.rate=2;f.run();check(f.clip.frame==1&&f.sprite.animation.frame==5);
      f.clip.max_frame=-3;f.run();check(f.sprite.animation.frame==1);
      f.sprite.animation.frame=-3;f.playback.state.rate=-2;f.run();check(f.sprite.animation.frame==-2);
      f.clip.max_frame=-1;f.sprite.animation.frame=hi;f.playback.state.rate=1;f.run();check(f.sprite.animation.frame==0);
      f.clip.max_frame=4;f.sprite.animation.frame=hi;f.run();check(f.sprite.animation.frame==0);
      f.sprite.animation.frame=lo;f.playback.state.rate=-1;f.run();check(f.sprite.animation.frame==3);
    }
    { Fixture f;f.sprite.animation.frame=4;f.playback.state={0,90};f.run();
      check(f.sprite.animation.clip==90&&f.sprite.animation.frame==0&&f.playback.state.return_clip==-1);
    }
    { Fixture f;f.clip.max_frame=100;f.resource.clips->push_back(&f.clip);f.state.resources->push_back(&f.resource);
      f.run();check(f.clip.frame==4&&f.sprite.animation.frame==1); // Shared references are not deduplicated.
    }
    { InitHost host;AnimationSystemState state;host.observing=&state;
      initialize_animation(state,&host);check(host.filters==std::vector<std::vector<std::int32_t>>{{2,10}});
      check(host.indices.size()==24&&host.indices.front()==0&&host.indices.back()==23&&state.resources->at(2)==nullptr);
      auto old=state.resources;host.fail_index=5;fails([&]{initialize_animation(state,&host);});
      check(state.resources!=old&&state.resources->at(4)==&host.f.resource&&state.resources->at(5)==nullptr&&old->at(23)==&host.f.resource);
      auto partial=state.resources;host.fail_subset=true;fails([&]{initialize_animation(state,&host);});check(state.resources==partial);
      fails([&]{initialize_animation(state,nullptr);});check(state.resources==partial);
      host.fail_subset=false;host.fail_index=-1;host.null_subset=true;initialize_animation(state,&host);check(!state.subset&&state.resources->size()==24);
    }
    { InitHost host;AnimationSystemState state;host.observing=&state;AnimationResources original;
      host.query=[&](int i){if(i==0){original=state.resources;state.resources=std::make_shared<std::vector<AnimationResource*>>(24,nullptr);}};
      initialize_animation(state,&host);check(original->at(0)==&host.f.resource&&original->at(1)==nullptr);
      check(!state.resources->at(0)&&state.resources->at(1)==&host.f.resource&&state.resources->at(23)==&host.f.resource);
    }
}
