#pragma once
#include "mission_entities.hpp"
#include <ostream>

namespace saga::desktop {
struct MissionView;
// Selected fresh map/viewport adapter. Native quest selection, touch input and save loading are separate.
class MissionSpatial final:private core::MissionMapHost,private core::MissionMapCursor,
    private core::QuestCameraInitializationWorld,private core::QuestCameraHost,
    private core::QuestViewportView,private core::QuestCameraMapWorld,private core::MemberFormationHost,
    private core::CellInitializationWorld,private core::CellVisibilityHost,private core::CellChangeEvents {
public:
    MissionSpatial(MissionEntities&,core::MapComponent&,MissionView&,core::StageQuestForm&,
                   std::shared_ptr<core::StageQuest>,core::EventInstruction chip_images);
    void advance_camera(std::int32_t pass=0,bool enabled=true);
    void advance_cells(std::int32_t pass=0,bool enabled=true);
    void advance_visibility(std::int32_t pass=0,bool enabled=true);
    core::QuestCameraHost& camera_host(){return *this;}
    core::MapComponent& map(){return map_;}
    core::EntitySubset& visible(){return visible_;}
    void report(std::ostream&);
private:
    struct Observer final:core::ComponentObserver {
        ComponentReceiver callback;bool adding;
        Observer(ComponentReceiver f,bool a):callback(std::move(f)),adding(a){}
        void added(core::ComponentSlots& e,std::int32_t i,core::Component* c)override{if(adding)callback(&e,i,c);}
        void removed(core::ComponentSlots& e,std::int32_t i,core::Component* c)override{if(!adding)callback(&e,i,c);}
    };
    // Append this projection without replacing pre-existing subset observers.
    struct Notifications final:core::ComponentObserver {
        core::EntitySubset* group=nullptr;core::ComponentObserver* previous=nullptr;core::ComponentEvents events;
        ~Notifications(){if(group&&group->notifications==this)group->notifications=previous;}
        void added(core::ComponentSlots&,std::int32_t,core::Component*)override;
        void removed(core::ComponentSlots&,std::int32_t,core::Component*)override;
        void changed(core::ComponentSlots&,std::int32_t,core::Component*,core::Component*)override;
    } notifications_;
    core::MapComponent* current_mission_map()override{return &map_;}
    core::MissionMapCursor* current_map_cursor()override{return this;}
    core::MapFactoryWorld* current_map_factory()override{return &world_.map_factory_world();}
    void set_cursor(std::int32_t x,std::int32_t y)override{core::set_quest_cursor(&form_,x,y,*this);}
    core::ComponentSlots* create_camera()override{return world_.create_camera();}
    core::QuestCameraForm* current_camera_form()override{return &form_;}
    core::CameraFormation* current_camera_members()override{return &formation_;}
    std::int32_t camera_view_width()override;
    std::int32_t camera_touch_count()override;
    std::int32_t quest_view_x()override;
    std::int32_t quest_view_y()override;
    core::EventInstruction camera_saved_integers()override{return save_;}
    std::int32_t game_width()override;
    std::int32_t game_height()override;
    core::MapComponent* camera_map()override{return &map_;}
    core::PlayerWorldState* recovery_world()override{return &world_.player_world();}
    core::EntitySubset* cell_subset(const std::vector<std::int32_t>& slots)override{return world_.subset(slots);}
    core::MapComponent* cell_map()override{return &map_;}
    core::CellInitializationWorld* current_cell_world()override{return this;}
    core::CellChangeEvents* current_cell_change_world()override{return this;}
    void subscribe_cell_added(core::EntitySubset&,ComponentReceiver)override;
    void subscribe_cell_removed(core::EntitySubset&,ComponentReceiver)override;
    void subscribe_cell_change(Receiver,std::optional<core::EventHashes>)override;
    void send_cell_change(core::ComponentSlots*,std::int32_t)override;
    std::int32_t cell_game_width()override{return cell_view_.cell_game_width();}
    std::int32_t cell_game_height()override{return cell_view_.cell_game_height();}
    core::EventInstruction cell_view_origin(std::int32_t x,std::int32_t z)override{return cell_view_.cell_view_origin(x,z);}
    void clear_cell_visibility()override{visible_.clear_entities();}
    void add_cell_visibility(core::ComponentSlots& e)override{visible_.add(&e);}
    std::int32_t cell_visibility_count()override{return static_cast<std::int32_t>(visible_.size());}
    void store_cell_visibility_count(std::int32_t count)override{visible_count_=count;}
    void bind_notifications(core::EntitySubset&);
    MissionEntities& world_;core::MapComponent& map_;MissionView& view_;
    core::HeroCameraFormation formation_;core::QuestCameraForm form_;
    core::QuestCameraSystem camera_;core::MissionMapSystem map_system_;
    core::CellSystemState cells_;core::CellIndexState index_;core::EntitySubset visible_;
    core::EventInstruction save_=std::make_shared<core::EventIntegers>(19,0);
    core::EventInstruction scratch_=std::make_shared<core::EventIntegers>(2,0);
    core::QuestCameraCellView cell_view_;
    std::deque<Observer> observers_;core::EntitySubset* observed_=nullptr;
    std::shared_ptr<const std::vector<Receiver>> cell_receivers_;
    std::uint64_t camera_steps_=0,cell_steps_=0,visibility_steps_=0,changes_=0;
    std::int32_t visible_count_=0;
};
}
