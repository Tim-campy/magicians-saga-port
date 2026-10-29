#pragma once
#include "bitmap_mesh_frame.hpp"
#include "mission_scene.hpp"
#include "mission_humans.hpp"
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
    MissionProfile(const std::filesystem::path&,bool native,ArchiveImageSet::Decoder = {},std::optional<MissionRuntimeConfig> = std::nullopt);
    MissionProfile(const MissionProfile&)=delete;
    MissionProfile& operator=(const MissionProfile&)=delete;
    void step(); // Selected actor/flight/motion/contact/effect passes, in native relative order.
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
};
} // namespace saga::desktop
