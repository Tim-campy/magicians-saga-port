#include "quest_rewards.hpp"
#include "table_catalog.hpp"
#include <algorithm>

namespace saga::core {
namespace {
std::int32_t add(std::int32_t a,std::int32_t b){
    const auto v=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    return v<=0x7fffffffU?static_cast<std::int32_t>(v):
        static_cast<std::int32_t>(static_cast<std::int64_t>(v)-0x100000000LL);
}
}
TreasureTable load_treasure_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    // TreasureData.Load 0x11737a4: record schema, with trailing tokens ignored.
    return detail::load_table_records<TreasureRecord>(text,normalize,parser,[](detail::TableTokens& t){
        TreasureRecord r;
        r.id=t.integer();r.name=std::string(t.take());r.resource=std::string(t.take());
        r.image=t.integer();r.experience=t.integer();r.money=t.integer();r.crystals=t.integer();
        r.weapon_id=t.integer();r.character_id=t.integer();r.cloth_id=t.integer();r.hat_id=t.integer();
        r.flags=t.integer();return r;
    });
}
void recruit_character(RecruitmentState& character){
    // CharaData.Recruit 0x116db84, followed by BaseData.Find(true), even if already found.
    character.recruited=1;character.discovery_state=1;character.is_new=true;
}
void update_quest_record(QuestRecord& record,std::int32_t result,std::int32_t experience,
                         std::int32_t money,QuestRecordHost& host){
    // StageSystem.UpdateQuestRecord 0x1165728. Keep host reads after preceding stores.
    record.challenges=add(record.challenges,1);
    const auto defeated=host.defeated_enemies();
    record.defeated=add(record.defeated,defeated);
    if(result!=0)return;
    record.best_experience=std::max(record.best_experience,experience);
    record.best_money=std::max(record.best_money,money);
    record.clears=add(record.clears,1);
    const auto prisoners=host.collected_prisoners();
    record.rescued=add(record.rescued,prisoners);
    record.best_damage=std::max(record.best_damage,host.total_damage());
}
QuestRewardResult grant_quest_rewards(QuestRewardHost& host){
    // StageSystem.ShowQuestResult 0x1164548: totals and display prisoner count are entry snapshots.
    const auto& drops=host.drops();
    QuestRewardResult result;
    result.experience=drops.experience;result.money=drops.money;result.prisoners=drops.prisoners;
    while(const auto id=host.next_treasure()){
        const auto& treasure=host.treasure(*id);
        result.experience=add(result.experience,treasure.experience);
        result.money=add(result.money,treasure.money);
        result.crystals=add(result.crystals,treasure.crystals);
        if(treasure.weapon_id!=-1){
            add_item_stock(host.item(RewardItem::weapon,treasure.weapon_id),1);
            result.weapons.push_back({treasure.weapon_id,1});
        }
        if(treasure.character_id!=-1){
            auto& character=host.character(treasure.character_id);
            if(character.discovery_state==0){character.discovery_state=1;character.is_new=true;}
            recruit_character(character);
            result.characters.push_back({treasure.character_id,1});
        }
        if(treasure.cloth_id!=-1){
            add_item_stock(host.item(RewardItem::cloth,treasure.cloth_id),1);
            result.clothes.push_back({treasure.cloth_id,1});
        }
        if(treasure.hat_id!=-1){
            add_item_stock(host.item(RewardItem::hat,treasure.hat_id),1);
            result.hats.push_back({treasure.hat_id,1});
        }
    }
    host.add_money(result.money);
    host.add_crystals(result.crystals);
    const auto prisoners=host.drops().prisoners; // Live count, not the display snapshot.
    auto& rescued=host.rescued_prisoners();rescued=add(rescued,prisoners);
    auto& record=host.quest_record();
    update_quest_record(record,0,result.experience,result.money,host);
    result.members=grant_quest_member_experience(result.experience,host.members());
    return result;
}
}
