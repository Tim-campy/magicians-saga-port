#pragma once
#include "assets/collation_tables.hpp"
#include <cstdint>
#include <vector>
namespace saga::core {
struct CodePointRange {std::int32_t start,end,count,index_start,index_end;};
class CodePointIndexer {
public:
    CodePointIndexer(const std::vector<std::int32_t>& starts,const std::vector<std::int32_t>& ends,
                     std::int32_t default_index,std::int32_t default_code_point);
    std::int32_t to_index(std::int32_t code_point)const;
    std::int32_t total_count()const{return total_count_;}
    const std::vector<CodePointRange>& ranges()const{return ranges_;}
private:
    std::vector<CodePointRange> ranges_;
    std::int32_t default_index_,default_code_point_,total_count_=0;
};
// Category/Level1/Level2/Level3 share this indexed-byte access contract.
std::uint8_t collation_weight(const assets::CollationTables&,assets::CollationTable,
                              const CodePointIndexer&,std::int32_t code_point);
// Returned pointer belongs to the supplied immutable table object.
const assets::CollationTailoring* find_collation_tailoring(const assets::CollationTables&,std::int32_t lcid);
}
