#include "quest_catalog.hpp"
#include "table_catalog.hpp"

namespace saga::core {
namespace {
StageScriptGroups groups(detail::TableTokens& t) {
    const auto count=t.integer();
    if(count<0)return nullptr;
    auto result=std::make_shared<std::vector<EventScript>>();
    for(std::int32_t i=0;i<count;++i){
        auto group=t.script();
        if(!group)throw std::length_error("negative quest group size");
        result->push_back(std::move(group));
    }
    return result;
}
void read(QuestDefinition& r,detail::TableTokens& t) {
    r.id=t.integer();r.name=t.take();r.image=t.integer();r.resource=t.take();
    auto chips=t.integers();r.stage.chip_images=chips?std::make_shared<EventIntegers>(std::move(*chips)):nullptr;
    t.take();t.take(); // Native skip(), not integer parsing.
    auto& q=r.stage;
    q.base_level=t.integer();q.boss_level=t.integer();q.distance=t.integer();q.time_limit=t.integer();
    q.enemy_interval=t.integer();q.enemy_maximum=t.integer();q.enemy_total_limit=t.integer();q.money_rate=t.integer();
    q.normal_enemies=groups(t);q.fixed_enemies=groups(t);q.boss_id=t.integer();
    q.events=groups(t);q.fixed_treasure=groups(t);q.prisoner_maximum=t.integer();
    q.reward_treasure=t.integer();q.cleared_reward_treasure=t.integer();q.flags=t.integer();r.need_stamina=t.integer();
}
}
QuestDefinition& load_quest_definition(QuestDefinition& receiver,std::optional<std::string_view> line,const EventIntegerParser& parser){
    if(line&&!line->empty()){detail::TableTokens t(*line,parser);read(receiver,t);}
    return receiver;
}
QuestTable load_quest_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    return detail::load_table_records<QuestDefinition>(text,normalize,parser,[](detail::TableTokens& t){QuestDefinition r;read(r,t);return r;});
}
} // namespace saga::core
