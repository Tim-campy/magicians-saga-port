#include "platform/desktop/mission_checkpoint.hpp"
#include "platform/desktop/mission_profile.hpp"
#include "core/gameplay/state_archive_access.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace saga::desktop;using namespace saga::core;
void check(bool condition){if(!condition)throw std::runtime_error("mission checkpoint contract");}
struct Fixture {
    std::filesystem::path path;
    Fixture(){path=std::filesystem::temp_directory_path()/"saga-checkpoint-synthetic.scene";std::ofstream(path)<<"world 1 0 0 0 1 0 0 0 1\n";}
    ~Fixture(){std::error_code error;std::filesystem::remove(path,error);}
    std::unique_ptr<MissionProfile> profile(){
        auto p=std::make_unique<MissionProfile>(path,true);p->catalog=std::make_unique<GameplayCatalog>();p->geometry=StageGeometry{24,24,0};
        p->entity_world=std::make_unique<MissionEntities>(*p->catalog,p->progress,35,&*p->geometry,
            [](std::int32_t)->AnimationResource*{return nullptr;},[](std::string_view)->std::int32_t{throw std::logic_error("no synthetic resource");});
        return p;
    }
};
void check_aliases_and_observational_reads(Fixture& f){
    auto a=f.profile(),b=f.profile();auto& ea=a->entity_world->raw_entity(0);auto& eb=b->entity_world->raw_entity(0);
    auto& pa=a->entity_world->acquire_position();auto& pb=b->entity_world->acquire_position();ea.add(0,&pa);eb.add(0,&pb);
    check(mission_state_bytes(*a)==mission_state_bytes(*b));
    const auto initial=mission_state_bytes(*a);check(initial==mission_state_bytes(*a));
    pa.offset_z=-0.0F;check(initial!=mission_state_bytes(*a));pa.offset_z=0.0F;check(initial==mission_state_bytes(*a));
    auto& ia=a->entity_world->acquire_image();auto& ib=b->entity_world->acquire_image();ea.add(7,&ia);eb.add(7,&ib);
    ia.arrays.textures=ia.arrays.resources=std::make_shared<std::vector<std::int32_t>>(3,4);
    ib.arrays.textures=ib.arrays.resources=std::make_shared<std::vector<std::int32_t>>(3,4);
    check(mission_state_bytes(*a)==mission_state_bytes(*b));
    ib.arrays.resources=std::make_shared<std::vector<std::int32_t>>(*ib.arrays.textures);
    check(mission_state_bytes(*a)!=mission_state_bytes(*b));
    ib.arrays.resources=ib.arrays.textures;pa.parent=&ea;pb.parent=&eb;
    check(mission_state_bytes(*a)==mission_state_bytes(*b));
    pb.offset_x=1;check(mission_state_digest(*a).logic_hash!=mission_state_digest(*b).logic_hash);
}
void check_tombstones_and_subscriptions(Fixture& f){
    auto a=f.profile(),b=f.profile();std::vector<ComponentSlots*> left,right;
    for(int i=0;i<3;++i){left.push_back(&a->entity_world->raw_entity(0));right.push_back(&b->entity_world->raw_entity(0));}
    a->entity_world->destroy(*left[0]);a->entity_world->destroy(*left[1]);
    b->entity_world->destroy(*right[1]);b->entity_world->destroy(*right[0]);
    check(a->entity_world->registry().find(2)==left[2]);check(b->entity_world->registry().find(2)==right[2]);
    check(mission_state_bytes(*a)!=mission_state_bytes(*b));
    auto c=f.profile(),d=f.profile();
    c->entity_world->events().subscribe(23,EntityEvents::Single{[](ComponentSlots*){}},std::nullopt,"native.A");
    d->entity_world->events().subscribe(23,EntityEvents::Single{[](ComponentSlots*){}},std::nullopt,"native.B");
    check(mission_state_digest(*c).logic_hash!=mission_state_digest(*d).logic_hash);
    auto unsupported=f.profile();unsupported->entity_world->events().subscribe(23,EntityEvents::Single{[](ComponentSlots*){}});
    bool rejected=false;try{mission_state_digest(*unsupported);}catch(const std::logic_error&){rejected=true;}check(rejected);
}
void check_unknown_component_and_rng(Fixture& f){
    struct Unknown final:Component{} unknown;
    auto p=f.profile();auto& e=p->entity_world->raw_entity(0);e.add(21,&unknown);
    bool rejected=false;try{mission_state_digest(*p);}catch(const std::logic_error&){rejected=true;}check(rejected);
    LegacySystemRandom source(42),control(42);std::vector<std::int32_t> seeds;
    StateArchiveAccess::visit(source,[&](const char*,const auto& array,const auto&,const auto&){seeds.assign(array.begin(),array.end());});
    check(seeds.size()==56);check(source.next()==control.next());
    std::vector<std::int32_t> advanced;StateArchiveAccess::visit(source,[&](const char*,const auto& array,const auto&,const auto&){advanced.assign(array.begin(),array.end());});
    check(seeds!=advanced);check(source.next()==control.next());
}
}
int main(){Fixture fixture;check_aliases_and_observational_reads(fixture);check_tombstones_and_subscriptions(fixture);check_unknown_component_and_rng(fixture);std::cout<<"mission checkpoint contracts ok\n";}
