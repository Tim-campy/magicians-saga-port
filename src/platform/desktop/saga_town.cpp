// Initial S2 host: native fresh Town data and building-cache contracts.
#include "gameplay_catalog.hpp"
#include "town_background.hpp"
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
            "Add --scenery for native birds, ground and clouds; --app-seed 42 --lib-seed 42 are separate provisional entry streams.\n"
            "Provisional desktop context: --width 600 --height 640 --view-x 0 --view-y 53; scales100.\n"
            "Camera requests: --camera-x 0 --camera-y -9999, clamped by native fresh Area/Town bounds.\n"
            "S2 data projection: fresh Town.Init, ResetBuildingList and GetBuildingList.\n"
            "Reads the existing resource index and APK-derived new-game defaults.\n"
            "Full SelectForm drawing, residents, quest selection and mission return remain unbound.\n";return 0;
    }
    std::optional<std::filesystem::path> root,capture;std::string language="English.lproj";std::int32_t dpi=1;
    bool background=false;TownBackgroundContext context;
    std::set<std::string> options;
    for(int i=1;i<argc;++i){
        const std::string key=argv[i];
        if(key=="--background"||key=="--scenery"){
            if(!options.insert(key).second)throw std::invalid_argument("duplicate town drawing option");
            background=true;if(key=="--scenery")context.scenery=true;continue;}
        const std::set<std::string> allowed{"--data","--language","--dpi","--capture","--width","--height","--view-x","--view-y","--camera-x","--camera-y","--app-seed","--lib-seed"};
        if(!allowed.count(key)||i+1>=argc||!options.insert(key).second)throw std::invalid_argument("unsupported, incomplete or duplicate town option");
        const std::string value=argv[++i];
        if(key=="--data")root=std::filesystem::u8path(value);
        else if(key=="--language")language=value;
        else if(key=="--capture"){capture=std::filesystem::u8path(value);background=true;}
        else{std::int32_t parsed=0;const auto n=std::from_chars(value.data(),value.data()+value.size(),parsed);
            if(n.ec!=std::errc{}||n.ptr!=value.data()+value.size())throw std::invalid_argument("town numeric option requires int32");
            if(key=="--dpi"){if(parsed<0)throw std::invalid_argument("town dpi requires nonnegative int32");dpi=parsed;}
            else if(key=="--width")context.width=parsed;else if(key=="--height")context.height=parsed;
            else if(key=="--view-x")context.view_x=parsed;else if(key=="--view-y")context.view_y=parsed;
            else if(key=="--camera-x")context.camera_request_x=parsed;else if(key=="--camera-y")context.camera_request_y=parsed;
            else if(key=="--app-seed")context.app_seed=parsed;else context.lib_seed=parsed;}
    }
    if(!root)throw std::invalid_argument("town data root required; see --help");
    if(!context.scenery&&(options.count("--app-seed")||options.count("--lib-seed")))throw std::invalid_argument("town RNG inputs require --scenery");
    if(!background)for(const auto& key:options)if(key!="--data"&&key!="--language"&&key!="--dpi")
        throw std::invalid_argument("town drawing context requires --background or --capture");
#ifndef SAGA_TOWN_WINDOW
    if(capture)throw std::invalid_argument("town capture requires viewer build");
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
    initialize_fresh_town(town.state,town);
    // SelectForm.Init 0x11eaf88 invokes this cache reset; this host projects only
    // the Town data subsequence, not the earlier decoration/resident RNG phase.
    town.rebuild_town();
    const auto buildings=get_town_buildings(town.state,town.cache,town);
    std::unique_ptr<TownBackground> backdrop;
    if(background){
        const auto draw_index=read_game_resource_index(*root,true);const GameArchiveEntry* selected=nullptr;bool matching_tables=false;
        std::set<std::int32_t> draw_ids;std::set<std::string> draw_names;
        for(const auto& entry:draw_index.entries){
            if(!draw_ids.insert(entry.id).second||!draw_names.insert(entry.name).second)throw std::invalid_argument("duplicate town background resource index entry");
            if(entry.kind=="tables"&&entry.id==-1&&entry.name=="xls"&&entry.path==tables->path&&!matching_tables)matching_tables=true;
            else if(entry.kind=="resource"&&entry.id==11&&entry.name=="select"&&!selected)selected=&entry;
            else if(entry.kind!="resource"||((entry.id!=8||entry.name!="resident")&&(entry.id!=13||entry.name!="select_stage")))
                throw std::invalid_argument("unexpected town background resource index entry");
        }
        if(!matching_tables||!selected)throw std::invalid_argument("town background requires matching tables and select resource");
        backdrop=std::make_unique<TownBackground>(archive,*selected,ResourceProfile{language,dpi},town.capacity,context);
        if(capture){
#ifdef SAGA_TOWN_WINDOW
            capture_town_background(*backdrop,*capture);
#else
            throw std::invalid_argument("town capture requires viewer build");
#endif
        }else{
            SpriteMatrix world;reset_matrix(world);auto& app=backdrop->application();
            BitmapBackend backend(app,app,backdrop->textures(),context.height,world);backdrop->draw(backend);
        }
    }
    std::cout<<"{\"entry\":\"fresh-town-data\",\"town_records\":"<<town.records->size()
        <<",\"initial_town_capacity\":"<<defaults.town_capacity<<",\"town_capacity\":"<<town.capacity<<",\"build_enable\":[";
    for(std::size_t i=0;i<town.state.build_enable->size();++i){if(i)std::cout<<',';std::cout<<static_cast<int>(town.state.build_enable->at(i));}
    std::cout<<"],\"source_buildings\":";write_rows(std::cout,town.state.building_list);
    std::cout<<",\"buildings\":";write_rows(std::cout,buildings);
    if(backdrop)backdrop->report(std::cout);
    std::cout<<",\"scope\":\""<<(context.scenery?"Native cloud/decor initialization and drawing through ground/cloud prefix; floor, buildings, residents, quest selection and mission return unbound":backdrop?"Town data and native background prefix; decorations, buildings, residents, quest selection and mission return unbound":"Town data/cache projection; SelectForm drawing, residents, quest selection and mission return unbound")<<"\"}\n";
    return 0;
}catch(const std::exception& error){std::cerr<<"saga_town: "<<error.what()<<'\n';return 1;}
