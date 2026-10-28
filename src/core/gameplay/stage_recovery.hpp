#pragma once
#include "stage_start.hpp"

namespace saga::core {
struct StageObjectWorld {EntityIdState& ids;EntityVisualHost& factory;};
// Current-world and stored-system boundaries are separate: callbacks can replace
// the global world between lookups, and pooled payloads must outlive all entities.
class StageRecoveryHost:public AnimationModificationPool {
public:
    virtual StageStartHost& recovery_start_host()=0;
    virtual PlayerWorldState* recovery_world()=0;
    virtual EntitySubset* recovery_effect_subset()=0; // Current-world And(26).
    virtual QuestEntityHost* recovery_destruction_world()=0; // Fresh world after snapshot.
    virtual QuestMemberId recovery_entity_id(ComponentSlots&)=0;
    virtual PartyCharacter* recovery_character(HumanRecordLink&)=0;
    virtual void recovery_change_animation(ComponentSlots&,std::int32_t clip,bool reset)=0;
    virtual DropEffectWorld* recovery_effect_world()=0;
    virtual std::int32_t invincible_time()=0; // Live HumanComponent.INVINCIBLE_TIME; never guessed.
    virtual void release_recovery_modification(Component*)=0; // After slot-15 removal; includes null.
    virtual ComponentSlots* create_stage_boss(StageEnemyStart&)=0; // EnemySystem.CreateBoss still required.
    virtual StageObjectWorld* stage_object_world()=0; // Captured before GetZ.
    virtual void play_boss_bgm()=0; // AppData.RESSOUND_BGM_BATTLE_02 +0x23c.
    virtual void play_boss_sound()=0; // AppData.RESSOUND_SE_BATTLE_BOSS +0x288.
};
bool stage_human_dead(const HumanRecordLink&);
// HumanComponent..cctor's duration assignment only; FPS is captured at static
// initialization, not recomputed on each retry. The other static fields are separate.
std::int32_t initial_human_invincible_time(std::int32_t initial_fps);
bool is_retry_removable_effect(ComponentSlots*); // <Retry>b__57_0: Seb animation ID ==100.
float member_formation_x(ComponentSlots*,StageRecoveryHost&); // MemberSystem.GetFormationX.
float member_formation_z(ComponentSlots*,StageRecoveryHost&); // MemberSystem.GetFormationZ.
void remove_recovery_modification(ComponentSlots&,StageRecoveryHost&);
// StageSystem.Retry 0x1162eb8. No reset of stage/state/frame, boss flags, rewards,
// attack timers or Human.frame; normal dialog caller clears its dialog afterwards.
void retry_stage(StageStartState&,StageRecoveryHost&);
// StageSystem.StartBossBattle 0x11641a8. Boss creation remains a required service;
// actual modifier/fence factory and partial-publication order are implemented.
void start_stage_boss_battle(StageStartState&,StageRecoveryHost&);
} // namespace saga::core
