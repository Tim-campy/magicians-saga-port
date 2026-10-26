#pragma once
#include "contact_collision.hpp"

namespace saga::core {
class HumanStateHost {
public:
    virtual ~HumanStateHost()=default;
    virtual bool playing_cut_in(ComponentSlots&)=0; // Requires Animation slot 10.
    virtual void change_animation(ComponentSlots&,std::int32_t clip,bool reset_frame)=0;
    virtual std::int32_t attack_interval(ComponentSlots&)=0; // Live FPS/stat binding remains external.
};
// EnemySystem.ChangeState 0x114d934: no transition guard, including same-state calls.
void change_enemy_state(ComponentSlots&,std::int32_t state);
// HumanSystem.PreChangeState / ChangedState / ChangeState, 0x1157080 / 0x11570d4 / 0x11562e4.
void before_human_state(ComponentSlots&,std::int32_t old_state,std::int32_t new_state,HumanStateHost&);
void after_human_state(ComponentSlots&,std::int32_t old_state,std::int32_t new_state,HumanStateHost&);
void change_human_state(ComponentSlots&,std::int32_t state,HumanStateHost&);
}
