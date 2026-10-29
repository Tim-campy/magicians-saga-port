#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <functional>
#include <utility>

namespace saga::core {
// Borrowed world identity; concrete initialization services belong to the host.
class InitializationWorld {public:virtual ~InitializationWorld()=default;};
class InitSystem {
public:
    virtual ~InitSystem()=default;
    virtual void init(InitializationWorld*)=0;
};
class InitList {
public:
    void append(InitSystem*);
    void replace(std::size_t,InitSystem*);
    void erase(std::size_t);
    void clear();
private:
    friend void initialize_systems(struct InitializationSchedule&,InitializationWorld*);
    std::vector<InitSystem*> entries_;
    std::uint32_t version_=0;
};
struct InitializationSchedule {
    InitList* systems=nullptr; // SystemManager +0x28.
    bool paused=false; // +0x70.
};
// SystemManager.Init 0x113c614. No enabled test; pause cleared only on success.
void initialize_systems(InitializationSchedule&,InitializationWorld*);
// Desktop bindings still use the same versioned dispatcher; every native factory slot initializes.
class CallbackInitializer final:public InitSystem {
public:
    explicit CallbackInitializer(std::function<void(InitializationWorld*)> body):body_(std::move(body)){}
    void init(InitializationWorld* world)override{body_(world);}
private:
    std::function<void(InitializationWorld*)> body_;
};
void append_gameplay_initializers(InitList&,const std::array<InitSystem*,32>&);
}
