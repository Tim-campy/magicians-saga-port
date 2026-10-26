#include "core/gameplay/collision_pass.hpp"
#include <array>
#include <cstdlib>
#include <functional>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host:CollisionPassHost {
    ComponentSlots a{35},b{35},c{35};std::array<ComponentSlots*,3> entities{{&a,&b,&c}};
    std::array<ContactPosition,3> positions;std::array<ProjectileCollision,3> collisions;
    std::vector<ComponentSlots*> entries{&a,&b,&c};std::vector<std::pair<ComponentSlots*,ComponentSlots*>> events;
    int opens=0,disposes=0,invalid_open=0;std::function<void()> on_send;
    struct Cursor final:CollisionCursor {
        Host& host;std::size_t index=0;int serial;
        Cursor(Host& h,int n):host(h),serial(n){}
        ~Cursor()override{++host.disposes;}
        std::optional<ComponentSlots*> next()override{
            if(index==host.entries.size())return std::nullopt;
            auto* e=host.entries[index++];if(serial==host.invalid_open&&index==3)return nullptr;
            return e;}
    };
    Host(){for(std::size_t i=0;i<3;++i){collisions[i].radius=10;collisions[i].visited=true;entities[i]->add(0,&positions[i]).add(32,&collisions[i]);}}
    std::unique_ptr<CollisionCursor> open_collision_cursor()override{return std::make_unique<Cursor>(*this,++opens);}
    void send_collision(std::int32_t id,ComponentSlots& x,ComponentSlots& y)override{check(id==29&&opens==disposes);events.emplace_back(&x,&y);if(on_send)on_send();}
};
int main(){
    {Host h;CollisionPass p;p.update(h);check(h.opens==5&&h.disposes==5);
        check(h.events==std::vector<std::pair<ComponentSlots*,ComponentSlots*>>{{&h.a,&h.b},{&h.a,&h.c},{&h.b,&h.c}});
        for(const auto& c:h.collisions)check(c.visited);p.update(h);check(h.events.size()==6);}
    // Independent truth table for exclusions; multi-tag entities must keep all exclusions.
    for(int a=0;a<16;++a)for(int b=0;b<16;++b){Host h;CollisionPass p;Component tag;h.entries.resize(2);
        const std::array<int,4> slots{{14,24,29,26}};
        for(int i=0;i<4;++i){if(a&(1<<i))h.a.add(slots[static_cast<std::size_t>(i)],&tag);if(b&(1<<i))h.b.add(slots[static_cast<std::size_t>(i)],&tag);}
        const bool excluded=((a&b&7)!=0)||((a&1)&&(b&12))||((b&1)&&(a&12));p.update(h);check(h.events.empty()==excluded);}
    for(const auto values:{std::pair<int,int>{0,0},{0,1},{2,-2},{1,0},{std::numeric_limits<int>::max(),1}}){
        Host h;CollisionPass p;h.entries.resize(2);DamageParameter hp;hp.value=values.first;hp.extra=values.second;
        DamageParameterMap map{{0,&hp}};CombatParameterLinks params;params.values=&map;h.a.add(23,&params);p.update(h);
        const bool alive=values==std::pair<int,int>{0,1}||values==std::pair<int,int>{1,0};check(!h.events.empty()==alive);}
    {Host h;CollisionPass p;h.positions[2].z=100;p.update(h);check(h.events.size()==1);}
    {Host h;CollisionPass p;h.positions[1].x=std::numeric_limits<float>::quiet_NaN();p.update(h);check(h.events.size()==1&&h.events[0].second==&h.c);}
    {Host h;CollisionPass p;h.on_send=[&]{h.a.remove(32);h.b.remove(32);h.c.remove(32);};p.update(h);check(h.events.size()==3);} // No delivery-time eligibility test.
    {Host h;CollisionPass p;h.on_send=[] {throw std::runtime_error("subscriber");};bool failed=false;
        try{p.update(h);}catch(const std::runtime_error&){failed=true;}check(failed&&h.events.size()==1);
        h.entries.clear();h.on_send=nullptr;p.update(h);check(h.events.size()==4);p.update(h);check(h.events.size()==4);} // Persistent queue retained on failure.
    {Host h;CollisionPass p;h.invalid_open=3;bool failed=false;try{p.update(h);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&h.events.empty()&&h.opens==h.disposes);h.entries.clear();p.update(h);check(h.events.size()==1);}
    {Host h;CollisionPass p;CombatParameterLinks params;h.a.add(23,&params);bool failed=false;
        try{p.update(h);}catch(const std::invalid_argument&){failed=true;}check(failed&&h.events.empty()&&h.opens==h.disposes);}
    {Host h;CollisionPass p;h.entries={&h.a,&h.a};p.update(h);check(h.events.empty());}
    {Host h;CollisionPass p;bool nested=false;h.on_send=[&]{if(!nested){nested=true;h.entries.clear();p.update(h);}};
        bool failed=false;try{p.update(h);}catch(const std::runtime_error&){failed=true;}
        check(failed&&h.events.size()==4);p.update(h);check(h.events.size()==4);} // Inner clear invalidates the outer delivery cursor.
    {struct HitHost final:Host,ProjectileHitHost {
        int damages=0,impacts=0;
        bool damage(ComponentSlots*,ComponentSlots&,std::int32_t value,std::int32_t attr)override{check(value==12&&attr==2);++damages;return false;}
        void defeat(ComponentSlots&)override{check(false);}
        void impact(ComponentSlots&,ComponentSlots*)override{++impacts;}
        void return_component(std::int32_t slot,Component*)override{check(slot==27||slot==32);}
        void send_collision(std::int32_t id,ComponentSlots& x,ComponentSlots& y)override{Host::send_collision(id,x,y);on_projectile_hit(&x,&y,*this);}
    } h;
        CollisionPass p;Component bullet;EnemyRecordLink b,c;ProjectileLink projectile;ProjectileSpeed speed;ProjectileDamage damage;
        damage.value=12;damage.attribute=2;h.a.add(29,&bullet).add(27,&projectile).add(1,&speed).add(33,&damage);h.b.add(24,&b);h.c.add(24,&c);
        p.update(h);check(h.events.size()==2&&h.damages==1&&h.impacts==1&&!h.a.contains(27)&&!h.a.contains(32));}
}
