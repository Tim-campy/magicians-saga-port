#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

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
}
