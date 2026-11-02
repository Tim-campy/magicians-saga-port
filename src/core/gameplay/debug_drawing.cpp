#include "debug_drawing.hpp"
#include "ecs_combat.hpp"
#include <array>
#include <charconv>
#include <initializer_list>
#include <stdexcept>
#include <string_view>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing debug drawing input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t mul(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,0);}
HumanRecordLink& human(ComponentSlots& e){return component<HumanRecordLink>(e,14);}
EnemyRecordLink& enemy(ComponentSlots& e){return component<EnemyRecordLink>(e,24);}
ProjectileCollision& collision(ComponentSlots& e){return component<ProjectileCollision>(e,32);}
std::u16string decimal(std::int32_t value){
    char buffer[16];const auto result=std::to_chars(buffer,buffer+sizeof buffer,value);
    if(result.ec!=std::errc{})throw std::runtime_error("debug integer formatting failed");
    return std::u16string(buffer,result.ptr);
}
std::u16string_view view(const DebugText& text){return text?std::u16string_view(*text):std::u16string_view{};}
DebugText text(std::u16string value){return std::make_shared<const std::u16string>(std::move(value));}
DebugText concatenate(std::initializer_list<std::u16string_view> parts){
    std::u16string result;for(const auto part:parts)result.append(part);return text(std::move(result));
}
DebugText format_attack(const DebugText& format,std::int32_t frame,std::int32_t interval,std::int32_t total){
    // Only the three plain numbered placeholders used by this caller. Formatting is public C++.
    std::u16string result=required(format.get());const std::array<std::u16string,3> values{{decimal(frame),decimal(interval),decimal(total)}};
    for(std::size_t i=0;i<values.size();++i){
        std::u16string token{u'{',static_cast<char16_t>(u'0'+i),u'}'};
        std::size_t at=0;while((at=result.find(token,at))!=std::u16string::npos){result.replace(at,token.size(),values[i]);at+=values[i].size();}
    }
    return text(std::move(result));
}
void context(DebugSystemContext value){if(!value)throw std::invalid_argument("missing debug system");}
DebugText named(DebugNames names,std::int32_t index){return required(names.get()).at(static_cast<std::size_t>(index));}
std::u16string parameter_value(ComponentSlots& e,std::int32_t id,DebugDrawingHost& host){
    auto& parameter=required(resolve_combat_parameter(e,id));const auto current=decimal(add(parameter.value,parameter.extra));
    const auto maximum=decimal(add(parameter.maximum,parameter.extra_maximum));
    const auto separator=host.debug_text(DebugTextKey::value_separator);
    std::u16string result=current;result.append(view(separator));result.append(maximum);return result;
}
void color(DebugDrawingHost& host,DebugPalette palette){const auto value=host.debug_palette(palette);host.debug_set_color(value);}
}

