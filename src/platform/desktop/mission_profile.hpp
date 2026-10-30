#pragma once
#include "bitmap_mesh_frame.hpp"
#include "mission_scene.hpp"
#include "mission_humans.hpp"
#include "mission_spatial.hpp"
#include "mission_events.hpp"
#include "core/ecs/update_scheduler.hpp"
#include "core/gameplay/quest_frame.hpp"
#include "core/gameplay/select_prelude.hpp"
#include <chrono>
#include <filesystem>
#include <tuple>
#include <ostream>

namespace saga::desktop {
struct SceneResource {
    ResourceArchive archive;ArchiveResourceManager manager;std::unique_ptr<ArchiveImageSet> images;
    SceneResource(const std::filesystem::path&,ResourceProfile,ArchiveImageSet::Decoder);
};
struct MissionSelection {
    std::optional<std::filesystem::path> data_root;
    std::optional<std::int32_t> quest;
};
// Shared strict scenario loader. An empty decoder decodes only images needed by Gauge22 logic.
// Scenario state remains explicit; selecting a table row does not perform Stage.Start.
class MissionProfile:private core::GaugeDrawingHost,private core::AuxiliaryInitializationWorld,private core::TownInitializationWorld,private core::SystemPauseManager,
    private core::QuestFrameHost,private core::SelectPreludeHost,private core::FreshTownHost,
    private core::MissionWorldEntryHost,private core::MissionWorldEntryTarget {
public:
    MissionProfile(const std::filesystem::path&,bool native,ArchiveImageSet::Decoder = {},std::optional<MissionRuntimeConfig> = std::nullopt,std::optional<std::filesystem::path> replay =std::nullopt,MissionSelection = {});
    MissionProfile(const MissionProfile&)=delete;
    MissionProfile& operator=(const MissionProfile&)=delete;
    bool step(); // False at an explicitly held deferred transition; no further old-mission logic.
    bool resizable_viewport()const{return offscreen_camera_.has_value();}
    bool accepts_live_viewport()const{return resizable_viewport()&&!replay_bound_;}
    void resize_viewport(std::int32_t width,std::int32_t height);
    std::uint64_t logic_steps=0,movement_steps=0;
    void write_combat_report(std::ostream&); // Diagnostic JSON fragment; not a complete-state snapshot.
    void draw_enemy_gauges(BitmapBackend&);
    void write_actor_parameters(core::ComponentSlots&,std::ostream&);
    std::size_t selected_count=0;
    BitmapApplication application,global;BitmapTextures textures;std::vector<std::unique_ptr<SceneResource>> owners;
    core::SpriteMatrix world;bool has_world=false;core::RenderQueue queue=core::make_render_queue();std::size_t commands=0;
    core::OffscreenMatrix projection;bool has_projection=false,has_gpu_depth=false;std::int32_t depth_bits=0,depth_step=0;
    std::map<std::int32_t,core::SubmissionMaterial> materials;
    std::map<std::tuple<std::size_t,std::uint32_t,std::int32_t>,std::int32_t> material_bindings;
    bool native_materials=false;
    std::unique_ptr<ResourceArchive> tables;std::unique_ptr<GameplayCatalog> catalog;MissionProgress progress;
    std::optional<std::int32_t> selected_quest;
    std::optional<core::StageGeometry> geometry;
    MissionView view{};bool has_view=false,has_slots=false,has_animation=false;std::int32_t slots=0,frames=0,automatic=0,application_update=0;
    std::optional<std::int32_t> tick_ms;
    std::optional<std::int32_t> simulate;
    bool collisions=false;
    bool enemy_gauges=false; // Explicit selected dynamic-draw binding; full World.DynamicDraw is separate.
    std::vector<core::ComponentSlots*> created;
    std::map<std::string,std::int32_t> resource_names;
    std::unique_ptr<MissionEntities> entity_world;std::unique_ptr<MissionScene> mission;
    std::optional<MissionRuntimeConfig> runtime;
    std::unique_ptr<MissionHumans> humans;
    std::unique_ptr<MissionSpatial> spatial;
    bool accepts_live_input()const{return humans&&humans->input_enabled()&&!humans->transition_pending()&&!replay_bound_;}
    bool native_stage_controls()const{return initialization_complete_&&humans&&humans->stage_input_enabled();}
    bool accepts_fast_forward()const{return quest_frame_&&quest_frame_->debug_controls;}
    bool talk_pending()const{return events_&&events_->pending();}
    bool accepts_live_gameplay_input()const{return accepts_live_input()&&!talk_pending();}
    std::string talk_title()const{return events_?events_->title():std::string{};}
private:
    std::unique_ptr<MissionEvents> events_;
    struct SavedMissionSelection {std::int32_t area8,quest9;};
    std::optional<SavedMissionSelection> saved_mission_selection_;
    struct QuestFrameConfig {bool debug_controls;std::int32_t orientation;};
    std::optional<QuestFrameConfig> quest_frame_;
    core::QuestFrameState quest_frame_state_; // SAGA_STUB(mission-form-fade-draw): timing is bound, fade overlay is not.
    std::int32_t parent_update_counter_=-1; // MyFormBase.Init +0xc8.
    std::shared_ptr<core::ImageAnimationValues> image_clocks_=std::make_shared<core::ImageAnimationValues>();
    std::uint64_t world_update_calls_=0,image_clock_resets_=0,image_clock_updates_=0;
    std::vector<std::int32_t> last_world_passes_;
    bool quest_frame_result_=true,debug_modal_=false;
    void base_update()override{core::update_my_form_base(parent_update_counter_,*this);}
    bool flag_check(std::uint32_t)override{return false;} // SAGA_STUB(mission-application-flags): Init clears flag0x100; subsequent producers are unbound.
    void flag_off(std::uint32_t)override{}
    void reset_image_clocks()override;
    void advance_image_clocks()override;
    bool config_debug_controls()override{return quest_frame_->debug_controls;}
    bool key_state(std::uint32_t mask)override{return humans->stage_frame_key(mask);}
    bool key_pulse(std::uint32_t mask)override{return humans->stage_frame_pulse(mask);}
    bool engine_key_down(std::int32_t)override{return false;} // SAGA_STUB(mission-engine-debug-keys): no Unity key source.
    void debug_show_result()override{humans->debug_show_result();}
    void debug_complete_quest()override{core::complete_stage_quest(*humans->quest());}
    void queue_debug_return()override{throw std::logic_error("mission DEBUG UI-thread town callback is unbound");}
    void push_debug_defeat(std::int32_t reason)override{humans->debug_defeat_form(reason);debug_modal_=true;}
    void push_debug_victory(std::int32_t enemy,bool completed)override{humans->debug_victory_form(enemy,completed);debug_modal_=true;}
    bool active()override{return is_top_form();}
    bool is_top_form()override{return (!events_||!events_->pending())&&!debug_modal_&&!humans->has_stage_dialog()&&!humans->transition_pending();} // SAGA_STUB(mission-form-top): other form kinds remain unbound.
    void update_world(std::int32_t pass)override;
    void change_state(std::int32_t)override{throw std::logic_error("mission native form layout transition is unbound");}
    std::int32_t saved_orientation()override{return quest_frame_->orientation;}
    bool enable_landscape()override{throw std::logic_error("mission saved landscape-unlock condition is unbound");}
    void set_orientation(std::int32_t value)override{quest_frame_->orientation=value;}
    std::int32_t game_width()override{return view.width;}
    std::int32_t view_x()override{return view.margin_x;}
    std::shared_ptr<core::ImageAnimationValues> image_animation_values()override{return image_clocks_;}
    void initialize_loaded_world();
    void register_world_initializers();
    core::MissionWorldEntryTarget* entry_world()override{return entity_world?this:nullptr;}
    core::MissionSystemBatch* make_world_systems()override;
    std::uint8_t entry_loaded_byte()override{return entity_world->player_world().loaded;}
    void add_world_systems(core::MissionSystemBatch*)override;
    void create_world_map(core::MapCreation)override;
    void init_world_systems()override;
    core::MissionSystemBatch created_system_batch_;
    bool original_world_entry_=false;
    std::vector<std::string> world_entry_order_;
    void pause_systems(bool)override;
    void construct_systems();
    bool construct_systems_=false,constructing_systems_=false,construction_complete_=false;
    std::optional<bool> construction_pause_;
    std::vector<std::int32_t> construction_order_;
    void refresh_enabled();
    bool native_stage_entry_=false;
    std::optional<BitmapCamera> offscreen_camera_;
    std::uint64_t viewport_changes_=0;
    void initialize_factory(std::int32_t,core::InitializationWorld*);
    void subscribe_auxiliary_button(std::int32_t,core::AuxiliaryInitializationWorld::Button)override;
    void subscribe_menu_orientation(std::function<void()> f)override{menu_orientation_=std::move(f);}
    void subscribe_town_return(std::function<void(const core::TownReturnEvent&)> f)override{town_return_=std::move(f);}
    core::PlayerWorldState* current_town_world()override{return &entity_world->player_world();}
    void initialize_fresh_town()override;
    std::shared_ptr<core::TownTable> town_records()override{return catalog->towns;}
    std::int32_t saved_town_count()override{return saved_town_count_.value();}
    void save_town_count(std::int32_t value)override{saved_town_count_=value;++town_count_writes_;}
    void rebuild_town()override{throw std::logic_error("town entity rebuild is unbound; fresh Init defers it");}
    core::TownInitializationState town_state_;
    std::optional<std::int32_t> saved_town_count_;
    std::uint64_t town_count_writes_=0;
    bool fresh_town_initialized_=false,fresh_hero_=false;
    std::optional<std::uint8_t> initialized_loaded_;
    core::MemberBootstrapWorld initialization_world_;
    core::InitList initializers_;
    core::InitializationSchedule initialization_{&initializers_,false};
    std::array<std::unique_ptr<core::CallbackInitializer>,32> initializer_owners_;
    std::vector<std::int32_t> initialized_order_;
    bool initialization_complete_=false,menu_visible_=true;
    core::MenuRowShapes menu_shapes_=core::initial_menu_row_shapes();
    std::function<void()> menu_orientation_;
    core::AuxiliaryInitializationWorld::Button menu_button_receiver_;
    std::function<void(const core::TownReturnEvent&)> town_return_;
    std::uint64_t submenu_touches_=0,menu_touches_=0;
    core::PixelImage* gauge_image(std::int32_t)override;
    std::int32_t gauge_view_width()override{return view.width;}
    std::int32_t gauge_view_height()override{return view.height;}
    std::shared_ptr<std::vector<float>> gauge_alpha(core::PixelImage&,std::int32_t,std::int32_t,std::int32_t,std::int32_t,
        std::shared_ptr<std::vector<float>>)override;
    core::GaugeDrawState gauge_scratch_;
    std::map<std::uint64_t,std::vector<core::GaugeCreation>> gauge_creations_; // Explicit diagnostic call timing before Update22.
    struct Timer final:core::UpdateStopwatch {
        using Clock=std::chrono::steady_clock;
        Clock::time_point started;Clock::duration elapsed{};
        void reset()override{elapsed=Clock::duration::zero();}
        void start()override{started=Clock::now();}
        void stop()override{elapsed+=Clock::now()-started;}
    } timer_;
    void bind_schedule();
    std::array<bool,32> configured_{},enabled_{};
    std::array<std::unique_ptr<core::GameplayUpdate>,32> updates_;
    std::vector<core::UpdateSystem*> update_list_;
    std::vector<std::int32_t> last_updates_;
    bool halted_=false;
    std::array<core::GameplaySystemState,32> system_states_;
    std::uint64_t pause_requests_=0;
    enum class InputKind {keys,attack,pulse,cursor,click,touch,viewport,answer};
    struct InputAction {InputKind kind;std::uint32_t value=0;std::int32_t x=0,y=0,z=0;};
    std::map<std::uint64_t,std::vector<InputAction>> replay_;
    bool replay_bound_=false;std::uint64_t replay_applied_=0;
    std::uint64_t replay_modal_ignored_=0;
    void load_replay(const std::filesystem::path&);
};
} // namespace saga::desktop
