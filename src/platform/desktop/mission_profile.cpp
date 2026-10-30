#include "mission_profile.hpp"
#include "assets/jar_archive.hpp"
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <variant>
#include <cmath>
#include <charconv>
#include <set>
#include <algorithm>

namespace saga::desktop {
using namespace core;
namespace {
std::vector<std::uint8_t> read(const std::filesystem::path& path){
    std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("cannot open decoded archive");
    const auto size=in.tellg();if(size<8||size>static_cast<std::streamoff>(saga::assets::JarLimits{}.max_bytes))throw std::length_error("decoded archive size outside limits");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));in.seekg(0);in.read(reinterpret_cast<char*>(bytes.data()),size);
    if(!in)throw std::runtime_error("incomplete archive read");return bytes;
}
void end(std::istringstream& row){row>>std::ws;if(!row.eof())throw std::invalid_argument("unexpected scene row fields");}
struct SelectedBullet {std::int32_t id,damage,attribute,parent;ProjectilePoint position;};
struct SelectedEffect {std::int32_t clip,image;bool loop;ProjectilePoint position;};
struct SelectedDrop {std::int32_t type,value,scale;ProjectilePoint position;};
struct SelectedFreshHero {};
struct SelectedBubble {std::int32_t parent,data_id,type,maximum,resource,clip;ProjectilePoint initial;};
struct SelectedGauge {std::uint64_t step=0;std::int32_t parent=0;GaugeCreation args;};
}
SceneResource::SceneResource(const std::filesystem::path& path,ResourceProfile profile,ArchiveImageSet::Decoder decoder)
    :archive(read(path),std::move(profile)),manager(archive){
    if(decoder)images=std::make_unique<ArchiveImageSet>(manager,std::move(decoder));
}
void MissionProfile::subscribe_auxiliary_button(std::int32_t factory,AuxiliaryInitializationWorld::Button receiver){
    if(factory==28)menu_button_receiver_=receiver;
    mission->observe_button_event(factory,std::move(receiver));
}
void MissionProfile::refresh_enabled(){
    for(std::size_t i=0;i<enabled_.size();++i)enabled_[i]=configured_[i]&&system_states_[i].enabled;
}
void MissionProfile::pause_systems(bool value){
    if(constructing_systems_){
        // CreateSystems finishes its array before AddSystems: this explicitly fresh manager has empty lists.
        ++pause_requests_;initialization_.paused=value;mission->set_manager_paused(value);return;
    }
    if(!initialization_complete_)throw std::logic_error("stage pause requires system_init's declared loaded-world lifecycle");
    ++pause_requests_;mission->set_manager_paused(value);
    try{pause_gameplay_systems(initialization_.paused,system_states_,value);}
    catch(...){refresh_enabled();throw;}
    refresh_enabled(); // Stage3 changes the gates of systems later in this same update.
}
void MissionProfile::initialize_factory(std::int32_t id,InitializationWorld* argument){
    if(argument!=&initialization_world_)throw std::invalid_argument("foreign mission initialization world");
    initialized_order_.push_back(id);
    switch(id){
    case 0:case 3:case 4:case 5:case 6:case 10:humans->initialize_system(id,argument);break;
    case 1:case 8:case 17:case 20:case 24:spatial->initialize_system(id);break;
    case 7:initialize_town_entry(this,[](const TownReturnEvent&){throw std::logic_error("town return requires S2 progression/event1133 services");});break;
    case 27:initialize_submenu(this,[this](const InputTouch* event,ComponentSlots*){submenu_button_touch(event);++submenu_touches_;});break;
    case 28:initialize_menu(menu_shapes_,this,menu_button_receiver_,[]{throw std::logic_error("native menu orientation layout is unbound");});break;
    case 29:initialize_status_render();break;
    default:
        mission->initialize_system(id);
        if(id==25)spatial->observe_cursor_click([this](CursorPointRef point){mission->deliver_info_click(std::move(point));});
        if(id==26)humans->observe_input_touch([this](const InputTouch* event){mission->deliver_button_touch(event);});
    }
}
void MissionProfile::initialize_loaded_world(){
    entity_world->player_world().loaded=*initialized_loaded_;
    initialization_world_.state=&entity_world->player_world();
    menu_button_receiver_=[this](const InputTouch* event,ComponentSlots*){
        menu_button_touch(menu_visible_,event,[]{throw std::logic_error("native menu layout/draw-off is unbound");});++menu_touches_;
    };
    system_states_[28].on_enabled=[this]{mission->observe_button_event(28,menu_button_receiver_);};
    system_states_[28].on_disabled=[this]{mission->remove_button_observer(28);};
    system_states_[25].on_pause=[this]{mission->pause_info();};
    system_states_[27].on_enabled=[this]{enable_pause_system(system_states_[27]);};
    system_states_[27].on_disabled=[this]{disable_pause_system(system_states_[27]);};
    system_states_[30].on_enabled=[this]{mission->set_debug_enabled(true);};
    system_states_[30].on_disabled=[this]{mission->set_debug_enabled(false);};
    if(construct_systems_&&*initialized_loaded_==0){
        // Fresh entry runs the original owner: construct/add -> map -> World.Init.
        initialize_mission_world(*this);original_world_entry_=true;
    }else{
        // Explicit initialized snapshots are diagnostic inputs, not a deserialized Continue world.
        if(construct_systems_)construct_systems();
        register_world_initializers();init_world_systems();
    }
    mission->set_manager_paused(initialization_.paused); // Init clears the flag without dispatching Pause(false).
    if(humans->stage_input_enabled())humans->bind_stage_pause([this](bool value){pause_systems(value);});
    if(!native_stage_entry_)spatial->load_selected_map();
    humans->apply_initialized_scenario(); // Explicit input/population snapshot; Start replaces its native entry fields.
}
void MissionProfile::register_world_initializers(){
    std::array<InitSystem*,32> factory{};
    for(std::size_t id=0;id<factory.size();++id){
        initializer_owners_[id]=std::make_unique<CallbackInitializer>([this,id](InitializationWorld* world){initialize_factory(static_cast<std::int32_t>(id),world);});
        factory[id]=initializer_owners_[id].get();
    }
    append_gameplay_initializers(initializers_,factory);
}
MissionSystemBatch* MissionProfile::make_world_systems(){
    construct_systems();world_entry_order_.push_back("create_systems");return &created_system_batch_;
}
void MissionProfile::add_world_systems(MissionSystemBatch* batch){
    if(batch!=&created_system_batch_||!construction_complete_)throw std::invalid_argument("foreign or incomplete mission system batch");
    register_world_initializers();world_entry_order_.push_back("add_systems");
}
void MissionProfile::create_world_map(MapCreation args){
    // The existing shared ComponentFactory.map_ object is retained by every deferred owner.
    entity_world->create_initial_map(std::move(args));world_entry_order_.push_back("create_map");
}
void MissionProfile::init_world_systems(){
    initialize_systems(initialization_,&initialization_world_);initialization_complete_=true;
    if(*initialized_loaded_==0)world_entry_order_.push_back("init_world");
}
void MissionProfile::reset_character_shop_records(MissionResultScenario& result){
    result.recruit_records.emplace();
    for(const auto& row:catalog->characters)if(row){
        auto& state=initial_character_records_[row->id];reset_character_for_new_game(state,*row);
        progress.characters.emplace(row->id,SavedCharacterProgression{state.level,state.experience});
        progress.sortie_counts.emplace(row->id,state.sorties);
        result.recruit_records->emplace(row->id,RecruitmentState{state.discovery_state,state.is_new,state.recruited});
    }
    // GetDataList calls Chara before MagicShop; the intervening Area reset is separate.
    for(const auto& row:catalog->shop)if(row){
        auto& state=initial_shop_records_[row->id];reset_shop_for_new_game(state,*row,*character_shop_new_game_mask_);
        progress.shop_purchases.emplace(row->id,state.purchases);
    }
}
void MissionProfile::initialize_fresh_town(){
    core::initialize_fresh_town(town_state_,*this);fresh_town_initialized_=true;
}
void MissionProfile::construct_systems(){
    // Native CreateSystems allocation order; storage owners precede this phase, with no Init subscriptions.
    SystemConstructionWorld world{this};constructing_systems_=true;menu_visible_=false; // CLR-zeroed Menu visible flag.
    for(std::size_t id=0;id<system_states_.size();++id){
        if(id==7)town_state_.building_list=std::make_shared<std::vector<EventInstruction>>(); // Town ctor payload precedes BaseSystem.
        if(id==8)spatial->construct_camera();
        auto& state=system_states_[id];state.enabled=false;
        construct_base_system(state,&world);
        if(id==6)humans->construct_member_roster();
        mission->construct_system_payload(static_cast<std::int32_t>(id));
        set_system_enabled(state,true); // Factory's second enable is an unchanged-value no-op.
        construction_order_.push_back(static_cast<std::int32_t>(id));
    }
    construction_pause_=initialization_.paused;constructing_systems_=false;construction_complete_=true;
}
void MissionProfile::reset_image_clocks(){reset_image_animations(*image_clocks_);++image_clock_resets_;}
void MissionProfile::advance_image_clocks(){advance_image_animations(*image_clocks_);++image_clock_updates_;}
// SAGA_STUB(mission-image-clock-registration): clock kernels execute, resource animation rows are not registered yet.
void MissionProfile::update_world(std::int32_t pass){
    refresh_enabled();last_world_passes_.push_back(pass);++world_update_calls_;
    UpdateSchedule schedule{&update_list_,&timer_};update_systems(schedule,pass);
}
void MissionProfile::resize_viewport(std::int32_t width,std::int32_t height){
    if(!offscreen_camera_||!has_view)throw std::invalid_argument("viewport resize requires mission_presentation");
    const auto matrix=bitmap_projection(width,height,*offscreen_camera_);
    if(view.width!=width||view.height!=height)++viewport_changes_;
    view.width=width;view.height=height;projection=matrix;
}
void MissionProfile::bind_schedule(){
    const auto bind=[&](std::int32_t id,bool enabled,std::function<void(std::int32_t)> body){
        const auto index=static_cast<std::size_t>(id);configured_[index]=enabled;
        updates_[index]=std::make_unique<GameplayUpdate>(id,enabled_[index],[this,id,body=std::move(body)](std::int32_t pass){
            last_updates_.push_back(id);
            try{body(pass);}catch(const std::out_of_range& error){
                throw std::out_of_range("mission factory"+std::to_string(id)+" pass"+std::to_string(pass)+": "+error.what());
            }
        });
    };
    bind(0,humans&&humans->input_enabled(),[this](auto pass){humans->advance_input(pass);});
    bind(2,true,[this](auto pass){mission->advance_battle(pass);});
    bind(3,static_cast<bool>(humans),[this](auto pass){
        halted_=!humans->advance_stage(pass);
        if(halted_||humans->transition_pending())for(std::size_t i=0;i<enabled_.size();++i)if(halted_||i!=21)enabled_[i]=false;
    });
    bind(4,static_cast<bool>(humans),[this](auto pass){humans->update(pass);});
    bind(5,static_cast<bool>(humans),[this](auto pass){humans->advance_enemies(pass);});
    bind(8,static_cast<bool>(spatial),[this](auto pass){spatial->advance_camera(pass);});
    bind(9,true,[this](auto pass){mission->advance_parameter_experience(pass);});
    bind(10,static_cast<bool>(humans),[this](auto pass){humans->advance_projectiles(pass);});
    bind(11,simulate.has_value(),[this](auto pass){mission->advance_movement(pass);++movement_steps;});
    bind(12,collisions,[this](auto pass){mission->advance_collisions(pass);});
    bind(13,true,[this](auto pass){mission->advance_rotation(pass);});
    bind(14,true,[this](auto pass){mission->advance_parameters(pass);});
    bind(15,simulate.has_value(),[this](auto pass){mission->advance_emitters(pass);});
    bind(16,true,[this](auto pass){mission->advance_bubbles(pass);});
    bind(17,static_cast<bool>(spatial),[this](auto pass){spatial->advance_cells(pass);});
    bind(18,simulate.has_value(),[this](auto pass){mission->advance_modifications(pass);});
    bind(19,true,[this](auto){mission->advance_animations({automatic!=0,application_update!=0});});
    bind(20,static_cast<bool>(spatial),[this](auto pass){spatial->advance_visibility(pass);});
    bind(21,true,[this](auto){queue=mission->render();if(!mission->faults().empty())throw std::invalid_argument("entity render fault: "+mission->faults().front().message);});
    bind(22,true,[this](auto pass){
        const auto at=gauge_creations_.find(logic_steps+1);
        if(pass==0&&at!=gauge_creations_.end())for(const auto args:at->second)entity_world->gauge(args,gauge_scratch_,*this);
        mission->advance_gauges(gauge_scratch_,*this,pass);
    });
    bind(23,true,[this](auto pass){mission->advance_effects(pass);});
    bind(24,static_cast<bool>(spatial),[this](auto pass){spatial->advance_cursor(pass);});
    bind(25,true,[this](auto pass){mission->advance_info(pass);});
    bind(26,true,[this](auto pass){mission->advance_buttons(pass);});
    // Render-only profiles without runtime declare DEBUG=false; this native diagnostic body has no state effects for either value.
    bind(30,true,[this](auto pass){mission->advance_debug(runtime?runtime->debug:false,pass);});
    bind(31,simulate.has_value(),[this](auto pass){mission->advance_garbage(pass);});
    std::array<UpdateSystem*,32> factory{};
    for(std::size_t i=0;i<factory.size();++i)factory[i]=updates_[i].get();
    update_list_=make_gameplay_update_list(factory);
    last_updates_.reserve(update_list_.size());
}
void MissionProfile::load_replay(const std::filesystem::path& path){
    if(std::filesystem::file_size(path)>8U*1024U*1024U)throw std::length_error("input replay exceeds8MiB");
    std::ifstream file(path);if(!file)throw std::invalid_argument("cannot read input replay");
    std::string line;std::uint64_t previous=0,count=0;
    while(std::getline(file,line)){
        std::istringstream row(line);row.imbue(std::locale::classic());row>>std::ws;if(row.eof()||row.peek()=='#')continue;
        std::string frame,kind,value;if(!(row>>frame>>kind>>value))throw std::invalid_argument("replay requires step keys|attack|pulse value or step cursor x y");
        if(kind=="answer"){
            end(row);std::uint64_t step=0;InputAction action{InputKind::answer};
            const auto a=std::from_chars(frame.data(),frame.data()+frame.size(),step),b=std::from_chars(value.data(),value.data()+value.size(),action.x);
            if(a.ec!=std::errc{}||a.ptr!=frame.data()+frame.size()||!step||step<previous||b.ec!=std::errc{}||b.ptr!=value.data()+value.size()||++count>100000)
                throw std::invalid_argument("invalid or unordered signed dialog answer");
            replay_[step].push_back(action);previous=step;continue;
        }
        if(kind=="touch"){
            std::string group,control;if(!(row>>group>>control))throw std::invalid_argument("touch replay requires type group value");end(row);
            std::uint64_t step=0;InputAction action{InputKind::touch};
            const auto a=std::from_chars(frame.data(),frame.data()+frame.size(),step);
            const auto b=std::from_chars(value.data(),value.data()+value.size(),action.x);
            const auto c=std::from_chars(group.data(),group.data()+group.size(),action.y),d=std::from_chars(control.data(),control.data()+control.size(),action.z);
            if(a.ec!=std::errc{}||a.ptr!=frame.data()+frame.size()||!step||step<previous||
               b.ec!=std::errc{}||b.ptr!=value.data()+value.size()||c.ec!=std::errc{}||c.ptr!=group.data()+group.size()||
               d.ec!=std::errc{}||d.ptr!=control.data()+control.size()||++count>100000)throw std::invalid_argument("invalid or unordered touch replay action");
            replay_[step].push_back(action);previous=step;continue;
        }
        if(kind=="cursor"||kind=="click"||kind=="viewport"){
            std::string y;if(!(row>>y))throw std::invalid_argument("point replay requires signed world x y");end(row);
            std::uint64_t step=0;InputAction action{kind=="cursor"?InputKind::cursor:kind=="click"?InputKind::click:InputKind::viewport};
            const auto a=std::from_chars(frame.data(),frame.data()+frame.size(),step);
            const auto b=std::from_chars(value.data(),value.data()+value.size(),action.x),c=std::from_chars(y.data(),y.data()+y.size(),action.y);
            if(a.ec!=std::errc{}||a.ptr!=frame.data()+frame.size()||!step||step<previous||
               b.ec!=std::errc{}||b.ptr!=value.data()+value.size()||c.ec!=std::errc{}||c.ptr!=y.data()+y.size()||++count>100000)
                throw std::invalid_argument("invalid or unordered cursor replay action");
            replay_[step].push_back(action);previous=step;continue;
        }
        end(row);
        std::uint64_t step=0;std::uint32_t argument=0;
        const auto a=std::from_chars(frame.data(),frame.data()+frame.size(),step),b=std::from_chars(value.data(),value.data()+value.size(),argument);
        if(a.ec!=std::errc{}||a.ptr!=frame.data()+frame.size()||!step||step<previous||b.ec!=std::errc{}||b.ptr!=value.data()+value.size()||
           (kind!="keys"&&kind!="attack"&&kind!="pulse")||(kind=="attack"&&argument>2)||++count>100000)throw std::invalid_argument("invalid or unordered input replay action");
        replay_[step].push_back({kind=="keys"?InputKind::keys:kind=="attack"?InputKind::attack:InputKind::pulse,argument});previous=step;
    }replay_bound_=true;
}
MissionProfile::MissionProfile(const std::filesystem::path& scene,bool native,ArchiveImageSet::Decoder decoder,std::optional<MissionRuntimeConfig> config,std::optional<std::filesystem::path> replay,MissionSelection selection):runtime(std::move(config)){
    if(selection.quest&&*selection.quest<0)throw std::invalid_argument("quest ID must be nonnegative");
    if(selection.data_root&&!std::filesystem::is_directory(*selection.data_root/"textassets"))throw std::invalid_argument("data root requires the extractor's textassets directory");
    const auto archive_path=[&](const std::string& path){
        auto resolved=std::filesystem::u8path(path);
        if(selection.data_root){
            // Resource IDs and archive names are declared by the scenario, not guessed from a directory scan.
            // Accept Windows paths in scenarios read on Linux as well.
            const auto leaf=path.substr(path.find_last_of("/\\")+1);
            if(leaf.empty()||leaf=="."||leaf=="..")throw std::invalid_argument("archive filename required for data selection");
            resolved=*selection.data_root/"textassets"/std::filesystem::u8path(leaf);
            if(!std::filesystem::is_regular_file(resolved))throw std::invalid_argument("selected data archive missing: "+resolved.u8string());
        }else if(resolved.is_relative())resolved=scene.parent_path()/resolved;
        return resolved;
    };
    const auto load_resource=[&](std::int32_t id,const std::filesystem::path& path,const std::string& language,std::int32_t dpi){
        auto** slot=application_resource_slot(application,id);
        auto** alias=application_resource_slot(global,id);
        if(!slot||*slot||!alias||*alias)throw std::invalid_argument("mission resource index has duplicate or unsupported AppData ID");
        auto value=std::make_unique<SceneResource>(path,ResourceProfile{language,dpi},decoder);
        if(value->images)textures.bind(*value->images);
        *slot=*alias=&value->manager.drawing();owners.push_back(std::move(value));
    };
    if(replay)load_replay(*replay);
    std::ifstream input(scene);if(!input)throw std::runtime_error("cannot open scene description");
    std::vector<std::variant<HumanCreation,EnemyCreation,SelectedBullet,SelectedEffect,SelectedDrop,SelectedFreshHero>> selected;
    std::optional<MissionHumanScenario> human_scenario;bool runtime_row=false;
    MissionEventScenario event_scenario;bool event_context=false;
    std::array<bool,32> enabled_rows{};bool menu_snapshot=false;
    std::optional<MissionRewardScenario> rewards;
    std::optional<MissionNotificationScenario> notifications;
    bool collection=false,drop_catalog=false;
    EventInstruction map_images;
    std::optional<MissionInputScenario> input_scenario;
    MissionResultScenario result_state{};bool wallet=false,record=false,recruits=false;std::array<bool,3> stocks{};
    MissionStageScenario stage_scenario{};bool stage_loop=false,stage_events=false;
    MissionEnemyScenario enemy{};bool enemy_config=false,enemy_population=false,enemy_pool=false;
    std::map<std::string,std::int32_t> gear_defaults;
    std::vector<std::size_t> players;
    std::vector<std::tuple<std::size_t,std::int32_t,float>> states;
    std::vector<std::pair<std::int32_t,ButtonCreation>> buttons;
    std::vector<MissionPopupScenario> info_popups;std::vector<MissionSlideScenario> info_slides;
    std::vector<SelectedBubble> bubbles;
    std::vector<SelectedGauge> gauges;
    std::set<std::string> quest_fields;
    std::int32_t rescued_prisoners=0;
    std::optional<std::int32_t> stage_entry,stage_area_id;
    std::shared_ptr<AreaDefinition> stage_area;
    EventIntegers formation;bool has_formation=false;
    for(std::string line;std::getline(input,line);){
        std::istringstream row(line);row.imbue(std::locale::classic());std::string operation;
        if(!(row>>operation)||operation.front()=='#')continue;
        if(operation=="mission_presentation"){
            BitmapCamera camera{};std::int32_t width=0,height=0,scale=0,auto_flag=0,update_flag=0;
            if(!native||has_world||has_projection||has_view||has_slots||has_animation||simulate||
               !(row>>width>>height>>scale>>auto_flag>>update_flag>>camera.near_clip>>camera.far_clip>>camera.z)||scale<1||
               (auto_flag!=0&&auto_flag!=1)||(update_flag!=0&&update_flag!=1))
                throw std::invalid_argument("mission_presentation requires width height initial-form-scale animation0/1 update0/1 near far cameraZ and no presentation overrides");
            // Spatial camera owns offsets. QuestForm margins0/0 and RenderSystem depth rate100 are native initializers.
            offscreen_camera_=camera;view={width,height,scale,0,0,0,0,100};
            has_world=has_projection=has_view=has_slots=has_animation=true;
            reset_matrix(world);slots=35; // World.cctor MyEntityManager(0x23).
            frames=0;automatic=auto_flag;application_update=update_flag;simulate=0; // Start-at-first-frame host policy.
            resize_viewport(width,height);end(row);continue;
        }
        if(operation=="mission_resources"){
            std::string language;std::int32_t dpi=0;
            if(!native||!selection.data_root||tables||!owners.empty()||!resource_names.empty()||!(row>>std::quoted(language)>>dpi))
                throw std::invalid_argument("mission_resources requires --data, language/DPI and no previous resource bindings");
            const auto index=*selection.data_root/"mission-resources.tsv";
            if(std::filesystem::file_size(index)>16384)throw std::length_error("mission resource index exceeds16KiB");
            std::ifstream list(index);std::string signature;std::int32_t version=0;
            if(!(list>>signature>>version)||signature!="mission_resources"||version!=1)throw std::invalid_argument("invalid mission resource index header");
            std::set<std::int32_t> remaining{1,3,7,18,19,20,21,22,23};
            for(std::string line;std::getline(list,line);){
                std::istringstream entry(line);std::string kind,path,name;std::int32_t id=0;
                entry>>std::ws;if(entry.eof())continue;
                if(!(entry>>kind>>id>>std::quoted(path)>>std::quoted(name)))throw std::invalid_argument("invalid mission resource index row");end(entry);
                const auto relative=std::filesystem::u8path(path);
                if(relative.is_absolute()||relative.parent_path()!=std::filesystem::path("textassets")||relative.filename().empty())throw std::invalid_argument("mission resource path must be a direct textassets child");
                const auto resolved=*selection.data_root/relative;
                if(kind=="tables"&&id==-1&&name=="xls"&&!tables)
                    tables=std::make_unique<ResourceArchive>(read(resolved),ResourceProfile{language,dpi},ArchiveContents::data_only);
                else if(kind=="resource"&&remaining.erase(id)&&!name.empty()&&resource_names.emplace(name,id).second)
                    load_resource(id,resolved,language,dpi);
                else throw std::invalid_argument("duplicate or unexpected mission resource index entry");
            }
            if(!list.eof()||!tables||!remaining.empty())throw std::invalid_argument("incomplete mission resource index");
            end(row);continue;
        }
        if(operation=="material_policy"){
            std::string policy;
            if(!native||native_materials||!materials.empty()||!(row>>policy)||policy!="native")throw std::invalid_argument("material_policy native is exclusive with explicit material rows");
            native_materials=true;end(row);continue;
        }
        if(operation=="mission_events"){
            std::string mode;std::int32_t short_words=0,held=0,print=0;
            if(event_context||!(row>>mode)||mode!="fresh")throw std::invalid_argument("mission_events requires one explicit fresh event catalog state");
            row>>std::ws;
            if(row.peek()=='"'){std::string args;if(!(row>>std::quoted(args)))throw std::invalid_argument("invalid decoded event save string0");event_scenario.save_string0=decode_mission_text(args);}
            else{std::string null;if(!(row>>null)||null!="null")throw std::invalid_argument("mission_events requires null or quoted decoded save string0");}
            if(!(row>>short_words>>held>>print)||(short_words!=0&&short_words!=1)||(held!=0&&held!=1)||(print!=0&&print!=1))throw std::invalid_argument("mission_events requires explicit IsShortWords, held-advance and Config.PRINT bools");
            event_scenario.short_words=short_words!=0;event_scenario.held_advance=held!=0;event_scenario.print=print!=0;event_context=true;end(row);continue;
        }
        if(operation=="talk_font"){
            std::string path;MissionFontScenario font;std::int32_t localized=0,japanese=0,style=0;
            if(event_scenario.font||!(row>>std::quoted(path)>>font.requested_scale>>font.application_ratio>>font.layout_scale>>localized>>japanese>>style>>font.load_flags>>font.kerning_mode)||
               localized!=0||(japanese!=0&&japanese!=1)||(style!=0&&style!=1))throw std::invalid_argument("talk_font requires path, font/app/layout scales, localization-disabled0, Japanese0/1, Japanese-style0/1, load flags and kerning mode");
            font.path=std::filesystem::u8path(path);if(font.path.is_relative())font.path=scene.parent_path()/font.path;
            font.japanese=japanese!=0;font.japanese_style=style!=0;event_scenario.font=std::move(font);end(row);continue;
        }
        if(operation=="talk_layout"){
            std::int32_t id=0,count=0;
            if(!(row>>id>>count)||id<0||count<1||count>10000||event_scenario.page_lengths.count(id))throw std::invalid_argument("talk_layout requires unique id, page count1..10000 and explicit layout-provider lengths");
            auto& lengths=event_scenario.page_lengths[id];for(std::int32_t i=0;i<count;++i){std::int32_t n=0;if(!(row>>n)||n<0||n>2147483)throw std::invalid_argument("talk_layout length outside selected host bounds");lengths.push_back(n);}end(row);continue;
        }
        if(operation=="system_init"){
            std::int32_t loaded=0;
            if(initialized_loaded_||!(row>>loaded)||loaded<0||loaded>255)throw std::invalid_argument("system_init requires one explicit loaded byte0..255; zero requires hero fresh and town_save19");
            initialized_loaded_=static_cast<std::uint8_t>(loaded);end(row);continue;
        }
        if(operation=="system_construct"){
            std::string mode;
            if(construct_systems_||!(row>>mode)||mode!="empty_manager")throw std::invalid_argument("system_construct requires one explicit empty_manager entry");
            end(row);construct_systems_=true;continue;
        }
        if(operation=="town_save19"){
            std::int32_t value=0;
            if(saved_town_count_||!(row>>value))throw std::invalid_argument("town_save19 requires one explicit decoded integer");
            saved_town_count_=value;end(row);continue;
        }
        if(operation=="quest_frame"){
            std::int32_t debug=0,orientation=0;
            if(quest_frame_||!(row>>debug>>orientation)||(debug!=0&&debug!=1))throw std::invalid_argument("quest_frame requires Config debug0/1 and saved orientation int32");
            end(row);quest_frame_=QuestFrameConfig{debug!=0,orientation};continue;
        }
        if(operation=="menu_visible"){
            std::int32_t value=0;
            if(menu_snapshot||!(row>>value)||(value!=0&&value!=1))throw std::invalid_argument("menu_visible requires one explicit bool0/1");
            end(row);menu_snapshot=true;menu_visible_=value!=0;continue;
        }
        if(operation=="system_enabled"){
            std::int32_t id=0,value=0;
            if(!(row>>id>>value)||id<0||id>=32||(value!=0&&value!=1)||enabled_rows[static_cast<std::size_t>(id)])
                throw std::invalid_argument("system_enabled requires one factory0..31 and bool0/1 per slot");
            end(row);enabled_rows[static_cast<std::size_t>(id)]=true;system_states_[static_cast<std::size_t>(id)].enabled=value!=0;continue;
        }
        if(operation=="stage_start"){
            std::int32_t stage=0;std::string area;
            if(stage_entry||!(row>>stage>>area)||(area!="null"&&area!="area"))throw std::invalid_argument("stage_start requires one stage int32 and null or area <id>");
            if(area=="area"){
                std::int32_t id=0;
                if(!(row>>id)||id<0)throw std::invalid_argument("stage_start area requires a nonnegative AreaData ID");
                stage_area_id=id;
            }
            stage_entry=stage;native_stage_entry_=true;end(row);continue;
        }
        if(operation=="party_formation"){
            if(has_formation)throw std::invalid_argument("duplicate party_formation");
            formation.resize(16);for(auto& id:formation)if(!(row>>id)||id< -1)throw std::invalid_argument("party_formation requires sixteen character IDs or -1");
            has_formation=true;end(row);continue;
        }
        if(operation=="sorties"){
            std::int32_t id=0,count=0;
            if(!(row>>id>>count)||id<0||!progress.sortie_counts.emplace(id,count).second)throw std::invalid_argument("sorties requires unique character ID and saved int32 count");
            end(row);continue;
        }
        if(operation=="mission_selection"){
            SavedMissionSelection saved{};
            if(!native||selected_quest||stage_entry||!(row>>saved.area8>>saved.quest9>>rescued_prisoners))
                throw std::invalid_argument("mission_selection requires decoded area-slot8 quest-slot9 rescued count, without quest/stage_start overrides");
            saved_mission_selection_=saved;
            selected_quest=selection.quest.value_or(saved.quest9);stage_entry=*selected_quest;stage_area_id=saved.area8;
            native_stage_entry_=true;
        }else if(operation=="quest"){
            std::int32_t id=0;
            if(!native||selected_quest||!(row>>id>>rescued_prisoners)||id<0)throw std::invalid_argument("quest requires one nonnegative ID and explicit saved rescued count");
            selected_quest=selection.quest.value_or(id);
        }else if(operation=="runtime"){
            MissionRuntimeConfig value;std::int32_t debug=0;
            if(!native||runtime_row||!(row>>value.fps>>debug>>value.backend>>value.seed)||value.fps<1||value.fps>1000||(debug!=0&&debug!=1)||value.backend!="system")throw std::invalid_argument("runtime requires FPS1..1000 DEBUG0/1 system seed-int32");
            value.debug=debug!=0;runtime_row=true;
            if(runtime&&(runtime->fps!=value.fps||runtime->debug!=value.debug||runtime->backend!=value.backend||runtime->seed!=value.seed))throw std::invalid_argument("scene runtime conflicts with CLI inputs");runtime=value;
        }else if(operation=="rewards"){
            MissionRewardScenario value;if(!native||rewards||!(row>>value.defeated>>value.remaining>>value.saved_defeat_count))throw std::invalid_argument("rewards requires initial defeated/remaining/saved defeat counts");rewards=value;
        }else if(operation=="enemy_system"){
            std::string mode;
            if(!native||enemy_config||!(row>>mode))throw std::invalid_argument("enemy_system requires inputs");
            if(mode=="quest"){
                if(!(row>>enemy.yugami))throw std::invalid_argument("enemy_system quest requires explicit saved yugami");
                quest_fields.insert(operation);
            }else{
                enemy.yugami=parse_event_decimal(mode);
                if(!(row>>enemy.maximum>>enemy.total>>enemy.interval>>enemy.level))throw std::invalid_argument("enemy_system requires yugami maximum total interval base level");
            }enemy_config=true;
        }else if(operation=="enemy_population"){
            std::int32_t boss=0;
            if(!native||enemy_population||!(row>>enemy.created>>enemy.defeated>>enemy.remaining>>enemy.loop_counter>>boss>>enemy.section_limit>>enemy.section_created)||(boss!=0&&boss!=1))throw std::invalid_argument("enemy_population requires created defeated remaining loop boss0/1 uniform section limit/count");enemy.boss_defeated=boss!=0;enemy_population=true;
        }else if(operation=="enemy_pool"){
            std::int32_t count=0;if(!native||enemy_pool||!(row>>count)||count<0||count>1000)throw std::invalid_argument("enemy_pool requires count0..1000 followed by enemy ID/weight pairs");
            for(std::int32_t i=0;i<count;++i){std::int32_t id=0,weight=0;if(!(row>>id>>weight))throw std::invalid_argument("enemy_pool requires every ID/weight pair");enemy.pool.emplace_back(id,weight);}enemy_pool=true;
        }else if(operation=="result_wallet"){
            if(!native||wallet||!(row>>result_state.money>>result_state.crystals>>result_state.rescued))throw std::invalid_argument("result_wallet requires money/crystals int64 and rescued int32");wallet=true;
        }else if(operation=="result_record"){
            auto& q=result_state.record;
            if(!native||record||!(row>>q.best_experience>>q.best_money>>q.challenges>>q.clears>>q.defeated>>q.rescued>>q.best_damage))throw std::invalid_argument("result_record requires seven initial quest counters");record=true;
        }else if(operation=="result_inventory"){
            std::string kind;std::int32_t found=0,is_new=0,stock=0;
            if(!native||!(row>>kind>>found>>is_new>>stock)||(is_new!=0&&is_new!=1)||(kind!="equipment"&&kind!="cloth"&&kind!="hat"))throw std::invalid_argument("result_inventory requires equipment/cloth/hat found is-new0/1 stock");
            const auto k=kind=="equipment"?0U:kind=="cloth"?1U:2U;if(stocks[k])throw std::invalid_argument("duplicate result inventory kind");stocks[k]=true;result_state.inventory[k]={found,is_new!=0,stock};
        }else if(operation=="result_recruits"){
            std::int32_t fresh=0;auto& r=result_state.recruits;
            if(!native||recruits||!(row>>r.discovery_state>>fresh>>r.recruited)||(fresh!=0&&fresh!=1))throw std::invalid_argument("result_recruits requires found is-new0/1 recruited");r.is_new=fresh!=0;recruits=true;
        }else if(operation=="stage_boss"){
            std::string mode;MissionBossScenario value{};
            if(!native||stage_scenario.boss||!(row>>mode))throw std::invalid_argument("stage_boss requires one selection");
            if(mode=="quest")quest_fields.insert(operation);
            else{value.id=parse_event_decimal(mode);if(!(row>>value.level))throw std::invalid_argument("stage_boss requires ID and level");}
            stage_scenario.boss=value;
        }else if(operation=="stage_loop"){
            std::int32_t boss=0;std::string time,policy;
            if(!native||stage_loop||!(row>>time>>boss>>stage_scenario.fade_frame>>policy)||(boss!=0&&boss!=1)||policy!="hold")throw std::invalid_argument("stage_loop requires remaining-time boss-started0/1 fade-frame hold");
            if(time=="quest"){quest_fields.insert(operation);stage_scenario.reset_timer=true;}
            else stage_scenario.remaining_time=parse_event_decimal(time);
            stage_scenario.boss_started=boss!=0;stage_loop=true;
        }else if(operation=="stage_events"){
            std::string policy;std::int32_t count=0;
            if(!native||stage_events||!(row>>policy))throw std::invalid_argument("stage_events requires a selection");
            if(policy=="quest")quest_fields.insert(operation);
            else if(policy!="fresh"||!(row>>count)||count<0||count>1000)throw std::invalid_argument("stage_events requires fresh count0..1000 and event-ID/progress pairs");
            for(std::int32_t i=0;i<count;++i){std::int32_t id=0,rate=0;if(!(row>>id>>rate))throw std::invalid_argument("missing stage event pair");stage_scenario.events.emplace_back(id,rate);}stage_events=true;
        }else if(operation=="stage_pulse"){
            row>>std::ws;if(row.peek()=='-')throw std::invalid_argument("stage_pulse requires a positive logic step");
            std::uint64_t step=0,mask=0;if(!native||!(row>>step>>mask)||!step||mask>0xffffffffULL||!stage_scenario.pulses.emplace(step,static_cast<std::uint32_t>(mask)).second)throw std::invalid_argument("stage_pulse requires unique positive logic step and uint32 mask");
        }else if(operation=="collection"){
            std::string kind;if(!native||collection||!(row>>kind)||kind!="fresh")throw std::invalid_argument("collection requires one explicit fresh selection");collection=true;
        }else if(operation=="stage_notifications"){
            MissionNotificationScenario value{};std::int32_t retried=0,completed=0;std::uint64_t keys=0;
            std::string flags;
            if(!native||notifications||!(row>>value.state>>value.frame>>retried>>flags>>completed>>keys>>value.retry_answer)||(retried!=0&&retried!=1)||(completed!=0&&completed!=1)||keys>0xffffffffULL)throw std::invalid_argument("stage_notifications requires state frame retried0/1 quest-flags completed0/1 held-keys-uint32 retry-answer");
            if(flags=="quest")quest_fields.insert(operation);else value.quest_flags=parse_event_decimal(flags);
            value.retried=retried!=0;value.quest_completed=completed!=0;value.held_keys=static_cast<std::uint32_t>(keys);notifications=value;
        }else if(operation=="collisions"){
            if(!native||collisions)throw std::invalid_argument("collisions requires native mode and one row with no fields");collisions=true;
        }else if(operation=="enemy_gauges"){
            if(!native||enemy_gauges)throw std::invalid_argument("enemy_gauges requires one native selection");enemy_gauges=true;
        }else if(operation=="input"){
            std::string mode;std::uint64_t held=0;std::int32_t front=0;MissionInputScenario value;
            if(!native||input_scenario||!(row>>mode>>held>>value.last_active>>front)||mode!="fresh"||held>0xffffffffULL||(front!=0&&front!=1))throw std::invalid_argument("input requires fresh held-uint32 last-active front0/1 and three state/frame pairs");
            for(auto& b:value.buttons)if(!(row>>b.state>>b.frame))throw std::invalid_argument("input requires all three attack state/frame pairs");
            value.held_keys=static_cast<std::uint32_t>(held);value.front_pushed=front!=0;input_scenario=value;
        }else if(operation=="mission_map"){
            std::string mode;EventIntegers values(4,0);
            if(!native||map_images||!(row>>mode))throw std::invalid_argument("mission_map requires a selection");
            if(mode=="quest")quest_fields.insert(operation);
            else{
                if(mode!="fresh")throw std::invalid_argument("mission_map requires fresh or quest");
                for(auto& value:values)if(!(row>>value))throw std::invalid_argument("mission_map requires start/middle/end/background chip IDs");
            }
            map_images=std::make_shared<EventIntegers>(std::move(values));
        }else if(operation=="human_system"){
            MissionHumanScenario value{};std::string distance;
            if(!native||human_scenario||!(row>>distance>>value.min_z_rate>>value.max_z_rate>>value.attribute>>value.button))throw std::invalid_argument("human_system requires distance min/max Z rates attribute button state");
            if(distance=="quest")quest_fields.insert(operation);else value.distance=parse_event_decimal(distance);
            human_scenario=value;
        }else if(operation=="character_shop_new_game"){
            std::int32_t mask=0;
            if(!native||character_shop_new_game_mask_||!(row>>mask))throw std::invalid_argument("character_shop_new_game requires one explicit shop discovery mask");
            character_shop_new_game_mask_=mask;
        }else if(operation=="gear_new_game"){
            if(!native||gear_new_game_)throw std::invalid_argument("gear_new_game requires native mode and one fresh-record reset request");
            gear_new_game_=true;
        }else if(operation=="gear_levels"){
            std::string kind;std::int32_t level=0;
            if(!native||!(row>>kind>>level)||(kind!="equipment"&&kind!="hat"&&kind!="cloth")||!gear_defaults.emplace(kind,level).second)throw std::invalid_argument("gear_levels requires unique equipment/hat/cloth kind and explicit saved level");
        }else if(operation=="player"){
            std::size_t ordinal=0;if(!native||!(row>>ordinal))throw std::invalid_argument("player requires an entity creation ordinal");players.push_back(ordinal);
        }else if(operation=="actor_state"){
            std::size_t ordinal=0;std::int32_t state=0;float speed=0;
            if(!native||!(row>>ordinal>>state>>speed)||!std::isfinite(speed))throw std::invalid_argument("actor_state requires ordinal state finite Z speed");states.emplace_back(ordinal,state,speed);
        }else if(operation=="world"){
            if(has_world)throw std::invalid_argument("duplicate world matrix");
            for(auto& v:*world.values)if(!(row>>v))throw std::invalid_argument("world requires nine binary32 values");
            has_world=true;
        }else if(operation=="projection"){
            if(has_projection||!native)throw std::invalid_argument("projection requires native mode and one matrix");
            for(auto& v:projection.values)if(!(row>>v))throw std::invalid_argument("projection requires sixteen binary32 values");has_projection=true;
        }else if(operation=="gpu_depth"){
            if(has_gpu_depth||!native||!(row>>depth_bits>>depth_step)||depth_bits<17||depth_bits>32||(depth_step!=0&&depth_step!=1))
                throw std::invalid_argument("gpu_depth requires native mode, depth bits17..32 and step0/1");
            has_gpu_depth=true;
        }else if(operation=="material"){
            std::int32_t texture=0,material_queue=0,id=0,uniform=0;std::uint32_t shader=0;
            if(!native||native_materials||!(row>>texture>>shader>>material_queue>>id>>uniform)||texture<0||(uniform!=0&&uniform!=1))throw std::invalid_argument("material requires texture shader queue identity uniform0/1 in native mode without material_policy");
            auto inserted=materials.emplace(id,SubmissionMaterial{});auto& value=inserted.first->second;
            if(!inserted.second&&(value.queue.absolute!=material_queue||value.uniform_color!=(uniform!=0)))throw std::invalid_argument("conflicting material identity");
            value.queue.absolute=material_queue;value.uniform_color=uniform!=0;
            if(!material_bindings.emplace(std::make_tuple(static_cast<std::size_t>(texture),shader,material_queue),id).second)throw std::invalid_argument("duplicate material input binding");
        }else if(operation=="tables"){
            std::string path,language;std::int32_t dpi=0;
            if(!native||tables||!(row>>std::quoted(path)>>std::quoted(language)>>dpi))throw std::invalid_argument("tables requires native mode and one quoted archive/language/DPI profile");
            auto resolved=archive_path(path);
            tables=std::make_unique<ResourceArchive>(read(resolved),ResourceProfile{language,dpi},ArchiveContents::data_only);
        }else if(operation=="progress"){
            std::int32_t id=0;SavedCharacterProgression saved;
            if(!native||!(row>>id>>saved.level>>saved.experience)||!progress.characters.emplace(id,saved).second)throw std::invalid_argument("progress requires unique character ID, saved level and EXP");
        }else if(operation=="purchase"){
            std::int32_t id=0,count=0;if(!native||!(row>>id>>count)||!progress.shop_purchases.emplace(id,count).second)throw std::invalid_argument("purchase requires unique shop ID and saved count");
        }else if(operation=="resource_name"){
            std::string name;std::int32_t id=0;if(!native||!(row>>std::quoted(name)>>id)||!resource_names.emplace(name,id).second)throw std::invalid_argument("resource_name requires unique quoted name and explicit AppData ID");
        }else if(operation=="geometry"){
            StageGeometry g;std::string mode;if(!native||geometry||!(row>>mode))throw std::invalid_argument("geometry requires a selection");
            if(mode=="quest")quest_fields.insert(operation);
            else{g.step_x=parse_event_decimal(mode);if(!(row>>g.step_z>>g.height))throw std::invalid_argument("geometry requires step X/Z and height");}geometry=g;
        }else if(operation=="view"){
            if(!native||has_view||!(row>>view.width>>view.height>>view.scale>>view.camera_x>>view.camera_y>>view.margin_x>>view.margin_y>>view.extra_depth_rate))throw std::invalid_argument("view requires eight int32 values in native mode");
            has_view=true;
        }else if(operation=="slots"){
            if(!native||has_slots||!(row>>slots)||slots<1||slots>1024)throw std::invalid_argument("slots requires one component count1..1024 in native mode");has_slots=true;
        }else if(operation=="tick_ms"){
            std::int32_t value=0;if(!native||tick_ms||!(row>>value)||value<1||value>1000)throw std::invalid_argument("tick_ms requires one diagnostic interval1..1000");tick_ms=value;
        }else if(operation=="simulate"){
            std::int32_t value=0;if(!native||simulate||!(row>>value)||value<0||value>10000)throw std::invalid_argument("simulate requires one initial movement-pass count0..10000");simulate=value;
        }else if(operation=="animate"){
            if(!native||has_animation||!(row>>frames>>automatic>>application_update)||frames<0||frames>10000||(automatic!=0&&automatic!=1)||(application_update!=0&&application_update!=1))throw std::invalid_argument("animate requires count0..10000 and two flag0/1 values");
            has_animation=true;
        }else if(operation=="hero"){
            std::string mode;
            if(!native||fresh_hero_||!(row>>mode)||mode!="fresh")throw std::invalid_argument("hero fresh requires one native bootstrap request");
            fresh_hero_=true;players.push_back(selected.size());selected.emplace_back(SelectedFreshHero{});
        }else if(operation=="human"){
            HumanCreation h;if(!native||!(row>>h.direction>>h.character>>h.level>>h.formation_x>>h.formation_z>>h.experience>>h.x>>h.y>>h.z))throw std::invalid_argument("human requires six int32 and XYZ binary32 inputs");selected.emplace_back(h);
        }else if(operation=="enemy"){
            EnemyCreation e;if(!native||!(row>>e.direction>>e.id>>e.level>>e.flags>>e.position.x>>e.position.y>>e.position.z))throw std::invalid_argument("enemy requires direction ID level flags and XYZ");selected.emplace_back(e);
        }else if(operation=="bullet"){
            SelectedBullet b;if(!native||!(row>>b.id>>b.damage>>b.attribute>>b.parent>>b.position.x>>b.position.y>>b.position.z))throw std::invalid_argument("bullet requires ID damage attribute parent ordinal and XYZ");selected.emplace_back(b);
        }else if(operation=="drop"){
            SelectedDrop d;if(!native||!(row>>d.type>>d.value>>d.scale>>d.position.x>>d.position.y>>d.position.z))throw std::invalid_argument("drop requires type value scale and XYZ");selected.emplace_back(d);drop_catalog=true;
        }else if(operation=="bubble"){
            SelectedBubble b{};
            if(!native||bubbles.size()>=100000||!(row>>b.parent>>b.data_id>>b.type>>b.maximum>>b.resource>>b.clip>>b.initial.x>>b.initial.y>>b.initial.z)||b.parent< -1)
                throw std::invalid_argument("bubble requires parent dataId type frameMax resource clip and initial XYZ");
            bubbles.push_back(b);
        }else if(operation=="gauge"){
            SelectedGauge selected_gauge;auto& g=selected_gauge.args;
            if(!native||gauges.size()>=100000||!(row>>selected_gauge.step>>selected_gauge.parent>>g.type>>g.from>>g.to>>g.maximum>>g.image>>g.frame_max>>g.flags>>g.option>>g.option2)||selected_gauge.step>100000||selected_gauge.parent<0)
                throw std::invalid_argument("gauge requires step0..100000 parentOrdinal type from to max image frameMax flags option option2");
            gauges.push_back(selected_gauge);
        }else if(operation=="info_popup"){
            MissionPopupScenario s{};std::int32_t single=0;
            if(!native||info_popups.size()>=100000||!(row>>s.id>>s.category>>s.type>>s.priority>>single>>s.height>>s.duration)||(single!=0&&single!=1))
                throw std::invalid_argument("info_popup requires id category type priority single0/1 height duration");
            s.single=single!=0;info_popups.push_back(s);
        }else if(operation=="info_slide"){
            MissionSlideScenario s{};
            if(!native||info_slides.size()>=100000||!(row>>s.id>>s.width>>s.height>>s.duration))
                throw std::invalid_argument("info_slide requires id width height duration");
            info_slides.push_back(s);
        }else if(operation=="button"){
            std::int32_t parent=-1,dynamic=0,auto_destroy=0,unique=0;ButtonCreation b{};auto& p=b.properties;
            if(!native||buttons.size()>=100000||!(row>>parent>>b.x>>b.y>>b.resource>>b.clip>>b.frame>>p.id>>p.value>>dynamic>>auto_destroy>>p.scale>>p.offset_x>>p.offset_y>>unique)||
               parent< -1||(dynamic!=0&&dynamic!=1)||(auto_destroy!=0&&auto_destroy!=1)||(unique!=0&&unique!=1))
                throw std::invalid_argument("button requires parent x y resource clip frame group value dynamic0/1 autoDestroy0/1 scale offsetX offsetY unique0/1");
            p.dynamic=dynamic!=0;p.auto_destroy=auto_destroy!=0;b.unique_value=unique!=0;
            // Explicit diagnostic scenario: no text payload and text offsets0; not recovered quest widget setup.
            buttons.emplace_back(parent,std::move(b));
        }else if(operation=="effect"){
            SelectedEffect e;std::int32_t repeat=0;
            if(!native||!(row>>e.clip>>e.image>>repeat>>e.position.x>>e.position.y>>e.position.z)||(repeat!=0&&repeat!=1))throw std::invalid_argument("effect requires clip image loop0/1 and XYZ");
            e.loop=repeat!=0;selected.emplace_back(e);
        }else if(operation=="global_alias"){
            std::int32_t target=0,source=0;if(!(row>>target>>source))throw std::invalid_argument("global_alias requires target/source AppData IDs");
            auto** slot=application_resource_slot(global,target);auto* value=get_application_resource(application,source);
            if(!slot||*slot||!value)throw std::invalid_argument("global alias needs an empty supported target and ready source");*slot=value;
        }else if(operation=="resource"||operation=="global"){
            int id=0,dpi=0;std::string path,language;
            if(!(row>>id>>std::quoted(path)>>std::quoted(language)>>dpi))throw std::invalid_argument("resource requires id, quoted archive, quoted language and DPI");
            auto** slot=application_resource_slot(operation=="global"?global:application,id);
            if(!slot||*slot)throw std::invalid_argument("unsupported or duplicate AppData resource ID");
            auto resolved=archive_path(path);
            auto value=std::make_unique<SceneResource>(resolved,ResourceProfile{language,dpi},decoder);if(value->images)textures.bind(*value->images);
            *slot=&value->manager.drawing();owners.push_back(std::move(value));
        }else if(operation=="draw"){
            if(commands>=100000)throw std::length_error("capture command budget exceeded");
            RenderCommand c;for(auto& v:c.values)if(!(row>>v))throw std::invalid_argument("draw requires nineteen int32 fields");
            insert_render_command(queue,c);++commands;
        }else throw std::invalid_argument("unknown scene row type");
        end(row);
    }
    if(selection.quest&&!selected_quest)throw std::invalid_argument("--quest requires a scenario quest row or mission_selection with explicit saved rescued count");
    if(!has_world)throw std::invalid_argument("an explicit world matrix is required");
    if(event_context&&(!quest_frame_||!tick_ms))throw std::invalid_argument("mission_events requires quest_frame and an explicit tick_ms clock");
    if(!event_context&&!event_scenario.page_lengths.empty())throw std::invalid_argument("talk_layout requires mission_events");
    if(event_scenario.font&&(!event_context||!event_scenario.page_lengths.empty()))throw std::invalid_argument("talk_font requires mission_events and excludes provisional talk_layout rows");
    if(fresh_hero_&&(!initialized_loaded_||*initialized_loaded_!=0||!construct_systems_||!stage_entry||!saved_town_count_||selected.size()!=1))
        throw std::invalid_argument("hero fresh requires system_init 0, system_construct, native stage entry, town_save19 and no other selected actors");
    if(!fresh_hero_&&((initialized_loaded_&&*initialized_loaded_==0)||saved_town_count_))
        throw std::invalid_argument("system_init 0 and town_save19 require hero fresh");
    const bool result_requested=wallet||record||recruits||stocks[0]||stocks[1]||stocks[2]||gear_new_game_||character_shop_new_game_mask_.has_value();
    if(character_shop_new_game_mask_&&(!fresh_hero_||!initialized_loaded_||*initialized_loaded_!=0||!construct_systems_||!progress.characters.empty()||!progress.shop_purchases.empty()||!progress.sortie_counts.empty()||recruits))
        throw std::invalid_argument("character_shop_new_game requires fresh hero/system_init0/system_construct and excludes progress/purchase/sorties/result_recruits snapshots");
    if(gear_new_game_&&(!fresh_hero_||!initialized_loaded_||*initialized_loaded_!=0||!construct_systems_||!gear_defaults.empty()||stocks[0]||stocks[1]||stocks[2]))
        throw std::invalid_argument("gear_new_game requires fresh hero/system_init0/system_construct and excludes gear_levels/result_inventory snapshots");
    if(construct_systems_&&(!initialized_loaded_||!stage_entry||menu_snapshot||std::find(enabled_rows.begin(),enabled_rows.end(),true)!=enabled_rows.end()))
        throw std::invalid_argument("system_construct requires system_init and stage_start without system_enabled/menu_visible snapshot overrides");
    if(menu_snapshot&&!initialized_loaded_)throw std::invalid_argument("menu_visible snapshot requires system_init");
    if(quest_frame_&&(!construct_systems_||!stage_entry||!stage_loop||stage_scenario.fade_frame!=0))
        throw std::invalid_argument("quest_frame requires system_construct, stage_start and explicit stage_loop fade0");
    if(std::find(enabled_rows.begin(),enabled_rows.end(),true)!=enabled_rows.end()&&!initialized_loaded_)
        throw std::invalid_argument("system_enabled snapshot requires system_init");
    if(offscreen_camera_&&(!initialized_loaded_||!map_images))throw std::invalid_argument("mission_presentation requires system_init and mission_map");
    if(stage_entry){
        if(!selected_quest||!initialized_loaded_||!human_scenario||!stage_loop||!result_requested||!has_formation||!states.empty())
            throw std::invalid_argument("stage_start requires quest, system_init, human_system, stage_loop, results, party_formation and no actor_state override");
        human_scenario->stage_entry=true;
    }else if(has_formation||!progress.sortie_counts.empty())throw std::invalid_argument("party_formation/sorties require stage_start");
    if(!quest_fields.empty()&&!selected_quest)throw std::invalid_argument("quest source requires a quest selection row");
    if(selected_quest){
        if(!tables||selected.empty()||!human_scenario||!notifications)throw std::invalid_argument("quest requires tables, selected actors, human_system and stage_notifications");
        for(const auto* required:{"human_system","stage_notifications"})if(!quest_fields.count(required))throw std::invalid_argument("selected quest requires quest source for static fields");
        for(const auto& field:std::vector<std::pair<bool,const char*>>{{enemy_config,"enemy_system"},{stage_events,"stage_events"},{stage_scenario.boss.has_value(),"stage_boss"},{static_cast<bool>(map_images),"mission_map"},{geometry.has_value(),"geometry"}})
            if(field.first&&!quest_fields.count(field.second))throw std::invalid_argument("selected quest conflicts with explicit static scenario fields");
        if(enemy_pool)throw std::invalid_argument("selected quest supplies enemy_pool; remove explicit override");
        catalog=std::make_unique<GameplayCatalog>(load_gameplay_catalog(*tables,false,parse_event_decimal,rewards.has_value()||collection||drop_catalog,result_requested,true,stage_area_id.has_value(),fresh_hero_));
        if(character_shop_new_game_mask_)reset_character_shop_records(result_state);
        if(fresh_hero_){
            const auto main=std::find_if(catalog->characters.begin(),catalog->characters.end(),[](const auto& r){return r&&(r->flags&2)!=0;});
            if(main==catalog->characters.end()||!progress.characters.count((*main)->id))throw std::invalid_argument("hero fresh requires saved progression for the first table main character");
        }
        const QuestDefinition* definition=nullptr;
        if(saved_mission_selection_){
            const auto records=select_mission_records(*catalog->areas,*catalog->quests,*stage_area_id,*selected_quest);
            definition=records.quest;
            if(records.area)stage_area=std::shared_ptr<AreaDefinition>(catalog->areas,records.area);
        }else if(stage_area_id){
            auto& area=catalog->areas->at(static_cast<std::size_t>(*stage_area_id));
            if(!area)throw std::invalid_argument("selected area ID is a table gap");
            stage_area=std::shared_ptr<AreaDefinition>(catalog->areas,&*area);
        }
        if(!saved_mission_selection_){
            const auto& row=catalog->quests->at(static_cast<std::size_t>(*selected_quest));definition=row?&*row:nullptr;
        }
        if(!definition)throw std::invalid_argument("selected quest ID is a table gap");
        auto q=std::make_shared<StageQuest>(definition->stage);q->rescued_prisoners=rescued_prisoners;
        human_scenario->distance=q->distance;human_scenario->quest=q;notifications->quest_flags=q->flags;
        enemy.maximum=q->enemy_maximum;enemy.total=q->enemy_total_limit;enemy.interval=q->enemy_interval;enemy.level=q->base_level;
        if(enemy_config)enemy_pool=true;
        if(stage_scenario.boss)stage_scenario.boss=MissionBossScenario{q->boss_id,q->boss_level};
        if(map_images){map_images=q->chip_images;if(!map_images||map_images->size()<4)throw std::invalid_argument("selected quest requires four map chips");}
        if(geometry){const auto initial=initial_mission_map_creation();*geometry={initial.step_x,initial.step_z,stage_quest_height(*q)};}
    }
    const bool enemy_requested=enemy_config||enemy_population||enemy_pool;
    if(initialized_loaded_&&(!native||!human_scenario||!input_scenario||!map_images))throw std::invalid_argument("system_init requires native Human, Input and mission_map providers");
    if(!gauges.empty()){
        if(!geometry)throw std::invalid_argument("gauge scenario requires geometry");
        auto* resource=get_application_resource(global,23);bool found=false;
        for(auto& owner:owners)if(&owner->manager.drawing()==resource){
            if(!owner->images){owner->images=std::make_unique<ArchiveImageSet>(owner->manager,decode_png);textures.bind(*owner->images);}
            found=true;break;
        }
        if(!found)throw std::invalid_argument("gauge scenario requires global resource23");
    }
    if(enemy_gauges&&(!rewards||!get_application_resource(application,23)))throw std::invalid_argument("enemy_gauges requires explicit rewards saved counts and resource23");
    if(collisions&&!human_scenario)throw std::invalid_argument("collisions requires human_system event receivers");
    if(notifications&&!human_scenario)throw std::invalid_argument("stage_notifications requires human_system");
    if(collection&&(!notifications||!human_scenario))throw std::invalid_argument("collection fresh requires stage_notifications and human_system");
    if(result_requested&&!(wallet&&record&&(recruits||character_shop_new_game_mask_)&&(gear_new_game_||(stocks[0]&&stocks[1]&&stocks[2]))&&stage_loop&&collection))throw std::invalid_argument("results require wallet/record, inventory/recruit providers, stage_loop and collection fresh");
    if((stage_loop||stage_events||!stage_scenario.pulses.empty())&&!(stage_loop&&stage_events&&collection))throw std::invalid_argument("stage_loop requires explicit stage_events and collection fresh");
    if(stage_scenario.boss&&!(stage_loop&&enemy_requested))throw std::invalid_argument("stage_boss requires stage_loop and enemy system inputs");
    if(enemy_requested&&!(enemy_config&&enemy_population&&enemy_pool&&human_scenario))throw std::invalid_argument("enemy system requires enemy_system/enemy_population/enemy_pool and human_system");
    if(input_scenario&&(!human_scenario||players.size()!=1))throw std::invalid_argument("input requires human_system and one selected player");
    if(replay_bound_&&!input_scenario)throw std::invalid_argument("input replay requires input fresh");
    if(map_images){
        if(!human_scenario||!geometry||players.size()!=1||!get_application_resource(application,18))throw std::invalid_argument("mission_map requires human_system, geometry, one player and AppData18 chips");
        const auto initial=initial_mission_map_creation();const auto distance=human_scenario->distance;
        if(distance<0||distance>1022)throw std::invalid_argument("mission_map diagnostic distance limit0..1022");
        if(geometry->step_x!=initial.step_x||geometry->step_z!=initial.step_z||geometry->height!=distance+2)throw std::invalid_argument("mission_map geometry must match InitWorld steps and quest height");
    }
    if(!buttons.empty()&&!map_images)throw std::invalid_argument("button scenario requires mission_map fresh; dedicated widget drawing is unbound");
    if(!selected.empty()){
        if(!tables||!has_view||!has_slots||!has_animation||commands)throw std::invalid_argument("entity capture needs tables/view/slots/animate and no manual draw rows");
        if(!catalog)catalog=std::make_unique<GameplayCatalog>(load_gameplay_catalog(*tables,false,parse_event_decimal,rewards.has_value()||collection||drop_catalog,result_requested));
        if(gear_new_game_){
            result_state.inventory_records.emplace();
            const std::array<const EquipmentCombatTable*,3> records{{&catalog->equipment,&catalog->cloth,&catalog->hats}};
            const std::array<std::map<std::int32_t,std::int32_t>*,3> levels{{&progress.equipment_levels,&progress.cloth_levels,&progress.hat_levels}};
            // GetDataList's relative order: Equipment, Event(RET), Hat, Cloth.
            for(const std::size_t kind:{0U,2U,1U})for(const auto& row:*records[kind])if(row){
                EquipmentNewGameState initial;reset_equipment_for_new_game(initial,*row);
                levels[kind]->emplace(row->id,initial.level);
                result_state.inventory_records->at(kind).emplace(row->id,initial.inventory);
                auto& counts=new_game_gear_counts_[kind];++counts[0];
                if(initial.inventory.discovery_state==1)++counts[1];if(initial.inventory.stock==1)++counts[2];
            }
        }
        for(const auto& entry:gear_defaults){
            const auto& data=entry.first=="equipment"?catalog->equipment:entry.first=="hat"?catalog->hats:catalog->cloth;
            auto& levels=entry.first=="equipment"?progress.equipment_levels:entry.first=="hat"?progress.hat_levels:progress.cloth_levels;
            for(std::size_t i=0;i<data.size();++i)if(data[i])levels.emplace(static_cast<std::int32_t>(i),entry.second);
        }
        entity_world=std::make_unique<MissionEntities>(*catalog,progress,slots,geometry?&*geometry:nullptr,[&](std::int32_t id){auto* r=get_application_resource(application,id);return r?r->animations:nullptr;},
            [&](std::string_view name){auto it=resource_names.find(std::string(name));if(it==resource_names.end())throw std::invalid_argument("unbound enemy resource name: "+std::string(name));return it->second;});
        const bool original_fresh_entry=construct_systems_&&initialized_loaded_&&*initialized_loaded_==0;
        MapComponent* live_map=map_images?(original_fresh_entry?entity_world->shared_map_component():&entity_world->create_initial_map()):nullptr;
        for(const auto& request:selected){ComponentSlots* entity=nullptr;
            if(std::holds_alternative<SelectedFreshHero>(request)){created.push_back(nullptr);continue;} // Member6.Init creates it after the earlier initializers.
            if(const auto* h=std::get_if<HumanCreation>(&request))entity=&entity_world->human(*h);
            else if(const auto* e=std::get_if<EnemyCreation>(&request))entity=entity_world->enemy(*e);
            else if(const auto* b=std::get_if<SelectedBullet>(&request)){ComponentSlots* parent=nullptr;
                if(b->parent!=-1)parent=created.at(static_cast<std::size_t>(b->parent));
                entity=&entity_world->bullet({b->id,b->damage,b->attribute,parent,b->position});}
            else if(const auto* d=std::get_if<SelectedDrop>(&request))entity=entity_world->drop(d->type,d->value,d->position,d->scale);
            else{const auto& fx=std::get<SelectedEffect>(request);entity=&entity_world->effect(fx.position,fx.clip,fx.image,fx.loop);}
            if(!entity)throw std::invalid_argument("selected entity record is unavailable");created.push_back(entity);
        }
        mission=std::make_unique<MissionScene>(*entity_world,application,*entity_world->subset({0,2}),view,live_map,initialized_loaded_.has_value());
        for(const auto ordinal:players){auto* actor=created.at(ordinal);if(!actor&&fresh_hero_)continue;if(!actor||!actor->contains(14))throw std::invalid_argument("selected player needs Human component");entity_world->mark_player(*actor);}
        if(human_scenario){
            if(!runtime||!geometry||!simulate)throw std::invalid_argument("human_system requires runtime, geometry and simulate rows");
            humans=std::make_unique<MissionHumans>(*entity_world,*catalog,progress,live_map?static_cast<StageGeometry&>(*live_map):*geometry,*runtime,*human_scenario,initialized_loaded_.has_value(),mission->gauge_service());
            if(rewards)humans->enable_rewards(*rewards);
            if(enemy_requested)humans->enable_enemies(std::move(enemy));
            if(notifications)humans->enable_notifications(*notifications);
            if(collection)humans->enable_collection();
            if(result_requested)humans->enable_results(std::move(result_state));
            if(stage_loop)humans->enable_stage_loop(std::move(stage_scenario));
            if(input_scenario)humans->enable_input(*input_scenario);
            if(input_scenario&&!initialized_loaded_)humans->observe_input_touch([this](const InputTouch* event){mission->deliver_button_touch(event);});
            if(map_images){spatial=std::make_unique<MissionSpatial>(*entity_world,*live_map,view,humans->quest_form(),humans->quest(),map_images,initialized_loaded_.has_value());mission->bind_spatial(*spatial,!initialized_loaded_);
                if(!initialized_loaded_)spatial->observe_cursor_click([this](CursorPointRef point){mission->deliver_info_click(std::move(point));});}
            if(initialized_loaded_)initialize_loaded_world();
            if(fresh_hero_){
                created.at(players.front())=member_hero(&entity_world->player_world());
                if(!created.at(players.front()))throw std::logic_error("fresh Member Init did not publish a hero");
            }
            if(stage_entry)humans->start_mission(*stage_entry,formation,*spatial,
                [this]{return initialization_.paused;},[this]{pause_systems(false);},std::move(stage_area));
            for(const auto& request:states)humans->set_state(*created.at(std::get<0>(request)),std::get<1>(request),std::get<2>(request));
        }else if(!states.empty())throw std::invalid_argument("actor_state requires human_system");
        for(auto& selected_button:buttons){
            auto& b=selected_button.second;const auto parent=selected_button.first;
            if(parent>=0)b.properties.parent=created.at(static_cast<std::size_t>(parent));
            // SAGA_STUB(mission-entity-button-draw): retain native entities/Update/touch; dedicated image/text/widget output is unbound.
            entity_world->button(b);
        }
        for(const auto& s:info_popups)mission->add_info_popup(s);
        for(const auto& s:info_slides)mission->add_info_slide(s);
        for(const auto& b:bubbles){
            auto* parent=b.parent<0?nullptr:created.at(static_cast<std::size_t>(b.parent));
            entity_world->bubble(parent,b.data_id,b.type,b.maximum,b.resource,b.clip,b.initial);
        } // SAGA_STUB(mission-bubble-text): real component/geometry/lifetime; dedicated text drawing is unbound.
        for(const auto& selected_gauge:gauges){
            auto g=selected_gauge.args;g.parent=created.at(static_cast<std::size_t>(selected_gauge.parent));
            if(selected_gauge.step==0)entity_world->gauge(g,gauge_scratch_,*this);
            else gauge_creations_[selected_gauge.step].push_back(g);
        } // Explicit native factory arguments; original quest producers/restored state remain unbound.
        bind_schedule();
        for(const auto& frame:replay_)for(const auto& action:frame.second)
            if(action.kind==InputKind::viewport&&!offscreen_camera_)throw std::invalid_argument("viewport replay requires mission_presentation");
            else if(action.kind==InputKind::answer&&(!stage_loop||!initialized_loaded_))throw std::invalid_argument("answer replay requires stage_loop and system_init");
            else if((action.kind==InputKind::cursor||action.kind==InputKind::click)&&!spatial)throw std::invalid_argument("cursor/click replay requires mission_map fresh");
        for(std::int32_t i=0;i<frames;++i)mission->advance_animations({automatic!=0,application_update!=0});
        if(spatial)spatial->advance_visibility();
        if(event_context){
            if(!tables||!humans||!humans->stage_input_enabled())throw std::invalid_argument("mission_events requires table-backed Stage Update");
            events_=std::make_unique<MissionEvents>(*tables,std::move(event_scenario),[this]{humans->clear_form_input();});
            humans->bind_stage_events([this](std::int32_t id){return events_->execute(id);});
        }
        if(simulate&&*simulate>0){for(std::int32_t i=0;i<*simulate;++i)if(!step())break;}else{queue=mission->render();if(!mission->faults().empty())throw std::invalid_argument("entity render fault: "+mission->faults().front().message);}
        commands=static_cast<std::size_t>(queue.counts->at(0))+static_cast<std::size_t>(queue.counts->at(1));
    }
    if(simulate&&!mission)throw std::invalid_argument("simulate requires a table-backed entity scene");
    if(initialized_loaded_&&!initialization_complete_)throw std::invalid_argument("system_init requires a table-backed entity scene");
    if(rewards&&!humans)throw std::invalid_argument("rewards requires human_system");
    if(!buttons.empty()&&!mission)throw std::invalid_argument("button requires a table-backed entity scene");
    if((!info_popups.empty()||!info_slides.empty())&&!mission)throw std::invalid_argument("info bars require a table-backed entity scene");
    if(!bubbles.empty()&&!mission)throw std::invalid_argument("bubble requires a table-backed entity scene");
    if(!gauges.empty()&&!mission)throw std::invalid_argument("gauge requires a table-backed entity scene");
    if((human_scenario||!states.empty()||!players.empty()||!gear_defaults.empty())&&!mission)throw std::invalid_argument("human scenario rows require a table-backed entity scene");
    selected_count=selected.size();
}
bool MissionProfile::step(){
    if(!mission)throw std::invalid_argument("selected logic requires a table-backed entity scene");
    if(humans&&humans->transition_pending())return false;
    const auto input=replay_.find(logic_steps+1);
    if(input!=replay_.end())for(const auto& action:input->second){
        if(talk_pending()&&action.kind!=InputKind::keys&&action.kind!=InputKind::pulse&&action.kind!=InputKind::viewport){++replay_modal_ignored_;continue;}
        if(action.kind==InputKind::keys)humans->set_input_keys(action.value);
        else if(action.kind==InputKind::attack)humans->select_input_attack(static_cast<std::int32_t>(action.value));
        else if(action.kind==InputKind::pulse)humans->pulse_stage_keys(action.value);
        else if(action.kind==InputKind::answer)humans->answer_stage_dialog(action.x);
        else if(action.kind==InputKind::viewport)resize_viewport(action.x,action.y);
        else if(action.kind==InputKind::cursor)spatial->touch_cursor(action.x,action.y);
        else if(action.kind==InputKind::click)spatial->click_cursor(action.x,action.y);
        else humans->send_logical_touch(action.x,action.y,action.z);
        ++replay_applied_;
    }
    last_updates_.clear();last_world_passes_.clear();halted_=false;
    if(quest_frame_){
        humans->begin_stage_frame(logic_steps+1);
        try{
            const bool modal=events_&&events_->advance(logic_steps+1,*tick_ms,humans->stage_frame_pulse(0x100000U),humans->stage_frame_key(0x40U));
            if(!modal)quest_frame_result_=update_quest_frame(quest_frame_state_,humans->quest_form().fade_frame,*this);
        }
        catch(...){humans->end_stage_frame();throw;}
        humans->end_stage_frame();
    }else update_world(0);
    if(halted_)return false;
    ++logic_steps;return true;
}
void MissionProfile::write_actor_parameters(ComponentSlots& actor,std::ostream& out){
    if(const auto* direction=dynamic_cast<ContactDirection*>(actor.get(12)))out<<",\"direction\":"<<direction->direction<<",\"direction_locked\":"<<(direction->locked?"true":"false");
    if(const auto* sprite=dynamic_cast<EntitySprite*>(actor.get(2)))out<<",\"animation_clip\":"<<sprite->animation.clip;
    if(const auto* cell=dynamic_cast<CellComponent*>(actor.get(5)))out<<",\"cell_xi\":"<<cell->xi<<",\"cell_yi\":"<<cell->yi;
    if(actor.contains(23)){
        out<<",\"hp\":"<<resolve_combat_parameter(actor,0)->value;
        if(actor.contains(14)){const auto& h=*dynamic_cast<HumanRecordLink*>(actor.get(14));out<<",\"mp\":"<<resolve_combat_parameter(actor,1)->value<<",\"human_experience\":"<<h.experience<<",\"human_level\":"<<h.level<<",\"human_dash_speed\":"<<h.dash_speed;}
    }
    if(const auto* enemy=dynamic_cast<EnemyRecordLink*>(actor.get(24)))out<<",\"enemy_state\":"<<enemy->state<<",\"enemy_frame\":"<<enemy->frame<<",\"enemy_flags\":"<<enemy->flags;
}
void MissionProfile::write_combat_report(std::ostream& out){
    out<<",\"replay_modal_ignored\":"<<replay_modal_ignored_;
    if(events_)events_->write_report(out);
    out<<",\"fresh_hero_bootstrap\":"<<(fresh_hero_?"true":"false")<<",\"fresh_town_initialized\":"<<(fresh_town_initialized_?"true":"false");
    out<<",\"original_world_entry\":"<<(original_world_entry_?"true":"false")<<",\"world_entry_order\":[";
    for(std::size_t i=0;i<world_entry_order_.size();++i){if(i)out<<',';out<<std::quoted(world_entry_order_[i]);}out<<']';
    out<<",\"gear_new_game\":"<<(gear_new_game_?"true":"false")<<",\"new_game_gear_counts\":[";
    for(std::size_t k=0;k<new_game_gear_counts_.size();++k){if(k)out<<',';const auto& n=new_game_gear_counts_[k];out<<'['<<n[0]<<','<<n[1]<<','<<n[2]<<']';}out<<']';
    out<<",\"character_shop_new_game\":"<<(character_shop_new_game_mask_?"true":"false");
    if(character_shop_new_game_mask_){
        const auto found=[](const auto& rows){std::size_t count=0;for(const auto& entry:rows)if(entry.second.discovery_state==1)++count;return count;};
        out<<",\"new_game_shop_find_mask\":"<<*character_shop_new_game_mask_<<",\"new_game_record_counts\":[["<<initial_character_records_.size()<<','<<found(initial_character_records_)<<"],["<<initial_shop_records_.size()<<','<<found(initial_shop_records_)<<"]]";
    }
    if(fresh_town_initialized_){
        out<<",\"town_save19\":"<<*saved_town_count_<<",\"town_save19_writes\":"<<town_count_writes_<<",\"town_build_enable\":[";
        for(std::size_t i=0;i<town_state_.build_enable->size();++i){if(i)out<<',';out<<static_cast<int>(town_state_.build_enable->at(i));}out<<"],\"town_buildings\":[";
        for(std::size_t i=0;i<town_state_.building_list->size();++i){if(i)out<<',';out<<'[';const auto& row=*town_state_.building_list->at(i);for(std::size_t j=0;j<row.size();++j){if(j)out<<',';out<<row[j];}out<<']';}out<<']';
    }
    out<<",\"native_mission_selection\":"<<(saved_mission_selection_?"true":"false");
    if(saved_mission_selection_)out<<",\"decoded_mission_selection\":{\"area_slot8\":"<<saved_mission_selection_->area8
        <<",\"quest_slot9\":"<<saved_mission_selection_->quest9<<",\"effective_quest_slot9\":"<<*selected_quest<<'}';
    out<<",\"quest_frame_enabled\":"<<(quest_frame_?"true":"false")<<",\"world_update_calls\":"<<world_update_calls_<<",\"last_world_passes\":[";
    for(std::size_t i=0;i<last_world_passes_.size();++i){if(i)out<<',';out<<last_world_passes_[i];}out<<']';
    if(quest_frame_){
        out<<",\"quest_form_update\":"<<parent_update_counter_<<",\"quest_form_frame\":"<<quest_frame_state_.frame
           <<",\"quest_form_fade_frame\":"<<humans->quest_form().fade_frame<<",\"quest_form_fade_type\":"<<quest_frame_state_.fade_type
           <<",\"quest_form_pinch_frame\":"<<quest_frame_state_.pinch_frame<<",\"quest_form_result\":"<<(quest_frame_result_?"true":"false")
           <<",\"image_clock_resets\":"<<image_clock_resets_<<",\"image_clock_updates\":"<<image_clock_updates_;
    }
    out<<",\"system_construction_complete\":"<<(construction_complete_?"true":"false")<<",\"construction_pause_before_init\":";
    if(construction_pause_)out<<(*construction_pause_?"true":"false");else out<<"null";
    out<<",\"construction_order\":[";for(std::size_t i=0;i<construction_order_.size();++i){if(i)out<<',';out<<construction_order_[i];}out<<']';
    out<<",\"menu_visible\":"<<(menu_visible_?"true":"false")<<",\"menu_button_receivers\":"<<(mission?mission->button_observer_count(28):0);
    if(offscreen_camera_){
        out<<",\"viewport\":{\"width\":"<<view.width<<",\"height\":"<<view.height<<",\"changes\":"<<viewport_changes_<<",\"projection\":[";
        for(std::size_t i=0;i<projection.values.size();++i){if(i)out<<',';out<<projection.values[i];}out<<"]}";
    }
    out<<",\"init_schedule_complete\":"<<(initialization_complete_?"true":"false")<<",\"init_world_loaded_byte\":";
    if(initialized_loaded_)out<<static_cast<unsigned>(entity_world->player_world().loaded);else out<<"null";
    out<<",\"init_order\":[";for(std::size_t i=0;i<initialized_order_.size();++i){if(i)out<<',';out<<initialized_order_[i];}out<<']';
    out<<",\"init_manager_paused\":"<<(initialization_.paused?"true":"false")<<",\"init_town_return_registered\":"<<(town_return_?"true":"false")
       <<",\"submenu_button_events\":"<<submenu_touches_<<",\"menu_button_events\":"<<menu_touches_;
    out<<",\"pause_requests\":"<<pause_requests_<<",\"system_enabled\":[";
    for(std::size_t i=0;i<system_states_.size();++i){if(i)out<<',';out<<(system_states_[i].enabled?"true":"false");}out<<']';
    out<<",\"enemy_gauges_enabled\":"<<(enemy_gauges?"true":"false")<<",\"enemy_gauge_requests\":[";
    if(enemy_gauges){bool first=true;mission->draw_enemy_gauges([&](const ParameterGaugeRequest& r){if(!first)out<<',';first=false;
        out<<"{\"entity\":"<<entity_world->id(*r.entity)<<",\"x\":"<<r.x<<",\"y\":"<<r.y<<",\"image\":"<<r.image<<",\"parameter\":"<<r.parameter<<",\"anchor\":"<<r.anchor<<",\"force\":"<<(r.force?"true":"false")<<'}';});}out<<']';
    out<<",\"selected_quest\":";if(selected_quest)out<<*selected_quest;else out<<"null";
    if(selected_quest&&humans){const auto q=humans->quest();out<<",\"quest_distance\":"<<q->distance<<",\"quest_flags\":"<<q->flags<<",\"quest_money_rate\":"<<q->money_rate<<",\"quest_completion_byte\":"<<static_cast<int>(q->completed)<<",\"quest_rescued\":"<<q->rescued_prisoners;}
    const auto write_indices=[&](const char* name,const auto& indices){out<<",\""<<name<<"\":[";bool first=true;for(const auto id:indices){if(!first)out<<',';first=false;out<<id;}out<<']';};
    std::vector<std::int32_t> bound,unbound;
    for(const auto id:gameplay_update_order)(updates_[static_cast<std::size_t>(id)]?bound:unbound).push_back(id);
    write_indices("bound_update_systems",bound);write_indices("unbound_update_systems",unbound);write_indices("last_update_order",last_updates_);
    out<<",\"update_schedule_complete\":"<<(unbound.empty()?"true":"false");
    if(mission)mission->write_pass_report(out);
    out<<",\"movement_steps\":"<<movement_steps;
    out<<",\"replay_bound\":"<<(replay_bound_?"true":"false")<<",\"replay_actions\":"<<replay_applied_;
    if(humans)humans->write_input_report(out);else out<<",\"input_enabled\":false";
    out<<",\"spatial_enabled\":"<<(spatial?"true":"false");if(spatial)spatial->report(out);
    out<<",\"shots\":"<<(humans?humans->shots():0)<<",\"total_damage\":"<<(humans?humans->total_damage():0)<<",\"projectiles\":[";
    bool first=true;
    if(entity_world){auto cursor=entity_world->subset({27,0,1})->cursor();while(const auto e=cursor.next()){
        const auto* p=dynamic_cast<ContactPosition*>((*e)->get(0));const auto* flight=dynamic_cast<ProjectileLink*>((*e)->get(27));
        if(!p||!flight)throw std::invalid_argument("missing diagnostic projectile fields");
        if(!first)out<<',';first=false;
        out<<"{\"id\":"<<entity_world->id(**e)<<",\"x\":"<<p->x<<",\"y\":"<<p->y<<",\"z\":"<<p->z<<",\"frame\":"<<flight->frame<<",\"max_frame\":"<<flight->max_frame<<'}';
    }}
    out<<']';
    if(humans){humans->write_reward_report(out);humans->write_enemy_report(out);humans->write_notification_report(out);humans->write_collection_report(out);humans->write_result_report(out);}
    out<<",\"collisions_enabled\":"<<(collisions?"true":"false")<<",\"collision_events\":"<<(mission?mission->collision_events():0)<<",\"area_collision_events\":"<<(mission?mission->area_collision_events():0);
    out<<",\"pending_drops\":[";first=true;
    if(entity_world){auto cursor=entity_world->subset({34})->cursor();while(const auto e=cursor.next()){
        const auto* drop=dynamic_cast<DropComponent*>((*e)->get(34));const auto* p=dynamic_cast<ContactPosition*>((*e)->get(0));
        if(!drop||!p)throw std::invalid_argument("missing diagnostic pending drop fields");
        if(!first)out<<',';first=false;out<<"{\"id\":"<<entity_world->id(**e)<<",\"type\":"<<drop->type<<",\"value\":"<<drop->value<<",\"x\":"<<p->x<<",\"z\":"<<p->z<<'}';
    }}out<<']';
}
void MissionProfile::draw_enemy_gauges(BitmapBackend& backend){
    if(enemy_gauges)mission->draw_enemy_gauges([&](const ParameterGaugeRequest& r){backend.draw_parameter_gauge(r,mission->gauge_camera_host(),&gauge_scratch_);});
}
PixelImage* MissionProfile::gauge_image(std::int32_t id){
    auto* resource=get_application_resource(global,23);
    if(!resource||!resource->images)throw std::invalid_argument("gauge logic requires global resource23 images");
    auto* image=dynamic_cast<PixelImage*>(resource->images->at(static_cast<std::size_t>(id)));
    if(!image)throw std::invalid_argument("gauge logic requires decoded resource23 image");return image;
}
std::shared_ptr<std::vector<float>> MissionProfile::gauge_alpha(PixelImage& image,std::int32_t x,std::int32_t y,
        std::int32_t w,std::int32_t h,std::shared_ptr<std::vector<float>> alpha){
    const auto* ready=textures.at(textures.key(image)).ready;
    if(!ready)throw std::invalid_argument("gauge logic requires decoded alpha");
    return decoded_image_alpha(ready->pixels,x,y,w,h,std::move(alpha));
}
} // namespace saga::desktop
