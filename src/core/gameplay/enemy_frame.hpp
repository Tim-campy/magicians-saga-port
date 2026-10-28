#pragma once
#include "enemy_population.hpp"

namespace saga::core {
class EnemyFrameHost:public EnemyPopulationHost,public GarbagePool {
public:
    // Records and replaced pooled components remain alive through this pass.
    virtual const ParameterMaximumLevels& frame_enemy_maximum_levels()=0;
    virtual std::int32_t enemy_frames_per_second()=0; // Live cfg.MyConfig.FPS.
    virtual void release_enemy_garbage(Component*)=0; // After removing slot 22; includes null capture.
};
bool enemy_is_normal(const EnemyRecordLink&);
bool enemy_waits_for_players(const EnemyRecordLink&);
bool enemy_is_alive(const EnemyRecordLink&);
bool enemy_is_dead(const EnemyRecordLink&);
// KairoMath.CheckHitPointCircle: binary32 X/Z squared distance, inclusive radius.
bool within_enemy_activation_circle(ProjectilePoint first,ProjectilePoint second,float radius);
void update_enemy_walking(EnemyCreationSystem&,ComponentSlots*,EnemyFrameHost&);
void update_enemy_bounding(EnemyCreationSystem&,ComponentSlots*,EnemyFrameHost&);
void update_enemy_run_away(ComponentSlots*);
// Full EnemySystem.Update 0x114c694: actual population prefix, then a live pass.
// Movement integration and Garbage expiry still run in their separate systems.
void update_enemies(EnemyCreationSystem&,EnemyFrameHost&);
} // namespace saga::core
