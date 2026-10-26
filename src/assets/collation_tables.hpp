#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace saga::assets {
enum class CollationTable:std::size_t { Ignorable,Category,Level1,Level2,Level3 };
struct CollationTailoring {
    std::int32_t lcid,index,count;
    bool french_sort;
};
class CollationTables {
public:
    // Owns the core bytes; returned views remain valid until this object is moved/destroyed.
    static CollationTables read(std::string core,std::string_view tailoring);
    std::string_view table(CollationTable)const;
    const std::vector<CollationTailoring>& tailoring()const{return tailoring_;}
    std::u16string_view tailoring_units()const{return units_;}
private:
    struct Range {std::size_t offset=0,size=0;};
    std::string core_;
    std::array<Range,5> ranges_{};
    std::vector<CollationTailoring> tailoring_;
    std::u16string units_;
};
}
