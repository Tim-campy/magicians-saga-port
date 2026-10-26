#include "core/gameplay/projectile_hit.hpp"
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool ok){if(!ok)std::abort();}
struct Host final:ProjectileHitHost,ComponentObserver {
    std::vector<std::string> calls;std::vector<Component*> returned;
    bool lethal=false;ComponentSlots* owner=nullptr;ComponentSlots* impact_owner=nullptr;
    std::function<void()> during_damage,during_impact;
    std::int32_t amount=0,attribute=0;
    bool damage(ComponentSlots* p,ComponentSlots&,std::int32_t a,std::int32_t b) override{
        calls.push_back("damage");owner=p;amount=a;attribute=b;if(during_damage)during_damage();return lethal;
    }
    void defeat(ComponentSlots&) override{calls.push_back("defeat");}
    void impact(ComponentSlots&,ComponentSlots* p) override{calls.push_back("impact");impact_owner=p;if(during_impact)during_impact();}
    void return_component(std::int32_t slot,Component* p) override{calls.push_back("pool"+std::to_string(slot));returned.push_back(p);}
    void removed(ComponentSlots&,std::int32_t slot,Component*) override{calls.push_back("remove"+std::to_string(slot));}
};
struct Fixture {
    Host host;ComponentSlots source{35,&host},target{35},owner{35},other{35};
    ProjectileLink projectile;ProjectileDamage damage;ProjectileCollision collision;ProjectileSpeed speed;
    EnemyRecordLink enemy;
    Fixture(){projectile.parent=&owner;damage.value=123;damage.attribute=2;speed.x=1;speed.y=-2;speed.z=3;
        source.add(projectile_slot,&projectile).add(projectile_damage_slot,&damage).add(projectile_collision_slot,&collision).add(projectile_speed_slot,&speed);
        target.add(combat_enemy_slot,&enemy);}
};
int main(){
    {Fixture f;f.host.lethal=true;f.host.during_damage=[&]{f.projectile.parent=&f.other;};
        on_projectile_hit(&f.source,&f.target,f.host);
        check(f.host.calls==std::vector<std::string>{"damage","defeat","impact","remove27","pool27","remove32","pool32"});
        check(f.host.owner==&f.owner&&f.host.impact_owner==&f.other&&f.host.amount==123&&f.host.attribute==2);
        check(f.host.returned==std::vector<Component*>{&f.projectile,&f.collision});
        check(!f.source.contains(projectile_slot)&&!f.source.contains(projectile_collision_slot));
        check(f.source.contains(projectile_damage_slot)&&f.speed.x==0&&f.speed.y==0&&f.speed.z==0);
    }
    {Fixture f;on_projectile_hit(&f.source,&f.target,f.host);check(f.host.calls[1]=="impact"&&f.speed.x==0);}
    {Fixture f;f.target.remove(combat_enemy_slot);on_projectile_hit(nullptr,&f.target,f.host);check(f.host.calls.empty());}
    {Fixture f;f.source.remove(projectile_slot);f.host.calls.clear();on_projectile_hit(&f.source,&f.target,f.host);check(f.host.calls.empty());}
    {Fixture f;f.source.remove(projectile_collision_slot);f.host.calls.clear();on_projectile_hit(&f.source,&f.target,f.host);check(f.host.calls.empty()&&f.speed.x==1);}
    {Fixture f;bool failed=false;try{on_projectile_hit(&f.source,nullptr,f.host);}catch(const std::invalid_argument&){failed=true;}check(failed);}
    {Fixture f;f.host.during_damage=[&]{f.source.remove(projectile_slot);};bool failed=false;
        try{on_projectile_hit(&f.source,&f.target,f.host);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.host.calls==std::vector<std::string>{"damage","remove27"}&&f.source.contains(projectile_collision_slot));
    }
    {Fixture f;f.host.during_impact=[&]{f.source.remove(projectile_collision_slot);};
        on_projectile_hit(&f.source,&f.target,f.host);check(f.host.returned==std::vector<Component*>{&f.projectile});}
    {Fixture f;f.source.remove(projectile_speed_slot);f.host.calls.clear();bool failed=false;
        try{on_projectile_hit(&f.source,&f.target,f.host);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.host.returned.size()==2&&!f.source.contains(projectile_slot)); // No rollback after late fault.
    }
}
