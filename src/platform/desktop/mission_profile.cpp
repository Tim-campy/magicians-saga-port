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
}
SceneResource::SceneResource(const std::filesystem::path& path,ResourceProfile profile,ArchiveImageSet::Decoder decoder)
    :archive(read(path),std::move(profile)),manager(archive){
    if(decoder)images=std::make_unique<ArchiveImageSet>(manager,std::move(decoder));
}
void MissionProfile::bind_schedule(){
    const auto bind=[&](std::int32_t id,bool enabled,std::function<void(std::int32_t)> body){
        const auto index=static_cast<std::size_t>(id);configured_[index]=enabled;
        updates_[index]=std::make_unique<GameplayUpdate>(id,enabled_[index],[this,id,body=std::move(body)](std::int32_t pass){last_updates_.push_back(id);body(pass);});
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
    bind(17,static_cast<bool>(spatial),[this](auto pass){spatial->advance_cells(pass);});
    bind(18,simulate.has_value(),[this](auto pass){mission->advance_modifications(pass);});
    bind(19,true,[this](auto){mission->advance_animations({automatic!=0,application_update!=0});});
    bind(20,static_cast<bool>(spatial),[this](auto pass){spatial->advance_visibility(pass);});
    bind(21,true,[this](auto){queue=mission->render();if(!mission->faults().empty())throw std::invalid_argument("entity render fault: "+mission->faults().front().message);});
    bind(23,true,[this](auto pass){mission->advance_effects(pass);});
    bind(31,simulate.has_value(),[this](auto pass){mission->advance_garbage(pass);});
    for(const auto id:gameplay_update_order)if(updates_[static_cast<std::size_t>(id)])update_list_.push_back(updates_[static_cast<std::size_t>(id)].get());
    last_updates_.reserve(update_list_.size());
}
void MissionProfile::load_replay(const std::filesystem::path& path){
    if(std::filesystem::file_size(path)>8U*1024U*1024U)throw std::length_error("input replay exceeds8MiB");
    std::ifstream file(path);if(!file)throw std::invalid_argument("cannot read input replay");
    std::string line;std::uint64_t previous=0,count=0;
    while(std::getline(file,line)){
        std::istringstream row(line);row.imbue(std::locale::classic());row>>std::ws;if(row.eof()||row.peek()=='#')continue;
        std::string frame,kind,value;if(!(row>>frame>>kind>>value))throw std::invalid_argument("replay requires step keys|attack|pulse value");end(row);
        std::uint64_t step=0;std::uint32_t argument=0;
        const auto a=std::from_chars(frame.data(),frame.data()+frame.size(),step),b=std::from_chars(value.data(),value.data()+value.size(),argument);
        if(a.ec!=std::errc{}||a.ptr!=frame.data()+frame.size()||!step||step<previous||b.ec!=std::errc{}||b.ptr!=value.data()+value.size()||
           (kind!="keys"&&kind!="attack"&&kind!="pulse")||(kind=="attack"&&argument>2)||++count>100000)throw std::invalid_argument("invalid or unordered input replay action");
        replay_[step].push_back({kind=="keys"?InputKind::keys:kind=="attack"?InputKind::attack:InputKind::pulse,argument});previous=step;
    }replay_bound_=true;
}
MissionProfile::MissionProfile(const std::filesystem::path& scene,bool native,ArchiveImageSet::Decoder decoder,std::optional<MissionRuntimeConfig> config,std::optional<std::filesystem::path> replay):runtime(std::move(config)){
    if(replay)load_replay(*replay);
    std::ifstream input(scene);if(!input)throw std::runtime_error("cannot open scene description");
    std::vector<std::variant<HumanCreation,EnemyCreation,SelectedBullet,SelectedEffect,SelectedDrop>> selected;
    std::optional<MissionHumanScenario> human_scenario;bool runtime_row=false;
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
    std::set<std::string> quest_fields;
    std::int32_t rescued_prisoners=0;
    for(std::string line;std::getline(input,line);){
        std::istringstream row(line);row.imbue(std::locale::classic());std::string operation;
        if(!(row>>operation)||operation.front()=='#')continue;
        if(operation=="quest"){
            std::int32_t id=0;
            if(!native||selected_quest||!(row>>id>>rescued_prisoners)||id<0)throw std::invalid_argument("quest requires one nonnegative ID and explicit saved rescued count");
            selected_quest=id;
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
            if(!native||!(row>>texture>>shader>>material_queue>>id>>uniform)||texture<0||(uniform!=0&&uniform!=1))throw std::invalid_argument("material requires texture shader queue identity uniform0/1 in native mode");
            auto inserted=materials.emplace(id,SubmissionMaterial{});auto& value=inserted.first->second;
            if(!inserted.second&&(value.queue.absolute!=material_queue||value.uniform_color!=(uniform!=0)))throw std::invalid_argument("conflicting material identity");
            value.queue.absolute=material_queue;value.uniform_color=uniform!=0;
            if(!material_bindings.emplace(std::make_tuple(static_cast<std::size_t>(texture),shader,material_queue),id).second)throw std::invalid_argument("duplicate material input binding");
        }else if(operation=="tables"){
            std::string path,language;std::int32_t dpi=0;
            if(!native||tables||!(row>>std::quoted(path)>>std::quoted(language)>>dpi))throw std::invalid_argument("tables requires native mode and one quoted archive/language/DPI profile");
            auto resolved=std::filesystem::u8path(path);if(resolved.is_relative())resolved=scene.parent_path()/resolved;
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
        }else if(operation=="human"){
            HumanCreation h;if(!native||!(row>>h.direction>>h.character>>h.level>>h.formation_x>>h.formation_z>>h.experience>>h.x>>h.y>>h.z))throw std::invalid_argument("human requires six int32 and XYZ binary32 inputs");selected.emplace_back(h);
        }else if(operation=="enemy"){
            EnemyCreation e;if(!native||!(row>>e.direction>>e.id>>e.level>>e.flags>>e.position.x>>e.position.y>>e.position.z))throw std::invalid_argument("enemy requires direction ID level flags and XYZ");selected.emplace_back(e);
        }else if(operation=="bullet"){
            SelectedBullet b;if(!native||!(row>>b.id>>b.damage>>b.attribute>>b.parent>>b.position.x>>b.position.y>>b.position.z))throw std::invalid_argument("bullet requires ID damage attribute parent ordinal and XYZ");selected.emplace_back(b);
        }else if(operation=="drop"){
            SelectedDrop d;if(!native||!(row>>d.type>>d.value>>d.scale>>d.position.x>>d.position.y>>d.position.z))throw std::invalid_argument("drop requires type value scale and XYZ");selected.emplace_back(d);drop_catalog=true;
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
            auto resolved=std::filesystem::u8path(path);if(resolved.is_relative())resolved=scene.parent_path()/resolved;
            auto value=std::make_unique<SceneResource>(resolved,ResourceProfile{language,dpi},decoder);if(value->images)textures.bind(*value->images);
            *slot=&value->manager.drawing();owners.push_back(std::move(value));
        }else if(operation=="draw"){
            if(commands>=100000)throw std::length_error("capture command budget exceeded");
            RenderCommand c;for(auto& v:c.values)if(!(row>>v))throw std::invalid_argument("draw requires nineteen int32 fields");
            insert_render_command(queue,c);++commands;
        }else throw std::invalid_argument("unknown scene row type");
        end(row);
    }
    if(!has_world)throw std::invalid_argument("an explicit world matrix is required");
    const bool result_requested=wallet||record||recruits||stocks[0]||stocks[1]||stocks[2];
    if(!quest_fields.empty()&&!selected_quest)throw std::invalid_argument("quest source requires a quest selection row");
    if(selected_quest){
        if(!tables||selected.empty()||!human_scenario||!notifications)throw std::invalid_argument("quest requires tables, selected actors, human_system and stage_notifications");
        for(const auto* required:{"human_system","stage_notifications"})if(!quest_fields.count(required))throw std::invalid_argument("selected quest requires quest source for static fields");
        for(const auto& field:std::vector<std::pair<bool,const char*>>{{enemy_config,"enemy_system"},{stage_events,"stage_events"},{stage_scenario.boss.has_value(),"stage_boss"},{static_cast<bool>(map_images),"mission_map"},{geometry.has_value(),"geometry"}})
            if(field.first&&!quest_fields.count(field.second))throw std::invalid_argument("selected quest conflicts with explicit static scenario fields");
        if(enemy_pool)throw std::invalid_argument("selected quest supplies enemy_pool; remove explicit override");
        catalog=std::make_unique<GameplayCatalog>(load_gameplay_catalog(*tables,false,parse_event_decimal,rewards.has_value()||collection||drop_catalog,result_requested,true));
        const auto& definition=catalog->quests->at(static_cast<std::size_t>(*selected_quest));
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
    if(enemy_gauges&&(!rewards||!get_application_resource(application,23)))throw std::invalid_argument("enemy_gauges requires explicit rewards saved counts and resource23");
    if(collisions&&!human_scenario)throw std::invalid_argument("collisions requires human_system event receivers");
    if(notifications&&!human_scenario)throw std::invalid_argument("stage_notifications requires human_system");
    if(collection&&(!notifications||!human_scenario))throw std::invalid_argument("collection fresh requires stage_notifications and human_system");
    if(result_requested&&!(wallet&&record&&recruits&&stocks[0]&&stocks[1]&&stocks[2]&&stage_loop&&collection))throw std::invalid_argument("results require wallet/record/all inventory kinds/recruits, stage_loop and collection fresh");
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
    if(!selected.empty()){
        if(!tables||!has_view||!has_slots||!has_animation||commands)throw std::invalid_argument("entity capture needs tables/view/slots/animate and no manual draw rows");
        if(!catalog)catalog=std::make_unique<GameplayCatalog>(load_gameplay_catalog(*tables,false,parse_event_decimal,rewards.has_value()||collection||drop_catalog,result_requested));
        for(const auto& entry:gear_defaults){
            const auto& data=entry.first=="equipment"?catalog->equipment:entry.first=="hat"?catalog->hats:catalog->cloth;
            auto& levels=entry.first=="equipment"?progress.equipment_levels:entry.first=="hat"?progress.hat_levels:progress.cloth_levels;
            for(std::size_t i=0;i<data.size();++i)if(data[i])levels.emplace(static_cast<std::int32_t>(i),entry.second);
        }
        entity_world=std::make_unique<MissionEntities>(*catalog,progress,slots,geometry?&*geometry:nullptr,[&](std::int32_t id){auto* r=get_application_resource(application,id);return r?r->animations:nullptr;},
            [&](std::string_view name){auto it=resource_names.find(std::string(name));if(it==resource_names.end())throw std::invalid_argument("unbound enemy resource name: "+std::string(name));return it->second;});
        MapComponent* live_map=map_images?&entity_world->create_initial_map():nullptr;
        for(const auto& request:selected){ComponentSlots* entity=nullptr;
            if(const auto* h=std::get_if<HumanCreation>(&request))entity=&entity_world->human(*h);
            else if(const auto* e=std::get_if<EnemyCreation>(&request))entity=entity_world->enemy(*e);
            else if(const auto* b=std::get_if<SelectedBullet>(&request)){ComponentSlots* parent=nullptr;
                if(b->parent!=-1)parent=created.at(static_cast<std::size_t>(b->parent));
                entity=&entity_world->bullet({b->id,b->damage,b->attribute,parent,b->position});}
            else if(const auto* d=std::get_if<SelectedDrop>(&request))entity=entity_world->drop(d->type,d->value,d->position,d->scale);
            else{const auto& fx=std::get<SelectedEffect>(request);entity=&entity_world->effect(fx.position,fx.clip,fx.image,fx.loop);}
            if(!entity)throw std::invalid_argument("selected entity record is unavailable");created.push_back(entity);
        }
        mission=std::make_unique<MissionScene>(*entity_world,application,*entity_world->subset({0,2}),view,live_map);
        for(const auto ordinal:players){auto& actor=*created.at(ordinal);if(!actor.contains(14))throw std::invalid_argument("selected player needs Human component");entity_world->mark_player(actor);}
        if(human_scenario){
            if(!runtime||!geometry||!simulate)throw std::invalid_argument("human_system requires runtime, geometry and simulate rows");
            humans=std::make_unique<MissionHumans>(*entity_world,*catalog,progress,live_map?static_cast<StageGeometry&>(*live_map):*geometry,*runtime,*human_scenario);
            if(rewards)humans->enable_rewards(*rewards);
            if(enemy_requested)humans->enable_enemies(std::move(enemy));
            if(notifications)humans->enable_notifications(*notifications);
            if(collection)humans->enable_collection();
            if(result_requested)humans->enable_results(std::move(result_state));
            if(stage_loop)humans->enable_stage_loop(std::move(stage_scenario));
            if(input_scenario)humans->enable_input(*input_scenario);
            if(map_images){spatial=std::make_unique<MissionSpatial>(*entity_world,*live_map,view,humans->quest_form(),humans->quest(),map_images);mission->bind_spatial(*spatial);}
            for(const auto& request:states)humans->set_state(*created.at(std::get<0>(request)),std::get<1>(request),std::get<2>(request));
        }else if(!states.empty())throw std::invalid_argument("actor_state requires human_system");
        bind_schedule();
        for(std::int32_t i=0;i<frames;++i)mission->advance_animations({automatic!=0,application_update!=0});
        if(spatial)spatial->advance_visibility();
        if(simulate&&*simulate>0){for(std::int32_t i=0;i<*simulate;++i)if(!step())break;}else{queue=mission->render();if(!mission->faults().empty())throw std::invalid_argument("entity render fault: "+mission->faults().front().message);}
        commands=static_cast<std::size_t>(queue.counts->at(0))+static_cast<std::size_t>(queue.counts->at(1));
    }
    if(simulate&&!mission)throw std::invalid_argument("simulate requires a table-backed entity scene");
    if(rewards&&!humans)throw std::invalid_argument("rewards requires human_system");
    if((human_scenario||!states.empty()||!players.empty()||!gear_defaults.empty())&&!mission)throw std::invalid_argument("human scenario rows require a table-backed entity scene");
    selected_count=selected.size();
}
bool MissionProfile::step(){
    if(!mission)throw std::invalid_argument("selected logic requires a table-backed entity scene");
    if(humans&&humans->transition_pending())return false;
    const auto input=replay_.find(logic_steps+1);
    if(input!=replay_.end())for(const auto& action:input->second){
        if(action.kind==InputKind::keys)humans->set_input_keys(action.value);
        else if(action.kind==InputKind::attack)humans->select_input_attack(static_cast<std::int32_t>(action.value));
        else humans->pulse_stage_keys(action.value);
        ++replay_applied_;
    }
    enabled_=configured_;last_updates_.clear();halted_=false;
    UpdateSchedule schedule{&update_list_,&timer_};update_systems(schedule,0);
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
    if(enemy_gauges)mission->draw_enemy_gauges([&](const ParameterGaugeRequest& r){backend.draw_parameter_gauge(r,mission->gauge_camera_host());});
}
} // namespace saga::desktop
