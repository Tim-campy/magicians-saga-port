#include "motion_pass.hpp"
#include "movement.hpp"
#include <stdexcept>

namespace saga::core {
void move_subset(EntitySubset& group){
    auto cursor=group.cursor();
    while(const auto entity=cursor.next())move_entity(**entity);
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
