#include "collision_pass.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* p){if(!p)throw std::invalid_argument("missing collision-pass input");return *p;}
ProjectileCollision& collision(ComponentSlots& e){return require(dynamic_cast<ProjectileCollision*>(e.get(projectile_collision_slot)));}
std::unique_ptr<CollisionCursor> cursor(CollisionPassHost& host){auto p=host.open_collision_cursor();require(p.get());return p;}
bool alive(ComponentSlots& e){
    if(!e.contains(combat_parameter_slot))return true;
    const auto& p=require(resolve_combat_parameter(e,0));
    const auto bits=static_cast<std::uint32_t>(p.value)+static_cast<std::uint32_t>(p.extra);
    const auto value=bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL;
    return value>=1;
}
bool eligible(ComponentSlots& a,ComponentSlots& b){
    if(a.contains(combat_human_slot)&&b.contains(combat_human_slot))return false;
    if(a.contains(combat_enemy_slot)&&b.contains(combat_enemy_slot))return false;
    if(a.contains(contact_bullet_slot)&&b.contains(contact_bullet_slot))return false;
    if(a.contains(contact_bullet_slot)&&b.contains(combat_human_slot))return false;
    if(a.contains(combat_human_slot)&&b.contains(contact_bullet_slot))return false;
    if(a.contains(26)&&b.contains(combat_human_slot))return false; // Effect.
    if(a.contains(combat_human_slot)&&b.contains(26))return false;
    return alive(a)&&alive(b);
}
}
void CollisionPass::update(CollisionPassHost& host){
    {
        auto reset=cursor(host);
        while(const auto entry=reset->next())collision(require(*entry)).visited=false;
    }
    {
        auto outer=cursor(host);
        while(const auto entry=outer->next()){
            auto& a=require(*entry);collision(a).visited=true;
            auto inner=cursor(host);
            while(const auto candidate=inner->next()){
                auto& b=require(*candidate);auto& c=collision(b); // Required even for self.
                if(&a==&b||c.visited)continue;
                if(!eligible(a,b))continue;
                collision(a);collision(b); // Update requires both; CheckHit's absent-slot shortcut is not used.
                if(check_collision(&a,&b)){pending_.emplace_back(&a,&b);++version_;}
            }
        }
    }
    const auto expected_version=version_;
    for(std::size_t i=0;;++i){
        if(version_!=expected_version)throw std::runtime_error("collision event list changed during enumeration");
        if(i==pending_.size())break;
        const auto pair=pending_[i]; // Callbacks can mutate entities, but not the captured pair.
        host.send_collision(29,require(pair.first),require(pair.second));
    }
    pending_.clear();++version_;
}
}