// ecs.DebugVisualizeSystem.DrawHumanInfo, ELF 0x11482a8.
void draw_debug_humans(DebugVisualizationState& state,DebugDrawingHost& host){
    static_cast<void>(host.debug_map()); // Native queries and discards the map and viewport before font changes.
    static_cast<void>(host.debug_game_width());static_cast<void>(host.debug_game_height());host.debug_validate_quest_form();
    host.debug_push_font(6);color(host,DebugPalette::white);
    const auto member=host.debug_member_system();context(member);
    auto& hero=required(host.debug_hero(member));
    std::int32_t header_y=110;
    for(const auto slot:{EquipmentSlot::weapon,EquipmentSlot::hat,EquipmentSlot::cloth}){
        const auto name=host.debug_equipment_name(hero,slot);
        const auto level=decimal(host.debug_equipment_level(hero,slot));
        const auto separator=host.debug_text(DebugTextKey::level_separator);
        host.debug_draw_text(concatenate({view(name),view(separator),level}),2.0F,static_cast<float>(header_y),17);
        header_y=add(header_y,7);
    }
    std::int32_t i=0,y=132;
    for(;;){
        const auto condition_ids=host.debug_parameter_ids();
        if(static_cast<std::size_t>(i)>=required(condition_ids.get()).size())break;
        const auto ids=host.debug_parameter_ids();const auto id=required(ids.get()).at(static_cast<std::size_t>(i));
        const auto total=host.debug_total_parameter(member,hero,id); // Always evaluated, even for HP/MP.
        const auto value=static_cast<std::uint32_t>(id)<=1U?parameter_value(hero,id,host):decimal(total);
        const auto name=named(host.debug_parameter_names(),id);
        const auto separator=host.debug_text(DebugTextKey::parameter_separator);
        host.debug_draw_text(concatenate({view(name),view(separator),value}),2.0F,static_cast<float>(y),17);
        i=add(i,1);y=add(y,7);
    }
    const auto system=host.debug_human_system(); // Null only faults when a live actor reaches its first query.
    auto cursor=required(state.subset).cursor();
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);const auto point=host.debug_project(dynamic_cast<ContactPosition*>(e.get(0)));
        const auto name=host.debug_character_name(e);
        host.debug_draw_text(name,static_cast<float>(point.x),static_cast<float>(add(point.y,11)),2);
        const auto names=host.debug_human_state_names();const auto state_id=human(e).state;
        host.debug_draw_text(named(names,state_id),static_cast<float>(point.x),static_cast<float>(add(point.y,18)),2);
        context(system);const auto interval=host.debug_attack_interval(system,e);
        const auto wait=host.debug_burst_interval(system,e);const auto count=host.debug_burst_count(system,e);
        const auto frame=human(e).attack_frame;
        const auto pattern=host.debug_text(DebugTextKey::attack_format);
        host.debug_draw_text(format_attack(pattern,frame,interval,add(interval,mul(count,wait))),
            static_cast<float>(point.x),static_cast<float>(add(point.y,25)),2);
        const auto x=position(e).x,y_position=position(e).y,z=position(e).z;
        const auto distance=host.debug_distance(system,e);
        const auto end=host.debug_project(x,y_position,z-static_cast<float>(mul(distance,28)));
        host.debug_draw_line(static_cast<float>(point.x),static_cast<float>(point.y),static_cast<float>(end.x),static_cast<float>(end.y),1.0F);
    }
    host.debug_pop_font(); // Font restoration is normal-return only; cursor disposal still unwinds.
}

// ecs.DebugVisualizeSystem.DrawQuestInfo, ELF 0x1148cac.
void draw_debug_quest(DebugDrawingHost& host){
    auto* quest=host.debug_quest();color(host,DebugPalette::collision);host.debug_render_mode(1,255,128);
    required(quest);
    for(std::int32_t progress=0;progress<101;progress=add(progress,10)){
        const auto height=stage_quest_height(*quest);
        const auto step_z=required(host.debug_map()).step_z;
        const auto step_x=required(host.debug_map()).step_x;
        const auto half_z=required(host.debug_map()).step_z/2;
        const auto span=mul(step_z,sub(height,3));
        const auto z=add(add(span,mul(progress,span)/-100),half_z);
        const auto left=host.debug_project(static_cast<float>(sub(step_x,110)),0.0F,static_cast<float>(z));
        const auto right=host.debug_project(static_cast<float>(add(step_x,100)),0.0F,static_cast<float>(z));
        host.debug_draw_line(static_cast<float>(left.x),static_cast<float>(left.y),static_cast<float>(right.x),static_cast<float>(right.y),3.0F);
        const auto number=decimal(progress);const auto suffix=host.debug_text(DebugTextKey::percent_suffix);
        host.debug_draw_text(concatenate({number,view(suffix)}),static_cast<float>(sub(left.x,5)),static_cast<float>(sub(left.y,5)),36);
    }
    auto cursor=required(host.debug_subset({14})).cursor();
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);if(!human(e).opponent)continue;
        const auto from=host.debug_project(dynamic_cast<ContactPosition*>(e.get(0)));
        auto& opponent=required(human(e).opponent);
        const auto to=host.debug_project(dynamic_cast<ContactPosition*>(opponent.get(0)));
        host.debug_draw_line(static_cast<float>(from.x),static_cast<float>(from.y),static_cast<float>(to.x),static_cast<float>(to.y),1.0F);
    }
    host.debug_reset_render_mode();
}

