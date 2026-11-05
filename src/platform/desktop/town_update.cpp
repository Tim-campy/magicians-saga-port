// Persistent SelectForm controller binding. Game rules remain in portable kernels.
#include "town_background.hpp"
#include "mission_events.hpp"
#include "core/gameplay/quest_rewards.hpp"
#include "core/gameplay/input_frame.hpp"
#include "core/gameplay/stage_menu.hpp"
#include "core/animation/seb_sprite.hpp"
#include <iomanip>
#include <charconv>
#include <algorithm>
#include <cmath>
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
    std::filesystem::path data_root;
    std::shared_ptr<StageStaminaValues> system_longs=std::make_shared<StageStaminaValues>(13,0);
    FormStackHost* talk_manager=nullptr;
    struct Stage;
    std::shared_ptr<Stage> stage;
    bool stage_pending=false,stage_initialized=false,stage_back_requested=false;
    bool quest_handoff=false,camera_reset_requested=false;
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
        if(stage_pending)return stage_widget(type,x,y);
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
    void cancel_widget(){if(widget_capture){widget_capture.reset();++widget_cancels;}cancel_stage_widget();}
    void clear_input(){++input_clears;button_pulses=pending_physical_pulses=0;stage_back_requested=false;cancel_pointer();cancel_widget();}
    void push(EventObject,EventObject,bool)override;
    bool advance_stage();
    void report_stage(std::ostream&)const;
    bool stage_widget(std::int32_t,std::int32_t,std::int32_t);
    void cancel_stage_widget();
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
        :town(value),tables(data_tables),initial(read_game_initialization(root)),events(data_tables,scenario(root,initial),[this]{clear_input();}),data_root(root){
        if(town.context_.input_replay)replay=read_town_replay(*town.context_.input_replay);
        if(tables.selected_profile().language_folder!="English.lproj")throw std::invalid_argument("town Update font binding requires English/default skin");
        const auto data=tables.data("Town.txt");if(!data)throw std::invalid_argument("town Update requires Town.txt");
        records=std::make_shared<TownTable>(load_town_table(std::string_view(reinterpret_cast<const char*>(data->bytes.data()),data->bytes.size()),false,parse_event_decimal));
        permissions.build_enable=town.building_enabled_;
        const auto defaults=read_new_game_save_defaults(root);const auto& save=*initial.town;
        system_longs->at(1)=save.stamina;
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
struct TownBackground::UpdateOwner::Stage final:StageMenuHost,StageStaminaHost,StageMenuDrawHost {
    UpdateOwner& owner;
    StageMenuState state;
    std::shared_ptr<StageNeutralScroll> scroll;
    std::array<std::int32_t,2> labels{};
    std::uint64_t blur_updates=0,blur_ends=0,key_queries=0;
    BitmapBackend* backend=nullptr;
    std::array<std::int32_t,2> origin{};
    std::vector<StageMenuRowRegion> rows;
    std::optional<StageMenuRowRegion> row_capture;
    std::map<std::string,std::string> literals;
    std::map<std::u16string,std::u16string> translations;
    std::map<std::tuple<std::u16string,std::int32_t,std::uint32_t>,std::unique_ptr<DesktopTextBitmap>> bitmaps;
    std::map<std::uint32_t,std::unique_ptr<ReadyArchiveImage>> colors;
    std::uint64_t row_events=0,row_rejected=0,stamina_uses=0,server_queries=0;
    std::size_t text_draws=0,missing_text=0;
    float name_offset=0;
    std::int32_t name_anchor=1;
    explicit Stage(UpdateOwner& value):owner(value){}
    SharedAnimationClip& clip(DrawingArchive& archive,std::int32_t id){
        auto* value=archive.manager.animations().clips->at(static_cast<std::size_t>(id));
        if(!value||!value->sprites)throw std::invalid_argument("missing stage menu clip");return *value;
    }
    void draw_clip(DrawingArchive& archive,std::int32_t id,std::int32_t frame,std::int32_t x,std::int32_t y,std::int32_t anchor){
        // ResourceManagerExtension.DrawSebEx0x10edd2c resolves -1 before DrawSebImg.
        if(frame==-1)frame=clip(archive,id).frame;
        backend->draw_seb_image(&archive.manager.drawing(),id,frame,x+origin[0],y+origin[1],100,anchor,archive.clocks.get());
    }
    void draw_frame(DrawingArchive& archive,std::int32_t id,std::int32_t frame,std::int32_t x,std::int32_t y){backend->draw_seb_frame(&archive.manager.drawing(),id,frame,static_cast<float>(x+origin[0]),static_cast<float>(y+origin[1]));}
    DesktopTextBitmap* text_bitmap(const std::string& source,std::int32_t font,std::uint32_t color){
        auto text=decode_mission_text(source);const auto found=translations.find(text);if(found!=translations.end())text=found->second;
        // SAGA_STUB(town-stage-translation): exact text.txt entries and public ASCII font provider.
        if(std::any_of(text.begin(),text.end(),[](char16_t c){return c>127;})){++missing_text;return nullptr;}
        const auto key=std::make_tuple(text,font,color);auto entry=bitmaps.find(key);
        if(entry==bitmaps.end()){
            auto result=std::make_unique<DesktopTextBitmap>();
            rasterize_desktop_text(*result,*owner.events.font_scenario(),*owner.events.font_provider(),text,font,color,owner.town.textures_);
            entry=bitmaps.emplace(key,std::move(result)).first;
        }
        return entry->second.get();
    }
    void text(const std::string& source,float x,float y,std::int32_t anchor,std::int32_t font,std::uint32_t color){
        ++text_draws;const auto* raster=text_bitmap(source,font,color);if(!raster||!raster->ready.image)return;
        if(anchor&2)x-=raster->width*0.5F;else if(anchor&4)x-=raster->width;
        if(anchor&32)y-=static_cast<float>(raster->font_height)*0.5F;else if(anchor&64)y-=static_cast<float>(raster->font_height);
        x+=origin[0]+static_cast<float>(raster->left)*raster->scale;
        y+=origin[1]+(raster->ascent+static_cast<float>(raster->top))*raster->scale;
        backend->draw_bitmap(*raster->ready.image,x,y,raster->ready.pixels.width*raster->scale,raster->ready.pixels.height*raster->scale);
    }
    float text_width(const std::string& source,std::int32_t font){const auto* value=text_bitmap(source,font,0xffffffffU);return value?value->width:0;}
    PixelImage* solid_color(std::uint32_t color){
        auto entry=colors.find(color);if(entry==colors.end()){
            auto ready=std::make_unique<ReadyArchiveImage>();ready->pixels={1,1,{static_cast<std::uint8_t>(color>>16),static_cast<std::uint8_t>(color>>8),static_cast<std::uint8_t>(color),static_cast<std::uint8_t>(color>>24)}};
            ready->image=std::make_unique<PixelImage>();ready->image->width=ready->image->height=ready->image->standard_width=ready->image->standard_height=1;
            owner.town.textures_.bind(*ready);entry=colors.emplace(color,std::move(ready)).first;
        }
        return entry->second->image.get();
    }
    void fill(std::int32_t x,std::int32_t y,std::int32_t w,std::int32_t h,std::uint32_t color){backend->draw_bitmap(*solid_color(color),static_cast<float>(x+origin[0]),static_cast<float>(y+origin[1]),static_cast<float>(w),static_cast<float>(h));}
    void line(std::int32_t x1,std::int32_t y1,std::int32_t x2,std::int32_t y2){backend->draw_line(*solid_color(0xffe3e3e3U),static_cast<float>(x1+origin[0]),static_cast<float>(y1+origin[1]),static_cast<float>(x2+origin[0]),static_cast<float>(y2+origin[1]),1);}
    bool widget(std::int32_t type,std::int32_t x,std::int32_t y){
        if(type==3){row_capture.reset();return true;}
        if(!owner.stage_initialized||owner.quest_handoff){++row_rejected;return false;}
        if(type==0){
            row_capture.reset();for(auto it=rows.rbegin();it!=rows.rend();++it)if(input_hit_rectangle(it->bounds,x,y)){row_capture=*it;break;}
            if(!row_capture){++row_rejected;return false;}
        }else if(type!=1)throw std::invalid_argument("unsupported stage widget pointer type");
        if(!row_capture){++row_rejected;return false;}
        auto captured=*row_capture;
        const auto kind=type==0?2:input_hit_rectangle(captured.bounds,x,y)?1:3;
        const StageMenuTouch event{{kind,&captured.control},x,y};
        // Stage handler runs first, then GameView.OnTouchList0x10fd17c.
        if(stage_menu_touch(state,&event,*this)&&(kind==1||kind==2)){
            const auto low=static_cast<std::uint32_t>(captured.control.value)&0xffffU;
            const auto index=low<=0x7fffU?static_cast<std::int32_t>(low):static_cast<std::int32_t>(low)-0x10000;
            set_stage_menu_touch_value(state,static_cast<std::uint32_t>(captured.control.value),index,*this);
            if(kind==1)owner.click_key(0x100000U); // Native row options5 never take optional flag0x10 branch.
        }
        if(type==1)row_capture.reset();++row_events;return true;
    }
    void draw(BitmapBackend& output){
        backend=&output;try{draw_stage_menu(state,*scroll,*this);}catch(...){output.set_clip_rectangle(std::nullopt);backend=nullptr;throw;}backend=nullptr;
    }
    std::int32_t raw_width()override{return owner.town.context_.width;}
    std::int32_t raw_height()override{return owner.town.context_.height;}
    std::int32_t center_x()override{return (raw_width()+1)/2-origin[0];}
    void draw_scroll_bar(InputRectangle r,std::int32_t total,std::int32_t visible,std::int32_t first,std::int32_t color)override{
        // AppData.DrawScrollBar0x1113360; actual viewport/list sizes are bounded by Init.
        if(color<1)return;if(color<255)backend->set_render_mode(1,color,255-color);
        fill(r.x+1,r.y+1,r.width-2,r.height-2,0xffc9b692U);
        if(total>visible&&total>0&&visible>0){
            const auto unit=(r.height-2)*100/total;const auto height=unit*visible/100+2;
            const auto y=r.y+1+unit*first/100;const auto overflow=std::max(y-r.y-r.height+height+1,0);
            fill(r.x+1,y,r.width-2,std::max(height-overflow,1),0xffeb7a00U);
        }
        const auto right=r.x+r.width-1,bottom=r.y+r.height-1;
        line(r.x,r.y,right,r.y);
        line(right,r.y,right,bottom);
        line(right,bottom,r.x,bottom);
        line(r.x,bottom,r.x,r.y);
        if(color<255)backend->set_render_mode(0,255);
    }
    bool first_camera_touch()override{return false;} // SAGA_STUB(town-stage-input): scroll gestures remain neutral.
    void add_camera_touch(const CameraTouchRegion&)override{} // Same explicit neutral-scroll provider.
    void push_clip(InputRectangle r)override{backend->set_clip_rectangle(std::array<float,4>{static_cast<float>(r.x+origin[0]),static_cast<float>(r.y+origin[1]),static_cast<float>(r.width),static_cast<float>(r.height)});}
    void pop_clip()override{backend->set_clip_rectangle(std::nullopt);}
    void validate_row_bounds()override{static_cast<void>(seb_bounding_rect(*clip(*owner.town.stage_archive_,0).sprites,0));}
    void draw_row_background(std::int32_t x,std::int32_t y)override{draw_clip(*owner.town.stage_archive_,0,-1,x,y,17);}
    void set_row_color()override{} // Desktop text rasters carry the native white color directly.
    void draw_quest_name(const std::string& value,std::int32_t x,std::int32_t y)override{text(value,x+name_offset,static_cast<float>(y),name_anchor,owner.town.context_.menu_font,0xffffffffU);}
    void draw_level(std::int32_t value,std::int32_t x,std::int32_t y)override{text(literals.at("level_prefix")+std::to_string(value),static_cast<float>(x),static_cast<float>(y),1,owner.town.context_.menu_font,0xffffffffU);}
    void draw_completed(std::int32_t x,std::int32_t y)override{draw_frame(*owner.town.stage_archive_,1,1,x,y);}
    std::int32_t pending_max_frame()override{return clip(*owner.town.common_,15).max_frame;}
    void draw_pending(std::int32_t frame,std::int32_t x,std::int32_t y)override{draw_frame(*owner.town.common_,15,frame,x,y);}
    void add_row_touch(const StageMenuRowRegion& value)override{auto row=value;row.bounds.x+=origin[0];row.bounds.y+=origin[1];rows.push_back(row);}
    void begin_stage_draw()override{rows.clear();text_draws=missing_text=0;origin={0,0};backend->set_render_mode(0,255);}
    std::array<std::int32_t,2> draw_origin()override{return origin;}
    void set_draw_origin(std::int32_t x,std::int32_t y)override{origin={x,y};}
    void draw_heading(std::int32_t center)override{draw_frame(*owner.town.common_,66,clip(*owner.town.common_,66).frame,center,0);text(literals.at("heading"),static_cast<float>(center),8,2,owner.town.context_.menu_font,0xffffffffU);}
    void draw_selected_details(const StageMenuState&)override;
    void end_stage_draw(const StageMenuState&)override{
        // SAGA_STUB(town-stage-presentation): blur, GL surface and production bold/filter effects.
        // Counters, selection, saves and the production41 handoff still use the native Update.
    }
    void base_initialize()override{} // SAGA_STUB(town-stage-presentation): no Unity form/resource renderer.
    void set_background_white()override{} // Same presentation boundary; native value is RGB255/255/255.
    void prepare_resources()override{
        auto& backdrop=owner.town;if(!backdrop.stage_archive_){
            if(!backdrop.stage_entry_)throw std::invalid_argument("stage menu requires select_stage resource13; regenerate --town-resources");
            backdrop.stage_archive_=std::make_unique<DrawingArchive>(*backdrop.stage_entry_,backdrop.archive_.selected_profile());
            backdrop.textures_.bind(backdrop.stage_archive_->images);owner.clocks->push_back(backdrop.stage_archive_->clocks);
        }
        const auto path=owner.data_root/"stage-menu.tsv";
        if(!std::filesystem::exists(path)||std::filesystem::file_size(path)>65536)throw std::invalid_argument("stage menu requires bounded --stage-menu extraction");
        std::ifstream input(path);input.imbue(std::locale::classic());std::string tag;std::int32_t version;
        if(!(input>>tag>>version)||tag!="stage_menu"||version!=1)throw std::invalid_argument("invalid stage menu text version");
        for(const auto* key:{"heading","name_format","level_prefix","best_experience","challenges","defeated","best_money","clears","rescued","separator","stamina","negative"}){
            std::string value;if(!(input>>tag>>std::quoted(value))||tag!=key)throw std::invalid_argument("invalid stage menu text field");literals[key]=std::move(value);
        }
        input>>std::ws;if(!input.eof())throw std::invalid_argument("trailing stage menu text fields");
        std::istringstream format(literals.at("name_format"));std::array<std::string,4> fields;
        for(auto& field:fields){if(!std::getline(format,field,','))throw std::invalid_argument("short stage name TextFormat");const auto first=field.find_first_not_of(' ');field=first==std::string::npos?"":field.substr(first,field.find_last_not_of(' ')-first+1);}
        if(!format.eof()||!fields[0].empty()||!fields[2].empty()||fields[1].empty()||fields[1][0]!='+')throw std::invalid_argument("unsupported stage name TextFormat branch");
        std::istringstream offset(fields[1].substr(1));offset.imbue(std::locale::classic());if(!(offset>>name_offset)||!std::isfinite(name_offset)||name_offset<-8192||name_offset>8192)throw std::invalid_argument("invalid stage name offset");offset>>std::ws;if(!offset.eof())throw std::invalid_argument("trailing stage name offset");
        // TextFormat stores tenths before GetXY mode1 adds value/10.
        name_offset=static_cast<float>(static_cast<std::int32_t>(name_offset*10.0F))/10.0F;
        name_anchor=parse_event_decimal(fields[3]);
        if(const auto data=owner.tables.data("text.txt")){
            std::istringstream text_rows(std::string(data->bytes.begin(),data->bytes.end()));
            for(std::string line;std::getline(text_rows,line);){if(!line.empty()&&line.back()=='\r')line.pop_back();const auto tab=line.find('\t');if(tab==std::string::npos||tab==0)continue;const auto next=line.find('\t',tab+1);const auto display=line.substr(tab+1,next==std::string::npos?next:next-tab-1);if(!display.empty())translations.emplace(decode_mission_text(std::string_view(line).substr(0,tab)),decode_mission_text(display));}
        }
        row_capture.reset();
    }
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
    EventObject make_renderer()override{return std::make_shared<int>(0);} // Retained identity for the bound portable row callback.
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
    std::int32_t stamina()override{const auto low=static_cast<std::uint32_t>(owner.system_longs->at(1));return low<=0x7fffffffU?static_cast<std::int32_t>(low):-1-static_cast<std::int32_t>(0xffffffffU-low);}
    std::shared_ptr<StageStaminaValues> stamina_values()override{return owner.system_longs;}
    // SAGA_STUB(town-stage-stamina-providers): explicit billing state and fixed server seconds.
    std::int32_t stamina_limit()override{return stamina_limit_from_item_state(owner.town.context_.stamina_up_state);}
    std::int64_t server_time()override{++server_queries;return owner.town.context_.server_time;}
    void use_stamina(std::int32_t value)override{
        try{use_stage_stamina(value,*this);}catch(...){owner.town.resident_conditions_.stamina=owner.system_longs->at(1);throw;}
        owner.town.resident_conditions_.stamina=owner.system_longs->at(1);++stamina_uses;
    }
    EventObject make_stamina_dialog()override{town_unbound("stage insufficient-stamina form");}
    void push_form(const EventObject&)override{town_unbound("stage child form");}
    bool soft_label_pressed(std::int32_t id)override{return id==3&&owner.stage_back_requested;}
    EventObject make_formation()override{town_unbound("stage formation form");}
    bool pop_form()override{owner.stage_pending=false;return false;}
    void play_bgm()override{++owner.bgm_requests;} // SAGA_STUB(town-update-bgm).
    void save_game(std::int32_t,bool)override{++owner.save_requests;} // SAGA_STUB(town-update-save).
    void save_system()override{} // SAGA_STUB(town-update-save).
    bool recover_save_exception(std::exception_ptr)override{return false;}
    void reset_town_camera()override{
        // SelectForm.ResetCameraPos0x11eb40c tail-calls SetCameraPos(0,-9999).
        set_select_camera_position(owner.town.camera_,0,-9999,owner.town);owner.camera_reset_requested=true;
    }
    void change_to_quest()override{
        // SAGA_STUB(town-stage-quest-handoff): hold the ChangeCurrentForm request,
        // retaining this owner and its save/catalog/event objects for mission binding.
        owner.quest_handoff=true;
    }
    void base_set_touch_value(std::uint32_t,std::int32_t)override{town_unbound("stage base touch selector");}
    std::int32_t base_get_touch_value(std::uint32_t,std::int32_t)override{town_unbound("stage base touch selector");}
    void scroll_camera_touch(const EventObject&,std::int32_t,std::int32_t,const StageMenuTouch&)override{town_unbound("stage scroll gesture");}
    void scroll_list_touch(const EventObject& value,std::int32_t type,std::int32_t,const StageMenuTouch& event)override{if(value!=scroll)throw std::logic_error("stage scroll identity changed");touch_stage_scroll_list(*scroll,type,&event);}
    void base_resume()override{} // Native MyFormBase.Resume writes no fields.
    void base_finish()override{} // SAGA_STUB(town-stage-touch-effects): native TouchEffectManager.RemoveSurface(view).
    void end_main_blur()override{++blur_ends;} // SAGA_STUB(town-stage-presentation).
};
void TownBackground::UpdateOwner::Stage::draw_selected_details(const StageMenuState& selected){
    const auto height=seb_bounding_rect(*clip(*owner.town.stage_archive_,2).sprites,0).at(3);
    auto y=raw_height()-height-1-origin[1];
    // MyFormBase.GetSoftKeyCalcPos0x11d94d0: IsLandspace_ is raw width>240,
    // not width>height. GamePad.cctor0x10f72ac initializes soft-key height46.
    if(raw_width()<=240)y=y-46+14;
    auto& archive=*owner.town.stage_archive_;
    backend->draw_seb_frame(&archive.manager.drawing(),2,clip(archive,2).frame,static_cast<float>(origin[0]+3),static_cast<float>(origin[1]+y));
    const auto& quest=owner.town.quest_records_.at(static_cast<std::size_t>(selected.entries->at(static_cast<std::size_t>(selected.selected))));
    if(!quest||!quest->record)throw std::invalid_argument("missing selected quest record");
    const auto& r=*quest->record;
    const std::array<std::int32_t,6> values{r.best_experience,r.challenges,r.defeated,r.best_money,r.clears,r.rescued};
    const std::array<const char*,6> names{"best_experience","challenges","defeated","best_money","clears","rescued"};
    constexpr auto label_color=0xff62cdffU,value_color=0xffffffffU;
    constexpr std::int32_t font=8; // Native PushFont(Jp?10:8); this host requires English.lproj.
    for(std::size_t i=0;i<values.size();++i){
        const auto column=static_cast<std::int32_t>(i/3),row=static_cast<std::int32_t>(i%3);
        const auto line=y+16*row+(row==2?4:5);
        text(literals.at(names[i]),static_cast<float>(116*column+8),static_cast<float>(line),1,font,label_color);
        auto value=std::to_string(values[i]);
        if(i==5){
            value+=literals.at("separator")+std::to_string(quest->stage.prisoner_maximum);
            const auto cost=literals.at("negative")+std::to_string(quest->need_stamina);
            const auto cost_width=text_width(cost,font),label_width=text_width(literals.at("stamina"),font);
            text(value,116.0F*column+110-label_width-cost_width,static_cast<float>(line),4,font,value_color);
            text(literals.at("stamina"),116.0F*column+114-cost_width,static_cast<float>(line),4,font,label_color);
            text(cost,116.0F*column+116,static_cast<float>(line),4,font,value_color);
        }else text(value,static_cast<float>(116*column+116),static_cast<float>(line),4,font,value_color);
    }
}
bool TownBackground::UpdateOwner::stage_widget(std::int32_t type,std::int32_t x,std::int32_t y){return stage&&stage->widget(type,x,y);}
void TownBackground::UpdateOwner::cancel_stage_widget(){if(stage)stage->row_capture.reset();}
void TownBackground::draw_stage_overlay(BitmapBackend& backend){
    if(update_owner_&&update_owner_->stage_pending&&update_owner_->stage_initialized)update_owner_->stage->draw(backend);
}
bool TownBackground::quest_handoff_pending()const{return update_owner_&&update_owner_->quest_handoff;}
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
    if(quest_handoff)return true;
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
       <<",\"scroll_key_queries\":"<<stage->key_queries<<",\"normal_flags\":"<<town.save_integers_->at(0)<<",\"saved_area\":"<<town.save_integers_->at(8)
       <<",\"saved_quest\":"<<town.save_integers_->at(9)<<",\"drawing_bound\":true"<<",\"text_draws\":"<<stage->text_draws
       <<",\"missing_text\":"<<stage->missing_text<<",\"row_events\":"<<stage->row_events<<",\"row_rejected\":"<<stage->row_rejected
       <<",\"row_captured\":"<<(stage->row_capture?"true":"false")<<",\"last_touched\":"<<(stage->scroll?stage->scroll->last_touched:-1)
       <<",\"stamina_uses\":"<<stage->stamina_uses<<",\"stamina_timer\":"<<system_longs->at(0)<<",\"stamina_timestamp\":"<<system_longs->at(5)
       <<",\"stamina_up_state\":"<<town.context_.stamina_up_state<<",\"server_time\":"<<town.context_.server_time<<",\"server_queries\":"<<stage->server_queries
       <<",\"menu_font\":"<<town.context_.menu_font<<",\"providers_provisional\":true,\"production_effects_bound\":false"
       <<",\"quest_handoff_pending\":"<<(quest_handoff?"true":"false")<<",\"camera_reset\":"<<(camera_reset_requested?"true":"false")
       <<",\"shared_owner_retained\":true,\"rows\":[";
    for(std::size_t i=0;i<stage->rows.size();++i){if(i)out<<',';const auto& r=stage->rows[i];out<<'['<<r.control.group<<','<<r.control.value<<','<<r.bounds.x<<','<<r.bounds.y<<','<<r.bounds.width<<','<<r.bounds.height<<']';}
    out<<"]}";
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
void TownBackground::cancel_widget_pointer(){if(update_owner_){if(update_owner_->stage_pending)update_owner_->stage_widget(3,0,0);update_owner_->cancel_widget();}}
void TownBackground::stage_back(){if(update_owner_&&update_owner_->stage_pending)update_owner_->stage_back_requested=true;}
void TownBackground::cancel_camera_pointer(){if(update_owner_)update_owner_->cancel_pointer();}
std::string TownBackground::window_title()const{
    if(quest_handoff_pending())return "SelectStage - quest entry retained; shared mission binding pending";
    if(update_owner_&&update_owner_->events.pending())return update_owner_->events.title();
    if(update_owner_&&update_owner_->stage_pending&&!update_owner_->stage_initialized)return "SelectStage - opening";
    if(update_owner_&&update_owner_->stage_pending)return "SelectStage - area "+std::to_string(update_owner_->stage->state.area)+" | "+
        std::to_string(update_owner_->stage->state.entries?update_owner_->stage->state.entries->size():0)+" quests | Left click selects/confirms; Backspace returns";
    return "Magician's Saga - Town | Right click walks; right drag pans; left click selects area/GO";
}
void TownBackground::report_update(std::ostream& out)const{
    const auto& owner=*update_owner_;
    out<<",\"update\":{\"neutral_input\":"<<((context_.input_replay||context_.physical_input)?"false":"true")<<",\"tick_ms\":40,\"low_aspect\":"<<(context_.motion_low_aspect?"true":"false")
       <<",\"provisional_font_context\":true,\"dispatches\":"<<owner.dispatches<<",\"calls\":"<<owner.updates<<",\"modal_dispatches\":"<<owner.modal_dispatches
       <<",\"loop_counter\":"<<town_frame_<<",\"base_update_counter\":"<<prelude_.update_counter
       <<",\"event_check_requested\":"<<(pass_->events.event_check_requested?"true":"false")<<",\"scene_resume\":"<<(pass_->markers.scene_resume?"true":"false")
       <<",\"weapon_new\":"<<(pass_->markers.weapon_new?"true":"false")<<",\"auto_save\":"<<(pass_->auto_save?"true":"false")
       <<",\"normal_talk_character\":"<<save_integers_->at(7)<<",\"system_flags\":"<<owner.system_save->at(1)<<",\"stamina\":"<<owner.system_longs->at(1)
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
