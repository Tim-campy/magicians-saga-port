#include "modification_fixture.hpp"
#include "core/gameplay/combat_rules.hpp"
#include <cmath>
#include <limits>

int main()try{
    {HumanFixture t;ModificationServices h(t,true);auto previous=h.state.expired;previous->add(&t.hero());h.null_subset=true;
     initialize_modifications(h.state,&h);check(!h.state.subset&&h.state.expired!=previous&&h.state.expired->size()==0);
     auto queue=h.state.expired;fails([&]{initialize_modifications(h.state,nullptr);});check(h.state.expired==queue);}
    {HumanFixture t;ModificationServices h(t,true);auto queue=h.state.expired;auto* group=h.state.subset;
     h.on_subset=[]{throw std::runtime_error("subset");};fails([&]{initialize_modifications(h.state,&h);});check(h.state.subset==group&&h.state.expired==queue);}
    {HumanFixture t;ModificationServices h(t,true);h.state.subset=nullptr;fails([&]{h.update();});}
    {HumanFixture t;ModificationServices h(t,true);h.state.expired.reset();fails([&]{h.update();});}
    // Death blink includes frame==maximum and only then destroys after completing the source pass.
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(3,10,false,0,true);auto& m=field<ProjectileModification>(e,15);
     for(int i=0;i<10;++i){h.update();check(e.contains(15)&&m.frame==i+1&&m.scale_x==static_cast<float>(i%2)&&h.destroyed.empty());}
     h.update();check(t.f.node(e).lifetime.destroyed&&!e.contains(15)&&m.frame==11&&m.scale_x==0&&h.destroyed==std::vector<ComponentSlots*>{&e});}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(8,0,false,-20);auto& m=field<ProjectileModification>(e,15);m.angle=77;
     h.on_return=[&](Component* old){check(old==&m&&!e.contains(15)&&m.frame==-20&&m.angle==77);};h.update();check(h.returned.size()==1);}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(8,3,false,-2);auto& m=field<ProjectileModification>(e,15);m.angle=77;
     h.update();check(m.frame==-1&&m.angle==77);h.update();check(m.frame==0&&m.angle==77);h.update();check(m.frame==1&&m.angle==0);}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(8,2,true);auto& m=field<ProjectileModification>(e,15);
     h.update();check(m.frame==1&&m.angle==0);h.update();check(m.frame==2&&m.angle==45);h.update();check(m.frame==0&&m.angle==90&&e.contains(15));}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(8,20,false,7);auto& m=field<ProjectileModification>(e,15);
     h.update();check(m.angle==315);h.update();check(m.angle==0);}
    for(const auto type:{-1,7,9,10,99}){HumanFixture t;ModificationServices h(t,true);auto& e=h.make(type,0,true,4);auto& m=field<ProjectileModification>(e,15);
     m.angle=77;m.scale_x=2;m.scale_y=3;m.x=4;m.y=5;m.alpha=123;h.update();
     check(m.frame==0&&m.angle==77&&m.scale_x==2&&m.scale_y==3&&m.x==4&&m.y==5&&m.alpha==123&&e.contains(15));}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(1,20,false,1);auto& m=field<ProjectileModification>(e,15);
     h.update();check(m.angle==3);h.update();check(m.angle==0);h.update();check(m.angle==-3);}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(0,20,false,5);auto& m=field<ProjectileModification>(e,15);
     h.update();check(m.y==-13&&m.scale_y>0.25F&&m.scale_y<1.0F&&m.angle==0);
     m.frame=11;h.update();check(m.scale_y==1&&m.angle==2);m.frame=18;h.update();check(m.angle==-3);}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(0,1);auto& m=field<ProjectileModification>(e,15);
     h.update();check(std::signbit(m.y)&&m.scale_y==0.25F&&m.angle==0);h.update();check(!e.contains(15));}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(2,10,false,5);auto& m=field<ProjectileModification>(e,15);
     h.update();check(m.alpha==46&&m.scale_x>0.18F&&m.scale_x<0.19F&&m.scale_y>1.4F&&m.scale_y<1.42F);
     m.frame=10;h.update();check(m.scale_x==0&&m.scale_y==1.5F&&m.alpha==0&&!e.contains(15));}
    for(const auto type:{5,6}){HumanFixture t;ModificationServices h(t,true);auto& e=h.make(type,10,false,5);auto& m=field<ProjectileModification>(e,15);
     h.update();check(m.alpha==208);m.frame=10;h.update();check(m.alpha==(type==5?0:255)&&!e.contains(15));}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(4,8,false,0);auto& m=field<ProjectileModification>(e,15);
     h.update();check(m.x==-4);h.update();check(m.x==3);m.frame=8;h.update();check(m.x==0&&std::signbit(m.x));}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(4,std::numeric_limits<std::int32_t>::min());auto& m=field<ProjectileModification>(e,15);
     h.update();check(m.x==1073741824.0F&&!e.contains(15));}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(8,std::numeric_limits<std::int32_t>::max(),false,std::numeric_limits<std::int32_t>::max());
     auto& m=field<ProjectileModification>(e,15);h.update();check(m.frame==std::numeric_limits<std::int32_t>::min()&&!e.contains(15));}
    // Removal captures before notifications; a replacement inserted by a callback survives and the old object returns.
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(7,0);auto* previous=e.get(15);ProjectileModification replacement;
     struct Observer final:ComponentObserver {ComponentSlots* e;ProjectileModification* p;void removed(ComponentSlots& entity,std::int32_t slot,Component*)override{
         if(&entity==e&&slot==15)entity.add(15,p);}} observer;observer.e=&e;observer.p=&replacement;
     // Use a separate slot store so this callback does not replace registry notification wiring.
     ComponentSlots local(35,&observer);observer.e=&local;local.add(15,previous);remove_animation_modification(local,h);
     check(local.get(15)==&replacement&&h.returned.back()==previous);}
    {HumanFixture t;ModificationServices h(t,true);ComponentSlots empty(35);remove_animation_modification(empty,h);check(h.returned==std::vector<Component*>{nullptr});}
    // Expiries read current components only after traversal; cleanup of the first can replace the second's destroy policy.
    {HumanFixture t;ModificationServices h(t,true);auto& a=h.make(7,0);auto& b=h.make(7,0);auto* previous=a.get(15);ProjectileModification replacement;replacement.destroy=true;
     h.on_return=[&](Component* old){check(old==previous);b.change(15,&replacement);};h.update();
     check(!a.contains(15)&&!t.f.node(a).lifetime.destroyed&&t.f.node(b).lifetime.destroyed&&h.destroyed==std::vector<ComponentSlots*>{&b});}
    {HumanFixture t;ModificationServices h(t,true);auto& a=h.make(7,0);auto& b=h.make(8,10);h.on_return=[&](Component*){check(field<ProjectileModification>(b,15).frame==1);};
     h.update();check(!a.contains(15)&&h.state.expired->size()==0);}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(7,0);h.on_return=[](Component*){throw std::runtime_error("pool");};
     fails([&]{h.update();});check(!e.contains(15)&&h.state.expired->size()==1);h.on_return={};fails([&]{h.update();});check(h.state.expired->size()==1);}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(7,0);h.on_return=[&](Component*){h.state.expired->add(&e);};
     fails([&]{h.update();});check(h.state.expired->size()==2&&!e.contains(15));}
    {HumanFixture t;ModificationServices h(t,true);h.make(7,0);auto captured=h.state.expired;h.on_return=[&](Component*){
         h.state.expired=std::make_shared<EmitterExpiryQueue>();h.state.expired->add(&t.hero());};h.update();check(captured->size()==1&&h.state.expired->size()==0);}
    {HumanFixture t;ModificationServices h(t,true);auto& e=h.make(7,1);e.change(15,nullptr);fails([&]{h.update();});}
    // Same shared sine conversion as both easing overloads; no new libm implementation.
    check(sine_degrees(0.0F)==0&&sine_degrees(90.0F)==1&&sine_degrees(270.0F)==-1);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
