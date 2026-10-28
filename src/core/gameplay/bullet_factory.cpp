#include "bullet_factory.hpp"
#include "table_catalog.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing bullet factory input");return *p;}
std::int32_t integer(float v){
    if(std::isnan(v))return 0;
    if(v==std::numeric_limits<float>::infinity()||v<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(v>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(v);
}
std::int32_t divide(std::int32_t a,std::int32_t b){if(!b)return 0;if(b==-1&&a==std::numeric_limits<std::int32_t>::min())return a;return a/b;}
void read_record(BulletRecord& r,detail::TableTokens& t){
    r.id=t.integer();r.category=t.integer();r.attribute=t.integer();r.resource=t.take();
    r.image=t.integer();r.trail_image=t.integer();r.impact_resource=t.take();r.effect_image=t.integer();r.flags=t.integer();
}
}
BulletRecord& load_bullet_record(BulletRecord& r,std::string_view line,const EventIntegerParser& parser){
    if(!line.empty()){detail::TableTokens tokens(line,parser);read_record(r,tokens);}return r;
}
BulletTable load_bullet_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    return detail::load_table_records<BulletRecord>(text,normalize,parser,[](detail::TableTokens& t){
        BulletRecord r;read_record(r,t);return r;
    });
}
const BulletRecord* equipment_bullet(const EquipmentCombatRecord* weapon,BulletFactoryHost& host){
    if(required(weapon).bullet_id==-1)return nullptr;
    const auto table=host.bullet_records();auto& row=required(table.get()).at(static_cast<std::size_t>(weapon->bullet_id));
    return row?&*row:nullptr;
}
ComponentSlots& create_bullet(BulletCreation r,BulletFactoryHost& host){
    const auto table=host.bullet_records();auto& row=required(table.get()).at(static_cast<std::size_t>(r.id));
    const auto* record=row?&*row:nullptr;
    auto& world=required(host.bullet_global_world());auto& e=create_entity(world.ids,0,false,world.entities);
    auto& bullet=host.acquire_bullet();bullet.data_id=r.id;e.add(29,&bullet);
    auto& p=host.acquire_position();p.x=r.position.x;p.y=r.position.y;p.z=r.position.z;
    p.offset_x=0;p.offset_y=0;p.offset_z=0;p.parent=nullptr;e.add(0,&p);
    const auto step_x=required(host.bullet_global_map()).step_x;
    const auto step_z=required(host.bullet_global_map()).step_z;
    host.add_cell(e,divide(integer(r.position.x),step_x),divide(integer(r.position.z),step_z));
    auto& speed=host.acquire_speed();speed.x=0;speed.y=0;speed.z=0;e.add(1,&speed);
    auto& sprite=host.acquire_sprite();sprite.animation={19,0,0};sprite.layers={-1};e.add(2,&sprite);
    host.add_image(e,{{required(record).image,19,-1,-1,-1,-1}});
    auto& depth=host.acquire_depth();depth.value=0;depth.extra=0;e.add(4,&depth);
    auto& emitter=host.acquire_emitter();emitter.category=0;emitter.type=0;emitter.interval=2;
    emitter.frame=0;emitter.maximum=-1;emitter.argument=0;e.add(31,&emitter);
    auto& collision=host.acquire_bullet_collision();collision.radius=20;collision.offset_x=0;collision.offset_y=0;
    collision.offset_z=0;collision.visited=false;e.add(32,&collision);
    auto& damage=host.acquire_bullet_damage();damage.value=r.damage;damage.attribute=r.attribute;damage.parent=r.parent;e.add(33,&damage);
    if((record->flags&1)!=0){auto& color=host.acquire_bullet_color();color.values={{255,255,255,3192}};e.add(6,&color);}
    return e;
}
}
