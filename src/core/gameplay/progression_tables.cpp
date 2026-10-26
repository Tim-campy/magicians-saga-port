#include "progression_tables.hpp"
#include "table_tokens.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
// Data.Load indexing helper 0x129fafc; framing follows the existing event reader.
template<class Record, class Read>
std::vector<std::optional<Record>> load(std::string_view text, bool normalize,
                                       const EventIntegerParser& parser, Read read) {
    if(text.substr(0,3)=="\xef\xbb\xbf") text.remove_prefix(3);
    std::vector<Record> rows;
    std::int32_t max_id=0;
    while(!text.empty()) {
        const auto end=text.find('\n');
        auto line=text.substr(0,end);
        if(normalize && end!=std::string_view::npos && !line.empty() && line.back()=='\r')
            line.remove_suffix(1);
        if(!line.empty()) {
            detail::TableTokens tokens(line,parser);
            auto row=read(tokens);
            max_id=std::max(max_id,row.id);
            rows.push_back(std::move(row));
        }
        if(end==std::string_view::npos) break;
        text.remove_prefix(end+1);
    }
    if(max_id==std::numeric_limits<std::int32_t>::max()) throw std::length_error("table size overflow");
    std::vector<std::optional<Record>> result(static_cast<std::size_t>(max_id)+1);
    for(auto& row:rows) result.at(static_cast<std::size_t>(row.id))=std::move(row);
    return result;
}
}
LevelTable load_level_table(std::string_view text, bool normalize, const EventIntegerParser& parser) {
    // TableData.Load 0x11732d0: ten sequential GetInt calls, trailing tokens ignored.
    return load<LevelTableRecord>(text,normalize,parser,[](detail::TableTokens& t){
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
    // CharaData.Load 0x116dcc4. Consume the entire schema, retaining only progression inputs.
    return load<CharacterProgressionRecord>(text,normalize,parser,[](detail::TableTokens& t){
        CharacterProgressionRecord r;
        r.id=t.integer();t.take();t.integer();t.take();t.integer();t.take();
        for(int i=0;i<5;++i) t.integer(); // weapon, recruit type/cost, initial level, rank
        r.required_experience_rate=t.integer();
        t.script(); // parameters_: count-prefixed int32[][], same framing as event scripts
        t.integer();t.integer();t.integer(); // voice, town, area
        t.script(); // residentTerm_
        t.integer();t.integer(); // appearRate, BaseData flag
        return r;
    });
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
