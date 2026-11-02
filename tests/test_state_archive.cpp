#include "core/ecs/state_archive.hpp"
#include "core/ecs/component_slots.hpp"
#include "core/ecs/entity_subset.hpp"
#include "core/ecs/entity_registration.hpp"
#include "core/ecs/dynamic_subsets.hpp"
#include "core/gameplay/game_random.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"state archive contract failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}check(caught);}

namespace saga::core {
// Compile-time proof of the narrow checkpoint access, without a mutable container API.
struct StateArchiveAccess {
    static_assert(std::is_same_v<decltype(std::declval<const ComponentSlots&>().slots_),std::vector<Component*>>);
    static_assert(std::is_same_v<decltype(std::declval<const EntitySubset&>().free_),std::size_t>);
    static_assert(std::is_same_v<decltype(std::declval<const EntitySubset&>().slots_),std::vector<EntitySubset::Slot>>);
    static_assert(std::is_same_v<decltype(std::declval<const EntitySubset::Cursor&>().index_),std::size_t>);
    static_assert(std::is_same_v<decltype(std::declval<const EntityRegistry&>().slots_),std::vector<EntityRegistry::Slot>>);
    static_assert(std::is_same_v<decltype(std::declval<const EntityRegistry&>().version_),std::uint32_t>);
    static_assert(std::is_same_v<decltype(std::declval<const EntityRegistry::Cursor&>().index_),std::size_t>);
    static_assert(std::is_same_v<decltype(std::declval<const ComponentEvents&>().added_),ComponentEvents::Channel>);
    static_assert(std::is_same_v<decltype(std::declval<const DynamicSubsets&>().order_),std::vector<std::shared_ptr<DynamicSubsets::Entry>>>);
    static_assert(std::is_same_v<decltype(std::declval<const ComponentGroupFilter&>().owned_all_),std::shared_ptr<const std::vector<std::int32_t>>>);
    static_assert(std::is_same_v<decltype(std::declval<const LegacySystemRandom&>().seeds_),std::array<std::int32_t,56>>);
    static_assert(std::is_same_v<decltype(std::declval<const SelectedRandomSource&>().sources_),std::array<RandomSource*,3>>);
};
}

