#include "platform/desktop/mission_checkpoint.hpp"
#include "platform/desktop/mission_profile.hpp"
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace saga::desktop;
using namespace saga::core;
void check(bool condition){if(!condition)throw std::runtime_error("mission gauge state contract");}
struct SceneFile {
    std::filesystem::path path=std::filesystem::temp_directory_path()/"saga-mission-gauge-state.scene";
    SceneFile(){std::ofstream output(path);output<<"world 1 0 0 0 1 0 0 0 1\n";if(!output)throw std::runtime_error("cannot write gauge state fixture");}
    ~SceneFile(){std::error_code error;std::filesystem::remove(path,error);}
};
void make_image(ReadyArchiveImage& ready,std::int32_t width){
    ready.pixels.width=width;ready.pixels.height=8;
    ready.pixels.rgba.assign(static_cast<std::size_t>(width)*8*4,0);
    ready.image=std::make_unique<PixelImage>();
    ready.image->width=ready.image->standard_width=width;
    ready.image->height=ready.image->standard_height=8;
    ready.alpha_type=0;
}
struct GaugeCase {
    // Borrowed pixels, component data and resource bindings outlive the profile.
    ReadyArchiveImage wide,narrow;
    DamageParameter hp;
    DamageParameterMap parameters;
    RenderDrawResource resource;
    std::unique_ptr<MissionProfile> profile;
    ComponentSlots* actor=nullptr;
    explicit GaugeCase(const std::filesystem::path& scene){
        make_image(wide,30);make_image(narrow,3);
        // Gauge's crop starts at y6 and has two rows. Only the wide crop's
        // cell12 is opaque; a later six-cell crop must retain that suffix.
        wide.pixels.rgba[(6U*30U+12U)*4U+3U]=255;
        resource.images=std::make_shared<std::vector<RenderImage*>>(std::initializer_list<RenderImage*>{wide.image.get(),narrow.image.get()});
        hp.value=hp.maximum=100;parameters.emplace(0,&hp);
        profile=std::make_unique<MissionProfile>(scene,true);
        profile->catalog=std::make_unique<GameplayCatalog>();profile->geometry=StageGeometry{24,24,0};
        profile->view={128,128,100,0,0,0,0,0};profile->has_view=true;
        profile->global.gauges=&resource;
        profile->textures.bind(wide);profile->textures.bind(narrow);
        profile->entity_world=std::make_unique<MissionEntities>(*profile->catalog,profile->progress,35,&*profile->geometry,
            [](std::int32_t)->AnimationResource*{return nullptr;},
            [](std::string_view)->std::int32_t{throw std::logic_error("no gauge fixture SEB resource");});
        actor=&profile->entity_world->raw_entity(0);
        auto& links=profile->entity_world->acquire_parameters();links.values=&parameters;actor->add(23,&links);
        profile->mission=std::make_unique<MissionScene>(*profile->entity_world,profile->application,
            *profile->entity_world->subset({0,2}),profile->view,nullptr);
        // Establish the same current QuestForm view projection before comparing
        // either output provider; the getter itself is a live host projection.
        profile->mission->gauge_camera_host().current_camera_form();
    }
    void draw(std::int32_t image,BitmapBackend* backend=nullptr){
        profile->draw_parameter_gauge_request({actor,20,20,image,0,17,true},backend);
    }
    void sequence(BitmapBackend* backend=nullptr){draw(0,backend);draw(1,backend);}
    std::string logic_hash()const{return mission_state_digest(*profile).logic_hash;}
};
void retained_alpha_and_output_parity(const std::filesystem::path& scene){
    GaugeCase silent(scene),bitmap(scene),narrow_only(scene);
    const auto initial=silent.logic_hash();check(initial==bitmap.logic_hash()&&initial==narrow_only.logic_hash());
    BitmapBackend backend(bitmap.profile->application,bitmap.profile->global,bitmap.profile->textures,128,bitmap.profile->world);
    silent.sequence();bitmap.sequence(&backend);narrow_only.draw(1);
    const auto mixed=silent.logic_hash();check(mixed==bitmap.logic_hash());
    // Both cases end with the same transparent narrow request. Only the wide
    // request's preserved alpha tail can distinguish the reached logical state.
    check(mixed!=initial&&mixed!=narrow_only.logic_hash());
    const auto transparent=narrow_only.logic_hash();narrow_only.draw(1);check(transparent==narrow_only.logic_hash());
    silent.sequence();backend.clear_quads();bitmap.sequence(&backend);
    check(silent.logic_hash()==mixed&&bitmap.logic_hash()==mixed);
    check(silent.hp.value==100&&silent.hp.maximum==100&&bitmap.hp.value==100&&bitmap.hp.maximum==100&&narrow_only.hp.value==100&&narrow_only.hp.maximum==100);
    const auto before=mission_state_bytes(*silent.profile);
    const auto first=mission_state_digest(*silent.profile);const auto second=mission_state_digest(*silent.profile);
    check(first.hash==second.hash&&first.logic_hash==mixed&&second.logic_hash==mixed&&before==mission_state_bytes(*silent.profile));
}
}
int main(){SceneFile scene;retained_alpha_and_output_parity(scene.path);std::cout<<"mission gauge state contracts ok\n";}
