#include "motion_pass.hpp"
#include "movement.hpp"
#include "entity_visual.hpp"
#include <cmath>
#include <stdexcept>

namespace saga::core {
void move_subset(EntitySubset& group){
    auto cursor=group.cursor();
    while(const auto entity=cursor.next())move_entity(**entity);
}
void rotate_subset(EntitySubset& group){
    const auto required=[](auto* p)->auto&{if(!p)throw std::invalid_argument("missing rotation component");return *p;};
    auto cursor=group.cursor();
    while(const auto entity=cursor.next()){
        auto& e=**entity;
        const auto* raw_speed=e.get(1);
        const auto* speed=dynamic_cast<const ProjectileSpeed*>(raw_speed);
        if(raw_speed&&!speed)throw std::invalid_argument("invalid rotation speed type");
        auto& direction=required(dynamic_cast<ContactDirection*>(e.get(12)));
        if(direction.locked)continue; // Missing Speed is not dereferenced in the locked branch.
        const auto& v=required(speed);
        if(v.x!=0.0F||v.z!=0.0F){
            if(v.z<0.0F&&std::fabs(v.z)>std::fabs(v.x))direction.direction=0;
            else if(v.x>0.0F&&std::fabs(v.x)>std::fabs(v.z))direction.direction=1;
            else if(v.z>0.0F&&std::fabs(v.z)>std::fabs(v.x))direction.direction=2;
            else if(v.x<0.0F&&std::fabs(v.x)>std::fabs(v.z))direction.direction=3;
        }
        auto* destination=dynamic_cast<EntitySprite*>(e.get(2));
        const auto clip=required(dynamic_cast<EntitySprite*>(e.get(2))).animation.clip;
        const auto bits=static_cast<std::uint32_t>((clip/4)*4)+static_cast<std::uint32_t>(direction.direction);
        required(destination).animation.clip=static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
        static_cast<void>(e.contains(14)); // Native unused hasHuman query occurs after the store.
    }
}
void ProjectilePass::update(EntitySubset& group,ProjectileMotionHost& host){
    {
        auto cursor=group.cursor();
        while(const auto entity=cursor.next()){
            if(advance_projectile(**entity,host)){completed_.push_back(*entity);++version_;}
        }
    }
    const auto expected_version=version_;
    for(std::size_t i=0;;++i){
        if(version_!=expected_version)throw std::runtime_error("projectile completion list changed during enumeration");
        if(i==completed_.size())break;
        auto* entity=completed_[i];
        finish_projectile(*entity,host);
    }
    completed_.clear();++version_;
}
}
