#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace saga::core {
class UpdateSystem {
public:
    virtual ~UpdateSystem()=default;
    virtual bool needs_update(std::int32_t pass)=0;
    virtual void update(std::int32_t pass)=0;
};
class UpdateStopwatch {
public:
    virtual ~UpdateStopwatch()=default;
    virtual void reset()=0;
    virtual void start()=0;
    virtual void stop()=0;
};
struct UpdateSchedule {
    std::vector<UpdateSystem*>* systems=nullptr; // Manager +0x30; borrowed live list.
    UpdateStopwatch* stopwatch=nullptr; // +0x68; required only on pass 0 with entries.
};
// SystemManager.Update 0x113c7d0. No manager-wide enabled or pause test.
void update_systems(UpdateSchedule&,std::int32_t pass);

// Factory indices in IUpdateSystem append order. Other factory entries do not update.
inline constexpr std::array<std::int32_t,26> gameplay_update_order{{
    0,2,3,4,5,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,30,31
}};
// Gates from the concrete game's 26 NeedsUpdate methods; unsupported indices throw.
bool gameplay_needs_update(std::int32_t factory_index,bool enabled,std::int32_t pass);
class GameplayUpdate final:public UpdateSystem {
public:
    GameplayUpdate(std::int32_t factory_index,const bool& enabled,std::function<void(std::int32_t)> body);
    bool needs_update(std::int32_t pass)override;
    void update(std::int32_t pass)override{body_(pass);}
private:
    std::int32_t index_;const bool& enabled_;std::function<void(std::int32_t)> body_;
};
// Host assembly helper: require every update body; never silently omit unported systems.
std::vector<UpdateSystem*> make_gameplay_update_list(const std::array<UpdateSystem*,32>& factory);
}
