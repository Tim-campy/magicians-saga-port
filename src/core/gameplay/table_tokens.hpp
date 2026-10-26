#pragma once
#include "event_catalog.hpp"
#include <stdexcept>
#include <optional>

namespace saga::core::detail {
class TableTokens {
public:
    TableTokens(std::string_view line, const EventIntegerParser& parser) : line_(line), parser_(parser) {}
    std::string_view take() {
        if (done_) throw std::out_of_range("truncated table record");
        const auto tab=line_.find('\t');
        const auto token=line_.substr(0,tab);
        if (tab==std::string_view::npos) done_=true;
        else line_.remove_prefix(tab+1);
        return token;
    }
    std::int32_t integer() { return parser_(take()); }
    std::optional<EventIntegers> integers(bool allow_null=true) {
        const auto count=integer();
        if(count<0){
            if(allow_null)return std::nullopt;
            throw std::length_error("negative nested integer array size");
        }
        EventIntegers values;
        for(std::int32_t i=0;i<count;++i)values.push_back(integer());
        return values;
    }
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
