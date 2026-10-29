#pragma once
#include "enemy_frame.hpp"

namespace saga::core {
struct HumanFrameSystem:StageHumanStart {
    EntitySubset* subset=nullptr;StageGeometry* map=nullptr; // HumanSystem +0x20/+0x28.
    std::int32_t loop_counter=0; // +0x38; increment only after a successful whole pass.
    StageStartState* stage=nullptr;EnemyCreationSystem* enemies=nullptr;MemberRoster* members=nullptr;
};
class HumanFrameHost:public HumanStateHost {
public:
    virtual const CombatRecordTables& human_records()=0;
    virtual std::int32_t human_parameter(ComponentSlots&,std::int32_t key)=0;
    virtual PlayerWorldState* human_world()=0; // Current global world, with the usual hero cache semantics.
    virtual StagePlacementHost& human_coordinates()=0;
    virtual float human_formation_x(ComponentSlots&)=0; // Bind existing MemberSystem formation kernels.
    virtual float human_formation_z(ComponentSlots&)=0;
    virtual bool human_target_destroyed(ComponentSlots&)=0;
    virtual std::int32_t human_frames_per_second()=0;
    virtual RandomSource& human_random()=0;
    virtual std::int32_t celebration_frame_period()=0; // Current character-library SEB clip 0 maxFrame.
    virtual void remove_human_modification(ComponentSlots&)=0; // Existing detach/pool-return kernel.
    // Bind the concrete shoot_human producer in human_shoot.hpp with its required
    // live catalogs/worlds/audio. No fallback success or projectile is supplied.
    virtual bool shoot_human(ComponentSlots& actor,ComponentSlots& target)=0;
};
bool stage_attack_active(const StageStartState&); // StageSystem.get_isAttackActive 0x115651c.
void accelerate_humans(HumanFrameSystem&); // HumanSystem.AccelerateSpeed 0x1158eec.
void reset_human_speeds(HumanFrameSystem&); // HumanSystem.ResetSpeed 0x11590bc; dash only.
ComponentSlots* nearest_attack_enemy(EnemyCreationSystem&,ContactPosition*,std::int32_t distance); // SearchNearestEnemy 0x1151384.
std::int32_t human_attack_attribute(HumanFrameSystem&,const CharacterProgressionRecord*,HumanFrameHost&); // GetAttackAttribute 0x1156e48.
std::int32_t human_mp_cost(HumanFrameSystem&,ComponentSlots*,std::int32_t attribute,HumanFrameHost&); // GetUseMp 0x1158d24.
bool human_has_mp(HumanFrameSystem&,ComponentSlots*,std::int32_t attribute,HumanFrameHost&); // CheckMp 0x1156ef8.
void use_human_mp(ComponentSlots*,HumanFrameHost&); // UseMp 0x11571c4; base weapon cost only.
std::int32_t human_attack_distance(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // GetDistance 0x1156f98.
std::int32_t human_burst_count(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // GetBurstCount 0x1156d64.
std::int32_t human_attack_speed(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // GetAttackSpeed 0x1158d94.
std::int32_t human_attack_interval(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // GetAttackInterval 0x1155f08.
std::int32_t human_burst_wait(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // GetBurstInterval 0x1156584.
float human_move_speed(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // GetMoveSpeed 0x11563d4.
bool search_human_attack(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // SearchAndAttack 0x1156074.
bool move_human_base(HumanFrameSystem&,ComponentSlots*,float x,float z,float speed,HumanFrameHost&); // MoveBase 0x1158ab8.
bool move_human(HumanFrameSystem&,ComponentSlots*,float x,float z,float speed,HumanFrameHost&); // Move 0x1158914.
void update_human_waiting(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // UpdateWaiting 0x1155438.
void update_human_walking(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // UpdateWalking 0x1155528.
void update_human_attacking(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // UpdateAttacking 0x1155b90.
void update_human_bounding(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // UpdateBounding 0x1155d18.
void update_human_celebration(ComponentSlots*,HumanFrameHost&); // UpdateBanzai 0x1155de8.
void limit_human_position(HumanFrameSystem&,ComponentSlots*,HumanFrameHost&); // LimitHumanPosition 0x1155fbc.
// Parameter.Add 0x1178d54: base-maximum clamp, wrapping add; extras do not affect the clamp.
std::int32_t add_human_parameter(DamageParameter&,std::int32_t amount);
// Full HumanSystem.Update 0x1155114; states 4/5 and unknown state values are inert.
// Calls the supplied concrete Shoot binding only when the recovered burst gate fires.
void update_humans(HumanFrameSystem&,HumanFrameHost&);
}
