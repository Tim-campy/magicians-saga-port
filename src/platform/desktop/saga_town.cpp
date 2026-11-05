// Native fresh Town/Area data and selected SelectForm drawing contracts.
#include "gameplay_catalog.hpp"
#include "town_background.hpp"
#include "mission_profile.hpp"
#include "decoded_image.hpp"
#ifdef SAGA_TOWN_WINDOW
#include "town_window.hpp"
#endif
#include <charconv>
#include <iostream>
#include <optional>
#include <set>

namespace {
using namespace saga::core;
using namespace saga::desktop;
struct TownData final:FreshTownHost {
    std::shared_ptr<TownTable> records;
    std::int32_t capacity;
    TownInitializationState state{std::make_shared<std::vector<EventInstruction>>(),{}};
    EventScript cache;
    std::shared_ptr<TownTable> town_records()override{return records;}
    std::int32_t saved_town_count()override{return capacity;}
    void save_town_count(std::int32_t value)override{capacity=value;}
    void rebuild_town()override{reset_town_buildings(state,cache,*this);}
};
void write_rows(std::ostream& out,const EventScript& rows){
    out<<'[';bool first=true;
    for(const auto& row:*rows){if(!first)out<<',';first=false;
        if(!row){out<<"null";continue;}
        out<<'[';for(std::size_t i=0;i<row->size();++i){if(i)out<<',';out<<row->at(i);}out<<']';
    }out<<']';
}
}
int main(int argc,char** argv)try{
    if(argc==2&&std::string_view(argv[1])=="--help"){
        std::cout<<"Usage: saga_town --data extracted-root [--language English.lproj] [--dpi 1]\n"
            "Background prefix: --background or --capture new.bmp (viewer build).\n"
            "Add --scenery for native birds, ground, clouds, floor and gate; --app-seed 42 --lib-seed 42 are separate provisional entry streams.\n"
            "Add --buildings for initial construction sprites and queued notices (implies --scenery).\n"
            "Add --areas for native area maps and the next locked fence (implies --buildings); --camera-x 449 exposes the initial map.\n"
            "Add --residents for native resident Init and lane0 drawing (implies --areas).\n"
            "Add --hero for native Member/hero creation, hero drawing and resident lanes1/2 (implies --residents).\n"
            "Add --foreground for the native fresh foreground, leaves and final notice flush (implies --hero).\n"
            "Diagnostic --motion-passes N advances selected counter/cloud/decor/resident blocks (requires --foreground); this is not full Update.\n"
            "Full controller: --update-passes N --foreground; optional --talk-pulse-every N supplies explicit dialogue inputs.\n"
            "Add --shared-world for native World initialization before town; retain its hero/formation/catalog/event owners through Stage.Start.\n"
            "--gameplay-seed 42 is a provisional independent gameplay stream; AppData/Lib retain their own seeds.\n"
            "--mission-replay file supplies S1 replay actions in mission-relative logic steps after town confirmation.\n"
            "Interactive viewer: --foreground --interactive; right click walks/right drag pans; left click selects areas/buildings and GO; Backspace returns from the selected menu; Enter/Space advances dialogue; Escape closes.\n"
            "Add --update-passes N --hidden for a bounded automated window; --physical-input enables the same hit path headlessly.\n"
            "Add --replay file for ordered native camera/selection/button/pulse input plus physical widget/stage-back/form-back actions; default town input is neutral.\n"
            "Requires v2 --mission-initialization and --mission-font extraction; default talk input is neutral.\n"
            "Mission menu requires --stage-menu extraction; left down selects a row, release inside confirms.\n"
            "Provisional menu providers: --stamina-up-state 0 (billing state), --server-time 0 (fixed seconds), --menu-font 10.\n"
            "Motion context: --motion-low-aspect 0 supplies the provisional cloud-wrap aspect gate.\n"
            "Resident context: --store-kairo-limit 0 --condition-characters 1 --debug 0; DEBUG1 requires --debug-rescue-count.\n"
            "Area clock context: --automatic-images 0 (provisional Seb.AutoAnimation gate; true branch unbound).\n"
            "Building context: --high-score 0 (provisional int64 system save), --top-form 1 (standalone form stack).\n"
            "Provisional desktop context: --width 600 --height 640 --view-x 0 --view-y 53; scales100.\n"
            "Camera requests: --camera-x 0 --camera-y -9999, clamped by native fresh Area/Town bounds.\n"
            "S2 data projection: fresh Town.Init, ResetBuildingList and GetBuildingList.\n"
            "Reads the existing resource index and APK-derived new-game defaults.\n"
            "Full Update binds camera, Area/GO, mission rows/stats and stamina; production41 starts the retained mission with --shared-world. Town return, list gestures, other menus and unlock information remain unbound.\n";return 0;
    }
    std::optional<std::filesystem::path> root,capture;std::string language="English.lproj";std::int32_t dpi=1;
    bool background=false,interactive=false,hidden=false;TownBackgroundContext context;std::int32_t update_passes=0,talk_pulse_every=0;
    std::set<std::string> options;
    for(int i=1;i<argc;++i){
        const std::string key=argv[i];
        if(key=="--interactive"||key=="--hidden"||key=="--physical-input"||key=="--shared-world"){
            if(!options.insert(key).second)throw std::invalid_argument("duplicate town window option");
            if(key=="--shared-world")context.shared_world=true;
            else if(key=="--hidden")hidden=true;
            else{context.physical_input=true;if(key=="--interactive"){interactive=true;context.full_update=true;}}
            continue;
        }
        if(key=="--background"||key=="--scenery"||key=="--buildings"||key=="--areas"||key=="--residents"||key=="--hero"||key=="--foreground"){
            if(!options.insert(key).second)throw std::invalid_argument("duplicate town drawing option");
            background=true;if(key!="--background")context.scenery=true;
            if(key=="--buildings"||key=="--areas"||key=="--residents"||key=="--hero"||key=="--foreground")context.buildings=true;
            if(key=="--areas"||key=="--residents"||key=="--hero"||key=="--foreground")context.areas=true;
            if(key=="--residents"||key=="--hero"||key=="--foreground")context.residents=true;
            if(key=="--hero"||key=="--foreground")context.hero=true;
            if(key=="--foreground")context.foreground=true;continue;}
        const std::set<std::string> allowed{"--data","--language","--dpi","--capture","--width","--height","--view-x","--view-y","--camera-x","--camera-y","--app-seed","--lib-seed","--high-score","--top-form","--automatic-images","--store-kairo-limit","--condition-characters","--debug","--debug-rescue-count","--motion-passes","--motion-low-aspect","--update-passes","--talk-pulse-every","--replay","--stamina-up-state","--server-time","--menu-font","--gameplay-seed","--mission-replay"};
        if(!allowed.count(key)||i+1>=argc||!options.insert(key).second)throw std::invalid_argument("unsupported, incomplete or duplicate town option");
        const std::string value=argv[++i];
        if(key=="--data")root=std::filesystem::u8path(value);
        else if(key=="--replay")context.input_replay=std::filesystem::u8path(value);
        else if(key=="--mission-replay")context.mission_replay=std::filesystem::u8path(value);
        else if(key=="--language")language=value;
        else if(key=="--capture"){capture=std::filesystem::u8path(value);background=true;}
        else if(key=="--high-score"||key=="--server-time"){
            auto& target=key=="--high-score"?context.high_score:context.server_time;
            const auto n=std::from_chars(value.data(),value.data()+value.size(),target);
            if(n.ec!=std::errc{}||n.ptr!=value.data()+value.size())throw std::invalid_argument("town numeric option requires int64");}
        else{std::int32_t parsed=0;const auto n=std::from_chars(value.data(),value.data()+value.size(),parsed);
            if(n.ec!=std::errc{}||n.ptr!=value.data()+value.size())throw std::invalid_argument("town numeric option requires int32");
            if(key=="--dpi"){if(parsed<0)throw std::invalid_argument("town dpi requires nonnegative int32");dpi=parsed;}
            else if(key=="--width")context.width=parsed;else if(key=="--height")context.height=parsed;
            else if(key=="--view-x")context.view_x=parsed;else if(key=="--view-y")context.view_y=parsed;
            else if(key=="--camera-x")context.camera_request_x=parsed;else if(key=="--camera-y")context.camera_request_y=parsed;
            else if(key=="--app-seed")context.app_seed=parsed;else if(key=="--lib-seed")context.lib_seed=parsed;
            else if(key=="--stamina-up-state")context.stamina_up_state=parsed;
            else if(key=="--gameplay-seed")context.gameplay_seed=parsed;
            else if(key=="--menu-font"){if(parsed<1||parsed>100)throw std::invalid_argument("town menu font requires1..100");context.menu_font=parsed;}
            else if(key=="--store-kairo-limit")context.store_limit=parsed;
            else if(key=="--debug-rescue-count")context.debug_rescue_count=parsed;
            else if(key=="--update-passes"){if(parsed<0)throw std::invalid_argument("negative town update passes");context.full_update=true;update_passes=parsed;}
            else if(key=="--talk-pulse-every"){if(parsed<1)throw std::invalid_argument("talk pulse period must be positive");talk_pulse_every=parsed;}
            else if(key=="--motion-passes"){if(parsed<0)throw std::invalid_argument("negative town motion passes");context.motion=true;context.motion_passes=parsed;}
            else if(key=="--motion-low-aspect"){if(parsed!=0&&parsed!=1)throw std::invalid_argument("motion aspect requires 0 or 1");context.motion_low_aspect=parsed!=0;}
            else if(key=="--condition-characters"){if(parsed<0)throw std::invalid_argument("negative condition context count");context.condition_character_count=parsed;}
            else{if(parsed!=0&&parsed!=1)throw std::invalid_argument("town boolean option requires 0 or 1");
                if(key=="--top-form")context.top_form=parsed!=0;else if(key=="--debug")context.debug=parsed!=0;else context.automatic_images=parsed!=0;}}
    }
    if(hidden&&(!interactive||update_passes==0))throw std::invalid_argument("hidden town window requires --interactive and positive --update-passes");
    if(options.count("--gameplay-seed")&&!context.shared_world)throw std::invalid_argument("gameplay seed requires shared world");
    if(context.mission_replay&&!context.shared_world)throw std::invalid_argument("mission replay requires shared world");
    if(context.shared_world&&!context.full_update)throw std::invalid_argument("shared world requires full town Update");
    if(context.physical_input&&!context.full_update)throw std::invalid_argument("physical input requires full town Update");
    if(interactive&&!context.foreground)throw std::invalid_argument("interactive town requires --foreground");
    if(!root)throw std::invalid_argument("town data root required; see --help");
    if(context.full_update&&(!context.foreground||context.motion))throw std::invalid_argument("full town Update requires --foreground and excludes --motion-passes");
    if(context.input_replay&&!context.full_update)throw std::invalid_argument("town replay requires --update-passes");
    if(!context.full_update)for(const auto* key:{"--stamina-up-state","--server-time","--menu-font"})if(options.count(key))throw std::invalid_argument("stage providers require full town Update");
    if(options.count("--talk-pulse-every")&&!context.full_update)throw std::invalid_argument("talk pulses require --update-passes");
    if(context.motion&&!context.foreground)throw std::invalid_argument("town motion requires --foreground");
    if(options.count("--motion-low-aspect")&&!context.motion&&!context.full_update)throw std::invalid_argument("motion aspect requires --motion-passes or --update-passes");
    if(!context.residents)for(const auto* key:{"--store-kairo-limit","--condition-characters","--debug","--debug-rescue-count"})if(options.count(key))throw std::invalid_argument("town resident context requires --residents");
    if(!context.areas&&options.count("--automatic-images"))throw std::invalid_argument("town render-clock context requires --areas");
    if(!context.buildings&&(options.count("--high-score")||options.count("--top-form")))throw std::invalid_argument("town building context requires --buildings");
    if(!context.scenery&&(options.count("--app-seed")||options.count("--lib-seed")))throw std::invalid_argument("town RNG inputs require --scenery");
    if(!background)for(const auto& key:options)if(key!="--data"&&key!="--language"&&key!="--dpi")
        throw std::invalid_argument("town drawing context requires --background or --capture");
#ifndef SAGA_TOWN_WINDOW
    if(capture||interactive)throw std::invalid_argument("town capture/window requires viewer build");
#endif
    if(capture&&std::filesystem::exists(*capture))throw std::invalid_argument("town capture destination already exists");
    const auto index=read_game_resource_index(*root);const GameArchiveEntry* tables=nullptr;
    std::set<std::int32_t> ids;std::set<std::string> names;
    for(const auto& entry:index.entries){
        if(entry.kind=="tables"&&entry.id==-1&&entry.name=="xls"&&!tables)tables=&entry;
        else if(entry.kind!="resource"||entry.id<0||entry.name.empty()||!ids.insert(entry.id).second||!names.insert(entry.name).second)
            throw std::invalid_argument("duplicate or unexpected town resource index entry");
    }
    if(!tables)throw std::invalid_argument("town tables archive missing from resource index");
    ResourceArchive archive(read_game_archive(tables->path),ResourceProfile{language,dpi},ArchiveContents::data_only);
    const auto table=archive.data("Town.txt");if(!table)throw std::invalid_argument("missing town table: Town.txt");
    const auto defaults=read_new_game_save_defaults(*root);
    TownData town;town.records=std::make_shared<TownTable>(load_town_table(std::string_view(reinterpret_cast<const char*>(table->bytes.data()),table->bytes.size()),false,parse_event_decimal));
    town.capacity=defaults.town_capacity;
    std::shared_ptr<MissionProfile> shared_world;
    if(context.shared_world){
        MissionBootstrapOptions b;b.data_root=*root;b.defer_stage_start=true;b.width=context.width;b.height=context.height;b.language=language;b.dpi=dpi;
        b.camera=context.projection;b.depth_bits=context.depth_bits;b.depth_step=context.depth_step?1:0;b.debug_controls=context.debug;
        const auto runtime=initial_game_frame_configuration;
        shared_world=std::make_shared<MissionProfile>(b,decode_png,MissionRuntimeConfig{runtime.fps,runtime.damage_debug,context.gameplay_seed,"system"},context.mission_replay);
        town.records=shared_world->catalog->towns;town.state=shared_world->initialized_town();town.capacity=shared_world->initialized_town_capacity();
    }else initialize_fresh_town(town.state,town);
    // SelectForm.Init 0x11eaf88 invokes this cache reset; this host projects only
    // the Town data subsequence, not the earlier decoration/resident RNG phase.
    town.rebuild_town();
    auto buildings=get_town_buildings(town.state,town.cache,town);
    std::unique_ptr<TownBackground> backdrop;
    if(background){
        const auto draw_index=read_game_resource_index(*root,true);const GameArchiveEntry* selected=nullptr;const GameArchiveEntry* bg_entry=nullptr;const GameArchiveEntry* common_entry=nullptr;const GameArchiveEntry* stage_entry=nullptr;const GameArchiveEntry* shop_entry=nullptr;bool matching_tables=false;
        std::set<std::int32_t> draw_ids;std::set<std::string> draw_names;
        for(const auto& entry:draw_index.entries){
            if(!draw_ids.insert(entry.id).second||!draw_names.insert(entry.name).second)throw std::invalid_argument("duplicate town background resource index entry");
            if(entry.kind=="tables"&&entry.id==-1&&entry.name=="xls"&&entry.path==tables->path&&!matching_tables)matching_tables=true;
            else if(entry.kind=="resource"&&entry.id==11&&entry.name=="select"&&!selected)selected=&entry;
            else if(entry.kind=="resource"&&entry.id==0&&entry.name=="bg"&&!bg_entry)bg_entry=&entry;
            else if(entry.kind=="resource"&&entry.id==2&&entry.name=="com"&&!common_entry)common_entry=&entry;
            else if(entry.kind=="resource"&&entry.id==13&&entry.name=="select_stage"&&!stage_entry)stage_entry=&entry;
            else if(entry.kind=="resource"&&entry.id==14&&entry.name=="shop"&&!shop_entry)shop_entry=&entry;
            else if(entry.kind!="resource"||((entry.id!=8||entry.name!="resident")&&(entry.id!=13||entry.name!="select_stage")))
                throw std::invalid_argument("unexpected town background resource index entry");
        }
        if(!matching_tables||!selected)throw std::invalid_argument("town background requires matching tables and select resource");
        backdrop=std::make_unique<TownBackground>(archive,*selected,ResourceProfile{language,dpi},town.capacity,context,buildings,town.state.build_enable,bg_entry,common_entry,&index,stage_entry,shared_world,shop_entry);
        for(std::int32_t pass=0;pass<context.motion_passes;++pass)backdrop->advance_motion();
        if(context.full_update)backdrop->initialize_update(archive,*root);
        if(interactive){
#ifdef SAGA_TOWN_WINDOW
            run_town_window(*backdrop,TownWindowOptions{true,hidden,static_cast<std::uint64_t>(update_passes),talk_pulse_every,capture});
#endif
        }else if(context.full_update){
            SpriteMatrix matrix;reset_matrix(matrix);auto& application=backdrop->application();
            BitmapBackend cpu_draw(application,application,backdrop->textures(),context.height,matrix);
            // Explicit desktop dispatch cadence: one draw after each selected
            // form update, including modal waits; resources register live clocks.
            for(std::uint64_t frame=1;frame<=static_cast<std::uint64_t>(update_passes);++frame){
                backdrop->advance_update(frame,talk_pulse_every>0&&frame%static_cast<std::uint64_t>(talk_pulse_every)==0);
                backdrop->draw(cpu_draw);
                if(backdrop->quest_handoff_pending())break;
            }
        }
        if(!interactive&&capture){
#ifdef SAGA_TOWN_WINDOW
            capture_town_background(*backdrop,*capture);
#else
            throw std::invalid_argument("town capture requires viewer build");
#endif
        }else if(!interactive){
            SpriteMatrix world;reset_matrix(world);auto& app=backdrop->application();
            BitmapBackend backend(app,app,backdrop->textures(),context.height,world);backdrop->draw(backend);
        }
    }
    if(backdrop&&context.buildings)buildings=backdrop->current_buildings();
    std::cout<<"{\"entry\":\"fresh-town-data\",\"town_records\":"<<town.records->size()
        <<",\"initial_town_capacity\":"<<defaults.town_capacity<<",\"town_capacity\":"<<town.capacity<<",\"build_enable\":[";
    for(std::size_t i=0;i<town.state.build_enable->size();++i){if(i)std::cout<<',';std::cout<<static_cast<int>(town.state.build_enable->at(i));}
    std::cout<<"],\"source_buildings\":";write_rows(std::cout,town.state.building_list);
    std::cout<<",\"buildings\":";write_rows(std::cout,buildings);
    if(backdrop)backdrop->report(std::cout);
    std::cout<<",\"scope\":\""<<(context.full_update?(context.shared_world?"Persistent town campaign with canonical progression, mission entry/return, construction, resident gifts and king rewards; remaining menus, full modal graphics, saves and Android timing unverified":context.physical_input?"Persistent full town Update with camera/area/GO and stage list/stats/selection/stamina; retained shared-world mission entry/combat; town return, list gestures, other menus and dialogue/bubble glyphs unbound":context.input_replay?"Persistent full town Update with native camera/selection replay, movement and resident voice selection; physical hit registration, dialogue/bubble glyphs and menu/quest/return forms unbound":"Persistent full town Update with real event selection/delays and modal pause; neutral town input, provisional font metrics, dialogue drawing and quest/return forms unbound"):context.motion?"Selected native active-motion subsequence and town drawing; full Update permission/event/form gates remain unbound":context.foreground?"Native fresh town drawing through foreground and notice flush; glyph/input/clip services stubbed; persistent Update, unlock information, quest selection and mission return unbound":context.hero?"Native Member/hero Init and drawing through resident lanes1/2; notices queued, glyph/input/clip services stubbed; foreground, quest selection and mission return unbound":context.residents?"Native resident Init and lane0 composition; notices queued, glyph/input/clip services stubbed; hero, later lanes/foreground, quest selection and mission return unbound":context.areas?"Native drawing through town areas and locked fence; notices queued, text/input, inherited clip, residents, foreground, quest selection and mission return unbound":context.buildings?"Native drawing through initial buildings; notices queued, text/input services, residents, foreground, quest selection and mission return unbound":context.scenery?"Native cloud/decor initialization and drawing through floor/gate prefix; buildings, residents, quest selection and mission return unbound":backdrop?"Town data and native background prefix; decorations, buildings, residents, quest selection and mission return unbound":"Town data/cache projection; SelectForm drawing, residents, quest selection and mission return unbound")<<"\"}\n";
    return 0;
}catch(const std::exception& error){std::cerr<<"saga_town: "<<error.what()<<'\n';return 1;}
