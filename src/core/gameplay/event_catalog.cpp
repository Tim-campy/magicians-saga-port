#include "event_catalog.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
bool space(char c) { return c==' ' || (c>='\t' && c<='\r'); }
class Tokens {
public:
    Tokens(std::string_view line, const EventIntegerParser& parser) : line_(line), parser_(parser) {}
    std::string_view take() {
        if (done_) throw std::out_of_range("truncated event record");
        const auto tab=line_.find('\t');
        const auto token=line_.substr(0,tab);
        if (tab==std::string_view::npos) done_=true;
        else line_.remove_prefix(tab+1);
        return token;
    }
    std::int32_t integer() { return parser_(take()); }
    EventScript script() {
        const auto count=integer();
        if (count<0) return nullptr;
        auto result=std::make_shared<std::vector<EventInstruction>>();
        for (std::int32_t i=0;i<count;++i) {
            const auto length=integer();
            if (length<0) throw std::length_error("negative event row size");
            auto row=std::make_shared<EventIntegers>();
            for (std::int32_t j=0;j<length;++j) row->push_back(integer());
            result->push_back(row);
        }
        return result;
    }
private:
    std::string_view line_;
    const EventIntegerParser& parser_;
    bool done_=false;
};
}
std::int32_t parse_event_decimal(std::string_view token) {
    while (!token.empty() && space(token.front())) token.remove_prefix(1);
    while (!token.empty() && space(token.back())) token.remove_suffix(1);
    bool negative=false;
    if (!token.empty() && (token.front()=='+' || token.front()=='-')) {
        negative=token.front()=='-';token.remove_prefix(1);
    }
    if (token.empty()) throw std::invalid_argument("empty event integer");
    std::uint32_t value=0,limit=negative ? 0x80000000U : 0x7fffffffU;
    for (char c:token) {
        if (c<'0' || c>'9') throw std::invalid_argument("unsupported event integer syntax");
        const auto digit=static_cast<std::uint32_t>(c-'0');
        if (value>(limit-digit)/10U) throw std::out_of_range("event integer overflow");
        value=value*10U+digit;
    }
    if (negative && value==0x80000000U) return std::numeric_limits<std::int32_t>::min();
    return negative ? -static_cast<std::int32_t>(value) : static_cast<std::int32_t>(value);
}
// EventData.Load 0x116fa4c; StringArrayStream; indexing helper 0x129fafc.
EventCatalog load_event_catalog(std::string_view text, bool normalize_crlf,
                                const EventIntegerParser& integer) {
    if (text.substr(0,3)=="\xef\xbb\xbf") text.remove_prefix(3);
    EventCatalog records;
    std::int32_t max_id=0;
    while (!text.empty()) {
        const auto end=text.find('\n');
        auto line=text.substr(0,end);
        // Only CR immediately followed by LF is normalized, not a final lone CR.
        if (normalize_crlf && end!=std::string_view::npos && !line.empty() && line.back()=='\r')
            line.remove_suffix(1);
        if (!line.empty()) {
            Tokens tokens(line,integer);
            auto record=std::make_shared<EventConditionRecord>();
            record->execution_count=0;
            record->id=tokens.integer();record->name=std::string(tokens.take());
            record->timing=tokens.integer();record->terms=tokens.script();record->effects=tokens.script();
            if (record->id>max_id) max_id=record->id;
            records.push_back(record); // Original ignores trailing fields.
        }
        if (end==std::string_view::npos) break;
        text.remove_prefix(end+1);
    }
    if (max_id==std::numeric_limits<std::int32_t>::max())
        throw std::length_error("event catalog size overflow");
    EventCatalog indexed(static_cast<std::size_t>(max_id)+1);
    for (const auto& record:records) indexed.at(static_cast<std::size_t>(record->id))=record;
    return indexed;
}
// GetDelayId 0x110caa0: pointer identity, not structural equality.
std::int32_t identify_event_script(const EventCatalog& records, const EventScript& script,
                                 const std::function<void()>& fallback) {
    for (std::size_t i=0;i<records.size();++i)
        if (records[i] && records[i]->effects==script) return static_cast<std::int32_t>(i);
    fallback();
    return -1;
}
// GetDelayScript 0x110d6dc: signed remainder; negative multiples of 1000 map to 0.
EventScript resolve_event_script(const EventCatalog& records, std::int32_t id) {
    if (id>999) return nullptr;
    const auto& record=records.at(static_cast<std::size_t>(id%1000));
    if (!record) throw std::invalid_argument("null event record");
    return record->effects;
}
}
