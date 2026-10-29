#include "mission_profile.hpp"
#include "assets/jar_archive.hpp"
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <variant>
#include <cmath>

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
}
SceneResource::SceneResource(const std::filesystem::path& path,ResourceProfile profile,ArchiveImageSet::Decoder decoder)
    :archive(read(path),std::move(profile)),manager(archive){
    if(decoder)images=std::make_unique<ArchiveImageSet>(manager,std::move(decoder));
}
MissionProfile::MissionProfile(const std::filesystem::path& scene,bool native,ArchiveImageSet::Decoder decoder,std::optional<MissionRuntimeConfig> config):runtime(std::move(config)){
    std::ifstream input(scene);if(!input)throw std::runtime_error("cannot open scene description");
    std::vector<std::variant<HumanCreation,EnemyCreation,SelectedBullet,SelectedEffect>> selected;
    std::optional<MissionHumanScenario> human_scenario;bool runtime_row=false;
    std::optional<MissionRewardScenario> rewards;
    MissionEnemyScenario enemy{};bool enemy_config=false,enemy_population=false,enemy_pool=false;
    std::map<std::string,std::int32_t> gear_defaults;
    std::vector<std::size_t> players;
    std::vector<std::tuple<std::size_t,std::int32_t,float>> states;
    for(std::string line;std::getline(input,line);){
        std::istringstream row(line);row.imbue(std::locale::classic());std::string operation;
        if(!(row>>operation)||operation.front()=='#')continue;
        if(operation=="runtime"){
            MissionRuntimeConfig value;std::int32_t debug=0;
            if(!native||runtime_row||!(row>>value.fps>>debug>>value.backend>>value.seed)||value.fps<1||value.fps>1000||(debug!=0&&debug!=1)||value.backend!="system")throw std::invalid_argument("runtime requires FPS1..1000 DEBUG0/1 system seed-int32");
            value.debug=debug!=0;runtime_row=true;
            if(runtime&&(runtime->fps!=value.fps||runtime->debug!=value.debug||runtime->backend!=value.backend||runtime->seed!=value.seed))throw std::invalid_argument("scene runtime conflicts with CLI inputs");runtime=value;
        }else if(operation=="rewards"){
            MissionRewardScenario value;if(!native||rewards||!(row>>value.defeated>>value.remaining>>value.saved_defeat_count))throw std::invalid_argument("rewards requires initial defeated/remaining/saved defeat counts");rewards=value;
        }else if(operation=="enemy_system"){
            if(!native||enemy_config||!(row>>enemy.yugami>>enemy.maximum>>enemy.total>>enemy.interval>>enemy.level))throw std::invalid_argument("enemy_system requires yugami maximum total interval base level");enemy_config=true;
        }else if(operation=="enemy_population"){
            std::int32_t boss=0;
            if(!native||enemy_population||!(row>>enemy.created>>enemy.defeated>>enemy.remaining>>enemy.loop_counter>>boss>>enemy.section_limit>>enemy.section_created)||(boss!=0&&boss!=1))throw std::invalid_argument("enemy_population requires created defeated remaining loop boss0/1 uniform section limit/count");enemy.boss_defeated=boss!=0;enemy_population=true;
        }else if(operation=="enemy_pool"){
            std::int32_t count=0;if(!native||enemy_pool||!(row>>count)||count<0||count>1000)throw std::invalid_argument("enemy_pool requires count0..1000 followed by enemy ID/weight pairs");
            for(std::int32_t i=0;i<count;++i){std::int32_t id=0,weight=0;if(!(row>>id>>weight))throw std::invalid_argument("enemy_pool requires every ID/weight pair");enemy.pool.emplace_back(id,weight);}enemy_pool=true;
        }else if(operation=="human_system"){
            MissionHumanScenario value;if(!native||human_scenario||!(row>>value.distance>>value.min_z_rate>>value.max_z_rate>>value.attribute>>value.button))throw std::invalid_argument("human_system requires distance min/max Z rates attribute button state");human_scenario=value;
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
            StageGeometry g;if(!native||geometry||!(row>>g.step_x>>g.step_z>>g.height))throw std::invalid_argument("geometry requires step X/Z and height in native mode");geometry=g;
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
    const bool enemy_requested=enemy_config||enemy_population||enemy_pool;
    if(enemy_requested&&!(enemy_config&&enemy_population&&enemy_pool&&human_scenario))throw std::invalid_argument("enemy system requires enemy_system/enemy_population/enemy_pool and human_system");
    if(!selected.empty()){
        if(!tables||!has_view||!has_slots||!has_animation||commands)throw std::invalid_argument("entity capture needs tables/view/slots/animate and no manual draw rows");
        catalog=std::make_unique<GameplayCatalog>(load_gameplay_catalog(*tables,false,parse_event_decimal,rewards.has_value()));
        for(const auto& entry:gear_defaults){
            const auto& data=entry.first=="equipment"?catalog->equipment:entry.first=="hat"?catalog->hats:catalog->cloth;
            auto& levels=entry.first=="equipment"?progress.equipment_levels:entry.first=="hat"?progress.hat_levels:progress.cloth_levels;
            for(std::size_t i=0;i<data.size();++i)if(data[i])levels.emplace(static_cast<std::int32_t>(i),entry.second);
        }
        entity_world=std::make_unique<MissionEntities>(*catalog,progress,slots,geometry?&*geometry:nullptr,[&](std::int32_t id){auto* r=get_application_resource(application,id);return r?r->animations:nullptr;},
            [&](std::string_view name){auto it=resource_names.find(std::string(name));if(it==resource_names.end())throw std::invalid_argument("unbound enemy resource name: "+std::string(name));return it->second;});
        for(const auto& request:selected){ComponentSlots* entity=nullptr;
            if(const auto* h=std::get_if<HumanCreation>(&request))entity=&entity_world->human(*h);
            else if(const auto* e=std::get_if<EnemyCreation>(&request))entity=entity_world->enemy(*e);
            else if(const auto* b=std::get_if<SelectedBullet>(&request)){ComponentSlots* parent=nullptr;
                if(b->parent!=-1)parent=created.at(static_cast<std::size_t>(b->parent));
                entity=&entity_world->bullet({b->id,b->damage,b->attribute,parent,b->position});}
            else{const auto& fx=std::get<SelectedEffect>(request);entity=&entity_world->effect(fx.position,fx.clip,fx.image,fx.loop);}
            if(!entity)throw std::invalid_argument("selected entity record is unavailable");created.push_back(entity);
        }
        mission=std::make_unique<MissionScene>(*entity_world,application,*entity_world->subset({0,2}),view,nullptr);
        for(const auto ordinal:players){auto& actor=*created.at(ordinal);if(!actor.contains(14))throw std::invalid_argument("selected player needs Human component");entity_world->mark_player(actor);}
        if(human_scenario){
            if(!runtime||!geometry||!simulate)throw std::invalid_argument("human_system requires runtime, geometry and simulate rows");
            humans=std::make_unique<MissionHumans>(*entity_world,*catalog,progress,*geometry,*runtime,*human_scenario);
            if(rewards)humans->enable_rewards(*rewards);
            if(enemy_requested)humans->enable_enemies(std::move(enemy));
            for(const auto& request:states)humans->set_state(*created.at(std::get<0>(request)),std::get<1>(request),std::get<2>(request));
        }else if(!states.empty())throw std::invalid_argument("actor_state requires human_system");
        for(std::int32_t i=0;i<frames;++i)mission->advance_animations({automatic!=0,application_update!=0});
        if(simulate&&*simulate>0)for(std::int32_t i=0;i<*simulate;++i)step();else{queue=mission->render();if(!mission->faults().empty())throw std::invalid_argument("entity render fault: "+mission->faults().front().message);}
        commands=static_cast<std::size_t>(queue.counts->at(0))+static_cast<std::size_t>(queue.counts->at(1));
    }
    if(simulate&&!mission)throw std::invalid_argument("simulate requires a table-backed entity scene");
    if(rewards&&!humans)throw std::invalid_argument("rewards requires human_system");
    if((human_scenario||!states.empty()||!players.empty()||!gear_defaults.empty())&&!mission)throw std::invalid_argument("human scenario rows require a table-backed entity scene");
    selected_count=selected.size();
}
void MissionProfile::step(){
    if(!mission)throw std::invalid_argument("selected logic requires a table-backed entity scene");
    if(humans){humans->update();humans->advance_enemies();humans->advance_projectiles();}
    if(simulate)mission->advance_movement(0);
    if(simulate)mission->advance_emitters(0);
    if(simulate)mission->advance_modifications(0);
    mission->advance_animations({automatic!=0,application_update!=0});queue=mission->render();
    if(!mission->faults().empty())throw std::invalid_argument("entity render fault: "+mission->faults().front().message);
    if(simulate)mission->advance_garbage(0);
}
void MissionProfile::write_actor_parameters(ComponentSlots& actor,std::ostream& out){
    if(actor.contains(23)){
        out<<",\"hp\":"<<resolve_combat_parameter(actor,0)->value;
        if(actor.contains(14))out<<",\"mp\":"<<resolve_combat_parameter(actor,1)->value;
    }
    if(const auto* enemy=dynamic_cast<EnemyRecordLink*>(actor.get(24)))out<<",\"enemy_state\":"<<enemy->state<<",\"enemy_frame\":"<<enemy->frame<<",\"enemy_flags\":"<<enemy->flags;
}
void MissionProfile::write_combat_report(std::ostream& out){
    out<<",\"shots\":"<<(humans?humans->shots():0)<<",\"total_damage\":"<<(humans?humans->total_damage():0)<<",\"projectiles\":[";
    bool first=true;
    if(entity_world){auto cursor=entity_world->subset({27,0,1})->cursor();while(const auto e=cursor.next()){
        const auto* p=dynamic_cast<ContactPosition*>((*e)->get(0));const auto* flight=dynamic_cast<ProjectileLink*>((*e)->get(27));
        if(!p||!flight)throw std::invalid_argument("missing diagnostic projectile fields");
        if(!first)out<<',';first=false;
        out<<"{\"id\":"<<entity_world->id(**e)<<",\"x\":"<<p->x<<",\"y\":"<<p->y<<",\"z\":"<<p->z<<",\"frame\":"<<flight->frame<<",\"max_frame\":"<<flight->max_frame<<'}';
    }}
    out<<']';
    if(humans){humans->write_reward_report(out);humans->write_enemy_report(out);}
    out<<",\"pending_drops\":[";first=true;
    if(entity_world){auto cursor=entity_world->subset({34})->cursor();while(const auto e=cursor.next()){
        const auto* drop=dynamic_cast<DropComponent*>((*e)->get(34));const auto* p=dynamic_cast<ContactPosition*>((*e)->get(0));
        if(!drop||!p)throw std::invalid_argument("missing diagnostic pending drop fields");
        if(!first)out<<',';first=false;out<<"{\"id\":"<<entity_world->id(**e)<<",\"type\":"<<drop->type<<",\"value\":"<<drop->value<<",\"x\":"<<p->x<<",\"z\":"<<p->z<<'}';
    }}out<<']';
}
} // namespace saga::desktop
