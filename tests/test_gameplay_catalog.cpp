#include "mission_data_fixture.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace saga::core;
using namespace saga::desktop;
namespace {
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
}
int main(int argc,char** argv)try{
    if(argc==3){std::ifstream file(argv[1],std::ios::binary|std::ios::ate);if(!file)throw std::runtime_error("cannot open decoded tables");
        const auto size=file.tellg();if(size<0||size>128*1024*1024)throw std::length_error("decoded table archive size");
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),size);
        if(!file)throw std::runtime_error("incomplete table archive");
        ResourceArchive archive(std::move(bytes),{argv[2],1},ArchiveContents::data_only);
        const auto data=load_gameplay_catalog(archive,false,parse_event_decimal);
        auto count=[](const auto& rows){std::size_t n=0;for(const auto& row:rows)if(row)++n;return n;};
        std::cout<<"characters="<<count(data.characters)<<" equipment="<<count(data.equipment)<<" hats="<<count(data.hats)<<" cloth="<<count(data.cloth)
                 <<" enemies="<<count(*data.enemies)<<" bullets="<<count(*data.bullets)<<" shop="<<count(data.shop)<<'\n';
        std::map<std::string,std::int32_t> first;for(const auto& row:*data.enemies)if(row)first.emplace(row->resource,row->id);
        for(const auto& entry:first)std::cout<<"enemy_resource="<<entry.first<<" first_id="<<entry.second<<'\n';return 0;}
    auto members=saga::test::mission_tables();ResourceArchive archive(saga::test::jar_fixture(members),{"English.lproj",1},ArchiveContents::data_only);
    fails([&]{archive.images();});fails([&]{archive.animations();});check(!archive.data("Missing.txt"));
    const auto data=load_gameplay_catalog(archive,false,parse_event_decimal);
    check(data.characters.size()==3&&!data.characters[0]&&data.characters[2]->image==8&&data.characters[2]->initial_level==99);
    check(data.equipment[1]->image==12&&data.equipment[1]->resource=="weapon"&&data.equipment[1]->bullet_id==3);
    check(data.hats[0]->image==9&&data.cloth[0]->image==11&&data.enemies->at(4)->image==7&&data.bullets->at(3)->image==5);
    check(data.members.size()==7&&data.members.at("Enemy.txt")=="English.lproj/Enemy.txt");
    members.pop_back();ResourceArchive missing(saga::test::jar_fixture(members),{"English.lproj",1},ArchiveContents::data_only);
    fails([&]{load_gameplay_catalog(missing,false,parse_event_decimal);});
    ResourceArchive fallback(saga::test::jar_fixture({{"Chara.txt",{42}}}),{"English.lproj",1},ArchiveContents::data_only);
    check(fallback.data("Chara.txt")->member=="Chara.txt");
    std::cout<<"gameplay catalog contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
