#include "core/gameplay/state_component_fields.hpp"
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"component state fields contract failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
struct Fields {
    std::string name;std::size_t calls=0;std::vector<const void*> addresses;
    template<class... T>void operator()(std::string_view type,const T&... values){
        name=std::string(type);++calls;addresses={static_cast<const void*>(std::addressof(values))...};
    }
};
template<class T,class=void>struct has_fields:std::false_type{};
template<class T>struct has_fields<T,std::void_t<decltype(visit_state_fields(std::declval<const T&>(),std::declval<Fields&>()))>>:std::true_type{};
struct UnknownComponent final:Component {std::int32_t future_field=0;};
static_assert(!has_fields<Component>::value&&!has_fields<UnknownComponent>::value,"unknown components must not lose their payload");
template<class T>void expect(const T& value,std::string_view name,std::initializer_list<const void*> addresses){
    Fields fields;visit_state_fields(value,fields);check(fields.calls==1&&fields.name==name&&fields.addresses==std::vector<const void*>(addresses));
}
void payloads_and_bases(){
    MemberExperience member{3,17};expect(member,"MemberExperience",{&member.level,&member.experience});
    DamageParameter parameter;parameter.value=2;parameter.maximum=3;parameter.extra=5;parameter.extra_maximum=7;
    parameter.id=11;parameter.level=13;parameter.max_level=17;parameter.experience=19;parameter.max_experience=23;parameter.experience_buffer=29;
    visit_state_fields(parameter,[](std::string_view name,const auto&... fields){
        check(name=="DamageParameter"&&std::vector<std::int32_t>{fields...}==std::vector<std::int32_t>{2,3,5,7,11,13,17,19,23,29});
    });
    SpriteAnimation animation{1,2,3};expect(animation,"SpriteAnimation",{&animation.resource,&animation.clip,&animation.frame});
    AnimationPlayback playback{4,-1};expect(playback,"AnimationPlayback",{&playback.rate,&playback.return_clip});
    ProjectilePoint point{1.5F,-2.5F,3.5F};expect(point,"ProjectilePoint",{&point.x,&point.y,&point.z});
    StageGeometry geometry{24,25,26};expect(geometry,"StageGeometry",{&geometry.step_x,&geometry.step_z,&geometry.height});
    HumanScriptCache scripts;scripts.count=2;scripts.array->push_back(std::make_shared<int>(7));scripts.array->push_back(scripts.array->front());
    expect(scripts,"HumanScriptCache",{&scripts.array,&scripts.count});
    visit_state_fields(scripts,[](std::string_view name,const auto& array,const auto& count){
        check(name=="HumanScriptCache"&&count==2&&array->size()==2&&array->at(0)==array->at(1));
    });
}
void positions_humans_and_maps(){
    ComponentSlots parent(35),other(35);ContactPosition p;p.x=1;p.y=2;p.z=3;p.offset_x=4;p.offset_y=5;p.offset_z=6;p.parent=&parent;
    expect(p,"ContactPosition",{&p.x,&p.y,&p.z,&p.offset_x,&p.offset_y,&p.offset_z,&p.parent});
    visit_state_fields(p,[&](std::string_view name,const float& x,const float& y,const float& z,const float& ox,const float& oy,const float& oz,ComponentSlots* const& owner){
        check(name=="ContactPosition"&&x==1&&y==2&&z==3&&ox==4&&oy==5&&oz==6&&owner==&parent);
    });
    p.offset_z=-9;p.parent=&other;visit_state_fields(p,[&](std::string_view,const auto&... fields){const auto tuple=std::tie(fields...);check(std::get<5>(tuple)==-9&&std::get<6>(tuple)==&other);});
    HumanRecordLink h;h.level=7;h.experience=103;h.dash_speed=1.25F;h.opponent=&parent;
    h.scripts=std::make_shared<std::vector<std::shared_ptr<void>>>();h.scripts->push_back(std::make_shared<int>(31));
    expect(h,"HumanRecordLink",{static_cast<const MemberExperience*>(&h),&h.character_id,&h.state,&h.invincible_time,&h.attack_frame,&h.frame,
        &h.opponent,&h.formation_x,&h.formation_z,&h.formation_index,&h.dash_speed,&h.scripts});
    visit_state_fields(h,[&](std::string_view name,const auto&... fields){
        const auto tuple=std::tie(fields...);check(name=="HumanRecordLink"&& &std::get<0>(tuple)==static_cast<const MemberExperience*>(&h));
        visit_state_fields(std::get<0>(tuple),[](std::string_view base,const auto& level,const auto& experience){check(base=="MemberExperience"&&level==7&&experience==103);});
        check(std::get<6>(tuple)==&parent&&std::get<10>(tuple)==1.25F&&std::get<11>(tuple)==h.scripts&&std::get<11>(tuple)->at(0)==h.scripts->at(0));
    });
    MapComponent map;map.step_x=11;map.step_z=12;map.height=13;map.width=14;map.size_width=15;map.size_height=16;map.area_width=17;map.area_height=18;
    map.chips=std::make_shared<std::vector<MapRow>>();auto row=std::make_shared<std::vector<ComponentSlots*>>(1,&parent);map.chips->push_back(row);map.chips->push_back(row);map.areas=map.chips;
    expect(map,"MapComponent",{static_cast<const StageGeometry*>(&map),&map.width,&map.size_width,&map.size_height,&map.area_width,&map.area_height,&map.chips,&map.areas});
    visit_state_fields(map,[&](std::string_view name,const auto&... fields){
        const auto tuple=std::tie(fields...);check(name=="MapComponent"&& &std::get<0>(tuple)==static_cast<const StageGeometry*>(&map));
        visit_state_fields(std::get<0>(tuple),[](std::string_view base,const auto& x,const auto& z,const auto& height){check(base=="StageGeometry"&&x==11&&z==12&&height==13);});
        check(std::get<6>(tuple)==std::get<7>(tuple)&&std::get<6>(tuple)->at(0)==std::get<6>(tuple)->at(1)&&std::get<6>(tuple)->at(0)->at(0)==&parent);
    });
}
void arrays_and_parameter_aliases(){
    EntityImage image;auto shared=std::make_shared<EventIntegers>(EventIntegers{4,5});
    image.arrays={shared,shared,nullptr,std::make_shared<EventIntegers>(),std::make_shared<EventIntegers>(1,6),std::make_shared<EventIntegers>(1,7)};
    expect(image,"EntityImage",{&image.arrays});
    expect(image.arrays,"ImageArrays",{&image.arrays.textures,&image.arrays.resources,&image.arrays.u,&image.arrays.v,&image.arrays.width,&image.arrays.height});
    visit_state_fields(image.arrays,[&](std::string_view name,const auto& textures,const auto& resources,const auto& u,const auto& v,const auto& width,const auto& height){
        check(name=="ImageArrays"&&textures==resources&&textures==shared&&!u&&v&&v->empty()&&width->at(0)==6&&height->at(0)==7);
    });
    DamageParameter parameter;DamageParameterMap values{{0,&parameter},{1,&parameter}};CombatParameterLinks links;links.values=&values;
    links.storage=std::make_shared<CharacterParameterSet>();links.extras=std::make_shared<std::vector<ComponentSlots*>>(2,nullptr);
    expect(links,"CombatParameterLinks",{&links.values,&links.storage,&links.extras});
    visit_state_fields(links,[&](std::string_view name,const auto& map,const auto& storage,const auto& extras){
        check(name=="CombatParameterLinks"&&map==&values&&map->at(0)==map->at(1)&&storage==links.storage&&extras==links.extras&&extras->size()==2);
    });
}
void remaining_components(){
    ProjectileSpeed speed;expect(speed,"ProjectileSpeed",{&speed.x,&speed.y,&speed.z});
    EntitySprite sprite;expect(sprite,"EntitySprite",{&sprite.animation,&sprite.layers});
    QuestCameraComponent camera;expect(camera,"QuestCameraComponent",{&camera.angle,&camera.scale});
    EntityDepth depth;expect(depth,"EntityDepth",{&depth.value,&depth.extra});
    CellComponent cell;expect(cell,"CellComponent",{&cell.xi,&cell.yi,&cell.parent,&cell.children});
    DefeatColor color;expect(color,"DefeatColor",{&color.values});
    EquipmentRecordLinks equipment;expect(equipment,"EquipmentRecordLinks",{&equipment.ids});
    MapChipComponent chip;expect(chip,"MapChipComponent",{&chip.data_id,&chip.staff_id,&chip.visiting_human_ids});
    EntityPlayback playback;expect(playback,"EntityPlayback",{&playback.state});
    ContactDirection direction;expect(direction,"ContactDirection",{&direction.direction,&direction.locked});
    ProjectileModification modification;expect(modification,"ProjectileModification",{&modification.type,&modification.x,&modification.y,&modification.z,&modification.scale_x,&modification.scale_y,
        &modification.angle,&modification.frame,&modification.max_frame,&modification.anchor,&modification.destroy,&modification.loop,&modification.alpha});
    EntityButton button;expect(button,"EntityButton",{&button.id,&button.value,&button.dynamic,&button.auto_destroy,&button.scale,&button.text,&button.text_offset_x,&button.text_offset_y,
        &button.parent,&button.offset_x,&button.offset_y,&button.hover});
    EntityBubble bubble;expect(bubble,"EntityBubble",{&bubble.data_id,&bubble.type,&bubble.frame,&bubble.frame_max,&bubble.offset_x,&bubble.offset_y,&bubble.offset_z,&bubble.parent});
    EntityGauge gauge;expect(gauge,"EntityGauge",{&gauge.type,&gauge.from,&gauge.to,&gauge.maximum,&gauge.frame,&gauge.frame_max,&gauge.offset_x,&gauge.offset_y,&gauge.offset_z,
        &gauge.flags,&gauge.option,&gauge.option2,&gauge.parent});
    HumanAI ai;expect(ai,"HumanAI",{&ai.scripts,&ai.state});
    GarbageLifetime garbage;expect(garbage,"GarbageLifetime",{&garbage.frames});
    EnemyRecordLink enemy;expect(enemy,"EnemyRecordLink",{&enemy.enemy_id,&enemy.level,&enemy.flags,&enemy.state,&enemy.frame,&enemy.opponent});
    EffectComponent effect;expect(effect,"EffectComponent",{&effect.type,&effect.value,&effect.frame,&effect.max_frame,&effect.depth,&effect.parent});
    ProjectileLink projectile;expect(projectile,"ProjectileLink",{&projectile.parent,&projectile.start,&projectile.end,&projectile.speed,&projectile.max_height,&projectile.frame,&projectile.max_frame});
    BulletComponent bullet;expect(bullet,"BulletComponent",{&bullet.data_id});
    EmitterComponent emitter;expect(emitter,"EmitterComponent",{&emitter.category,&emitter.type,&emitter.interval,&emitter.frame,&emitter.maximum,&emitter.argument});
    ProjectileCollision collision;expect(collision,"ProjectileCollision",{&collision.radius,&collision.offset_x,&collision.offset_y,&collision.offset_z,&collision.visited});
    ProjectileDamage damage;expect(damage,"ProjectileDamage",{&damage.value,&damage.attribute,&damage.parent});
    DropComponent drop;expect(drop,"DropComponent",{&drop.type,&drop.value,&drop.x0,&drop.z0,&drop.frame});
}
int main(){payloads_and_bases();positions_humans_and_maps();arrays_and_parameter_aliases();remaining_components();}
