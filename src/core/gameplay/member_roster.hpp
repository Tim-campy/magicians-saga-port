#pragma once
#include "combat_state.hpp"
#include <functional>
#include <memory>

namespace saga::core {
// Ordered borrowed entity identities. Mutations invalidate an initialization pass;
// replacing MemberRoster.members keeps an already captured list alive.
class PartyMembers {
public:
    const std::vector<ComponentSlots*>& entities() const{return entries_;}
    std::uint32_t version() const{return version_;}
    bool contains(ComponentSlots*) const;
    void append(ComponentSlots*); // Direct List.Add can retain duplicates/null.
    bool remove(ComponentSlots*); // First occurrence only; misses do not invalidate.
    void clear();
private:
    std::vector<ComponentSlots*> entries_;
    std::uint32_t version_=0;
};
struct MemberRoster {
    // Managed-array projection: keep lengths fixed and within signed int32 range;
    // callbacks can replace references. Defaults are the constructor's list/array state.
    std::shared_ptr<std::vector<std::int32_t>> formation=std::make_shared<std::vector<std::int32_t>>(16,-1);
    std::shared_ptr<PartyMembers> members=std::make_shared<PartyMembers>();
    std::shared_ptr<PartyMembers> sub_members=std::make_shared<PartyMembers>();
};
// MemberSystem.AddMember / RemoveMember 0x115d0a4 / 0x115d90c.
void add_member(MemberRoster&,ComponentSlots*);
void remove_member(MemberRoster&,ComponentSlots*);
// SetCharaDataId 0x115c39c. Only clearing an occupied slot queries the old
// character's main flag. The callback may replace the formation reference.
bool set_formation_character(MemberRoster&,std::int32_t index,std::int32_t character,
                             const std::function<bool(std::int32_t)>& is_main);
bool set_formation_empty_slot(MemberRoster&,std::int32_t character,
                              const std::function<bool(std::int32_t)>& is_main);
void swap_formation_slots(MemberRoster&,std::int32_t first,std::int32_t second);
std::int32_t formation_number(const MemberRoster&,std::int32_t character);
// Parameter.Full 0x1178dc8: zero base maximum leaves the old value intact.
void fill_parameter(DamageParameter&);
class MemberInitializationHost {
public:
    virtual ~MemberInitializationHost()=default;
    // GetFormationInitLeftTopX/Z: world map/start depth binding remains external.
    virtual std::int32_t formation_left()=0;
    virtual std::int32_t formation_top()=0;
    virtual std::int32_t total_parameter(ComponentSlots&,std::int32_t index)=0;
    virtual void change_animation(ComponentSlots&,std::int32_t clip,bool reset)=0;
    // Resolve the current world's HumanSystem after the first animation change.
    virtual HumanStateHost* human_system()=0;
    // CharaData.sortieCount_ (+0x48), resolved from the captured HumanComponent.
    // Return null for a missing record; the earlier writes remain committed.
    virtual std::int32_t* sortie_count(HumanRecordLink&)=0;
};
// MemberSystem.InitMembers 0x115d200: ordered positioning, full parameters,
// waiting animation/state and combat-field reset. No new entities or save I/O.
void initialize_members(MemberRoster&,MemberInitializationHost&);
} // namespace saga::core
