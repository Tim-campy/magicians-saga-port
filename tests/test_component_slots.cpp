#include "core/ecs/component_slots.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace saga::core;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
struct Item : Component {};
struct Event { char kind; std::int32_t id; Component* old; Component* next; };
struct Observer : ComponentObserver {
    std::vector<Event> events;
    bool fail = false;
    Component* insert_later = nullptr;
    Component* insert_visited = nullptr;
    void added(ComponentSlots& slots, std::int32_t id, Component* next) override {
        check(slots.get(id) == next, "add visible before notification");
        events.push_back({'a', id, nullptr, next});
        if (fail) throw std::runtime_error("observer failure");
    }
    void changed(ComponentSlots& slots, std::int32_t id, Component* old, Component* next) override {
        check(slots.get(id) == next, "change visible before notification");
        events.push_back({'c', id, old, next});
        if (fail) throw std::runtime_error("observer failure");
    }
    void removed(ComponentSlots& slots, std::int32_t id, Component* old) override {
        check(!slots.contains(id), "remove visible before notification");
        events.push_back({'r', id, old, nullptr});
        if (insert_later && id == 0) slots.add(2, insert_later);
        if (insert_visited && id == 1) {
            slots.add(0, insert_visited);
            slots.add(1, insert_visited);
        }
        if (fail) throw std::runtime_error("observer failure");
    }
};
template<class F> void fails(F operation) {
    bool failed = false;
    try { operation(); } catch (const std::exception&) { failed = true; }
    check(failed, "operation must fail");
}
int main() {
    Item a, b;
    Observer observer;
    ComponentSlots slots(3, &observer);
    check(!slots.contains(0), "slots start null");
    check(&slots.add(0, &a) == &slots, "fluent identity");
    slots.add(0, &b);
    check(slots.get(0) == &a && observer.events.size() == 1, "duplicate add ignored");
    slots.change(0, &b);
    check(observer.events.back().old == &a && observer.events.back().next == &b, "change arguments");
    slots.change(0, &b);
    check(observer.events.size() == 3, "identical change still notifies");
    slots.change(0, nullptr);
    check(observer.events.back().kind == 'c' && !slots.contains(0), "null change is not Remove");
    slots.remove(0);
    check(observer.events.size() == 4, "missing remove ignored");
    slots.add(0, nullptr);
    slots.change(0, nullptr);
    check(observer.events.size() == 6 && observer.events.back().kind == 'a', "null add is notified and absent change delegates to Add");

    observer.events.clear();
    observer.insert_later = &b;
    slots.add(0, &a); slots.add(1, &b);
    observer.events.clear();
    slots.remove_all();
    check(observer.events.size() == 4, "remove_all observes inserted later slot");
    check(observer.events[0].kind == 'r' && observer.events[0].id == 0 &&
          observer.events[1].kind == 'a' && observer.events[1].id == 2 &&
          observer.events[2].kind == 'r' && observer.events[2].id == 1 &&
          observer.events[3].kind == 'r' && observer.events[3].id == 2, "ascending live callback order");
    observer.insert_later = nullptr;
    slots.add(1, &a);
    observer.insert_visited = &b;
    slots.remove_all();
    check(slots.get(0) == &b && slots.get(1) == &b,
          "remove_all does not revisit same or earlier slots populated by callback");
    observer.insert_visited = nullptr;
    slots.remove_all();
    observer.fail = true;
    fails([&] { slots.add(0, &a); });
    check(slots.get(0) == &a, "failed add callback does not roll back");
    fails([&] { slots.change(0, &b); });
    check(slots.get(0) == &b, "failed change callback does not roll back");
    observer.fail = false; slots.add(1, &a); observer.fail = true;
    fails([&] { slots.remove_all(); });
    check(!slots.contains(0) && slots.get(1) == &a, "failed callback stops remove_all after committed removal");
    observer.fail = false;
    fails([&] { slots.get(-1); });
    fails([&] { slots.add(3, &a); });
    fails([&] { ComponentSlots invalid(-1); });
    ComponentSlots empty(0);
    empty.remove_all();
    fails([&] { empty.get(0); });
    check(empty.contains_all({}) && !empty.contains_any({}), "empty predicate identities");
    ComponentSlots predicates(3);
    predicates.add(0, &a).add(2, &b);
    check(predicates.contains_all({0, 2, 0}), "all permits repeated indices");
    check(!predicates.contains_all({0, 1, 2}), "all detects missing component");
    check(predicates.contains_any({1, 2}), "any finds later component");
    check(!predicates.contains_any({1, 1}), "any missing components");
    check(!predicates.contains_all({1, -1}), "all stops before invalid later index");
    check(predicates.contains_any({0, 3}), "any stops before invalid later index");
    fails([&] { predicates.contains_all({0, -1}); });
    fails([&] { predicates.contains_any({1, 3}); });
    predicates.change(0, nullptr);
    check(!predicates.contains_all({0}) && !predicates.contains_any({0}), "null replacement changes predicate presence");
    std::cout << "component slot contracts passed\n";
}
