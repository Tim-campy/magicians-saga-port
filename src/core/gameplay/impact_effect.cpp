#include "impact_effect.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t bits(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return bits(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return bits(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
template<class T>T& require(T* p){if(!p)throw std::invalid_argument("missing impact effect input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return require(dynamic_cast<T*>(e.get(slot)));}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,contact_position_slot);}
std::int32_t cell_coordinate(float position){
    const float v=position/24.0F;
    if(std::isnan(v))return 0;
    if(v==std::numeric_limits<float>::infinity()||v<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(v>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(v);
}
void visit(ImpactCells& out,std::int32_t cx,std::int32_t cz,std::int32_t x,std::int32_t z,std::int32_t depth){
    constexpr std::array<std::array<int,2>,4> directions{{{{0,-1}},{{1,0}},{{0,1}},{{-1,0}}}};
    for(const auto& d:directions){
        const auto nx=add(x,d[0]),nz=add(z,d[1]);bool found=false;
        for(std::size_t i=0;i<out.size;++i)if(out.cells[i].x==nx&&out.cells[i].z==nz){found=true;break;}
        if(!found)out.cells.at(out.size++)={nx,nz,sub(nx,cx),sub(nz,cz)};
        // Already-seen neighbors still recurse; traversal is not breadth-first.
        if(depth>1)visit(out,cx,cz,nx,nz,depth-1);
    }
}
void modification(ComponentSlots& e,std::int32_t type,float scale,std::int32_t angle,
                  std::int32_t frames,std::int32_t anchor,bool destroy,bool loop,ImpactEffectHost& host){
    auto& m=host.acquire_modification();m.type=type;m.x=m.y=m.z=0;m.scale_x=m.scale_y=scale;
    m.angle=angle;m.frame=0;m.max_frame=frames;m.anchor=anchor;m.destroy=destroy;m.loop=loop;m.alpha=255;
    e.add(modify_animation_slot,&m);
}
}
ImpactCells impact_cells(std::int32_t x,std::int32_t z,std::int32_t range){
    if(range>4)throw std::out_of_range("impact range exceeds caller cap");
    ImpactCells result;result.cells[0]={x,z,0,0};result.size=1;
    if(range>=2)visit(result,x,z,x,z,range-1);
    return result;
}
void create_impact_effect(ComponentSlots* bullet,ComponentSlots* owner,const CombatRecordTables& tables,ImpactEffectHost& host){
    auto& e=require(bullet);require(e.get(contact_bullet_slot));
    const auto* data=host.bullet_data(e);
    auto& parent=require(owner);
    const auto& weapon=require(resolve_combat_equipment(parent,EquipmentSlot::weapon,tables));
    const auto& record=require(data);
    const auto attribute=record.attribute,weapon_range=weapon.range;
    const auto range=std::min(add(weapon_range,host.attribute_value(parent,attribute)/25),4);
    const auto cx=cell_coordinate(position(e).x),cz=cell_coordinate(position(e).z);
    const auto cells=impact_cells(cx,cz,range);
    for(std::size_t i=0;i<cells.size;++i){
        const auto& cell=cells.cells[i];
        const float x=position(e).x,y=position(e).y,z=position(e).z;
        const ProjectilePoint at{x+static_cast<float>(bits(static_cast<std::uint32_t>(cell.offset_x)*36U)),
            y,z+static_cast<float>(bits(static_cast<std::uint32_t>(cell.offset_z)*36U))};
        auto* effect=host.create_effect(101,record.effect_image,0,at);
        if(cell.x!=cx||cell.z!=cz){
            modification(require(effect),10,0.7F,0,0,34,false,true,host);
            auto& area=require(host.create_entity(0));
            auto& p=host.acquire_position();p.x=at.x;p.y=y+5.0F;p.z=at.z;
            p.offset_x=p.offset_y=p.offset_z=0;p.parent=nullptr;area.add(contact_position_slot,&p);
            auto& c=host.acquire_collision();c.radius=18;c.offset_x=c.offset_y=c.offset_z=0;c.visited=false;
            area.add(projectile_collision_slot,&c);
            const auto damage=component<ProjectileDamage>(e,projectile_damage_slot).value;
            const auto damage_attribute=component<ProjectileDamage>(e,projectile_damage_slot).attribute;
            auto& d=host.acquire_damage();d.value=damage;d.attribute=damage_attribute;d.parent=owner;
            area.add(projectile_damage_slot,&d);host.add_garbage(area,2);
        }
        if((record.flags&1)!=0)host.add_color(require(effect),255,255,255,3192);
    }
    if(record.category==1){
        if(e.contains(modify_animation_slot)){
            auto* old=e.get(modify_animation_slot);e.remove(modify_animation_slot);host.return_component(modify_animation_slot,old);
        }
        modification(e,5,1.0F,180,20,0,true,false,host);
    }else host.add_garbage(e,1);
}
}
