#include "collation_index.hpp"
#include <stdexcept>
namespace saga::core {
namespace {
std::int32_t signed32(std::uint32_t n){return n<=0x7fffffffU?static_cast<std::int32_t>(n):
    static_cast<std::int32_t>(static_cast<std::int64_t>(n)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return signed32(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t subtract(std::int32_t a,std::int32_t b){return signed32(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
}
// Read the extracted MSCompatUnicodeTableUtil configuration without normalizing ranges.
std::vector<CodePointIndexer> read_collation_indexers(std::string_view data){
    if(data.size()<8||data.substr(0,4)!="SCI1")throw std::invalid_argument("collation indexer header");
    std::size_t pos=4;
    auto integer=[&](){
        if(data.size()-pos<4)throw std::invalid_argument("truncated collation indexer");
        std::uint32_t n=0;for(unsigned j=0;j<4;++j)n|=static_cast<std::uint32_t>(static_cast<unsigned char>(data[pos++]))<<(8*j);
        return signed32(n);
    };
    if(integer()!=7)throw std::invalid_argument("collation indexer count");
    std::vector<CodePointIndexer> result;result.reserve(7);
    for(int i=0;i<7;++i){
        const auto count=integer(),fallback=integer(),default_point=integer();
        if(count<0||static_cast<std::size_t>(count)>(data.size()-pos)/8)throw std::invalid_argument("collation range count");
        std::vector<std::int32_t> starts,ends;starts.reserve(count);ends.reserve(count);
        for(int j=0;j<count;++j)starts.push_back(integer());
        for(int j=0;j<count;++j)ends.push_back(integer());
        result.emplace_back(starts,ends,fallback,default_point);
    }
    if(pos!=data.size())throw std::invalid_argument("trailing collation indexer data");
    return result;
}
// CodePointIndexer..ctor 0x1cd3ea0.
CodePointIndexer::CodePointIndexer(const std::vector<std::int32_t>& starts,const std::vector<std::int32_t>& ends,
                                 std::int32_t default_index,std::int32_t default_code_point)
    :default_index_(default_index),default_code_point_(default_code_point){
    ranges_.reserve(starts.size());
    for(std::size_t i=0;i<starts.size();++i){
        if(i>=ends.size())throw std::out_of_range("missing code-point range end");
        auto count=subtract(ends[i],starts[i]);
        auto begin=total_count_;total_count_=add(total_count_,count);
        ranges_.push_back({starts[i],ends[i],count,begin,total_count_});
    }
}
// CodePointIndexer.ToIndex 0x1cd4010: signed comparisons, first interval/early gap.
std::int32_t CodePointIndexer::to_index(std::int32_t point)const{
    for(const auto& range:ranges_){
        if(range.start>point)return default_index_;
        if(range.end>point)return add(subtract(point,range.start),range.index_start);
    }
    return default_index_;
}
// Category 0x1cd4cf8, Level1 0x1cd4d9c, Level2 0x1cd4e40, Level3 0x1cd4ee4.
std::uint8_t collation_weight(const assets::CollationTables& tables,assets::CollationTable table,
                             const CodePointIndexer& indexer,std::int32_t point){
    const auto bytes=tables.table(table);const auto index=indexer.to_index(point);
    // Original trusts its generated indexer; reject invalid supplied data at the port boundary.
    if(index<0||static_cast<std::size_t>(index)>=bytes.size())throw std::out_of_range("collation weight index");
    return static_cast<std::uint8_t>(bytes[static_cast<std::size_t>(index)]);
}
// MSCompatUnicodeTable.GetTailoringInfo 0x1cd4244: first exact int32 LCID match.
const assets::CollationTailoring* find_collation_tailoring(const assets::CollationTables& tables,std::int32_t lcid){
    for(const auto& record:tables.tailoring())if(record.lcid==lcid)return &record;
    return nullptr;
}
}
