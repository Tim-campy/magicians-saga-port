#pragma once
#include "table_tokens.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

namespace saga::core::detail {
// Data.Load indexing helper 0x129fafc; framing follows the existing event reader.
template<class Record, class Read>
std::vector<std::optional<Record>> load_table_records(std::string_view text, bool normalize,
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
