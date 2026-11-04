// Persistent SelectForm controller binding. Game rules remain in portable kernels.
#include "town_background.hpp"
#include "mission_events.hpp"
#include "core/gameplay/quest_rewards.hpp"
#include "core/gameplay/input_frame.hpp"
#include "core/gameplay/stage_menu.hpp"
#include <charconv>
#include <fstream>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
[[noreturn]]void town_unbound(const char* what){throw std::logic_error(std::string("town path unbound: ")+what);}
struct TownReplayAction {
    enum class Kind { Camera, Pointer, PointerCancel, Widget, StageBack, Selection, Buttons, TalkPulse } kind;
    std::uint64_t frame=0;
    std::array<std::int32_t,6> camera{}; // type,x,dx,dy,pinch delta,pointer count.
    std::uint32_t mask=0;
    std::int32_t value=0;
};
std::uint32_t replay_mask(std::string_view text){
    const auto hex=text.substr(0,2)=="0x";if(hex)text.remove_prefix(2);
    std::uint32_t result=0;const auto converted=std::from_chars(text.data(),text.data()+text.size(),result,hex?16:10);
    if(text.empty()||converted.ec!=std::errc{}||converted.ptr!=text.data()+text.size())throw std::invalid_argument("town replay mask requires uint32 decimal/hex");
    return result;
}
std::vector<TownReplayAction> read_town_replay(const std::filesystem::path& path){
    if(std::filesystem::file_size(path)>1024U*1024U)throw std::length_error("town replay exceeds1MiB");
    std::ifstream input(path);input.imbue(std::locale::classic());std::vector<TownReplayAction> actions;
    for(std::string line;std::getline(input,line);){
        std::istringstream row(line);row.imbue(std::locale::classic());row>>std::ws;
        if(row.eof()||row.peek()=='#')continue;
        TownReplayAction action{};std::string frame_text,command,mask;std::uint32_t frame=0;
        if(!(row>>frame_text>>command))throw std::invalid_argument("town replay requires frame and command");
        const auto parsed=std::from_chars(frame_text.data(),frame_text.data()+frame_text.size(),frame);
        if(parsed.ec!=std::errc{}||parsed.ptr!=frame_text.data()+frame_text.size()||frame==0||frame>0x7fffffffU||(!actions.empty()&&frame<actions.back().frame)||actions.size()>=100000)
            throw std::invalid_argument("town replay requires ordered positive int32 frames");
        action.frame=frame;
        if(command=="camera"){
            action.kind=TownReplayAction::Kind::Camera;
            for(auto& value:action.camera)if(!(row>>value))throw std::invalid_argument("camera requires type,x,dx,dy,pinch,pointers int32");
            if(action.camera[5]<0)throw std::invalid_argument("negative replay pointer count");
        }else if(command=="pointer"){
            std::string type;if(!(row>>type))throw std::invalid_argument("pointer requires type or cancel");
            if(type=="cancel")action.kind=TownReplayAction::Kind::PointerCancel;
            else{
                action.kind=TownReplayAction::Kind::Pointer;
                action.camera[0]=parse_event_decimal(type);
                if(action.camera[0]!=0&&action.camera[0]!=1&&action.camera[0]!=8)throw std::invalid_argument("town pointer supports down0/up1/drag8");
                for(std::size_t i=1;i<5;++i)if(!(row>>action.camera[i]))throw std::invalid_argument("pointer requires type,x,y,dx,dy");
            }
        }else if(command=="widget"){
            action.kind=TownReplayAction::Kind::Widget;
            for(std::size_t i=0;i<3;++i)if(!(row>>action.camera[i]))throw std::invalid_argument("widget requires type,x,y");
            if(action.camera[0]!=0&&action.camera[0]!=1&&action.camera[0]!=3)throw std::invalid_argument("widget supports down0/up1/cancel3");
        }else if(command=="stage-back")action.kind=TownReplayAction::Kind::StageBack;
        else if(command=="select"){
            action.kind=TownReplayAction::Kind::Selection;
            if(!(row>>mask>>action.value))throw std::invalid_argument("select requires flags and int32 value");action.mask=replay_mask(mask);
        }else if(command=="buttons"){
            action.kind=TownReplayAction::Kind::Buttons;
            if(!(row>>mask))throw std::invalid_argument("buttons requires mask");action.mask=replay_mask(mask);
        }else if(command=="pulse")action.kind=TownReplayAction::Kind::TalkPulse;
        else throw std::invalid_argument("unknown town replay command");
        row>>std::ws;if(!row.eof())throw std::invalid_argument("unexpected town replay fields");actions.push_back(action);
    }
    if(!input.eof())throw std::invalid_argument("incomplete town replay");return actions;
}
}
struct TownBackground::UpdateOwner final:SelectPreludeHost,SelectFormPassHost,SelectMarkerHost,
    SelectNavigationHost,SelectBuildingHost,SelectTownButtonsHost,SelectGuidanceSaveHost,
    SelectReviewHost,SelectTriggerHost,SelectGuideHost,SelectNotificationHost,TownPermissionHost,SelectTouchHost,SelectWidgetHost,FormStackHost {
    TownBackground& town;
    ResourceArchive& tables;
    GameInitialization initial;
    MissionEvents events;
    FormStackHost* talk_manager=nullptr;
    struct Stage;
    std::shared_ptr<Stage> stage;
    bool stage_pending=false,stage_initialized=false,stage_back_requested=false;
    std::uint64_t stage_opened=0,stage_closed=0,stage_dispatches=0;
    TownInitializationState permissions;
    std::shared_ptr<TownTable> records;
    EventInstruction system_save=std::make_shared<EventIntegers>(24);
    std::shared_ptr<ImageAnimationValues> clocks;
    std::shared_ptr<ConditionCatalog<AreaMarkerRecord>> marker_areas=std::make_shared<ConditionCatalog<AreaMarkerRecord>>();
    std::shared_ptr<ConditionCatalog<AreaQuestState>> marker_quests=std::make_shared<ConditionCatalog<AreaQuestState>>();
    std::array<std::shared_ptr<ConditionCatalog<EquipmentMarkerState>>,3> marker_items;
    std::shared_ptr<GuideTowns> guide_towns=std::make_shared<GuideTowns>();
    TownBubbleState bubble{-1,std::make_shared<const std::string>(),0};
    std::shared_ptr<TownVoices> voice_rows;
    std::shared_ptr<TownSpeakers> speaker_rows=std::make_shared<TownSpeakers>();
    TownBubbleTouchHost bubble_touch{bubble,*this};
    SelectTouchState touch{town.camera_,town.movement_,town.selected_chip_,town.selected_area_,town.form_state_,town.context_.form_scale};
    std::int32_t pointer_count=0;
    bool pointer_captured=false;
    std::uint64_t pointer_events=0,pointer_rejected=0,pointer_cancels=0;
    std::int32_t pointer_x=0,pointer_y=0;
    std::optional<SelectAreaTouchRegion> widget_capture;
    std::uint32_t pending_physical_pulses=0;
    std::uint64_t widget_events=0,widget_rejected=0,widget_cancels=0;
    void select_value(std::uint32_t flags,std::int32_t value)override{set_select_touch_value(touch,flags,value,*this);}
    void click_key(std::uint32_t mask)override{pending_physical_pulses|=mask;}
    bool widget(std::int32_t type,std::int32_t x,std::int32_t y){
        if(!town.context_.physical_input)throw std::invalid_argument("town widget requires physical input");
        if(type==3){cancel_widget();return true;}
        if(type!=0&&type!=1)throw std::invalid_argument("unsupported town widget pointer type");
        if(!top()){++widget_rejected;return false;}
        if(type==0){
            cancel_widget();
            for(auto it=town.area_pointer_regions_.rbegin();it!=town.area_pointer_regions_.rend();++it)
                if(input_hit_rectangle(it->bounds,x,y)){widget_capture=*it;break;}
            if(!widget_capture){++widget_rejected;return false;}
            InputTouch event{2,&widget_capture->control};select_widget_touch(&event,*this);++widget_events;return true;
        }
        if(!widget_capture){++widget_rejected;return false;}
        const auto hit=input_hit_rectangle(widget_capture->bounds,x,y);
        auto captured=*widget_capture;widget_capture.reset();
        InputTouch event{hit?(captured.control.group==4?5:1):3,&captured.control};
        select_widget_touch(&event,*this);++widget_events;return true;
    }
    void cancel_widget(){if(widget_capture){widget_capture.reset();++widget_cancels;}}
    void clear_input(){++input_clears;button_pulses=pending_physical_pulses=0;stage_back_requested=false;cancel_pointer();cancel_widget();}
    void push(EventObject,EventObject,bool)override;
    bool advance_stage();
    void report_stage(std::ostream&)const;
    bool pointer(std::int32_t type,std::int32_t x,std::int32_t y,std::int32_t dx,std::int32_t dy){
        if(!town.context_.physical_input)throw std::invalid_argument("town pointer requires --physical-input or --interactive");
        if(type!=0&&type!=1&&type!=8)throw std::invalid_argument("unsupported town camera pointer type");
        const auto region=camera_touch_region({0,0,town.context_.width,town.context_.height},pointer_captured);
        pointer_x=x;pointer_y=y;
        if((type==1||type==8)&&!pointer_captured){++pointer_rejected;return false;}
        if(type!=1&&(!top()||!input_hit_rectangle(region.bounds,x,y))){++pointer_rejected;return false;}
        InputControl control{17,0};const SelectFormTouch event{type,x,dx,dy,0,&control};pointer_count=type==1?0:1;
        on_select_form_touch(touch,&event,*this);
        pointer_captured=type!=1;++pointer_events;return true;
    }
    void cancel_pointer(){
        if(!pointer_captured)return;
        pointer(1,pointer_x,pointer_y,0,0);++pointer_cancels;
    }
    std::vector<TownReplayAction> replay;
    std::size_t replay_index=0;
    std::uint32_t button_pulses=0;
    std::uint64_t replay_applied=0,replay_modal_ignored=0;
    bool apply_input(std::uint64_t frame,bool pulse){
        button_pulses=0;
        while(replay_index<replay.size()&&replay[replay_index].frame<=frame){
            const auto& action=replay[replay_index++];
            if(action.kind==TownReplayAction::Kind::TalkPulse){pulse=true;++replay_applied;continue;}
            if(action.kind==TownReplayAction::Kind::Widget){widget(action.camera[0],action.camera[1],action.camera[2]);++replay_applied;continue;}
            if(action.kind==TownReplayAction::Kind::StageBack){if(!stage_pending)throw std::invalid_argument("stage-back requires selected menu");stage_back_requested=true;++replay_applied;continue;}
            if(action.kind==TownReplayAction::Kind::PointerCancel){cancel_pointer();++replay_applied;continue;}
            if(action.kind==TownReplayAction::Kind::Pointer){const auto& p=action.camera;
                pointer(p[0],p[1],p[2],p[3],p[4]);++replay_applied;continue;}
            // Deliver raw town input only to the selected parent. Closing a
            // modal clears input; it cannot replay a hidden town click later.
            if(!top()){++replay_modal_ignored;continue;}
            if(action.kind==TownReplayAction::Kind::Camera){
                const auto& values=action.camera;pointer_count=values[5];InputControl control{17,0};
                const SelectFormTouch event{values[0],values[1],values[2],values[3],values[4],&control};
                if(on_select_form_touch(touch,&event,*this))throw std::logic_error("camera event not consumed");
            }else if(action.kind==TownReplayAction::Kind::Selection)set_select_touch_value(touch,action.mask,action.value,*this);
            else button_pulses|=action.mask;
            ++replay_applied;
        }
        button_pulses|=pending_physical_pulses;pending_physical_pulses=0;return pulse;
    }
    std::shared_ptr<int> notification_owner=std::make_shared<int>(0);
    std::uint64_t dispatches=0,updates=0,modal_dispatches=0,input_clears=0,save_requests=0,bgm_requests=0,notification_passes=0;
    static MissionEventScenario scenario(const std::filesystem::path& root,const GameInitialization& seed){
        if(!seed.town)throw std::invalid_argument("town Update requires v2 --mission-initialization extraction");
        MissionEventScenario result;result.new_game_context=true;result.save_string0=decode_mission_text(seed.event_arguments);
        // Same declared desktop font context as the mission; GUIStyle equivalence is unverified.
        result.font=MissionFontScenario{mission_font_path(root),100,0,0,100.0F,100.0F,false,false};return result;
    }
    UpdateOwner(TownBackground& value,ResourceArchive& data_tables,const std::filesystem::path& root)
        :town(value),tables(data_tables),initial(read_game_initialization(root)),events(data_tables,scenario(root,initial),[this]{clear_input();}){
        if(town.context_.input_replay)replay=read_town_replay(*town.context_.input_replay);
        if(tables.selected_profile().language_folder!="English.lproj")throw std::invalid_argument("town Update font binding requires English/default skin");
        const auto data=tables.data("Town.txt");if(!data)throw std::invalid_argument("town Update requires Town.txt");
        records=std::make_shared<TownTable>(load_town_table(std::string_view(reinterpret_cast<const char*>(data->bytes.data()),data->bytes.size()),false,parse_event_decimal));
        permissions.build_enable=town.building_enabled_;
        const auto defaults=read_new_game_save_defaults(root);const auto& save=*initial.town;
        town.save_integers_->at(0)=save.normal_flags;town.save_integers_->at(7)=save.talk_character;
        town.save_integers_->at(8)=defaults.area8;town.save_integers_->at(9)=defaults.quest9;
        system_save->at(1)=save.system_flags;system_save->at(21)=save.review_coin;system_save->at(22)=save.reviewed;
        events.bind_normal_save(town.save_integers_);town.pass_->form=events.form_owner();
        talk_manager=town.pass_->form.manager;town.pass_->form.manager=this;
        events.bind_application_push([this](EventObject child){
            // FormManager._execute pins this current form until Update returns,
            // even when an earlier synchronous Push has inserted another child.
            push_select_form(town.pass_->form,std::move(child),town.pass_->auto_save,town.pass_->events.event_check_requested);
        });
        town.town_state_.resident_present_value=town.residents_.present_value;
        auto& conditions=town.resident_conditions_;
        conditions.money=defaults.money;conditions.crystals=defaults.crystals;conditions.stamina=save.stamina;
        conditions.high_score=town.context_.high_score;conditions.town_capacity=town.save_integers_->at(19);
        conditions.buildings=town.building_cache_;
        conditions.town_flags=std::make_shared<ConditionCatalog<std::uint32_t>>(records->size());
        guide_towns->resize(records->size());
        for(std::size_t i=0;i<records->size();++i)if(records->at(i)){
            const auto& row=*records->at(i);
            conditions.town_flags->at(i)=std::make_shared<std::uint32_t>(static_cast<std::uint32_t>(row.flags));
            guide_towns->at(i)=std::make_shared<GuideTownRecord>(GuideTownRecord{row.flags,row.id});
        }
        conditions.characters=std::make_shared<ConditionCatalog<CharacterConditionState>>(town.characters_->size());
        speaker_rows->resize(town.characters_->size());
        for(std::size_t i=0;i<town.characters_->size();++i)if(town.characters_->at(i)){
            const auto& row=*town.characters_->at(i);
            conditions.characters->at(i)=std::make_shared<CharacterConditionState>(CharacterConditionState{town.character_states_[i]->recruited,static_cast<std::uint8_t>(row.flags)});
            speaker_rows->at(i)=std::make_shared<TownSpeaker>();
        }
        marker_areas->resize(town.areas_.size());
        for(std::size_t i=0;i<town.areas_.size();++i)if(town.areas_[i])marker_areas->at(i)=std::make_shared<AreaMarkerRecord>(AreaMarkerRecord{town.areas_[i]->stages});
        marker_quests->resize(town.quest_records_.size());
        for(std::size_t i=0;i<town.quest_records_.size();++i)if(town.quest_records_[i]){
            const auto& row=*town.quest_records_[i];marker_quests->at(i)=std::make_shared<AreaQuestState>(AreaQuestState{row.discovery_state,static_cast<std::uint8_t>(row.stage.flags),row.stage.completed!=0});
        }
        for(auto kind:{EquipmentMarkerKind::Equipment,EquipmentMarkerKind::Hat,EquipmentMarkerKind::Cloth}){
            auto& projected=marker_items[static_cast<std::size_t>(kind)];const auto& items=town.marker_equipment(kind);
            projected=std::make_shared<ConditionCatalog<EquipmentMarkerState>>(items.size());
            for(std::size_t i=0;i<items.size();++i)if(items[i]){
                EquipmentNewGameState state;reset_equipment_for_new_game(state,*items[i]);
                projected->at(i)=std::make_shared<EquipmentMarkerState>(EquipmentMarkerState{state.inventory.stock,static_cast<std::uint32_t>(items[i]->flags),state.inventory.is_new});
            }
        }
        // Retain actual row aliases in this host's explicit resource-registration
        // order; no Update callback changes dictionary membership during a pass.
        clocks=town.image_clock_values_;
    }
    bool top()const{return !events.pending()&&!stage_pending;}
    std::int32_t game_width()override{return town.game_width();}
    std::int32_t game_height()override{return town.game_height();}
    std::int32_t view_x()override{return town.context_.view_x;}
    std::int32_t saved_orientation()override{return initial.orientation;}
    bool enable_landscape()override{town_unbound("automatic landscape configuration");}
    void set_orientation(std::int32_t)override{town_unbound("automatic landscape configuration");}
    std::shared_ptr<ImageAnimationValues> image_animation_values()override{return clocks;}
    bool capture_input(std::uint32_t mask)override{return (button_pulses&mask)!=0;}
    bool pressed(std::uint32_t mask)override{return (button_pulses&mask)!=0;}
    bool soft_label(std::int32_t)override{return false;}
    void change_state(std::int32_t)override{town_unbound("capture-state labels");}
    void finish_capture()override{town_unbound("capture finish");}
    void capture()override{town_unbound("capture callback");}
    void play_bgm()override{++bgm_requests; /* SAGA_STUB(town-update-bgm): audio output. */}
    void update_notifications()override{update_select_notifications(*this);}
    bool acquire_balance()override{return false;} // SAGA_STUB(town-update-balance): no online request in flight.
    void set_active_x()override{set_select_active_x(town.camera_,town);}
    void update_decorations()override{update_select_decorations(town.scenery_,town.decoration_motion_,town);}
    void update_town_humans()override{update_town_residents(town);}
    std::int32_t town_enable_count()override{return town.save_integers_->at(19);}
    bool update_town_enable(std::int32_t index)override{return update_town_permission(permissions,index,*this);}
    void check_guide_arrow(bool force)override{check_select_guide(town.guide_state_,force,*this);}
    std::shared_ptr<TownTable> town_records()override{return records;}
    EventInstruction active_range()override{return town.active_range();}
    std::int32_t touch_count()override{return pointer_count;}
    void set_camera_position(std::int32_t x,std::int32_t y)override{set_select_camera_position(town.camera_,x,y,town);}
    void set_scale(std::int32_t)override{town_unbound("pinch SetScale lifecycle");}
    void base_set_touch_value(std::uint32_t,std::int32_t)override{town_unbound("base form touch-value selector");}
    std::int32_t base_get_touch_value(std::uint32_t,std::int32_t)override{town_unbound("base form touch-value selector");}
    std::shared_ptr<EventConditionHost> condition_context()override{return town.resident_condition_host_;}
    bool is_top_form()override{return top();}
    bool debug_enabled()override{return town.context_.debug;}
    std::shared_ptr<ConditionCatalog<AreaMarkerRecord>> areas()override{return marker_areas;}
    std::shared_ptr<AreaQuestState> quest(std::int32_t id)override{return marker_quests->at(static_cast<std::size_t>(id));}
    std::shared_ptr<ConditionCatalog<EquipmentMarkerState>> equipment(EquipmentMarkerKind kind)override{return marker_items.at(static_cast<std::size_t>(kind));}
    std::int64_t stamina()override{return *town.resident_conditions_.stamina;}
    std::uint32_t system_flags()override{return static_cast<std::uint32_t>(system_save->at(1));}
    EventInstruction save_integers()override{return town.save_integers_;}
    EventInstruction system_save_integers()override{return system_save;}
    std::shared_ptr<BuildingSlots> building_slots()override{return town.building_cache_;}
    bool can_build(std::int32_t index)override{return (town_permission(permissions,index)&1U)!=0;}
    std::int32_t sum_quest_challenges()override{return town.resident_challenges_;}
    std::shared_ptr<GuideTowns> towns()override{return guide_towns;}
    std::shared_ptr<TownHumanRows> town_humans()override{return town.residents_.residents;}
    std::shared_ptr<TownVoices> voices()override{
        if(!voice_rows){
            const auto data=tables.data("Voice.txt");if(!data)throw std::invalid_argument("town bubble requires Voice.txt");
            voice_rows=std::make_shared<TownVoices>(load_town_voice_table(std::string_view(reinterpret_cast<const char*>(data->bytes.data()),data->bytes.size()),false,parse_event_decimal));
        }
        return voice_rows;
    }
    std::shared_ptr<TownSpeakers> speakers()override{
        for(std::size_t i=0;i<speaker_rows->size();++i)if(speaker_rows->at(i)){
            const auto& data=*town.characters_->at(i);const auto& state=*town.character_states_[i];
            *speaker_rows->at(i)={data.voice_id,state.living_address,state.recruited};
        }
        return speaker_rows;
    }
    void play_button_sound()override{ /* SAGA_STUB(town-update-button-sound): audio output. */ }
    void make_transition(std::int32_t,bool,std::uint32_t,std::function<void()>)override{town_unbound("room transition");}
    EventObject room_form()override{town_unbound("room form");}
    EventObject make_shop(std::int32_t)override{town_unbound("shop form");}
    std::shared_ptr<BuildingMenuCatalog> buildings()override{town_unbound("building menu records");}
    std::shared_ptr<std::vector<EventObject>> living_members()override{town_unbound("building resident owners");}
    std::int32_t system_share_value()override{town_unbound("share save slot");}
    void set_shop_selection(EventObject,std::int32_t)override{town_unbound("shop selection");}
    EventObject make_build_list(std::int32_t)override{town_unbound("build list form");}
    EventObject make_human_house(EventObject,std::int32_t,bool)override{town_unbound("house form");}
    EventObject make_subform(std::int32_t)override{town_unbound("subform factory");}
    EventObject make_share_window(bool)override{town_unbound("share form");}
    EventObject area_form()override;
    EventObject make_obstacle(std::int32_t)override{town_unbound("obstacle form");}
    EventObject make_store_invite()override{town_unbound("store form");}
    std::int32_t store_limit_slot()override{town_unbound("store limit slot");}
    void add_money(std::int32_t)override{town_unbound("present progression owner");}
    FormText format_present_value(std::int32_t)override{town_unbound("present text");}
    EventObject make_present_popup(FormText)override{town_unbound("present popup");}
    void save_game(bool,bool)override{++save_requests; /* SAGA_STUB(town-update-save): save envelope deferred; memory retained. */}
    void report_autosave_error(const SelectSaveError&)override{town_unbound("save error adapter");}
    void save_system()override{ /* SAGA_STUB(town-update-save): persistence output. */ }
    void show_offerwall()override{ /* SAGA_STUB(town-update-billing): external offers. */ }
    bool print_enabled()override{return false;}
    void report_offerwall_error(const SelectSaveError&)override{}
    std::int32_t random(std::int32_t bound)override{return town.lib_random(bound);}
    std::int32_t area_state(std::int32_t index)override{return town.progress_.at(static_cast<std::size_t>(index)).value().discovery_state;}
    std::int32_t reviewed_slot()override{return 22;} // AppData.cctor +0x310, ELF0x1119744..0x1119764.
    std::int32_t review_coin_slot()override{return 21;} // +0x30c.
    std::int32_t platform_code()override{return 2;} // Config.cctor0x1bed2d4, ELF0x625fb0.
    EventObject make_reviewed(bool)override{town_unbound("review form");}
    EventObject notification_manager()override{++notification_passes;return notification_owner;}
    std::int32_t value_count(EventObject)override{return 0;} // SAGA_STUB(town-update-notifications): no external producer.
    NotificationText value(EventObject,std::int32_t)override{town_unbound("notification entry");}
    bool starts_currency_sale(NotificationText)override{town_unbound("notification culture prefix");}
    EventObject billing_manager()override{town_unbound("billing manager");}
    std::shared_ptr<NotificationApplyTask> make_apply_task(NotificationText)override{town_unbound("billing task provider");}
    void add_apply_task(EventObject,std::shared_ptr<NotificationApplyTask>,bool)override{town_unbound("billing task queue");}
    void show_currency_sale(NotificationText)override{town_unbound("currency offer");}
    void show_free_movie()override{town_unbound("movie offer");}
    void remove_value(EventObject,std::int32_t)override{town_unbound("notification removal");}
    void update_apply_tasks(EventObject,bool)override{town_unbound("billing tasks");}
};
struct TownBackground::UpdateOwner::Stage final:StageMenuHost {
    UpdateOwner& owner;
    StageMenuState state;
    std::shared_ptr<StageNeutralScroll> scroll;
    std::array<std::int32_t,2> labels{};
    std::uint64_t blur_updates=0,blur_ends=0,key_queries=0;
    explicit Stage(UpdateOwner& value):owner(value){}
    void base_initialize()override{} // SAGA_STUB(town-stage-presentation): no Unity form/resource renderer.
    void set_background_white()override{} // Same presentation boundary; native value is RGB255/255/255.
    void prepare_resources()override{} // Offscreen/blur render targets stay behind the presentation boundary.
    EventInstruction save_integers()override{return owner.town.save_integers_;}
    void system_flag_on(std::uint32_t flags)override{
        const auto bits=static_cast<std::uint32_t>(owner.system_save->at(1))|flags;
        owner.system_save->at(1)=bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
    }
    std::shared_ptr<AreaTable> areas()override{return {owner.town.update_owner_,&owner.town.areas_};}
    std::shared_ptr<QuestTable> quests()override{return {owner.town.update_owner_,&owner.town.quest_records_};}
    bool debug()override{return owner.town.context_.debug;}
    void set_soft_labels(std::int32_t left,std::int32_t right)override{labels={left,right};}
    EventObject make_scroll(const StageMenuScrollParameters& spec)override{
        const auto count=spec.integers[2];
        if(spec.integers!=std::array<std::int32_t,8>{1,3,count,count,40,5,0x20000,0}||!spec.option||spec.listener)
            town_unbound("non-fresh stage scroll configuration");
        scroll=std::make_shared<StageNeutralScroll>();scroll->count=count;return scroll;
    }
    void scroll_element_size(const EventObject& value,std::int32_t width,std::int32_t height)override{
        if(value!=scroll||width!=210||height!=40)town_unbound("stage scroll dimensions");
    }
    void scroll_touch_flags(const EventObject& value,std::int32_t flags)override{if(value!=scroll||flags!=17)town_unbound("stage scroll touch flags");}
    EventObject make_renderer()override{return std::make_shared<int>(0);} // Opaque retained draw delegate identity; no invocation fabricated.
    void base_update()override{update_my_form_base(state.base_update_counter,owner);}
    void reset_image_clocks()override{reset_image_animations(*owner.clocks);}
    void advance_image_clocks()override{advance_image_animations(*owner.clocks);}
    void update_main_blur()override{++blur_updates;} // SAGA_STUB(town-stage-presentation).
    void update_scroll(const EventObject& value,bool repeat)override{
        if(value!=scroll)throw std::logic_error("stage scroll identity changed");
        update_neutral_stage_scroll(*scroll,repeat,false,[this](std::uint32_t,bool){++key_queries;return false;});
        // SAGA_STUB(town-stage-input): neutral scroll provider; gestures/navigation are not bound.
    }
    bool pressed(std::uint32_t mask)override{return owner.pressed(mask);}
    void play_button_sound()override{owner.play_button_sound();}
    std::int32_t stamina()override{town_unbound("stage live stamina provider");}
    void use_stamina(std::int32_t)override{town_unbound("stage stamina mutation");}
    EventObject make_stamina_dialog()override{town_unbound("stage insufficient-stamina form");}
    void push_form(const EventObject&)override{town_unbound("stage child form");}
    bool soft_label_pressed(std::int32_t id)override{return id==3&&owner.stage_back_requested;}
    EventObject make_formation()override{town_unbound("stage formation form");}
    bool pop_form()override{owner.stage_pending=false;return false;}
    void play_bgm()override{++owner.bgm_requests;} // SAGA_STUB(town-update-bgm).
    void save_game(std::int32_t,bool)override{++owner.save_requests;} // SAGA_STUB(town-update-save).
    void save_system()override{} // SAGA_STUB(town-update-save).
    bool recover_save_exception(std::exception_ptr)override{return false;}
    void reset_town_camera()override{town_unbound("stage quest-entry camera reset");}
    void change_to_quest()override{town_unbound("stage retained quest transition");}
    void base_set_touch_value(std::uint32_t,std::int32_t)override{town_unbound("stage base touch selector");}
    std::int32_t base_get_touch_value(std::uint32_t,std::int32_t)override{town_unbound("stage base touch selector");}
    void scroll_camera_touch(const EventObject&,std::int32_t,std::int32_t,const StageMenuTouch&)override{town_unbound("stage scroll gesture");}
    void scroll_list_touch(const EventObject&,std::int32_t,std::int32_t,const StageMenuTouch&)override{town_unbound("stage list touch");}
    void base_resume()override{} // Native MyFormBase.Resume writes no fields.
    void base_finish()override{} // SAGA_STUB(town-stage-touch-effects): native TouchEffectManager.RemoveSurface(view).
    void end_main_blur()override{++blur_ends;} // SAGA_STUB(town-stage-presentation).
};
EventObject TownBackground::UpdateOwner::area_form(){if(!stage)stage=std::make_shared<Stage>(*this);return stage;}
void TownBackground::UpdateOwner::push(EventObject parent,EventObject child,bool option){
    if(stage&&child.get()==stage.get()){
        if(parent!=town.pass_->form.identity||option||stage_pending||events.pending())throw std::invalid_argument("unsupported stage menu push");
        stage_pending=true;stage_initialized=false;++stage_opened;cancel_pointer();cancel_widget();return;
    }
    talk_manager->push(std::move(parent),std::move(child),option);
}
bool TownBackground::UpdateOwner::advance_stage(){
    if(!stage_pending)return false;
    ++stage_dispatches;
    if(!stage_initialized){clear_input();initialize_stage_menu(stage->state,*stage);resume_stage_menu(*stage);stage_initialized=true;}
    update_stage_menu(stage->state,*stage,{1,0});stage_back_requested=false;
    if(stage_pending)return true;
    finish_stage_menu(*stage);stage_initialized=false;++stage_closed;clear_input();return false;
}
void TownBackground::UpdateOwner::report_stage(std::ostream& out)const{
    out<<",\"widgets\":{\"captured\":"<<(widget_capture?"true":"false")<<",\"events\":"<<widget_events<<",\"rejected\":"<<widget_rejected
       <<",\"cancels\":"<<widget_cancels<<",\"regions\":[";
    for(std::size_t i=0;i<town.area_pointer_regions_.size();++i){if(i)out<<',';const auto& r=town.area_pointer_regions_[i];
        out<<'['<<r.control.group<<','<<r.control.value<<','<<r.bounds.x<<','<<r.bounds.y<<','<<r.bounds.width<<','<<r.bounds.height<<']';}
    out<<"]},\"stage_menu\":";
    if(!stage){out<<"null";return;}
    out<<"{\"pending\":"<<(stage_pending?"true":"false")<<",\"initialized\":"<<(stage_initialized?"true":"false")
       <<",\"opened\":"<<stage_opened<<",\"closed\":"<<stage_closed<<",\"dispatches\":"<<stage_dispatches
       <<",\"area\":"<<stage->state.area<<",\"state\":"<<stage->state.state<<",\"frame\":"<<stage->state.frame
       <<",\"selected\":"<<stage->state.selected<<",\"production_frame\":"<<stage->state.production_frame
       <<",\"base_update_counter\":"<<stage->state.base_update_counter<<",\"entries\":[";
    if(stage->state.entries)for(std::size_t i=0;i<stage->state.entries->size();++i){if(i)out<<',';out<<stage->state.entries->at(i);}
    out<<"],\"scroll_color\":"<<(stage->scroll?stage->scroll->color:255)<<",\"scroll_fade\":"<<(stage->scroll?stage->scroll->fade_progress:-10)
       <<",\"scroll_key_queries\":"<<stage->key_queries<<",\"normal_flags\":"<<town.save_integers_->at(0)<<",\"saved_area\":"<<town.save_integers_->at(8)<<'}';
}
void TownBackground::initialize_update(ResourceArchive& tables,const std::filesystem::path& root){
    if(update_owner_||!context_.foreground||context_.motion||!context_.top_form)throw std::invalid_argument("full town Update requires fresh foreground/top form and excludes selected motion");
    update_owner_=std::make_shared<UpdateOwner>(*this,tables,root);
}
void TownBackground::advance_update(std::uint64_t frame,bool pulse){
    if(!update_owner_)throw std::logic_error("town Update not initialized");auto& owner=*update_owner_;++owner.dispatches;
    pulse=owner.apply_input(frame,pulse);
    if(owner.advance_stage()){++owner.modal_dispatches;context_.top_form=false;return;}
    if(owner.events.advance(frame,40,pulse,false)){++owner.modal_dispatches;context_.top_form=false;return;}
    context_.top_form=true;
    SelectRecoveredPassHost pass(owner,guide_state_,owner,camera_,*this,owner);
    SelectUpdateBindings bindings{prelude_,owner,pass_,pass,*this,owner,owner.events.delays(),owner.events.executor(),*resident_condition_host_,movement_,owner.bubble_touch,owner,update_owner_};
    update_select_form(bindings,building_state_,owner,town_state_,owner.bubble,owner,owner,camera_,*this,owner);
    ++owner.updates;context_.top_form=owner.top();if(!context_.top_form)owner.cancel_pointer();
}
bool TownBackground::camera_pointer(std::int32_t type,std::int32_t x,std::int32_t y,std::int32_t dx,std::int32_t dy){
    if(!update_owner_)throw std::logic_error("town pointer requires full Update");return update_owner_->pointer(type,x,y,dx,dy);
}
bool TownBackground::widget_pointer(std::int32_t type,std::int32_t x,std::int32_t y){
    if(!update_owner_)throw std::logic_error("town widget requires full Update");return update_owner_->widget(type,x,y);
}
void TownBackground::cancel_widget_pointer(){if(update_owner_)update_owner_->cancel_widget();}
void TownBackground::stage_back(){if(update_owner_&&update_owner_->stage_pending)update_owner_->stage_back_requested=true;}
void TownBackground::cancel_camera_pointer(){if(update_owner_)update_owner_->cancel_pointer();}
std::string TownBackground::window_title()const{
    if(update_owner_&&update_owner_->events.pending())return update_owner_->events.title();
    if(update_owner_&&update_owner_->stage_pending&&!update_owner_->stage_initialized)return "SelectStage - opening";
    if(update_owner_&&update_owner_->stage_pending)return "SelectStage - area "+std::to_string(update_owner_->stage->state.area)+" | "+
        std::to_string(update_owner_->stage->state.entries?update_owner_->stage->state.entries->size():0)+" quests | Backspace returns; list input unbound";
    return "Magician's Saga - Town | Right click walks; right drag pans; left click selects area/GO";
}
void TownBackground::report_update(std::ostream& out)const{
    const auto& owner=*update_owner_;
    out<<",\"update\":{\"neutral_input\":"<<((context_.input_replay||context_.physical_input)?"false":"true")<<",\"tick_ms\":40,\"low_aspect\":"<<(context_.motion_low_aspect?"true":"false")
       <<",\"provisional_font_context\":true,\"dispatches\":"<<owner.dispatches<<",\"calls\":"<<owner.updates<<",\"modal_dispatches\":"<<owner.modal_dispatches
       <<",\"loop_counter\":"<<town_frame_<<",\"base_update_counter\":"<<prelude_.update_counter
       <<",\"event_check_requested\":"<<(pass_->events.event_check_requested?"true":"false")<<",\"scene_resume\":"<<(pass_->markers.scene_resume?"true":"false")
       <<",\"weapon_new\":"<<(pass_->markers.weapon_new?"true":"false")<<",\"auto_save\":"<<(pass_->auto_save?"true":"false")
       <<",\"normal_talk_character\":"<<save_integers_->at(7)<<",\"system_flags\":"<<owner.system_save->at(1)<<",\"stamina\":"<<owner.initial.town->stamina
       <<",\"platform_code\":2,\"review_coin\":"<<owner.system_save->at(21)<<",\"reviewed\":"<<owner.system_save->at(22)
       <<",\"save_requests\":"<<owner.save_requests<<",\"bgm_requests\":"<<owner.bgm_requests<<",\"notification_passes\":"<<owner.notification_passes
       <<",\"input_clears\":"<<owner.input_clears<<",\"delay_count\":"<<owner.events.delays().size()<<",\"delays\":[";
    bool first=true;for(const auto& delay:owner.events.delays()){if(!first)out<<',';first=false;out<<delay->wait;}
    out<<"],\"image_clock_ticks\":[";
    for(std::size_t i=0;i<owner.clocks->size();++i){if(i)out<<',';out<<'[';const auto& group=owner.clocks->at(i);
        for(std::size_t j=0;j<group->size();++j){if(j)out<<',';out<<group->at(j)->at(1);}out<<']';}
    out<<"],\"replay_bound\":"<<(context_.input_replay?"true":"false")<<",\"replay_applied\":"<<owner.replay_applied
       <<",\"replay_modal_ignored\":"<<owner.replay_modal_ignored<<",\"replay_remaining\":"<<(owner.replay.size()-owner.replay_index)
       <<",\"touch\":{\"enabled\":"<<(movement_.enabled?"true":"false")<<",\"dragging\":"<<(camera_.dragging?"true":"false")
       <<",\"build_drag\":"<<(owner.touch.build_drag?"true":"false")<<",\"history\":"<<owner.touch.history->size()
       <<",\"velocity\":["<<camera_.velocity_x<<','<<camera_.velocity_y<<"],\"collision\":"<<movement_.collision_id
       <<"},\"bubble\":{\"index\":"<<owner.bubble.index<<",\"frames\":"<<owner.bubble.frames<<",\"text_bytes\":"<<(owner.bubble.text?owner.bubble.text->size():0)
       <<",\"text_present\":"<<(owner.bubble.text?"true":"false")<<",\"voice_catalog_bound\":"<<(owner.voice_rows?"true":"false")<<'}';
    const auto region=camera_touch_region({0,0,context_.width,context_.height},owner.pointer_captured);
    out<<",\"pointer\":{\"captured\":"<<(owner.pointer_captured?"true":"false")<<",\"events\":"<<owner.pointer_events
       <<",\"rejected\":"<<owner.pointer_rejected<<",\"cancels\":"<<owner.pointer_cancels
       <<",\"region\":["<<region.bounds.x<<','<<region.bounds.y<<','<<region.bounds.width<<','<<region.bounds.height<<"],\"flags\":"<<region.flags
       <<",\"outer_draw_prefix\":"<<(context_.physical_input?"true":"false")<<",\"view_content_width\":"<<cloud_view_width_<<'}';
    owner.report_stage(out);owner.events.write_report(out);out<<'}';
}
} // namespace saga::desktop
