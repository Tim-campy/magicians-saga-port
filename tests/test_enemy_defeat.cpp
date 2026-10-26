#include "core/gameplay/enemy_defeat.hpp"
#include <cstdlib>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Pending {std::int32_t type,value;ProjectilePoint position;std::int32_t scale;};
struct Host final:EnemyDefeatHost,RandomSource {
    std::array<ProjectileLink,2> projectiles;std::array<ProjectileModification,2> modifications;
    ComponentSlots exp_drop{35},treasure_drop{35};DropDepth exp_depth,treasure_depth;
    std::int32_t count=3;int draws=0,pi=0,mi=0;bool null_drop=false;
    std::vector<Pending> drops;std::vector<std::string> calls;
    std::function<void()> count_callback,drop_callback;
    Host(){exp_drop.add(4,&exp_depth);treasure_drop.add(4,&treasure_depth);}
    ProjectileLink& acquire_projectile()override{calls.push_back("launch");return projectiles.at(static_cast<std::size_t>(pi++));}
    ProjectileModification& acquire_modification()override{calls.push_back("modify");return modifications.at(static_cast<std::size_t>(mi++));}
    void add_animation(ComponentSlots&,std::int32_t,std::int32_t)override{check(false);}
    void return_component(std::int32_t slot,Component*)override{check(slot==15);calls.push_back("pool");}
    std::int32_t& saved_defeat_count(const EnemyRecord&)override{calls.push_back("count");if(count_callback)count_callback();return count;}
    ComponentSlots* create_drop(std::int32_t type,std::int32_t value,ProjectilePoint p,std::int32_t scale)override{
        calls.push_back("drop"+std::to_string(type));drops.push_back({type,value,p,scale});if(drop_callback)drop_callback();
        return null_drop?nullptr:type==0?&exp_drop:&treasure_drop;
    }
    std::int32_t next()override{calls.push_back("draw");++draws;return 0;}
};
struct Fixture {
    Host host;ComponentSlots e{35};EnemyRecordLink enemy;ContactPosition pos;ProjectileSpeed speed;DefeatColor color;
    EnemyDefeatCounters counters{2,3};CharacterProgressionTable characters;EnemyTable enemies{EnemyRecord{},EnemyRecord{}};
    EquipmentCombatTable equipment;CombatRecordTables tables{characters,enemies,equipment,equipment,equipment};
    Fixture(){pos.x=20;pos.y=30;pos.z=40;speed.x=1;speed.y=2;speed.z=3;enemy.level=1;enemy.frame=9;
        enemies[0]->experience=EventIntegers{10,10};enemies[0]->treasures=EnemyTreasureGroups{{{7,1000},{8,1000}}};
        enemies[1]->experience=EventIntegers{0,0};enemies[1]->treasures=EnemyTreasureGroups{{{9,1000}}};
        e.add(0,&pos).add(1,&speed).add(6,&color).add(24,&enemy);}
    void run(){defeat_enemy(e,tables,counters,host,host);}
};
int main(){
    {Fixture f;f.host.count_callback=[&]{check(f.enemy.state==4&&f.enemy.frame==0&&f.color.values==std::array<std::int32_t,4>{{255,0,255,0}});};
        f.run();check(f.host.calls==std::vector<std::string>{"draw","draw","launch","modify","count","drop0","draw","drop3"});
        check(f.host.count==4&&f.counters.defeated_enemies==3&&f.counters.remaining_enemies==2);
        check(f.speed.x==0&&f.speed.y==0&&f.speed.z==0&&f.host.projectiles[0].end.x==-80&&f.host.projectiles[0].end.z==-60);
        check(f.host.drops.size()==2&&f.host.drops[0].value==10&&f.host.drops[0].position.x==15&&f.host.drops[0].position.y==30&&f.host.drops[0].position.z==30);
        check(f.host.drops[1].value==7&&f.host.drops[1].position.y==0&&f.host.drops[1].scale==25);
        check(f.host.exp_depth.extra==-10&&f.host.treasure_depth.extra==-10);}
    for(int flags=0;flags<4;++flags){Fixture f;f.enemies[0]->flags=flags&1;f.enemy.flags=(flags>>1)&1;f.counters.remaining_enemies=0;
        f.run();check(f.counters.defeated_enemies==((flags&1)?2:3)&&f.counters.remaining_enemies==((flags&2)?0:1));}
    {Fixture f;f.host.drop_callback=[&]{f.enemy.enemy_id=1;f.pos.x=100;};f.run();
        check(f.host.drops[0].value==10&&f.host.drops[1].value==9&&f.host.drops[1].position.x==100);}
    {Fixture f;f.host.count_callback=[&]{f.enemies[0]->flags=1;f.enemy.flags=1;};f.counters.remaining_enemies=0;f.run();
        check(f.counters.defeated_enemies==2&&f.counters.remaining_enemies==0);}
    {Fixture f;f.enemies[0]->experience=EventIntegers{0,0};f.enemies[0]->treasures=EnemyTreasureGroups{{}};
        f.run();f.run();check(f.host.count==5&&f.counters.defeated_enemies==4&&f.host.draws==4&&f.host.drops.empty());}
    {std::int32_t count=std::numeric_limits<std::int32_t>::max();add_enemy_defeat_count(count,1);check(count==0);
        count=2;add_enemy_defeat_count(count,-3);check(count==0);count=-1;add_enemy_defeat_count(count,std::numeric_limits<std::int32_t>::min());check(count==std::numeric_limits<std::int32_t>::max());}
    {Fixture f;f.counters.defeated_enemies=std::numeric_limits<std::int32_t>::max();f.counters.remaining_enemies=std::numeric_limits<std::int32_t>::min();f.run();
        check(f.counters.defeated_enemies==std::numeric_limits<std::int32_t>::min()&&f.counters.remaining_enemies==std::numeric_limits<std::int32_t>::max());}
    {Fixture f;f.e.remove(6);bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.enemy.state==4&&f.e.contains(27)&&f.e.contains(15)&&f.host.count==3);}
    {Fixture f;f.host.exp_drop.remove(4);bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.host.drops.size()==1&&f.host.draws==2&&f.host.count==4&&f.counters.remaining_enemies==2);}
    {Fixture f;f.enemies[0]->treasures.reset();bool failed=false;try{f.run();}catch(const std::invalid_argument&){failed=true;}
        check(failed&&f.host.drops.size()==1&&f.host.exp_depth.extra==-10);}
}
