#pragma once
#include "gameplay_catalog.hpp"
#include "archive_image_set.hpp"
#include "bitmap_backend.hpp"
#include "core/gameplay/select_form_pass.hpp"
#include "core/gameplay/select_camera.hpp"
#include "core/gameplay/input_frame.hpp"
#include "core/gameplay/game_random.hpp"
#include "core/gameplay/select_building.hpp"
#include "core/gameplay/progression_tables.hpp"
#include "core/gameplay/human_drawing.hpp"
#include "core/gameplay/town_residents.hpp"
#include "core/gameplay/select_late.hpp"
#include "core/gameplay/select_guide.hpp"
#include <ostream>
#include <stdexcept>

namespace saga::desktop {
class MissionProfile;
struct TownBackgroundContext {
    // Provisional desktop viewport/scale; AppData cctor supplies VIEW_X/Y=0/53.
    std::int32_t width=600,height=640,view_x=0,view_y=53;
    float view_scale=100.0F;
    std::int32_t form_scale=100,camera_request_x=0,camera_request_y=-9999;
    BitmapCamera projection{0,1500100,-1};
    std::int32_t depth_bits=24;
    bool depth_step=true;
    bool scenery=false;
    std::int32_t app_seed=42,lib_seed=42; // Separate provisional streams at the cloud-init boundary.
    bool buildings=false,top_form=true; // Provisional standalone form-stack context.
    bool areas=false;
    bool automatic_images=false; // Provisional Seb.AutoAnimation gate; only its false branch is bound.
    std::int64_t high_score=0; // Provisional system-save slot8, independent of normal NewGame.
    bool residents=false,hero=false,foreground=false,debug=false;
    std::int32_t store_limit=0,condition_character_count=1; // Provisional persisted/gateway inputs.
    std::optional<std::int32_t> debug_rescue_count;
    bool motion=false,motion_low_aspect=false; // Selected active-motion diagnostic; aspect gate is provisional.
    std::int32_t motion_passes=0;
    bool full_update=false;
    bool physical_input=false; // Selected outer Draw prefix + control17 registration.
    bool shared_world=false; // Retained application world; legacy drawing diagnostics omit it.
    std::int32_t gameplay_seed=42; // Independent provisional gameplay RNG; never AppData/Lib.
    std::int32_t stamina_up_state=0,menu_font=10; // Provisional billing/text provider inputs.
    std::int64_t server_time=0; // Provisional fixed GetServerTime result, seconds.
    std::optional<std::filesystem::path> input_replay;
};
class TownBackground final:private core::SelectBackgroundHost,private core::SelectActiveAreaHost,private core::SelectCameraHost,private core::SelectSceneryHost,private core::SelectFloorHost,private core::SelectTownDrawingHost,private core::KingRewardAvailabilityHost,private core::SelectAreaDrawingHost,private core::TownResidentInitializationHost,private core::TownResidentGeometryHost,private core::ScaledHumanDrawingHost,private core::SelectResidentDrawingHost,private core::SelectHeroDrawingHost,private core::SelectForegroundDrawingHost,private core::SelectNoticeDrawingHost,private core::TownResidentUpdateHost,private core::SelectCloudHost {
public:
    TownBackground(ResourceArchive& tables,const GameArchiveEntry& selected,ResourceProfile,std::int32_t capacity,TownBackgroundContext,core::EventScript buildings=nullptr,std::shared_ptr<std::vector<std::uint8_t>> enabled=nullptr,const GameArchiveEntry* background=nullptr,const GameArchiveEntry* common=nullptr,const GameResourceIndex* actor_resources=nullptr,const GameArchiveEntry* stage=nullptr,std::shared_ptr<MissionProfile> world=nullptr);
    ~TownBackground();
    void draw(BitmapBackend&);
    // Exact selected state0 counter/cloud/decor/resident blocks, not a complete
    // Update: event/permission/form gates still need their persistent owners.
    void advance_motion();
    void initialize_update(ResourceArchive&,const std::filesystem::path& root);
    void advance_update(std::uint64_t frame,bool talk_pulse=false);
    bool camera_pointer(std::int32_t type,std::int32_t x,std::int32_t y,std::int32_t dx,std::int32_t dy);
    bool widget_pointer(std::int32_t type,std::int32_t x,std::int32_t y);
    void cancel_widget_pointer();
    void stage_back();
    bool quest_handoff_pending()const;
    void cancel_camera_pointer();
    std::string window_title()const;
    void report(std::ostream&)const;
    BitmapApplication& application(){return application_;}
    BitmapTextures& textures(){return textures_;}
    const TownBackgroundContext& context()const{return context_;}
private:
    void draw_stage_overlay(BitmapBackend&);
    struct Draw {std::int32_t clip,frame;float x,y;};
    core::SelectPreludeState prelude_{0,0,0,0};
    core::SelectTouchMoveState movement_{false,40.0F,40.0F,1,0};
    std::shared_ptr<core::SelectFormPassState> pass_=std::make_shared<core::SelectFormPassState>(core::SelectFormPassState{0,false,false,{}, {true,false,{}},{0,true}});
    core::SelectBuildingState building_state_{-1,prelude_.camera_x,0,movement_.drag_distance,0,-1};
    core::SelectTownButtonsState town_state_{-1,100};
    core::SelectGuideState guide_state_{std::make_shared<std::vector<std::uint8_t>>(2,std::uint8_t{0})};
    struct UpdateOwner;
    std::shared_ptr<MissionProfile> shared_world_;
    std::shared_ptr<UpdateOwner> update_owner_;
    void report_update(std::ostream&)const;
    core::SelectCameraState camera_;
    TownBackgroundContext context_;
    std::int32_t& camera_x_=prelude_.camera_x;
    std::int32_t& camera_y_=building_state_.camera_y;
    core::SelectSceneryState scenery_{std::make_shared<std::vector<core::EventInstruction>>(),nullptr,nullptr,camera_.left_extent,camera_.right_extent};
    core::SelectDecorationMotionState decoration_motion_;
    std::int32_t motion_passes_=0;
    std::int32_t cloud_view_width_=240; // AppData.VIEW_W cctor, before outer Draw refresh.
    std::vector<core::SelectAreaTouchRegion> area_pointer_regions_;
    struct Random final:core::RandomSource {
        explicit Random(std::int32_t seed):source(seed){}
        core::LegacySystemRandom source;std::uint64_t draws=0;
        std::int32_t next()override{++draws;return source.next();}
    };
    Random app_random_,lib_random_;
    core::AreaTable local_areas_;
    core::AreaTable& areas_;
    std::vector<std::optional<core::AreaProgressState>> progress_;
    std::shared_ptr<core::CameraAreas> camera_areas_;
    core::EventInstruction save_integers_;
    ResourceArchive archive_;
    ArchiveResourceManager manager_;
    ArchiveImageSet images_;
    BitmapApplication application_;
    BitmapTextures textures_;
    ReadyArchiveImage solid_;
    BitmapBackend* backend_=nullptr;
    std::uint32_t color_=0;
    std::array<std::int32_t,2> fill_{};
    std::vector<Draw> draws_;
    struct SceneryDraw {std::int32_t clip,frame;float x,y;std::int32_t layer,anchor,reverse;};
    std::vector<SceneryDraw> scenery_draws_;
    struct FloorDraw {std::int32_t frame,x,y;};
    std::vector<FloorDraw> floor_draws_;
    core::SebStaticWork floor_work_;
    std::int32_t floor_font_=0;
    std::vector<std::int32_t> fonts_;
    std::vector<std::array<std::int32_t,2>> font_requests_;
    core::BuildingTable building_records_;
    core::EventScript building_cache_,notices_=std::make_shared<std::vector<core::EventInstruction>>();
    std::shared_ptr<std::vector<std::uint8_t>> building_enabled_;
    std::vector<std::shared_ptr<core::TownBuildingView>> building_views_;
    std::shared_ptr<core::CharacterProgressionTable> characters_=std::make_shared<core::CharacterProgressionTable>();
    std::vector<std::shared_ptr<core::CharacterNewGameState>> character_states_;
    std::shared_ptr<core::KingRewardRows> king_rewards_=std::make_shared<core::KingRewardRows>();
    std::shared_ptr<core::ImageAnimationGroup> image_clocks_;
    std::shared_ptr<core::ImageAnimationValues> image_clock_values_=std::make_shared<core::ImageAnimationValues>();
    std::int32_t& town_frame_=pass_->events.loop_counter;
    std::int32_t& selected_chip_=building_state_.selected_chip;
    struct ChipDraw {std::int32_t clip,frame,image,x,y;};
    std::vector<ChipDraw> chip_draws_;
    struct Label {core::FormText text;std::int32_t x,y;};
    std::vector<Label> labels_;
    std::shared_ptr<core::SelectAreaDrawingRows> area_views_=std::make_shared<core::SelectAreaDrawingRows>();
    core::EventInstruction& area_marks_=pass_->markers.area_marks;
    std::int32_t& selected_area_=town_state_.selected_area;
    std::int32_t& form_state_=pass_->state;
    struct DrawingArchive {
        ResourceArchive archive;
        ArchiveResourceManager manager;
        ArchiveImageSet images;
        std::shared_ptr<core::ImageAnimationGroup> clocks;
        DrawingArchive(const GameArchiveEntry&,ResourceProfile);
    };
    std::unique_ptr<DrawingArchive> area_background_,common_,stage_archive_;
    std::optional<GameArchiveEntry> stage_entry_;
    struct AreaDraw {std::int32_t resource,clip;std::optional<std::int32_t> frame;std::int32_t x,y;};
    std::vector<AreaDraw> area_draws_;
    std::vector<Label> area_labels_;
    std::vector<std::array<std::int32_t,5>> area_touches_;
    std::int32_t area_color_requests_=0;
    std::shared_ptr<core::EquipmentCombatTable> resident_weapons_=std::make_shared<core::EquipmentCombatTable>();
    std::shared_ptr<core::TownResidentCharacters> resident_characters_=std::make_shared<core::TownResidentCharacters>();
    core::TownResidentInitializationState residents_;
    core::EventInstruction resident_active_range_;
    core::DecodedEventConditionState resident_conditions_;
    std::shared_ptr<core::DecodedEventConditionHost> resident_condition_host_;
    core::QuestTable local_quest_records_;
    core::QuestTable& quest_records_;
    std::int32_t resident_challenges_=0;
    std::map<std::int32_t,GameArchiveEntry> actor_entries_;
    std::map<std::int32_t,std::unique_ptr<DrawingArchive>> actor_archives_;
    core::SebStaticWork human_work_;
    struct HeroOwner;
    std::unique_ptr<HeroOwner> hero_owner_;
    core::SelectHeroDrawingState hero_drawing_state()const{return {movement_.witch_x,movement_.direction,movement_.drag_distance};}
    std::vector<std::array<std::int32_t,5>> hero_draws_;
    std::int32_t terrain_floor_y_=0;
    std::int32_t& information_frame_=building_state_.unlock_info_frames;
    std::int32_t& information_number_=building_state_.unlock_info_number;
    std::shared_ptr<std::vector<std::uint8_t>>& guides_=guide_state_.flags;
    std::vector<std::array<std::int32_t,5>> foreground_draws_;
    std::vector<core::SelectForegroundImage> foreground_images_;
    std::vector<core::SelectForegroundTouch> foreground_touches_;
    core::EventScript notice_rows_;
    std::vector<std::array<std::int32_t,6>> notice_draws_;
    std::int32_t notice_resource_id_=0;
    std::size_t notice_quads_before_=0,notice_quads_after_=0;
    std::int32_t hero_equipment_image(core::EquipmentSlot);
    const core::EquipmentCombatTable& marker_equipment(core::EquipmentMarkerKind);
    struct ResidentDraw {std::int32_t character,clip,frame,x,y;};
    std::vector<ResidentDraw> resident_draws_;
    std::vector<std::array<std::int32_t,3>> resident_touches_;
    std::vector<std::array<std::int32_t,5>> resident_labels_;
    void initialize_resident_data(ResourceArchive&,const GameResourceIndex*);
    core::RenderDrawResource* actor_resource(std::int32_t);
    std::vector<std::array<std::int32_t,4>> touches_;
    std::int32_t king_queries_=0;
    std::size_t quads_=0;
    std::shared_ptr<core::EventIntegers> normal_save_integers()override{return save_integers_;}
    std::shared_ptr<core::CameraAreas> camera_areas()override{return camera_areas_;}
    std::int32_t view_width()override;
    std::int32_t view_height()override;
    std::int32_t game_width()override;
    std::int32_t game_height()override;
    std::array<float,3> background_pixel()override;
    void background_color(std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    std::int32_t background_width()override{return game_width();}
    std::int32_t background_height()override{return game_height();}
    void fill_background(std::int32_t,std::int32_t)override;
    core::SelectBackgroundResource background_resource()override{return &manager_.drawing();}
    std::int32_t background_view_height()override{return view_height();}
    std::int32_t background_clip()override{return 0;} // RESSELECT_SEB_BG; cctor 0x11192b0.
    void draw_background(core::SelectBackgroundResource,std::int32_t,std::int32_t,float,float)override;
    std::int32_t raw_view_height()override{return view_height();}
    std::int32_t cloud_right_edge()override{ // Init +0x160; live AppData.VIEW_W, native ADD32.
        const auto bits=490U+static_cast<std::uint32_t>(cloud_view_width_);
        return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
    }
    std::int32_t scenery_clip(std::int32_t)override;
    std::int32_t app_random(std::int32_t bound)override{return core::app_random_bounded(app_random_,bound);}
    std::int32_t lib_random(std::int32_t bound)override{return core::lib_random_bounded(lib_random_,bound);}
    std::int32_t lib_random(std::int32_t low,std::int32_t high)override{return core::random_inclusive(lib_random_,low,high);}
    core::SelectBackgroundResource scenery_resource()override{return &manager_.drawing();}
    std::array<std::int32_t,4> scenery_bounds(std::int32_t,std::int32_t)override;
    void scenery_reverse(std::int32_t value)override{backend_->graphics().reverse=value;}
    void scenery_draw(core::SelectBackgroundResource,std::int32_t,std::int32_t,float,float,std::int32_t)override;
    void scenery_draw_indexed(core::SelectBackgroundResource,std::int32_t,std::int32_t,float,float,std::int32_t,std::int32_t)override;
    std::int32_t floor_view_height()override{return view_height();}
    core::SebSpriteState* floor_sprite()override;
    core::RenderImage* floor_image()override{return manager_.drawing().images->at(3);} // RESSELECT_IMG_SELECT_FLOOR.
    void floor_draw(core::SebSpriteState*,core::RenderImage*,std::int32_t,std::int32_t,std::int32_t)override;
    void floor_push_font(std::int32_t value)override{floor_font_=value;town_push_font(value);}
    core::SelectBackgroundResource gate_resource()override{return &manager_.drawing();}
    std::int32_t gate_clip()override{return 3;} // RESSELECT_SEB_GATE, AppData static+0x170.
    void gate_draw(core::SelectBackgroundResource,std::int32_t,std::int32_t,std::int32_t,float,float)override;
    std::int32_t town_count()override{return save_integers_->at(19);}
    core::EventScript town_buildings()override{return building_cache_;}
    std::shared_ptr<std::vector<core::EventObject>> town_occupants()override;
    bool town_enabled(std::int32_t index)override{return (building_enabled_->at(static_cast<std::size_t>(index))&1)!=0;}
    std::shared_ptr<core::TownBuildingView> town_building(std::int32_t index)override{return building_views_.at(static_cast<std::size_t>(index));}
    std::int32_t town_view_height()override{return view_height();}
    core::SelectBackgroundResource town_resource()override{return &manager_.drawing();}
    std::int32_t town_clip(std::int32_t field)override;
    std::int32_t town_clip_frames(std::int32_t clip)override{return manager_.animations().clips->at(static_cast<std::size_t>(clip))->max_frame;}
    void town_draw_chip(core::SelectBackgroundResource,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    void town_push_font(std::int32_t)override;
    void town_pop_font()override;
    void town_push_dim()override{backend_->graphics().push_render_mode(0,160,0);}
    void town_pop_dim()override{backend_->graphics().pop_render_mode();}
    void town_label(core::FormText,std::int32_t,std::int32_t)override;
    bool town_king_reward_available()override{++king_queries_;return core::king_reward_available(*this);}
    std::int64_t high_score()override{return context_.high_score;}
    std::shared_ptr<core::KingRewardRows> rewards()override{return king_rewards_;}
    std::int32_t town_share_count()override{throw std::logic_error("system share-count restoration is unbound");}
    bool town_share_active()override{return false;} // MyConfig..cctor 0x11743f8.
    std::int32_t town_form_state()override{return 0;} // Native fresh SetState(0).
    void town_share_label(std::int32_t,std::int32_t)override{throw std::logic_error("share label provider is unbound");}
    void town_touch(std::int32_t,bool,std::int32_t,std::int32_t)override;
    std::shared_ptr<core::SelectAreaDrawingRows> drawing_areas()override{return area_views_;}
    std::int32_t area_view_height()override{return view_height();}
    void area_push_font(std::int32_t value)override{town_push_font(value);}
    void area_draw(std::int32_t,std::int32_t,std::optional<std::int32_t>,std::int32_t,std::int32_t)override;
    std::int32_t area_clip(std::int32_t)override;
    std::int32_t area_clip_frames(std::int32_t,std::int32_t)override;
    std::int32_t area_number()override{return core::select_area_number(*this);}
    void area_label_color()override{++area_color_requests_;} // Color output belongs to town-building-text stub.
    void area_label(core::FormText text,std::int32_t x,std::int32_t y)override{area_labels_.push_back({std::move(text),x,y});}
    void area_touch(std::int32_t index,bool selected,bool locked,std::int32_t x,std::int32_t y)override;
    std::int32_t challenge_count()override{return resident_challenges_;}
    std::int32_t store_limit()override{return context_.store_limit;}
    std::shared_ptr<core::TownResidentCharacters> characters()override{return resident_characters_;}
    std::shared_ptr<core::TownResidentCharacters> living_submembers()override;
    core::EventInstruction active_range()override;
    std::array<std::int32_t,2> building_position(std::int32_t index)override{return core::town_building_position(index,view_height());}
    std::array<std::int32_t,2> area_position(std::int32_t index)override{return core::select_area_position(index,view_height());}
    bool conditions(const core::EventScript& terms)override{return core::check_event_conditions(terms,*resident_condition_host_);}
    std::int32_t resident_view_height()override{return view_height();}
    std::int32_t resident_town_capacity()override{return town_count();}
    std::int32_t resident_town_record_count()override{return static_cast<std::int32_t>(building_enabled_->size());}
    std::shared_ptr<core::CameraAreas> resident_geometry_areas()override{return camera_areas_;}
    std::shared_ptr<core::CharacterProgressionTable> human_characters()override{return characters_;}
    std::shared_ptr<core::EquipmentCombatTable> human_weapons()override{return resident_weapons_;}
    std::int32_t hero_cloth_image()override{return hero_equipment_image(core::EquipmentSlot::cloth);}
    std::int32_t hero_hat_image()override{return hero_equipment_image(core::EquipmentSlot::hat);}
    std::int32_t hero_weapon_image()override{return hero_equipment_image(core::EquipmentSlot::weapon);}
    core::RenderMatrix& human_scale_matrix()override{return backend_->matrix();}
    core::RenderDrawResource* human_character_resource()override{return actor_resource(1);}
    core::RenderDrawResource* human_resource(std::int32_t resource)override{return actor_resource(resource);}
    bool resident_top_form()override{return context_.top_form;}
    core::EventScript resident_rows()override{return residents_.residents;}
    void resident_draw_human(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    std::int32_t resident_character_flags(std::int32_t id)override{return characters_->at(static_cast<std::size_t>(id)).value().flags;}
    std::int32_t resident_arrow_clip()override{return 17;}
    std::int32_t resident_arrow_frames(std::int32_t clip)override{return town_clip_frames(clip);}
    void resident_push_font(std::int32_t font)override{town_push_font(font);}
    void resident_pop_font()override{town_pop_font();}
    void resident_touch(std::int32_t kind,std::int32_t x,std::int32_t y)override{resident_touches_.push_back({kind,x,y});}
    void resident_label_color()override{} // SAGA_STUB(town-building-text): native color+0x34, no glyph output.
    void resident_special_label(std::int32_t x,std::int32_t y)override{resident_labels_.push_back({0,x,y,4,8});}
    void resident_level_label(std::int32_t,std::int32_t,std::int32_t)override;
    void resident_attribute_label(std::int32_t,std::int32_t,std::int32_t)override;
    core::SelectBackgroundResource hero_marker_resource()override{return &manager_.drawing();}
    std::int32_t hero_view_height()override{return view_height();}
    std::int32_t hero_marker_clip()override{return 16;} // AppData cctor +0x1a4.
    void hero_target_marker(core::SelectBackgroundResource,std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    void hero_target_effect(std::int32_t,std::int32_t,std::int32_t)override;
    core::ComponentSlots* town_hero()override;
    void hero_draw(core::ComponentSlots*,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    std::int32_t foreground_view_height()override{return view_height();}
    std::int32_t foreground_view_width()override{return view_width();}
    std::int32_t foreground_capacity()override{return save_integers_->at(19);}
    std::int32_t foreground_town_count()override{return static_cast<std::int32_t>(building_enabled_->size());}
    std::array<std::int32_t,2> foreground_obstacle_position()override{return core::town_obstacle_position({camera_x_,camera_y_,camera_.left_extent,camera_.right_extent,context_.view_x,context_.view_y},*this);}
    std::shared_ptr<core::SelectAreaDrawingRows> foreground_areas()override{return area_views_;}
    core::SelectBackgroundResource foreground_resource()override{return &manager_.drawing();}
    std::int32_t foreground_clip(std::int32_t)override;
    core::SharedAnimationClip* foreground_animation(std::int32_t clip)override{return manager_.animations().clips->at(static_cast<std::size_t>(clip));}
    void foreground_draw(core::SelectBackgroundResource,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    void foreground_image(core::SelectBackgroundResource,const core::SelectForegroundImage&)override;
    void foreground_touch(const core::SelectForegroundTouch& value)override{foreground_touches_.push_back(value);} // SAGA_STUB(town-building-touch).
    void foreground_information(const core::SelectForegroundDrawingState&,std::int32_t,std::int32_t,core::SelectBackgroundOrigin)override{throw std::logic_error("town unlock-information game/script owner is unbound");}
    std::unique_ptr<core::SelectNoticeCursor> open_notice_cursor(core::EventScript)override;
    core::SelectBackgroundResource notice_resource(std::int32_t)override;
    void notice_draw(core::SelectBackgroundResource,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    float resident_hero_x()override{return movement_.witch_x;}
    std::shared_ptr<core::SelectClouds> clouds()override{return scenery_.clouds;}
    std::int32_t right_edge()override{return cloud_right_edge();}
    std::int32_t cloud_resource()override{return 14;}
    bool low_aspect()override{return context_.motion_low_aspect;}
    std::int32_t random(std::int32_t bound)override{return app_random(bound);}
};
} // namespace saga::desktop
