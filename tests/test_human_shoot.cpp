#include "bullet_fixture.hpp"
#include "core/gameplay/motion_pass.hpp"

struct LiveCollisions final:CollisionPassHost {
    HumanFixture& t;explicit LiveCollisions(HumanFixture& value):t(value){}
    struct Cursor final:CollisionCursor {
        EntitySubset::Cursor cursor;explicit Cursor(EntitySubset& s):cursor(s.cursor()){}
        std::optional<ComponentSlots*> next()override{return cursor.next();}
    };
    std::unique_ptr<CollisionCursor> open_collision_cursor()override{
        return std::make_unique<Cursor>(t.f.groups.get(std::make_shared<ComponentGroupFilter>(EventIntegers{32,0})));
    }
    void send_collision(std::int32_t id,ComponentSlots& a,ComponentSlots& b)override{t.host.events.send(id,&a,&b);}
};

int main()try{
    {HumanFixture t;Shooter s(t);check(!s.shoot(nullptr)&&s.pools.queries==0);auto& enemy=t.enemy_at();
     t.f.node(enemy).lifetime.destroyed=true;check(!s.shoot(&enemy)&&s.pools.queries==0);
     t.f.node(enemy).lifetime.destroyed=false;field<EnemyRecordLink>(enemy,24).state=4;check(!s.shoot(&enemy)&&s.pools.queries==0);}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();t.host.system.stage=nullptr;s.pools.records->clear();check(!s.shoot(&enemy));
     s.pools.records->push_back(BulletRecord{});fails([&]{s.shoot(&enemy);});}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();s.pools.records->at(0).reset();t.f.state.attack_attribute=1;
     fails([&]{s.shoot(&enemy);});check(resolve_combat_parameter(t.hero(),1)->value==100);}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();t.f.state.attack_attribute=17;check(!s.shoot(&enemy)&&s.sounds.empty());}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();auto& actor=t.hero();t.f.state.attack_attribute=1;
     t.host.parameters=[](ComponentSlots&,std::int32_t k){return k==3?47:k==11?50:0;};
     field<ContactPosition>(actor,0).y=7;field<ContactPosition>(actor,0).x=40;field<ContactPosition>(actor,0).z=60;
     auto& w=*t.host.gear[0];w.type=8;w.max_height=77;const auto count=t.f.nodes.size();
     check(s.shoot(&enemy));check(s.sounds==std::vector<bool>{true}&&s.effects==0&&t.f.nodes.size()==count+1);
     auto& b=t.f.nodes.back().entity;check(field<BulletComponent>(b,29).data_id==1&&field<ProjectileDamage>(b,33).value==47);
     check(field<ProjectileDamage>(b,33).attribute==1&&field<ProjectileDamage>(b,33).parent==&actor);
     auto& flight=field<ProjectileLink>(b,27);check(flight.start.y==27&&field<ContactPosition>(b,0).y==17&&flight.max_height==77&&flight.parent==&actor);
     check(flight.end.x==field<ContactPosition>(enemy,0).x&&flight.end.z==field<ContactPosition>(enemy,0).z);
     auto& mod=field<ProjectileModification>(b,15);check(mod.type==7&&mod.loop&&mod.scale_x==2&&mod.scale_y==2&&mod.x==20&&mod.y==40);
     check(field<EntitySprite>(actor,2).animation.clip==52&&resolve_combat_parameter(actor,1)->value==97&&b.contains(10));}
    // Nonmain uses equipped bullet ID independently of Stage.attribute; -1 exits before table lookup.
    {HumanFixture t;Shooter s(t);auto& sub=t.sub();auto& enemy=t.enemy_at();auto& equipped=field<EquipmentRecordLinks>(sub,8);
     const auto id=equipped.ids->at(0);auto& w=*t.host.gear.at(static_cast<std::size_t>(id));w.bullet_id=-1;
     check(!s.shoot(&enemy,&sub)&&s.pools.queries==0);w.bullet_id=2;w.bullet_speed=5;w.max_height=4;
     t.host.system.stage=nullptr;check(s.shoot(&enemy,&sub));check(field<BulletComponent>(t.f.nodes.back().entity,29).data_id==2&&s.sounds==std::vector<bool>{true});}
    // Gate cost includes attribute/4, actual consumption is only current weapon base cost.
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();t.flat_parameters(120);auto& mp=*resolve_combat_parameter(t.hero(),1);mp={30,30,0,0};
     check(s.shoot(&enemy)&&mp.value==27&&s.effects==1);auto& effect=t.f.nodes[t.f.nodes.size()-2].entity;
     auto& data=field<EffectComponent>(effect,26);check(data.type==7&&data.value==3&&data.max_frame==30&&!data.depth);
     check(effect.contains(22)&&effect.contains(10)&&field<EntitySprite>(effect,2).animation.resource==0);}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();auto& mp=*resolve_combat_parameter(t.hero(),1);mp={2,100,0,0};
     const auto count=t.f.nodes.size();check(!s.shoot(&enemy)&&mp.value==2&&t.f.nodes.size()==count);}
    // Consumed MP is retained if a required factory or presentation service fails.
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();s.missing_bullet_world=true;fails([&]{s.shoot(&enemy);});check(resolve_combat_parameter(t.hero(),1)->value==97);}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();s.clips.reset();const auto count=t.f.nodes.size();fails([&]{s.shoot(&enemy);});
     check(t.f.nodes.size()==count+1&&t.f.nodes.back().entity.contains(27)&&s.sounds.empty());}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();t.host.gear[0]->type=-1;fails([&]{s.shoot(&enemy);});
     check(resolve_combat_parameter(t.hero(),1)->value==97&&t.f.nodes.back().entity.contains(27));}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();auto& h=field<HumanRecordLink>(t.hero(),14);
     *resolve_combat_parameter(t.hero(),1)={3,100,0,0};h.state=2;h.opponent=&enemy;h.attack_frame=human_attack_interval(t.host.system,&t.hero(),t.host);
     update_human_attacking(t.host.system,&t.hero(),t.host);
     check(resolve_combat_parameter(t.hero(),1)->value==0&&s.effects==1&&s.sounds.size()==1&&h.state==0&&!h.opponent&&h.attack_frame==0);}
    // Weapon captured at entry still supplies speed/height/animation after component callbacks change equipment.
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();auto& old=*t.host.gear[0];old.bullet_speed=7;old.max_height=31;old.type=9;
     t.host.gear[1]->bullet_speed=99;t.host.gear[1]->max_height=90;t.host.gear[1]->type=8;
     t.f.on_added=[&](ComponentSlots&,std::int32_t slot){if(slot==29)set_combat_equipment(t.hero(),EquipmentSlot::weapon,1);};
     check(s.shoot(&enemy));auto& flight=field<ProjectileLink>(t.f.nodes.back().entity,27);
     check(flight.speed==7&&flight.max_height==31&&field<EntitySprite>(t.hero(),2).animation.clip==44);}
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();auto& w=*t.host.gear[0];w.flags=8;
     check(s.shoot(&enemy)&&s.effects==1&&field<EntitySprite>(t.hero(),2).animation.clip==28);
     auto& effect=t.f.nodes.back().entity;check(field<EntityImage>(effect,7).arrays.textures->at(0)==29&&field<DefeatColor>(effect,6).values[3]==3192&&field<EntityDepth>(effect,4).extra==-10);}
    // First match wins even if later catalog entries also match; no RNG selection.
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();s.pools.records->at(1)->attribute=0;
     const auto draws=t.f.random.index;check(s.shoot(&enemy));check(field<BulletComponent>(t.f.nodes.back().entity,29).data_id==0&&t.f.random.index==draws&&s.sounds==std::vector<bool>{false});}
    // Actual frame attack dispatch invokes the producer and creates a launch-ready entity.
    {HumanFixture t;Shooter s(t);auto& enemy=t.enemy_at();auto& h=field<HumanRecordLink>(t.hero(),14);h.state=2;h.opponent=&enemy;
     h.attack_frame=human_attack_interval(t.host.system,&t.hero(),t.host);const auto before=t.f.nodes.size();
     update_human_attacking(t.host.system,&t.hero(),t.host);check(t.host.shots==1&&t.f.nodes.size()==before+1);
     check(field<ProjectileLink>(t.f.nodes.back().entity,27).parent==&t.hero()&&field<EntitySprite>(t.hero(),2).animation.frame==0);}
    // Real attack state -> bullet factory -> flight/movement -> live collision -> numeric damage -> impact/cleanup.
    for(const bool lethal:{false,true}){HumanFixture t;Shooter s(t);BulletImpactServices impacts(s);auto& actor=t.hero();auto& enemy=t.enemy_at({0,0,0});
     field<ContactPosition>(actor,0).x=0;field<ContactPosition>(actor,0).z=100;
     field<ProjectileSpeed>(actor,1).z=0;field<ProjectileSpeed>(enemy,1).z=0;
     auto* hp=resolve_combat_parameter(enemy,0);const auto starting_hp=lethal?1:100000;hp->maximum=starting_hp;hp->value=starting_hp;
     t.host.parameters=[](ComponentSlots&,std::int32_t k){return k==3?1000:0;};
     t.host.on_impact=[&](ComponentSlots& e,ComponentSlots* parent){create_impact_effect(&e,parent,t.host.records,impacts);};
     t.host.on_read_bullet=[&](ComponentSlots& e){check(impacts.bullet_data(e));};
     t.host.on_bullet_impact=[&](ComponentSlots& e,ComponentSlots& parent){create_impact_effect(&e,&parent,t.host.records,impacts);};
     ProjectileSystemState projectiles;initialize_projectiles(projectiles,&t.enemy_world,t.host);
     auto& h=field<HumanRecordLink>(actor,14);h.state=2;h.opponent=&enemy;h.attack_frame=human_attack_interval(t.host.system,&actor,t.host);
     update_human_attacking(t.host.system,&actor,t.host);auto& bullet=t.f.nodes.back().entity;
     auto* moves=t.enemy_world.subset({0,1});LiveCollisions host(t);CollisionPass collision;
     t.f.random.index=0;t.f.random.values={0,5,99,0,0,0};int frames=0;
     for(;frames<50&&bullet.contains(27);++frames){projectiles.pass->update(*projectiles.subset,t.host);move_subset(*moves);collision.update(host);}
     check(frames>0&&frames<50&&hp->value<starting_hp&&t.host.damage_amount>0);
     check(!bullet.contains(27)&&!bullet.contains(32)&&bullet.contains(22));
     check(resolve_combat_parameter(actor,1)->value==97&&field<ProjectileSpeed>(bullet,1).z==0&&s.sounds.size()==1);
     if(lethal){check(hp->value==0&&field<EnemyRecordLink>(enemy,24).state==4&&t.host.defeat_counts[2]==1);
         bool experience=false;for(auto& n:t.f.nodes)if(n.entity.contains(34)&&n.drop.type==0&&n.drop.value==10)experience=true;
         check(experience&&t.f.enemy_system.defeated_enemies==1);}
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