float binary32(std::uint32_t bits){float value;std::memcpy(&value,&bits,sizeof value);return value;}
void integer_bytes(){
    StateArchive a;a.number(false);a.number(true);
    a.number(std::numeric_limits<std::int8_t>::min());a.number(std::numeric_limits<std::int8_t>::max());a.number(std::numeric_limits<std::uint8_t>::max());
    a.number(std::numeric_limits<std::int16_t>::min());a.number(std::numeric_limits<std::int16_t>::max());a.number(std::numeric_limits<std::uint16_t>::max());
    a.number(std::numeric_limits<std::int32_t>::min());a.number(std::numeric_limits<std::int32_t>::max());a.number(std::numeric_limits<std::uint32_t>::max());
    a.number(std::numeric_limits<std::int64_t>::min());a.number(std::numeric_limits<std::int64_t>::max());a.number(std::numeric_limits<std::uint64_t>::max());
    check(a.data()==std::vector<std::uint8_t>{0,1,128,127,255,
        0,128,255,127,255,255,
        0,0,0,128,255,255,255,127,255,255,255,255,
        0,0,0,0,0,0,0,128,255,255,255,255,255,255,255,127,255,255,255,255,255,255,255,255});
    StateArchive endian;endian.number(std::uint16_t{0x1234});endian.number(std::uint32_t{0x89abcdef});endian.number(std::uint64_t{0x0123456789abcdef});
    check(endian.data()==std::vector<std::uint8_t>{0x34,0x12,0xef,0xcd,0xab,0x89,0xef,0xcd,0xab,0x89,0x67,0x45,0x23,0x01});
    StateArchive negative;negative.number(std::int8_t{-1});negative.number(std::int16_t{-2});negative.number(std::int32_t{-3});negative.number(std::int64_t{-4});
    check(negative.data()==std::vector<std::uint8_t>{255,254,255,253,255,255,255,252,255,255,255,255,255,255,255});
}
void float_bytes(){
    StateArchive a;for(const auto bits:{0U,0x80000000U,0x3f800000U,0xbf800000U,0x7f800000U,0xff800000U,0x7fc01234U,0xffc05678U,1U})a.number(binary32(bits));
    check(a.data()==std::vector<std::uint8_t>{0,0,0,0,0,0,0,128,0,0,128,63,0,0,128,191,
        0,0,128,127,0,0,128,255,0x34,0x12,0xc0,0x7f,0x78,0x56,0xc0,0xff,1,0,0,0});
    StateArchive positive,negative;positive.number(0.0F);negative.number(-0.0F);check(positive.data()!=negative.data());
}
void counted_values(){
    StateArchive bytes;const std::uint8_t payload[]{0,128,255};bytes.bytes(payload,3);bytes.bytes(nullptr,0);
    check(bytes.data()==std::vector<std::uint8_t>{3,0,0,0,0,0,0,0,0,128,255,0,0,0,0,0,0,0,0});
    const auto before=bytes.data();fails([&]{bytes.bytes(nullptr,1);});check(bytes.data()==before);
    StateArchive narrow;narrow.text(std::string_view("a\0z",3));
    check(narrow.data()==std::vector<std::uint8_t>{3,0,0,0,0,0,0,0,97,0,122});
    const std::u16string units{u'A',static_cast<char16_t>(0xd800),static_cast<char16_t>(0xdc00),u'\0',static_cast<char16_t>(0xffff)};
    StateArchive wide;wide.text(units);check(wide.data()==std::vector<std::uint8_t>{5,0,0,0,0,0,0,0,65,0,0,216,0,220,0,0,255,255});
    StateArchive empty;empty.text(std::string_view{});empty.text(std::u16string_view{});check(empty.data()==std::vector<std::uint8_t>(16,0));
    StateArchive tag,text;tag.tag("world/v1");text.text("world/v1");check(tag.data()==text.data());
    StateArchive count;count.count(0);count.count(1);count.count(static_cast<std::size_t>(0xffffffffU));
    check(count.data()==std::vector<std::uint8_t>{0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,255,255,255,255,0,0,0,0});
}
void references(){
    std::array<std::array<std::int32_t,2>,5> nested{};StateArchive typed;
    check(static_cast<const void*>(&nested)==static_cast<const void*>(&nested[0]));
    check(typed.reference("array",&nested)!=typed.reference("array",&nested[0]));
    const auto& constant=nested[0];check(typed.reference("array",&constant)==typed.reference("array",&nested[0]));
    int first=7,second=7;StateArchive a;
    check(a.reference("x",nullptr)==0);check(a.reference("x",&first)==1);check(a.reference("x",&first)==1);
    check(a.reference("x",&second)==2);check(a.reference("y",&first)==3);check(a.reference("y",nullptr)==0);
    StateArchive expected;for(const auto id:{0U,1U,1U,2U}){expected.text("x");expected.number(static_cast<std::uint64_t>(id));}
    expected.text("y");expected.number(std::uint64_t{3});expected.text("y");expected.number(std::uint64_t{0});check(a.data()==expected.data());
    // The namespace is part of the bytes, even if both graphs introduce exactly one object.
    StateArchive x,y;x.reference("x",&first);y.reference("y",&first);check(x.data()!=y.data());
    const auto pair=[](const int* left,const int* right){StateArchive s;s.count(2);
        for(const auto* p:{left,right}){s.reference("cell",p);s.number(static_cast<std::int32_t>(*p));}return s.data();};
    int relocated_first=7,relocated_second=7;
    check(pair(&first,&first)==pair(&relocated_first,&relocated_first));
    check(pair(&first,&second)==pair(&relocated_first,&relocated_second));
    check(pair(&first,&first)!=pair(&first,&second));
    // Cycles and forward references terminate because the caller writes the ordered graph once.
    struct Node {std::int32_t value;Node* next;};Node graph[3]{{10,nullptr},{20,nullptr},{30,nullptr}},moved[3]{{10,nullptr},{20,nullptr},{30,nullptr}};
    for(auto* nodes:{graph,moved}){nodes[0].next=&nodes[2];nodes[1].next=&nodes[2];nodes[2].next=&nodes[0];}
    const auto archive=[](Node* nodes){StateArchive s;s.tag("graph/v1");s.count(3);
        for(int i=0;i<3;++i){s.reference("node",&nodes[i]);s.number(nodes[i].value);s.reference("node",nodes[i].next);}return s.data();};
    check(archive(graph)==archive(moved));moved[1].next=&moved[1];check(archive(graph)!=archive(moved));
}
int main(){integer_bytes();float_bytes();counted_values();references();}
