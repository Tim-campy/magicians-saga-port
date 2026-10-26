#include "core/gameplay/impact_effect.hpp"
#include "core/gameplay/garbage_lifetime.hpp"
#include <cstdlib>
#include <deque>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:ImpactEffectHost,ComponentObserver,ProjectileHitHost,GarbagePool,GarbageUpdateHost {
    BulletImpactData data{0,2,17,0};std::int32_t bonus=0;
    std::deque<ComponentSlots> effects,areas;
    std::deque<ContactPosition> positions;
    std::deque<ProjectileCollision> collisions;
    std::deque<ProjectileDamage> damages;
    std::deque<ProjectileModification> modifications;
    std::vector<ProjectilePoint> points;std::vector<int> images;
    std::vector<std::string> calls;std::vector<std::pair<ComponentSlots*,int>> garbage;
    std::deque<GarbageLifetime> garbage_components;std::deque<QuestEntityLifetime> lifetimes;
    struct Cursor final:GarbageCursor {
        Host& host;std::size_t index=0;explicit Cursor(Host& h):host(h){}
        std::optional<GarbageEntity> next()override{
            while(index<host.garbage.size()){
                const auto id=index++;auto* e=host.garbage[id].first;
                if(e->contains(garbage_slot))return GarbageEntity{static_cast<QuestMemberId>(id),e};
            }return std::nullopt;
        }
    };
    GarbageLifetime& acquire_garbage()override{garbage_components.emplace_back();return garbage_components.back();}
    std::unique_ptr<GarbageCursor> open_garbage_cursor()override{return std::make_unique<Cursor>(*this);}
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{return lifetimes.at(static_cast<std::size_t>(id));}
    void delete_from_manager(QuestMemberId id,bool remove)override{check(remove);garbage.at(static_cast<std::size_t>(id)).first->remove_all();}
    std::function<void()> on_effect,on_collision,on_bonus;bool null_effect=false,null_data=false;
    Component* returned=nullptr;
    const CombatRecordTables* tables=nullptr;
    bool damage(ComponentSlots*,ComponentSlots&,std::int32_t value,std::int32_t attribute)override{check(value==123&&attribute==4);calls.push_back("hit");return false;}
    void defeat(ComponentSlots&)override{throw std::logic_error("unexpected defeat");}
    void impact(ComponentSlots& e,ComponentSlots* parent)override{create_impact_effect(&e,parent,*tables,*this);}
    const BulletImpactData* bullet_data(ComponentSlots&)override{calls.push_back("data");return null_data?nullptr:&data;}
    std::int32_t attribute_value(ComponentSlots&,std::int32_t a)override{check(a==2);calls.push_back("bonus");if(on_bonus)on_bonus();return bonus;}
    ComponentSlots* create_effect(std::int32_t a,std::int32_t b,std::int32_t c,ProjectilePoint p)override{
        check(a==101&&c==0);calls.push_back("effect");points.push_back(p);images.push_back(b);
        effects.emplace_back(35);if(on_effect)on_effect();return null_effect?nullptr:&effects.back();}
    ComponentSlots* create_entity(std::int32_t kind)override{check(kind==0);calls.push_back("entity");areas.emplace_back(35,this);return &areas.back();}
    ContactPosition& acquire_position()override{calls.push_back("position");positions.emplace_back();return positions.back();}
    ProjectileCollision& acquire_collision()override{calls.push_back("collision");collisions.emplace_back();return collisions.back();}
    ProjectileDamage& acquire_damage()override{calls.push_back("damage");damages.emplace_back();return damages.back();}
    ProjectileModification& acquire_modification()override{calls.push_back("modifier");modifications.emplace_back();return modifications.back();}
    void add_garbage(ComponentSlots& e,std::int32_t n)override{calls.push_back("garbage"+std::to_string(n));add_garbage_lifetime(e,n,*this);garbage.emplace_back(&e,n);lifetimes.emplace_back();}
    void add_color(ComponentSlots&,std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d)override{
        check(a==255&&b==255&&c==255&&d==3192);calls.push_back("color");}
    void return_component(std::int32_t slot,Component* p)override{check(slot==15||slot==27||slot==32);returned=p;calls.push_back("pool");}
    void added(ComponentSlots&,std::int32_t slot,Component*)override{if(slot==32&&on_collision)on_collision();}
    void removed(ComponentSlots&,std::int32_t slot,Component*)override{if(slot==15)calls.push_back("remove");}
    ProjectileLink& acquire_projectile()override{throw std::logic_error("unexpected projectile");}
    void add_animation(ComponentSlots&,std::int32_t,std::int32_t)override{throw std::logic_error("unexpected animation");}
};
struct Fixture {
    Host host;ComponentSlots e{35,&host},owner{35};Component bullet;
    ContactPosition position;ProjectileDamage damage;EquipmentRecordLinks equipment_link;
    CharacterProgressionTable characters;EnemyTable enemies;EquipmentCombatTable equipment{EquipmentCombatRecord{}};
    CombatRecordTables tables{characters,enemies,equipment,equipment,equipment};
    Fixture(){position.x=48;position.y=7;position.z=-24;damage.value=123;damage.attribute=4;
        e.add(0,&position).add(29,&bullet).add(33,&damage);
        equipment_link.ids=std::make_shared<EventIntegers>(EventIntegers{0,0,0,0});owner.add(8,&equipment_link);host.tables=&tables;}
    void run(){create_impact_effect(&e,&owner,tables,host);}
};
int main(){
    const int counts[]{1,1,5,13,25};
    for(int r=0;r<=4;++r){const auto list=impact_cells(9,-2,r);check(list.size==static_cast<std::size_t>(counts[r]));
        for(std::size_t i=0;i<list.size;++i){const auto& c=list.cells[i];check(c.x==9+c.offset_x&&c.z==-2+c.offset_z);
            check(std::abs(c.offset_x)+std::abs(c.offset_z)<=std::max(0,r-1));
            for(std::size_t j=0;j<i;++j)check(c.x!=list.cells[j].x||c.z!=list.cells[j].z);}}
    const std::array<std::array<int,2>,13> order{{{{0,0}},{{0,-1}},{{0,-2}},{{1,-1}},{{-1,-1}},{{1,0}},{{2,0}},{{1,1}},{{0,1}},{{0,2}},{{-1,1}},{{-1,0}},{{-2,0}}}};
    const auto list=impact_cells(0,0,3);for(std::size_t i=0;i<order.size();++i)check(list.cells[i].x==order[i][0]&&list.cells[i].z==order[i][1]);
    check(impact_cells(0,0,std::numeric_limits<int>::min()).size==1);
    const auto wrap=impact_cells(std::numeric_limits<int>::max(),std::numeric_limits<int>::min(),2);
    check(wrap.cells[1].z==std::numeric_limits<int>::max()&&wrap.cells[1].offset_z==-1);
    check(wrap.cells[2].x==std::numeric_limits<int>::min()&&wrap.cells[2].offset_x==1);
    {Fixture f;f.run();check(f.host.calls==std::vector<std::string>{"data","bonus","effect","garbage1"});check(f.host.areas.empty());}
    {Fixture f;f.equipment[0]->range=1;f.host.bonus=25;f.host.data.flags=1;f.run();
        check(f.host.effects.size()==5&&f.host.areas.size()==4&&f.host.garbage.size()==5);
        check(f.host.points[1].x==48&&f.host.points[1].y==7&&f.host.points[1].z==-60);
        for(std::size_t i=0;i<4;++i){const auto& p=f.host.positions[i];const auto& c=f.host.collisions[i];const auto& d=f.host.damages[i];
            check(p.y==12&&p.offset_x==0&&p.parent==nullptr&&c.radius==18&&c.offset_z==0);
            check(d.value==123&&d.attribute==4&&d.parent==&f.owner&&f.host.garbage[i].second==2);
            const auto& m=f.host.modifications[i];check(m.type==10&&m.scale_x==0.7F&&m.scale_y==0.7F&&m.anchor==34&&m.loop&&!m.destroy&&m.alpha==255);}
        check(f.host.calls[2]=="effect"&&f.host.calls[3]=="color");}
    {Fixture f;f.equipment[0]->range=100;f.run();check(f.host.effects.size()==25);}
    {Fixture f;f.equipment[0]->range=2;f.host.bonus=-24;f.run();check(f.host.effects.size()==5);}
    {Fixture f;f.equipment[0]->range=2;f.host.bonus=-25;f.run();check(f.host.effects.size()==1);}
    {Fixture f;f.equipment[0]->range=std::numeric_limits<int>::max();f.host.bonus=25;f.run();check(f.host.effects.size()==1);}
    {Fixture f;f.host.null_effect=true;f.run();check(f.host.garbage.size()==1);} // Unused null central effect allowed.
    {Fixture f;f.host.null_effect=true;f.host.data.flags=1;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}check(failed&&f.host.garbage.empty());}
    {Fixture f;f.equipment[0]->range=2;f.host.null_effect=true;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}check(failed&&f.host.effects.size()==2&&f.host.areas.empty());}
    {Fixture f;f.equipment[0]->range=2;f.host.on_bonus=[&]{f.equipment[0]->range=4;};
        f.host.on_effect=[&]{f.position.x+=10;f.host.data.effect_image=19;};
        f.host.on_collision=[&]{f.damage.value=456;f.damage.attribute=7;};f.run();
        check(f.host.effects.size()==5&&f.host.points[1].x==58&&f.host.images[0]==17&&f.host.images[1]==19);
        check(f.host.positions[0].x==58&&f.host.damages[0].value==456&&f.host.damages[0].attribute==7);}
    {Fixture f;ProjectileModification old;f.e.add(15,&old);f.host.on_effect=[&]{f.host.data.category=1;};f.run();
        check(f.host.returned==&old&&f.host.garbage.empty());const auto& m=f.host.modifications.back();
        check(m.type==5&&m.angle==180&&m.max_frame==20&&m.destroy&&!m.loop&&m.scale_x==1&&m.anchor==0);
        check(f.host.calls[f.host.calls.size()-3]=="remove"&&f.host.calls[f.host.calls.size()-2]=="pool");}
    {Fixture f;f.host.null_data=true;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}check(failed&&f.host.calls==std::vector<std::string>{"data"});}
    {Fixture f;f.equipment[0].reset();bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}check(failed&&f.host.calls==std::vector<std::string>{"data"});}
    {Fixture f;f.e.remove(33);f.equipment[0]->range=2;bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.host.areas.size()==1&&f.host.areas[0].contains(32)&&!f.host.areas[0].contains(33)&&f.host.garbage.empty());}
    {Fixture f;ComponentSlots enemy{35};EnemyRecordLink link;enemy.add(24,&link);
        ProjectileLink projectile;projectile.parent=&f.owner;ProjectileCollision collision;ProjectileSpeed speed;speed.x=5;
        f.e.add(27,&projectile).add(32,&collision).add(1,&speed);f.equipment[0]->range=2;
        on_projectile_hit(&f.e,&enemy,f.host);
        check(f.host.calls.front()=="hit"&&f.host.areas.size()==4&&f.host.garbage.size()==5);
        check(!f.e.contains(27)&&!f.e.contains(32)&&speed.x==0&&speed.y==0&&speed.z==0);
        check(f.host.damages[0].parent==&f.owner&&f.host.damages[0].value==123);}
    {Fixture f;f.equipment[0]->range=2;f.run();GarbagePass pass;
        check(f.e.contains(22)&&f.host.areas[0].contains(32));
        pass.update(f.host);check(f.host.lifetimes[4].destroyed&&!f.e.contains(22));
        for(std::size_t i=0;i<4;++i)check(!f.host.lifetimes[i].destroyed&&f.host.areas[i].contains(32)&&f.host.garbage_components[i].frames==1);
        pass.update(f.host);for(std::size_t i=0;i<4;++i)check(f.host.lifetimes[i].destroyed&&!f.host.areas[i].contains(32)&&!f.host.areas[i].contains(33));
        pass.update(f.host); // Destroyed entities no longer belong to the synthetic slot-22 group.
    }
}
