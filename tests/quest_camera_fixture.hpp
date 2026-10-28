#pragma once
#include "human_frame_fixture.hpp"
#include "core/gameplay/quest_camera.hpp"

struct CameraServices final:QuestCameraFactoryHost,QuestCameraInitializationWorld,QuestCameraHost,
                            QuestViewportView,QuestCameraMapWorld {
    HumanFixture& t;MapComponent& map;MapComponent* mapped=&map;
    HeroCameraFormation formation;CameraFormation* members=&formation;
    QuestCameraSystem state;QuestCameraForm form;QuestCameraForm* current=&form;
    std::int32_t pixels_width=480,pixels_height=640,view_x=0,view_y=0,touches=1;
    float view_scale=100;EventInstruction save=std::make_shared<EventIntegers>(19,0);
    EventInstruction scratch=std::make_shared<EventIntegers>(2,0);QuestCameraCellView cell_view;
    std::deque<QuestCameraComponent> components;
    std::vector<std::string> calls;std::function<void(const std::string&)> hook;
    struct Clock final:UpdateStopwatch {
        int resets=0,starts=0,stops=0;
        void reset()override{++resets;}void start()override{++starts;}void stop()override{++stops;}
    } clock;
    CameraServices(HumanFixture& fixture,MapComponent& value):t(fixture),map(value),formation(t.f),
        form{t.f.form,this,this},cell_view(*this,scratch){}
    void step(const std::string& s){calls.push_back(s);if(hook)hook(s);}
    bool registered_id(std::int32_t id)override{return t.f.registered_id(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{step("create");return t.f.create(type,id);}
    void register_entity(ComponentSlots& e,bool temporary)override{step("register");t.f.register_entity(e,temporary);}
    ContactPosition& acquire_camera_position()override{step("position");return t.f.acquire_position();}
    QuestCameraComponent& allocate_camera_component()override{step("new-camera");components.emplace_back();return components.back();}
    ComponentSlots* create_camera()override{step("create-camera");return &create_quest_camera(t.f.ids,*this);}
    QuestCameraForm* current_camera_form()override{step("form");return current;}
    CameraFormation* current_camera_members()override{step("members");return members;}
    std::int32_t camera_view_width()override{step("camera-view-width");return game_width();}
    std::int32_t camera_touch_count()override{step("touches");return touches;}
    std::int32_t quest_view_x()override{step("view-x");return view_x;}
    std::int32_t quest_view_y()override{step("view-y");return view_y;}
    std::int32_t game_width()override{step("width");return scaled_view_dimension(pixels_width,view_scale);}
    std::int32_t game_height()override{step("height");return scaled_view_dimension(pixels_height,view_scale);}
    EventInstruction camera_saved_integers()override{step("save");return save;}
    MapComponent* camera_map()override{step("map");return mapped;}
    void init(){initialize_quest_camera(state,this);}
};
