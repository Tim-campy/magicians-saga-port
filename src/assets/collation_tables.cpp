#include "collation_tables.hpp"
#include <stdexcept>
#include <utility>
namespace saga::assets {
namespace {
class Reader {
public:
    explicit Reader(std::string_view data):data_(data){}
    std::size_t position=0;
    std::string_view take(std::size_t n){
        if(n>data_.size()-position)throw std::invalid_argument("truncated collation resource");
        auto result=data_.substr(position,n);position+=n;return result;
    }
    std::uint8_t byte(){return static_cast<std::uint8_t>(take(1)[0]);}
    std::uint16_t word(){auto b=take(2);return static_cast<std::uint16_t>(
        static_cast<unsigned char>(b[0])|(static_cast<unsigned char>(b[1])<<8));}
    std::uint32_t integer(){auto b=take(4);std::uint32_t result=0;
        for(unsigned i=0;i<4;++i)result|=static_cast<std::uint32_t>(static_cast<unsigned char>(b[i]))<<(i*8);
        return result;
    }
    std::size_t count(std::size_t width){
        auto n=integer();
        if(n>0x7fffffffU||n>(data_.size()-position)/width)throw std::invalid_argument("invalid collation count");
        return n;
    }
    void end(){if(position!=data_.size())throw std::invalid_argument("trailing collation resource data");}
private:
    std::string_view data_;
};
std::int32_t signed32(std::uint32_t n){
    return n<=0x7fffffffU?static_cast<std::int32_t>(n):static_cast<std::int32_t>(static_cast<std::int64_t>(n)-0x100000000LL);
}
}
// MSCompatUnicodeTable..cctor 0x1cd5748: decoded table/record portion.
// Bounds/exact-end checks are portable input validation, not native trusted-memory behavior.
CollationTables CollationTables::read(std::string core,std::string_view tailoring){
    Reader c(core),t(tailoring);
    if(c.byte()!=3||t.byte()!=3)throw std::invalid_argument("unsupported collation version");
    CollationTables result;
    for(auto& range:result.ranges_){
        range.size=c.integer();range.offset=c.position;c.take(range.size);
    }
    c.end();
    const auto records=t.count(13);result.tailoring_.reserve(records);
    for(std::size_t i=0;i<records;++i){
        const auto lcid=signed32(t.integer()),index=signed32(t.integer()),count=signed32(t.integer());
        result.tailoring_.push_back({lcid,index,count,t.byte()!=0});
    }
    t.take(2); // Original skips this word without inspecting it.
    const auto units=t.count(2);result.units_.reserve(units);
    for(std::size_t i=0;i<units;++i)result.units_.push_back(static_cast<char16_t>(t.word()));
    t.end();result.core_=std::move(core);return result;
}
std::string_view CollationTables::table(CollationTable kind)const{
    const auto& range=ranges_.at(static_cast<std::size_t>(kind));
    return std::string_view(core_).substr(range.offset,range.size);
}
}
