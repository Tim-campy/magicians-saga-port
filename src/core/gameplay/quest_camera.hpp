#pragma once
#include "cell_system.hpp"
#include "select_camera.hpp"
#include "stage_recovery.hpp"

namespace saga::core {
struct QuestCameraComponent final:Component {std::int32_t angle=0,scale=0;}; // Slot 3, fields +0x10/+0x14.
class QuestCameraFactoryHost:public EntityCreationHost {
public:
    virtual ContactPosition& acquire_camera_position()=0;
    virtual QuestCameraComponent& allocate_camera_component()=0; // New, not pooled.
};
ComponentSlots& add_quest_camera(ComponentSlots&,std::int32_t angle,std::int32_t scale,QuestCameraFactoryHost&);
ComponentSlots& create_quest_camera(EntityIdState&,QuestCameraFactoryHost&); // World.CreateCamera 0x1133520.
class QuestCameraInitializationWorld {
public:
    virtual ~QuestCameraInitializationWorld()=default;
    virtual ComponentSlots* create_camera()=0;
};
struct QuestCameraSystem {
    ComponentSlots* camera=nullptr;
    bool dirty=false,dragging=false,cursor_revision=false;
    std::int32_t pinch_scale=0;
    std::shared_ptr<std::vector<EventInstruction>> history;
    EventInstruction scale_range=std::make_shared<EventIntegers>(2,0); // Derived ctor field; base constructor is external.
};
void initialize_quest_camera(QuestCameraSystem&,QuestCameraInitializationWorld*); // CameraSystem.Init 0x1143ea4.
class QuestViewportView {
public:
    virtual ~QuestViewportView()=default;
    virtual std::int32_t game_width()=0; // Bind existing surface.GameView scaled_view_dimension.
    virtual std::int32_t game_height()=0;
};
class QuestCameraMapWorld {
public:
    virtual ~QuestCameraMapWorld()=default;
    virtual MapComponent* camera_map()=0;
};
// Live projection of the same QuestForm scale used by stage startup, not a second scale.
struct QuestCameraForm {
    StageQuestForm& state;QuestCameraMapWorld* world=nullptr;QuestViewportView* view=nullptr;
    std::int32_t x=0,y=0;bool update_static_render=false; // QuestForm +0x110/+0x114/+0x1fc.
};
class QuestFormCameraHost {
public:
    virtual ~QuestFormCameraHost()=default;
    virtual std::int32_t quest_view_x()=0; // QuestForm static VIEW_X/VIEW_Y, not BaseSystem fields.
    virtual std::int32_t quest_view_y()=0;
    virtual EventInstruction camera_saved_integers()=0; // Decoded normal-save SecureInt slots 17/18; no default values.
};
std::int32_t quest_game_width(QuestCameraForm*,QuestFormCameraHost&); // QuestForm.GetGameWidth 0x11df354.
std::int32_t quest_game_height(QuestCameraForm*,QuestFormCameraHost&);
void set_quest_cursor(QuestCameraForm*,std::int32_t x,std::int32_t y,QuestFormCameraHost&); // 0x11e1344.
void set_quest_camera_position(QuestCameraForm*,std::int32_t x,std::int32_t y,QuestFormCameraHost&); // 0x11e00a0.
class CameraFormation {
public:
    virtual ~CameraFormation()=default;
    virtual float x(std::int32_t index)=0;
    virtual float z(std::int32_t index)=0;
};
class HeroCameraFormation final:public CameraFormation {
public:
    explicit HeroCameraFormation(StageRecoveryHost& host):host_(host){}
    float x(std::int32_t i)override{return indexed_member_formation_x(i,host_);}
    float z(std::int32_t i)override{return indexed_member_formation_z(i,host_);}
private:StageRecoveryHost& host_;
};
class QuestCameraHost:public QuestFormCameraHost {
public:
    virtual QuestCameraForm* current_camera_form()=0; // Fresh BaseSystem.apdat_.frmQuest_, requiring its owner.
    virtual CameraFormation* current_camera_members()=0; // Fresh global-world MemberSystem.
    virtual std::int32_t camera_view_width()=0; // Current BaseSystem.view_.GetGameWidth.
    virtual std::int32_t camera_touch_count()=0; // Platform touch count; no Unity dependency.
};
void set_camera_cursor(float x,float y,QuestCameraHost&); // CameraSystem.SetCursor 0x11440b8.
void set_camera_position(float x,float y,QuestCameraHost&); // CameraSystem.SetCameraPosition 0x1144654.
void update_quest_camera(QuestCameraSystem&,QuestCameraHost&); // 0x1143f70; pass-zero gate remains in scheduler.
std::int32_t quest_camera_scale_min(QuestCameraHost&); // Width<=240 ->10, else75.
std::int32_t quest_camera_scale_max(); // 200.
EventInstruction quest_camera_scale_range(QuestCameraSystem&,QuestCameraHost&);
void set_quest_camera_scale(QuestCameraSystem&,std::int32_t,QuestCameraHost&); // 0x1144498.
struct QuestCameraTouch {std::int32_t type=0,dx=0,dy=0;}; // TouchEvent +0x10/+0x1c/+0x20.
void on_quest_camera_touch(QuestCameraSystem&,const QuestCameraTouch*,QuestCameraHost&); // 0x1144158.
// Concrete cell-visibility inputs from the current QuestForm and AppData scratch.
class QuestCameraCellView final:public CellVisibilityView {
public:
    QuestCameraCellView(QuestCameraHost& host,EventInstruction& scratch):host_(host),scratch_(scratch){}
    std::int32_t cell_game_width()override;
    std::int32_t cell_game_height()override;
    EventInstruction cell_view_origin(std::int32_t step_x,std::int32_t half_step_z)override;
private:QuestCameraHost& host_;EventInstruction& scratch_;
};
}
