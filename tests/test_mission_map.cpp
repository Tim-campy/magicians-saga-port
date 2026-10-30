#include "stage_update_fixture.hpp"
#include "core/gameplay/mission_map.hpp"

template<class T>T& field(ComponentSlots& e,std::int32_t slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p);return *p;}
struct WorldEntryFixture final:MissionWorldEntryHost {
    std::vector<std::string> calls;
    std::string failure;
    std::function<void()> on_factory,on_map;
    MissionSystemBatch batch;
    struct World final:MissionWorldEntryTarget {
        WorldEntryFixture& owner;std::string name;std::uint8_t loaded=0;
        int additions=0,maps=0,inits=0;
        World(WorldEntryFixture& f,std::string label):owner(f),name(std::move(label)){}
        void record(const char* operation){owner.calls.push_back(std::string(operation)+":"+name);if(owner.failure==operation)throw std::runtime_error("entry failure");}
        std::uint8_t entry_loaded_byte()override{record("loaded");return loaded;}
        void add_world_systems(MissionSystemBatch* value)override{++additions;record("add");check(value==&owner.batch);}
        void create_world_map(MapCreation r)override{
            ++maps;record("map");
            check(r.width==3&&r.height==8&&r.size_width==624&&r.size_height==312&&r.step_x==312&&r.step_z==312);
            check(r.area_width==16&&r.area_height==16&&!r.chips);
            if(owner.on_map)owner.on_map();
        }
        void init_world_systems()override{++inits;record("init");}
    } a{*this,"a"},b{*this,"b"};
    World* current=&a;
    int queries=0,null_at=0;
    MissionWorldEntryTarget* entry_world()override{calls.push_back("world");return ++queries==null_at?nullptr:current;}
    MissionSystemBatch* make_world_systems()override{
        calls.push_back("factory");if(failure=="factory")throw std::runtime_error("factory failure");
        if(on_factory)on_factory();return &batch;
    }
};
struct MapPools final:MapFactoryHost {
    UpdateFixture& f;MapComponent map;bool missing=false;std::deque<MapChipComponent> chips;
    std::function<void()> on_chip;
    explicit MapPools(UpdateFixture& fixture):f(fixture){}
    bool registered_id(std::int32_t id)override{return f.registered_id(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{return f.create(type,id);}
    void register_entity(ComponentSlots& e,bool temporary)override{f.register_entity(e,temporary);}
    ContactPosition& acquire_position()override{return f.acquire_position();}
    ProjectileSpeed& acquire_speed()override{return f.acquire_speed();}
    EntitySprite& acquire_sprite()override{return f.acquire_sprite();}
    EntityDepth& acquire_depth()override{return f.acquire_depth();}
    CellComponent& acquire_cell()override{return f.acquire_cell();}
    EntityImage& acquire_image()override{return f.acquire_image();}
    EntityPlayback& acquire_playback()override{return f.acquire_playback();}
    MapChipComponent& acquire_map_chip()override{if(on_chip)on_chip();chips.emplace_back();return chips.back();}
    MapComponent* shared_map_component()override{return missing?nullptr:&map;}
};
struct MissionFixture:UpdateFixture,MissionMapHost,MissionMapCursor {
    MapPools pools{*this};MapFactoryWorld map_factory{ids,pools};MissionMapSystem map_system;
    MissionMapEntry map_entry{map_system,*this};MapComponent* global_map=&pools.map;
    std::vector<std::pair<std::int32_t,std::int32_t>> cursors;
    std::size_t global_queries=0,factory_queries=0;
    bool missing_cursor=false,missing_factory=false;std::function<void()> on_cursor,on_map_factory,on_current_map;
    MissionFixture(){create_map({3,6,288,96,144,96,0,0,nullptr},ids,pools);initialize_mission_map(map_system,*this);
        state.quest->chip_images=std::make_shared<EventIntegers>(EventIntegers{111,222,333,444});}
    MapComponent* current_mission_map()override{++global_queries;if(on_current_map)on_current_map();return global_map;}
    MissionMapCursor* current_map_cursor()override{return missing_cursor?nullptr:this;}
    MapFactoryWorld* current_map_factory()override{++factory_queries;if(on_map_factory)on_map_factory();return missing_factory?nullptr:&map_factory;}
    void set_cursor(std::int32_t x,std::int32_t y)override{cursors.emplace_back(x,y);if(on_cursor)on_cursor();}
    StageMapLoader* current_map_system()override{trace.push_back("map-system");return no_map_system?nullptr:&map_entry;}
    StageGeometry* stage_map()override{++map_queries;if(on_map_query)on_map_query(map_queries);return global_map;}
    ComponentSlots& tile(std::int32_t x,std::int32_t z){return *map_system.map->chips->at(static_cast<std::size_t>(z))->at(static_cast<std::size_t>(x));}
    void load(){load_mission_map(map_system,state.quest,*this);}
};
int main()try{
    {WorldEntryFixture f;initialize_mission_world(f);
     check(f.calls==std::vector<std::string>{"world","loaded:a","factory","add:a","world","map:a","world","init:a"});
     check(f.a.loaded==0&&f.a.additions==1&&f.a.maps==1&&f.a.inits==1);
     initialize_mission_world(f);check(f.a.additions==2&&f.a.maps==2&&f.a.loaded==0);}
    for(const auto loaded:{1,2,255}){WorldEntryFixture f;f.a.loaded=static_cast<std::uint8_t>(loaded);initialize_mission_world(f);
     check(f.calls==std::vector<std::string>{"world","loaded:a","world","map:a","world","init:a"});
     check(f.a.additions==0&&f.a.maps==1&&f.a.inits==1&&f.a.loaded==loaded);}
    {WorldEntryFixture f;f.on_factory=[&]{f.current=&f.b;};f.on_map=[&]{f.current=&f.a;};initialize_mission_world(f);
     check(f.a.additions==1&&f.b.additions==0&&f.a.maps==0&&f.b.maps==1&&f.a.inits==1&&f.b.inits==0);
     check(f.calls==std::vector<std::string>{"world","loaded:a","factory","add:a","world","map:b","world","init:a"});}
    for(const auto failure:{"factory","add","map","init"}){WorldEntryFixture f;f.failure=failure;fails([&]{initialize_mission_world(f);});
     const std::vector<std::string> full{"world","loaded:a","factory","add:a","world","map:a","world","init:a"};
     const auto end=std::string(failure)=="factory"?3U:std::string(failure)=="add"?4U:std::string(failure)=="map"?6U:8U;
     check(f.calls==std::vector<std::string>(full.begin(),full.begin()+end));
     check(f.a.loaded==0&&f.a.inits==(std::string(failure)=="init"?1:0));}
    for(int query=1;query<=3;++query){WorldEntryFixture f;f.null_at=query;fails([&]{initialize_mission_world(f);});
     check(f.a.additions==(query>1?1:0)&&f.a.maps==(query>2?1:0)&&f.a.inits==0);}
    {MissionFixture f;check(quest_map_width()==3&&quest_map_start_image(*f.state.quest)==111&&quest_map_middle_image(*f.state.quest)==222);
     check(quest_map_end_image(*f.state.quest)==333&&quest_map_background_image(*f.state.quest)==444);
     f.state.quest->chip_images.reset();fails([&]{quest_map_start_image(*f.state.quest);});
     f.state.quest->chip_images=std::make_shared<EventIntegers>(EventIntegers{1});fails([&]{quest_map_background_image(*f.state.quest);});}
    // Map creation retains the ComponentFactory singleton and its unmodified areas reference.
    {MissionFixture f;auto& map=f.pools.map;auto areas=make_map_grid(1,1);map.areas=areas;
     auto explicit_grid=make_map_grid(2,1);auto& a=create_map({7,8,9,10,11,12,13,14,explicit_grid},f.ids,f.pools);
     check(a.contains(11)&&&field<MapComponent>(a,11)==&map&&map.width==7&&map.height==8&&map.chips==explicit_grid&&map.areas==areas);
     check(map.size_width==9&&map.size_height==10&&map.step_x==11&&map.step_z==12&&map.area_width==13&&map.area_height==14);
     auto& b=create_map({3,2,1,2,3,4,5,6,nullptr},f.ids,f.pools);
     check(&field<MapComponent>(b,11)==&map&&field<MapComponent>(a,11).width==3&&map.chips->size()==2&&map.chips->at(0)->size()==3);
     check(map.chips->at(0)!=map.chips->at(1)&&!map.chips->at(1)->at(2));}
    {MissionFixture f;const auto count=f.nodes.size();fails([&]{create_map({3,-1,0,0,0,0,0,0,nullptr},f.ids,f.pools);});
     check(f.nodes.size()==count+1&&f.registry.contains(f.nodes.back().id)&&!f.nodes.back().entity.contains(11));
     f.pools.missing=true;fails([&]{create_map({0,0,0,0,0,0,0,0,nullptr},f.ids,f.pools);});check(!f.nodes.back().entity.contains(11));}
    // Actual map-chip payloads, slot order, explicit Cell coordinates and optional playback.
    {MissionFixture f;std::vector<std::int32_t> slots;f.on_added=[&](ComponentSlots&,std::int32_t slot){slots.push_back(slot);};
     auto& e=create_map_chip({2,3,-7,18,5,9,4,77,144,96,true,123,-8},f.ids,f.pools);
     check(slots==EventIntegers{0,5,2,7,4,9,10}&&!e.contains(1));
     auto& p=field<ContactPosition>(e,0);check(p.x==288&&p.y==-7&&p.z==288&&p.offset_x==0&&!p.parent);
     auto& c=field<CellComponent>(e,5);check(c.xi==2&&c.yi==3&&!c.parent&&c.children&&c.children->empty());
     auto& s=field<EntitySprite>(e,2);check(s.animation.resource==18&&s.animation.clip==5&&s.animation.frame==4&&*s.layers==EventIntegers{9});
     auto& image=field<EntityImage>(e,7);check(*image.arrays.textures==EventIntegers{77}&&*image.arrays.resources==EventIntegers{-1});
     check(field<EntityDepth>(e,4).value==0&&field<EntityDepth>(e,4).extra==-8);
     auto& chip=field<MapChipComponent>(e,9);check(chip.data_id==123&&chip.staff_id==-1&&*chip.visiting_human_ids==EventIntegers(5,-1));
     check(field<EntityPlayback>(e,10).state.rate==1&&field<EntityPlayback>(e,10).state.return_clip==-1);}
    {MissionFixture f;auto& e=create_map_chip({std::numeric_limits<std::int32_t>::max(),-2,0,0,0,0,0,-1,2,3,false,0,0},f.ids,f.pools);
     check(field<ContactPosition>(e,0).x==-2&&field<ContactPosition>(e,0).z==-6&&!e.contains(10));
     check(field<CellComponent>(e,5).xi==std::numeric_limits<std::int32_t>::max()&&field<CellComponent>(e,5).yi==-2);}
    {MissionFixture f;f.pools.on_chip=[](){throw std::runtime_error("chip pool");};
     fails([&]{create_map_chip({},f.ids,f.pools);});auto& e=f.nodes.back().entity;
     check(e.contains(0)&&e.contains(4)&&e.contains(7)&&!e.contains(9)&&!e.contains(10));}
    // Range checks accept original zero/negative footprint quirks and int32 wrapping.
    {MissionFixture f;auto* m=&f.pools.map;check(map_location_in_range(m,2,5,1,1,f));
     check(!map_location_in_range(m,3,5,1,1,f)&&!map_location_in_range(m,-1,0,1,1,f));
     check(map_location_in_range(m,3,6,0,0,f)&&map_location_in_range(m,4,7,-1,-1,f));
     check(map_location_in_range(m,std::numeric_limits<std::int32_t>::max(),0,2,1,f));
     auto calls=f.global_queries;check(map_location_in_range(nullptr,0,0,1,1,f)&&f.global_queries==calls+1);
     f.global_map=nullptr;check(!map_location_in_range(nullptr,-1,0,1,1,f));fails([&]{map_location_in_range(nullptr,0,0,1,1,f);});}
    {MissionFixture f;auto* m=&f.pools.map;auto& e=f.nodes.front().entity;set_mission_map_chip(m,1,2,&e,f);
     check(&f.tile(1,2)==&e);set_mission_map_chip(m,1,2,nullptr,f);check(!m->chips->at(2)->at(1));
     m->chips.reset();set_mission_map_chip(m,-1,0,&e,f);fails([&]{set_mission_map_chip(m,0,0,&e,f);});
     m->chips=make_map_grid(1,1);fails([&]{set_mission_map_chip(m,1,0,&e,f);});
     m->chips->at(0).reset();fails([&]{set_mission_map_chip(m,0,0,&e,f);});
     // Null uses the global map only for validation; a valid location still requires the original argument.
     fails([&]{set_mission_map_chip(nullptr,0,0,&e,f);});}
    {MissionFixture f;f.global_map=nullptr;initialize_mission_map(f.map_system,f);check(!f.map_system.map);
     fails([&]{f.load();});f.map_system.map=&f.pools.map;
     fails([&]{load_mission_map(f.map_system,nullptr,f);});}
    // 3x6 map: final row uses start, first row end, intermediate edges background and center middle.
    {MissionFixture f;const auto before=f.nodes.size();auto old=f.pools.map.chips;f.load();auto& m=f.pools.map;
     check(m.width==3&&m.height==6&&m.chips!=old&&f.nodes.size()==before+18&&f.factory_queries==18);
     check(f.cursors==std::vector<std::pair<std::int32_t,std::int32_t>>{{-576,192}});
     for(std::int32_t z=0;z<6;++z)for(std::int32_t x=0;x<3;++x){auto& e=f.tile(x,z);
         const auto expected=z==5?111:z==0?333:x==1?222:444;
         check(field<EntityImage>(e,7).arrays.textures->at(0)==expected&&field<EntitySprite>(e,2).animation.resource==18);
         check(field<ContactPosition>(e,0).x==static_cast<float>(x*144)&&field<ContactPosition>(e,0).z==static_cast<float>(z*96));
         const auto depth=z<5&&x<=1?-300+(x-2)*(5-z):0;
         check(field<EntityDepth>(e,4).extra==depth&&!e.contains(1)&&!e.contains(10)&&field<MapChipComponent>(e,9).data_id==0);}
     auto* first=&f.tile(0,0);f.load();check(f.registry.contains(f.node(*first).id)&&f.nodes.size()==before+36&&&f.tile(0,0)!=first);}
    // The selected form is required after publishing dimensions/grid, before creating tiles.
    {MissionFixture f;auto old=f.pools.map.chips;f.missing_cursor=true;fails([&]{f.load();});
     check(f.pools.map.width==3&&f.pools.map.height==6&&f.pools.map.chips!=old&&f.factory_queries==0);}
    {MissionFixture f;f.state.quest->distance=-3;auto old=f.pools.map.chips;fails([&]{f.load();});
     check(f.pools.map.width==3&&f.pools.map.height==-1&&f.pools.map.chips==old&&f.cursors.empty());}
    {MissionFixture f;f.state.quest->distance=-2;f.state.quest->chip_images.reset();f.load();
     check(f.pools.map.chips->empty()&&f.factory_queries==0&&f.cursors.back()==std::pair<std::int32_t,std::int32_t>{288,-96});}
    {MissionFixture f;f.state.quest->distance=-1;f.state.quest->chip_images=std::make_shared<EventIntegers>(EventIntegers{77});f.load();
     check(f.pools.map.chips->size()==1&&field<EntityImage>(f.tile(1,0),7).arrays.textures->at(0)==77&&field<EntityDepth>(f.tile(0,0),4).extra==0);}
    // Short image arrays fail only upon the first reached branch, preserving earlier tiles.
    {MissionFixture f;f.state.quest->chip_images=std::make_shared<EventIntegers>(EventIntegers{111,222,333});fails([&]{f.load();});
     check(f.factory_queries==3&&f.pools.map.chips->at(0)->at(2)&&!f.pools.map.chips->at(1)->at(0));}
    {MissionFixture f;f.missing_factory=true;fails([&]{f.load();});check(f.factory_queries==1&&!f.pools.map.chips->at(0)->at(0));}
    // Dimensions/quest receiver are captured, but images and geometry are live after cursor/entity callbacks.
    {MissionFixture f;auto original=f.state.quest;f.on_cursor=[&]{original->distance=0;original->chip_images->at(2)=555;
         f.state.quest=std::make_shared<StageQuest>();};
     f.on_added=[&](ComponentSlots&,std::int32_t slot){if(slot==9){f.pools.map.step_x=200;original->chip_images->at(2)=666;}};
     load_mission_map(f.map_system,original,f);
     check(f.pools.map.chips->size()==6&&field<EntityImage>(f.tile(0,0),7).arrays.textures->at(0)==555);
     check(field<EntityImage>(f.tile(1,0),7).arrays.textures->at(0)==666&&field<ContactPosition>(f.tile(1,0),0).x==200);}
    {MissionFixture f;MapComponent other;other.width=1;other.height=1;other.chips=make_map_grid(1,1);other.step_x=7;other.step_z=9;
     bool replaced=false;f.on_added=[&](ComponentSlots&,std::int32_t slot){if(slot==9&&!replaced){replaced=true;f.map_system.map=&other;}};
     f.load();check(f.factory_queries==18&&other.chips->at(0)->at(0)&&!f.pools.map.chips->at(0)->at(0));
     check(field<ContactPosition>(f.nodes.back().entity,0).x==14&&field<ContactPosition>(f.nodes.back().entity,0).z==45);}
    // StageSystem.Start now reaches the actual loader and full enemy entry through production-shaped adapters.
    {MissionFixture f;f.begin();check(f.pools.map.chips->size()==6&&f.cursors.size()==1&&f.enemy_system.subset->size()==1);
     auto* hero=member_hero(&f.world);check(field<ContactPosition>(*hero,0).x==144&&field<ContactPosition>(*hero,0).z==408);
     f.enemy_system.loop_counter=1;f.random.index=0;f.random.values={0,1};update_enemies(f.enemy_system,f);
     auto& e=f.nodes.back().entity;check(field<EnemyRecordLink>(e,24).state==1&&field<EnemyRecordLink>(e,24).frame==1);
     check(field<ContactPosition>(e,0).z==216&&!e.contains(15)&&f.random.index==2);}
    // Exact map configuration passed by AppData.InitWorld, with the same real mission-entry chain.
    {MissionFixture f;create_map(initial_mission_map_creation(),f.ids,f.pools);auto& map=f.pools.map;
     check(map.width==3&&map.height==8&&map.size_width==624&&map.size_height==312&&map.area_width==16&&map.area_height==16);
     f.begin();check(map.height==6&&map.step_x==312&&map.step_z==312&&f.cursors.back()==std::pair<std::int32_t,std::int32_t>{-1248,624});
     check(field<ContactPosition>(*member_hero(&f.world),0).z==1164&&field<ContactPosition>(f.tile(2,5),0).z==1560);
     f.enemy_system.loop_counter=1;f.random.index=0;f.random.values={0,1};update_enemies(f.enemy_system,f);
     check(field<ContactPosition>(f.nodes.back().entity,0).z==540&&!f.nodes.back().entity.contains(15));}
}catch(const std::exception& e){std::cerr<<"uncaught mission map error: "<<e.what()<<'\n';return 1;}
