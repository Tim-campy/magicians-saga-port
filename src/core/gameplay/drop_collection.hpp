#pragma once
#include "drop_factory.hpp"
#include "effect_factory.hpp"
#include "../ecs/entity_subset.hpp"

namespace saga::core {
struct DropCollectionState {
    StageRewards* rewards=nullptr; // StageSystem.drop_ +0x40.
    bool boss_defeated=false,reward_box_picked_up=false; // +0x85/+0x86.
    std::shared_ptr<std::vector<ComponentSlots*>> collecting; // +0x98; live indexed list.
};
// Initialized storage, int32 list lengths and borrowed entity/world lifetimes
// through callbacks are host contracts. Audio IDs and stage-call cadence remain external.
struct DropEffectWorld {EntityIdState& ids;EffectFactoryHost& factory;};
class DropCollectionHost:public GarbagePool {
public:
    virtual ComponentSlots* front_line_human()=0;
    virtual EntitySubset* drop_subset()=0; // Fresh World.GetSubSet(And({34})) lookup per CollectDrop.
    virtual const TreasureRecord& treasure(std::int32_t id)=0;
    virtual ComponentSlots* hero()=0;
    virtual DropEffectWorld* effect_world()=0; // Capture before position reads; null checked afterwards.
    virtual void play_collection_sound()=0; // Effective AppData SE field +0x258, not a guessed ID.
    virtual void play_reward_jingle()=0; // Effective jingle field +0x27c.
};
bool drop_is_collecting(const DropComponent&) noexcept; // get_isCollecting 0x113edf8.
bool treasure_is_boss_reward(const TreasureRecord&) noexcept; // Inherited flag mask 1, 0x11738dc.
// These mutate stage totals only; permanent grants remain in ShowQuestResult.
void on_collected_drop(DropCollectionState&,ComponentSlots*,DropCollectionHost&); // 0x1165988.
void collect_drops(DropCollectionState&,DropCollectionHost&); // 0x1163738.
void update_collecting_drops(DropCollectionState&,DropCollectionHost&); // 0x1163c18.
} // namespace saga::core
