// Initial S2 host: native fresh Town data and building-cache contracts.
#include "gameplay_catalog.hpp"
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
            "S2 data projection: fresh Town.Init, ResetBuildingList and GetBuildingList.\n"
            "Reads the existing resource index and APK-derived new-game defaults.\n"
            "SelectForm drawing, residents, quest selection and mission return remain unbound.\n";return 0;
    }
    std::optional<std::filesystem::path> root;std::string language="English.lproj";std::int32_t dpi=1;
    std::set<std::string> options;
    for(int i=1;i<argc;++i){
        const std::string key=argv[i];
        if((key!="--data"&&key!="--language"&&key!="--dpi")||i+1>=argc||!options.insert(key).second)throw std::invalid_argument("unsupported, incomplete or duplicate town option");
        const std::string value=argv[++i];
        if(key=="--data")root=std::filesystem::u8path(value);
        else if(key=="--language")language=value;
        else{const auto n=std::from_chars(value.data(),value.data()+value.size(),dpi);
            if(n.ec!=std::errc{}||n.ptr!=value.data()+value.size()||dpi<0)throw std::invalid_argument("town dpi requires nonnegative int32");}
    }
    if(!root)throw std::invalid_argument("town data root required; see --help");
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
    std::cout<<"{\"entry\":\"fresh-town-data\",\"town_records\":"<<town.records->size()
        <<",\"initial_town_capacity\":"<<defaults.town_capacity<<",\"town_capacity\":"<<town.capacity<<",\"build_enable\":[";
    for(std::size_t i=0;i<town.state.build_enable->size();++i){if(i)std::cout<<',';std::cout<<static_cast<int>(town.state.build_enable->at(i));}
    std::cout<<"],\"source_buildings\":";write_rows(std::cout,town.state.building_list);
    std::cout<<",\"buildings\":";write_rows(std::cout,buildings);
    std::cout<<",\"scope\":\"Town data/cache projection; SelectForm drawing, residents, quest selection and mission return unbound\"}\n";
    return 0;
}catch(const std::exception& error){std::cerr<<"saga_town: "<<error.what()<<'\n';return 1;}
