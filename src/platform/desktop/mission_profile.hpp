#pragma once
#include "bitmap_mesh_frame.hpp"
#include "mission_scene.hpp"
#include "mission_humans.hpp"
#include "mission_spatial.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <chrono>
#include <filesystem>
#include <tuple>
#include <ostream>

namespace saga::desktop {
struct SceneResource {
    ResourceArchive archive;ArchiveResourceManager manager;std::unique_ptr<ArchiveImageSet> images;
    SceneResource(const std::filesystem::path&,ResourceProfile,ArchiveImageSet::Decoder);
};
// Shared strict scenario loader. An empty decoder prepares SEBs but performs no PNG/GPU work.
// This is a selected scene, not the original quest/map/bootstrap or full system schedule.
class MissionProfile {
public:
    MissionProfile(const std::filesystem::path&,bool native,ArchiveImageSet::Decoder = {},std::optional<MissionRuntimeConfig> = std::nullopt,std::optional<std::filesystem::path> replay =std::nullopt);
    MissionProfile(const MissionProfile&)=delete;
    MissionProfile& operator=(const MissionProfile&)=delete;
    bool step(); // False at an explicitly held deferred transition; no further old-mission logic.
    std::uint64_t logic_steps=0,movement_steps=0;
    void write_combat_report(std::ostream&); // Diagnostic JSON fragment; not a complete-state snapshot.
    void write_actor_parameters(core::ComponentSlots&,std::ostream&);
    std::size_t selected_count=0;
    BitmapApplication application,global;BitmapTextures textures;std::vector<std::unique_ptr<SceneResource>> owners;
    core::SpriteMatrix world;bool has_world=false;core::RenderQueue queue=core::make_render_queue();std::size_t commands=0;
    core::OffscreenMatrix projection;bool has_projection=false,has_gpu_depth=false;std::int32_t depth_bits=0,depth_step=0;
    std::map<std::int32_t,core::SubmissionMaterial> materials;
    std::map<std::tuple<std::size_t,std::uint32_t,std::int32_t>,std::int32_t> material_bindings;
    std::unique_ptr<ResourceArchive> tables;std::unique_ptr<GameplayCatalog> catalog;MissionProgress progress;
    std::optional<core::StageGeometry> geometry;
    MissionView view{};bool has_view=false,has_slots=false,has_animation=false;std::int32_t slots=0,frames=0,automatic=0,application_update=0;
    std::optional<std::int32_t> tick_ms;
    std::optional<std::int32_t> simulate;
    bool collisions=false;
    std::vector<core::ComponentSlots*> created;
    std::map<std::string,std::int32_t> resource_names;
    std::unique_ptr<MissionEntities> entity_world;std::unique_ptr<MissionScene> mission;
    std::optional<MissionRuntimeConfig> runtime;
    std::unique_ptr<MissionHumans> humans;
    std::unique_ptr<MissionSpatial> spatial;
    bool accepts_live_input()const{return humans&&humans->input_enabled()&&!humans->transition_pending()&&!replay_bound_;}
private:
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
    enum class InputKind {keys,attack,pulse};
    struct InputAction {InputKind kind;std::uint32_t value;};
    std::map<std::uint64_t,std::vector<InputAction>> replay_;
    bool replay_bound_=false;std::uint64_t replay_applied_=0;
    void load_replay(const std::filesystem::path&);
};
} // namespace saga::desktop
