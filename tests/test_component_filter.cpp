#include "core/ecs/component_filter.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace saga::core;
void check(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
template<class F> void invalid_index(F action) {
    bool caught = false;
    try { action(); } catch (const std::out_of_range&) { caught = true; }
    check(caught, "reachable invalid index must fail");
}
struct Item : Component {};
int main() {
    Item item;
    ComponentSlots entity(35);
    std::vector<std::int32_t> all{2, 10}, any{12, 10}, none{3}, empty, invalid{-1};
    entity.add(2, &item).add(10, &item);
    check(matches_component_filter(&entity, &all, &any, &none), "combined accepted criteria");
    check(matches_component_filter(&entity, nullptr, &empty, nullptr), "empty OR is skipped");
    check(!matches_component_filter(nullptr, &invalid, &invalid, &invalid), "null entity skips criteria");
    entity.remove(10);
    check(!matches_component_filter(&entity, &all, &invalid, &invalid), "AND rejects before OR/NOR");
    check(!matches_component_filter(&entity, nullptr, &any, &invalid), "OR rejects before NOR");
    entity.add(10, &item).add(3, &item);
    check(!matches_component_filter(&entity, &all, &any, &none), "NOR excludes present component");
    invalid_index([&] { matches_component_filter(&entity, &invalid, nullptr, nullptr); });
    invalid_index([&] { matches_component_filter(&entity, &all, &invalid, nullptr); });
    invalid_index([&] { matches_component_filter(&entity, &all, &any, &invalid); });
    none[0] = 4;
    check(matches_component_filter(&entity, &all, &any, &none), "borrowed criteria read again");
    entity.change(2, nullptr);
    check(!matches_component_filter(&entity, &all, &any, &none), "predicate observes null replacement");
    std::cout << "component filter contracts passed\n";
}
