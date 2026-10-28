#pragma once
#include "human_frame.hpp"
#include "bullet_factory.hpp"

namespace saga::core {
// EquipmentData..cctor 0x116f928: initial ATTACK_SEB_IDS. Effective mutable table is supplied below.
EventInstruction initial_attack_animation_ids();
class HumanShootHost {
public:
    virtual ~HumanShootHost()=default;
    virtual BulletRecords shoot_bullet_records()=0;
    virtual BulletFactoryHost* shoot_bullet_world()=0; // Captured World.world_ before argument evaluation.
    virtual DropEffectWorld* shoot_effect_world()=0;
    virtual ProjectileLaunchHost& shoot_launch_host()=0;
    virtual EventInstruction shoot_attack_animations()=0; // Effective EquipmentData.ATTACK_SEB_IDS.
    virtual void play_attack_sound(bool elemental)=0; // Effective AppData +0x26c if true, +0x268 otherwise.
};
// HumanSystem.<Shoot>b__75_0 0x115931c; requires the row before the stored Stage.
bool human_spell_matches(HumanFrameSystem&,const BulletRecord*);
// HumanSystem.Shoot 0x1156638. Actual catalog/factory/launch/MP and visual mutations;
// live global worlds, catalogs and audio remain required host inputs.
bool shoot_human(HumanFrameSystem&,ComponentSlots* actor,ComponentSlots* target,HumanFrameHost&,HumanShootHost&);
}
