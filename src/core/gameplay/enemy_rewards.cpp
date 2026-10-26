#include "enemy_rewards.hpp"
#include "combat_rules.hpp"
#include "table_catalog.hpp"
#include <stdexcept>

namespace saga::core {
EnemyTable load_enemy_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    // EnemyData.Load 0x116ec2c; GetIntTripleArray 0x11271bc permits null only at outer level.
    return detail::load_table_records<EnemyRecord>(text,normalize,parser,[](detail::TableTokens& t){
        EnemyRecord r;r.id=t.integer();r.name=std::string(t.take());r.enemy_seb=t.integer();
        r.experience=t.integers();
        const auto groups=t.integer();
        if(groups>=0){
            r.treasures.emplace();
            for(std::int32_t i=0;i<groups;++i){
                const auto count=t.integer();if(count<0)throw std::length_error("negative treasure group size");
                std::vector<EventIntegers> rows;
                for(std::int32_t j=0;j<count;++j)rows.push_back(*t.integers(false));
                r.treasures->push_back(std::move(rows));
            }
        }
        r.voice=t.integer();r.attribute=t.integer();r.resource=std::string(t.take());
        r.image=t.integer();r.seb=t.integer();r.frame=t.integer();r.rank=t.integer();
        r.parameters=t.script();r.flags=t.integer();return r;
    });
}
std::int32_t enemy_experience(const EnemyRecord& enemy,std::int32_t level){
    // EnemyData.GetExp 0x116e9c0, Graph.Easing 0x1c5df9c with amplitude=0.
    if(!enemy.experience)throw std::invalid_argument("null enemy experience array");
    const auto start=enemy.experience->at(0),end=enemy.experience->at(1);
    const auto bits=static_cast<std::uint32_t>(level)-1U;
    const auto progress=static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):
                                               static_cast<std::int64_t>(bits)-0x100000000LL);
    return easing_zero_amplitude(start,end,999,progress);
}
namespace {
const std::vector<EventIntegers>& first_group(const EnemyRecord& enemy){
    if(!enemy.treasures)throw std::invalid_argument("null enemy treasure groups");
    return enemy.treasures->at(0);
}
}
void spawn_enemy_rewards(std::int32_t level,EnemyRewardHost& host){
    const auto experience=enemy_experience(host.enemy_data(),level);
    if(experience>0)host.spawn_experience_drop(experience);
    // Original resolves data again after EXP spawn, and twice before a nonempty treasure loop.
    if(first_group(host.enemy_data()).empty())return;
    const auto& group=first_group(host.enemy_data());
    for(std::size_t i=0;i<group.size();++i){
        const auto id=group.at(i).at(0),threshold=group.at(i).at(1);
        if(host.hits1000(threshold)){host.spawn_treasure_drop(id);break;}
    }
}
}
