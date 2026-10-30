#pragma once
#include "stage_placement.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace saga::core {
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
};
using QuestTable=std::vector<std::optional<QuestDefinition>>;
// QuestData.Load 0x1172b44: null/empty preserves the receiver; failures keep successful stores.
QuestDefinition& load_quest_definition(QuestDefinition&,std::optional<std::string_view>,const EventIntegerParser&);
QuestTable load_quest_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
} // namespace saga::core
