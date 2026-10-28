#pragma once
#include "core/gameplay/cell_system.hpp"
#include "core/ecs/entity_registration.hpp"
#include <deque>
#include <string>
#include <unordered_set>

// Explicit test adapters; no production event-manager or global singleton is implied.
struct CellServices final:saga::core::CellInitializationWorld,saga::core::CellVisibilityHost,saga::core::CellChangeEvents {
    using ComponentReceiver=saga::core::CellIndexHost::ComponentReceiver;
    using Receiver=saga::core::CellChangeEvents::Receiver;
    using Group=saga::core::EntitySubset;
    using Entity=saga::core::ComponentSlots;
    std::function<Group*(const std::vector<std::int32_t>&)> groups;
    saga::core::MapComponent map; saga::core::MapComponent* map_result=&map;
    saga::core::CellInitializationWorld* global_world=this;
    saga::core::CellChangeEvents* event_world=this;
    saga::core::CellSystemState frame;saga::core::CellIndexState index;
    std::vector<std::string> calls;std::function<void(const std::string&)> hook;
    std::vector<std::pair<Entity*,std::int32_t>> changes;
    std::vector<Receiver> receivers;std::vector<std::optional<saga::core::EventHashes>> hashes;
    std::int32_t game_width=0,game_height=0,camera_x=0,camera_y=0,visible_count=-1;
    saga::core::EventInstruction origin=std::make_shared<saga::core::EventIntegers>(2,0);
    std::unordered_set<Entity*> visible;std::vector<Entity*> visibility_calls;
    struct Observer final:saga::core::ComponentObserver {
        ComponentReceiver callback;bool adding;
        Observer(ComponentReceiver f,bool a):callback(std::move(f)),adding(a){}
        void added(Entity& e,std::int32_t i,saga::core::Component* c)override{if(adding)callback(&e,i,c);}
        void removed(Entity& e,std::int32_t i,saga::core::Component* c)override{if(!adding)callback(&e,i,c);}
    };
    saga::core::ComponentEvents notifications;std::deque<Observer> observers;
    explicit CellServices(std::function<Group*(const std::vector<std::int32_t>&)> g):groups(std::move(g)){
        map.width=3;map.step_x=24;map.step_z=24;map.size_width=48;
    }
    void step(const std::string& s){calls.push_back(s);if(hook)hook(s);}
    Group* cell_subset(const std::vector<std::int32_t>& all)override{step(all.size()==3?"moving-subset":"cell-subset");return groups(all);}
    saga::core::MapComponent* cell_map()override{step("map");return map_result;}
    saga::core::CellInitializationWorld* current_cell_world()override{step("global-world");return global_world;}
    saga::core::CellChangeEvents* current_cell_change_world()override{step("event-world");return event_world;}
    void subscribe_cell_added(Group& group,ComponentReceiver f)override{
        step("subscribe-added");group.notifications=&notifications;observers.emplace_back(std::move(f),true);
        notifications.subscribe_added(observers.back());
    }
    void subscribe_cell_removed(Group& group,ComponentReceiver f)override{
        step("subscribe-removed");group.notifications=&notifications;observers.emplace_back(std::move(f),false);
        notifications.subscribe_removed(observers.back());
    }
    void subscribe_cell_change(Receiver f,std::optional<saga::core::EventHashes> h)override{
        step("subscribe14");receivers.push_back(std::move(f));hashes.push_back(h);
    }
    void send_cell_change(Entity* e,std::int32_t key)override{
        changes.emplace_back(e,key);step("send14");
        const auto captured=receivers;for(const auto& f:captured)f(e,key);
    }
    void clear_cell_visibility()override{step("clear-visible");visible.clear();}
    std::int32_t cell_game_width()override{step("view-width");return game_width;}
    std::int32_t cell_game_height()override{step("view-height");return game_height;}
    saga::core::EventInstruction cell_view_origin(std::int32_t sx,std::int32_t sz)override{
        step("view-origin");return saga::core::project_cell_location(origin,camera_x,camera_y,sx,sz);
    }
    void add_cell_visibility(Entity& e)override{step("add-visible");visibility_calls.push_back(&e);visible.insert(&e);}
    std::int32_t cell_visibility_count()override{step("visible-count");return static_cast<std::int32_t>(visible.size());}
    void store_cell_visibility_count(std::int32_t count)override{step("store-count");visible_count=count;}
    void init(){saga::core::initialize_cells(frame,this);saga::core::initialize_cell_index(index,this,*this);}
};
