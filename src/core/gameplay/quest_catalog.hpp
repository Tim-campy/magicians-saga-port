#pragma once
#include "stage_placement.hpp"
#include <optional>
#include <functional>
#include <string>
#include <string_view>

namespace saga::core {
struct QuestRecord;
// AreaData.Load only supplies static fields; discovery/arrival/damage are saved separately.
struct AreaDefinition {
    std::int32_t id=-1,image=0,flags=0;
    std::string name;
    EventInstruction stages;
};
using AreaTable=std::vector<std::optional<AreaDefinition>>;
AreaDefinition& load_area_definition(AreaDefinition&,std::optional<std::string_view>,const EventIntegerParser&);
AreaTable load_area_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
// Static QuestData projection. Saved completion/prisoner fields in stage are not read by Load.
struct QuestDefinition {
    std::int32_t id=-1,image=0,need_stamina=0;
    std::string name,resource;
    StageQuest stage;
    std::int32_t discovery_state=0;
    bool is_new=false;
    std::shared_ptr<QuestRecord> record;
};
using QuestTable=std::vector<std::optional<QuestDefinition>>;
// QuestData.Load 0x1172b44: null/empty preserves the receiver; failures keep successful stores.
QuestDefinition& load_quest_definition(QuestDefinition&,std::optional<std::string_view>,const EventIntegerParser&);
QuestTable load_quest_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
// QuestData.Find 0x1172f0c ignores its bool argument and preserves nonzero discovery.
void find_quest_record(QuestDefinition&,bool);
// QuestData.NewGame 0x1172e88: conditional virtual Find, clear completion, new record.
// Callbacks expose virtual dispatch/allocation failures without emulating the managed runtime.
void reset_quest_for_new_game(QuestDefinition&,const std::function<void(bool)>& find={},
                             const std::function<std::shared_ptr<QuestRecord>()>& allocate={});
struct MissionSelectionRecords {
    AreaDefinition* area=nullptr;
    QuestDefinition* quest=nullptr;
};
// QuestForm.Init selection projection (0x11d9d18..0x11da1b8): callers supply
// decoded normal-save slots8/9. BootForm.Load caches alias these full catalogs.
// StartExplore's area bound precedes quest indexing; gaps retain null identity.
MissionSelectionRecords select_mission_records(AreaTable&,QuestTable&,std::int32_t area_slot8,std::int32_t quest_slot9);
} // namespace saga::core
