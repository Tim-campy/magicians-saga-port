#include "party_fixture.hpp"
#include <cmath>
#include <limits>

int main()try{
    {CharacterProgressionRecord record;record.id=5;record.flags=0x104;record.image=77;record.weapon_id=9;
     auto data=bind_party_character(record,{12,88});check(data.id==5&&data.flags==0x104&&data.image==77&&data.weapon_id==9);
     check(data.saved.level==12&&data.saved.experience==88&&data.parameters.values==record.parameters&&data.parameters.level==12);}
    {Fixture f;std::vector<std::int32_t> inserted;
     f.on_added=[&](ComponentSlots&,std::int32_t slot){inserted.push_back(slot);};
     auto& e=saga::core::create_human({3,1,7,36,72,99,49,5,-25},f.ids,f);
     auto& n=f.node(e);
     check(inserted==std::vector<std::int32_t>({0,5,4,7,2,1,10,12,14,23,8,32,20}));
     check(f.registry.contains(n.id)&&!e.contains(28)&&!e.contains(30)&&e.contains(20));
     check(n.position.x==49&&n.position.y==5&&n.position.z==-25&&n.position.offset_x==0&&!n.position.parent);
     check(n.cell.xi==2&&n.cell.yi==-1&&n.depth.value==0&&n.depth.extra==10);
     check(*n.image.arrays.textures==EventIntegers({200,51,51,-2,604}));
     check(*n.image.arrays.resources==EventIntegers({1,1,1,21,22}));
     check(n.image.arrays.u!=n.image.arrays.v&&*n.image.arrays.u==EventIntegers{-1});
     check(n.sprite.animation.clip==39&&n.sprite.animation.frame==0&&*n.sprite.layers==EventIntegers{-1});
     check(n.speed.x==0&&n.speed.y==0&&n.speed.z==0&&n.playback.state.rate==1&&n.playback.state.return_clip==-1);
     check(n.direction.direction==3&&!n.direction.locked);
     check(n.human.character_id==1&&n.human.level==7&&n.human.experience==99&&n.human.formation_x==36&&n.human.formation_z==72);
     check(n.human.state==0&&n.human.frame==0&&n.human.attack_frame==0&&!n.human.opponent&&n.human.invincible_time==0&&n.human.formation_index==0&&n.human.dash_speed==0);
     check(n.human.scripts&&n.human.scripts->empty()&&n.ai.scripts&&n.ai.scripts->array->empty()&&n.ai.scripts->count==0&&n.ai.state==0);
     check(n.parameters.storage&&n.parameters.values->size()==13&&n.parameters.extras&&n.parameters.extras->empty());
     auto* hp=resolve_combat_parameter(e,0);auto* defense=resolve_combat_parameter(e,2);
     check(hp->value==100&&hp->maximum==100&&hp->id==0&&hp->level==7&&hp->max_level==1000);
     check(defense->value==102&&defense->maximum==0&&defense->extra==0&&defense->experience_buffer==0);
     check(*n.equipment.ids==EventIntegers({4,0,0,0}));}
    {Fixture f;HumanHat first{0,11},chosen{2,22},later{2,33};
     *f.hat_rows={&first,&chosen,&later};auto& e=saga::core::create_human({-1,0,4,0,0,0,0,0,0},f.ids,f);auto& n=f.node(e);
     check(n.image.arrays.textures->at(3)==22&&e.contains(28)&&!e.contains(20)&&!e.contains(30));
     check(n.sprite.animation.clip==35&&n.direction.direction==-1);}
    {Fixture f;f.hat_rows->clear();const auto before=f.ids.counter;
     fails([&]{saga::core::create_human({0,0,1},f.ids,f);});check(f.ids.counter==before&&f.nodes.size()==1);
     f.hat_rows->push_back(nullptr);fails([&]{saga::core::create_human({0,0,1},f.ids,f);});check(f.ids.counter==before);
     saga::core::create_human({0,1,1},f.ids,f);check(f.nodes.size()==2); // Non-main never reads hats.
    }
    {Fixture f;f.parameter_keys.reset();auto& group=f.groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{14}));
     fails([&]{saga::core::create_human({0,1,1},f.ids,f);});auto& n=f.nodes.back();
     check(f.ids.counter==2&&f.registry.contains(n.id)&&n.entity.contains(14)&&!n.entity.contains(23)&&!n.entity.contains(8));
     check(group.size()==2&&n.human.scripts&&n.human.scripts->empty());}
    {Fixture f;f.on_added=[&](ComponentSlots&,std::int32_t slot){if(slot==14)f.records[1].flags=2;};
     auto& e=saga::core::create_human({0,1,1},f.ids,f);auto& n=f.node(e);
     check(n.image.arrays.textures->at(3)==-2&&e.contains(28)&&!e.contains(20)); // Main flag is read twice.
    }
    {Fixture f;f.on_added=[](ComponentSlots&,std::int32_t slot){if(slot==7)throw std::runtime_error("image listener");};
     fails([&]{saga::core::create_human({0,1,1},f.ids,f);});auto& n=f.nodes.back();
     check(f.registry.contains(n.id)&&n.entity.contains(7)&&!n.entity.contains(2)&&!n.entity.contains(14));}
    {Fixture f;auto& e=saga::core::create_human({std::numeric_limits<std::int32_t>::max(),1,1,0,0,0,
        std::numeric_limits<float>::infinity(),0,std::numeric_limits<float>::quiet_NaN()},f.ids,f);auto& n=f.node(e);
     check(n.cell.xi==factory_cell_coordinate(std::numeric_limits<float>::infinity())&&n.cell.yi==0);
     check(n.sprite.animation.clip==std::numeric_limits<std::int32_t>::min()+35);}
    {Fixture f;auto& a=saga::core::create_human({0,1,1},f.ids,f);auto& b=saga::core::create_human({0,1,1},f.ids,f);
     auto& one=f.node(a);auto& two=f.node(b);check(one.human.scripts!=two.human.scripts&&one.ai.scripts!=two.ai.scripts);
     check(one.parameters.storage!=two.parameters.storage&&one.parameters.extras!=two.parameters.extras&&one.equipment.ids!=two.equipment.ids);}
}catch(const std::exception& e){std::cerr<<"uncaught human factory error: "<<e.what()<<'\n';return 1;}