// ecs.DebugVisualizeSystem.DrawCollisionInfo, ELF 0x1149224.
void draw_debug_collisions(DebugDrawingHost& host){
    color(host,DebugPalette::collision);auto cursor=required(host.debug_subset({32})).cursor();
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);
        const auto x=position(e).x;const auto ox=collision(e).offset_x;
        const auto y=position(e).y;const auto oy=collision(e).offset_y;
        const auto z=position(e).z;const auto oz=collision(e).offset_z;
        const auto point=host.debug_project(x+static_cast<float>(ox),y+static_cast<float>(oy),z+static_cast<float>(oz));
        const auto radius=collision(e).radius;
        host.debug_fill_rect(static_cast<float>(point.x),static_cast<float>(point.y),1.0F,1.0F);
        const auto diameter=mul(radius,2),half=diameter/2;
        host.debug_draw_arc(static_cast<float>(sub(point.x,half)),static_cast<float>(sub(point.y,half)),
            static_cast<float>(diameter),static_cast<float>(diameter),0.0F,360.0F,1.0F);
    }
}

// ecs.DebugVisualizeSystem.DrawEnemyInfo, ELF 0x11495f4.
void draw_debug_enemies(DebugDrawingHost& host){
    color(host,DebugPalette::white);host.debug_push_font(6);
    auto cursor=required(host.debug_subset({24})).cursor();
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);const auto point=host.debug_project(dynamic_cast<ContactPosition*>(e.get(0)));
        auto* hp=resolve_combat_parameter(e,0);
        const auto names=host.debug_enemy_state_names();const auto state=enemy(e).state;
        host.debug_draw_text(named(names,state),static_cast<float>(point.x),static_cast<float>(add(point.y,15)),2);
        auto& captured_hp=required(hp);
        const auto current=decimal(add(captured_hp.value,captured_hp.extra));const auto maximum=decimal(add(captured_hp.maximum,captured_hp.extra_maximum));
        const auto separator=host.debug_text(DebugTextKey::value_separator);const auto hp_prefix=host.debug_text(DebugTextKey::hp_prefix);
        host.debug_draw_text(concatenate({view(hp_prefix),current,view(separator),maximum}),
            static_cast<float>(point.x),static_cast<float>(add(point.y,22)),2);
        const auto* record=host.debug_enemy_record(e);const auto level=enemy(e).level;
        const auto experience=decimal(enemy_experience(required(record),level));
        const auto prefix=host.debug_text(DebugTextKey::experience_prefix);
        host.debug_draw_text(concatenate({view(prefix),experience}),static_cast<float>(point.x),static_cast<float>(add(point.y,29)),2);
    }
    host.debug_pop_font();
}

// ecs.DebugVisualizeSystem.DrawDynamic, ELF 0x1148094.
void draw_debug_dynamic(DebugVisualizationState& state,DebugDrawingHost& host){
    if(!host.debug_controls()||state.flags==0)return;
    for(std::int32_t mask=1;mask<=256;mask*=2){
        const auto active=check_debug_flag(state,mask); // Fresh flags after every preceding handler.
        if(!active)continue;
        switch(mask){
        case 8:draw_debug_humans(state,host);break;
        case 64:draw_debug_quest(host);break;
        case 128:draw_debug_collisions(host);break;
        case 256:draw_debug_enemies(host);break;
        default:break; // Native discards these BitUtil.Check results.
        }
    }
}
void draw_debug_visualize_system(DebugDrawingHost&) noexcept{}
void draw_debug_node_cost(DebugDrawingHost&) noexcept{}
void draw_debug_node_connect(DebugDrawingHost&) noexcept{}
void draw_debug_map_chip_info(DebugDrawingHost&) noexcept{}
void draw_debug_town_area_info(DebugDrawingHost&) noexcept{}
void draw_debug_node_info(DebugDrawingHost&,std::int32_t) noexcept{}
}
