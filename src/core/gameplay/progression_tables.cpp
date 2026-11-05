#include "progression_tables.hpp"
#include "table_catalog.hpp"
#include "stage_rewards.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {

LevelTable load_level_table(std::string_view text, bool normalize, const EventIntegerParser& parser) {
    // TableData.Load 0x11732d0: ten sequential GetInt calls, trailing tokens ignored.
    return detail::load_table_records<LevelTableRecord>(text,normalize,parser,[](detail::TableTokens& t){
        LevelTableRecord r;
        r.id=t.integer();r.level=t.integer();r.experience=t.integer();
        for(auto& value:r.shop_parameters) value=t.integer();
        r.unlock_money=t.integer();r.unlock_crystal=t.integer();
        r.unlock_web_page=t.integer();r.unlock_rescue=t.integer();
        return r;
    });
}
CharacterProgressionTable load_character_progression_table(std::string_view text, bool normalize,
                                                          const EventIntegerParser& parser) {
    // CharaData.Load 0x116dcc4. Consume the entire schema, retaining progression and combat inputs.
    return detail::load_table_records<CharacterProgressionRecord>(text,normalize,parser,[](detail::TableTokens& t){
        CharacterProgressionRecord r;
        r.id=t.integer();r.base_name=std::make_shared<const std::string>(t.take());r.unlock_level=t.integer();r.resource=t.take();r.image=t.integer();r.secondary_resource=t.take();
        r.weapon_id=t.integer();t.integer();r.recruit_cost=t.integer();
        r.initial_level=t.integer();t.integer(); // rank
        r.required_experience_rate=t.integer();
        r.parameters=t.script(); // Same framing as event scripts.
        r.voice_id=t.integer();r.town_id=t.integer();r.area_id=t.integer();
        r.resident_terms=t.script();
        r.appear_rate=t.integer();r.flags=t.integer();
        return r;
    });
}
void append_character_shop_entries(std::vector<std::int32_t>& output,const CharacterProgressionTable& records,
    const std::function<bool(const CharacterProgressionRecord&)>& found,
    const std::function<bool(const CharacterProgressionRecord&)>& recruited){
    // ShopForm.Reset0x120a8e0: Where(nonnull), OrderBy(lockLevel), ToArray,
    // then isFound, Check(0x14), !isRecruit and publication of the live record.id.
    struct Ranked {const CharacterProgressionRecord* record;std::int32_t key;};
    std::vector<Ranked> ordered;ordered.reserve(records.size());
    for(const auto& row:records)if(row)ordered.push_back({&*row,row->unlock_level});
    std::stable_sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){return a.key<b.key;});
    for(const auto& entry:ordered){const auto& row=*entry.record;
        if(found(row)&&(static_cast<std::uint32_t>(row.flags)&0x14U)!=0&&!recruited(row))output.push_back(row.id);
    }
}
void reset_character_for_new_game(CharacterNewGameState& state,const CharacterProgressionRecord& record,const std::function<void(bool)>& find){
    state.name=record.base_name;state.status_definition=record.parameters;
    if(!record.parameters)throw std::invalid_argument("missing new-game character parameters");
    const auto count=record.parameters->size();
    if(count>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("character parameter count outside managed array bounds");
    state.status=std::make_shared<EventIntegers>(count,0);
    for(std::size_t i=0;i<count;++i){
        if(!state.status_definition)throw std::invalid_argument("missing character status definition");
        const auto& row=state.status_definition->at(i);
        if(!row)throw std::invalid_argument("missing character status row");
        state.status->at(i)=row->at(0);
    }
    state.witch_experience=std::make_shared<EventIntegers>(EventIntegers{0,300,0});
    if((static_cast<std::uint32_t>(record.flags)&1U)!=0)find(true);
    const auto level=record.initial_level;
    // MOVI D0 with encoded imm8=0x0f is 0x00000000ffffffff, not two -1 words.
    state.living_address=-1;state.recruited=0;state.level=level;state.experience=0;
}
void reset_character_for_new_game(CharacterNewGameState& state,const CharacterProgressionRecord& record){
    reset_character_for_new_game(state,record,[&state](bool fresh){find_data_record(state.discovery_state,state.is_new,fresh);});
}
std::int32_t TableExperienceHost::table_count() {
    if(levels_.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("table size overflow");
    return static_cast<std::int32_t>(levels_.size());
}
std::int32_t TableExperienceHost::next_experience(std::int32_t index) {
    const auto& character=characters_.at(static_cast<std::size_t>(character_id_));
    if(!character) throw std::invalid_argument("missing character row");
    const auto& level=levels_.at(static_cast<std::size_t>(std::clamp(index,1,1200)));
    if(!level) throw std::invalid_argument("missing level row");
    return scaled_level_experience(level->experience,character->required_experience_rate);
}
}
