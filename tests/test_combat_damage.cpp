#include "core/gameplay/combat_damage.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace saga::core;
void check(bool ok,const char* why) {
    if(!ok){std::cerr<<why<<'\n';std::exit(1);}
}
struct Draws final:RandomSource {
    std::vector<std::int32_t> values;
    std::size_t cursor=0;
    std::vector<std::string>* log=nullptr;
    explicit Draws(std::vector<std::int32_t> sequence):values(std::move(sequence)){}
    std::int32_t next() override {
        if(log)log->push_back("draw");
        return values.at(cursor++);
    }
};
struct Host final:DamageHost {
    CombatKind attacker=CombatKind::human,receiver=CombatKind::enemy;
    DamageParameter hp{200,200,0},replacement{0,200,0};
    DamageParameter* captured=&hp;
    ReceiverDefence defence{0,2,0};
    AttributeAffinityTables tables=initial_attribute_affinity_tables();
    std::int32_t accuracy=0,evade=0,critical=0,rate=25;
    bool debug=false,miss=false;
    DamageNumbers result;
    int rate_reads=0,notifications=0;
    std::vector<std::string> log;
    std::function<void()> callback;
    CombatKind kind(CombatSide side) override {
        return side==CombatSide::attacker?attacker:receiver;
    }
    std::int32_t parameter(CombatSide side,std::int32_t index) override {
        log.push_back("p"+std::to_string(index));
        if(side==CombatSide::receiver&&index==9)return evade;
        if(side==CombatSide::attacker&&index==7)return accuracy;
        if(side==CombatSide::attacker&&index==8)return critical;
        throw std::logic_error("unexpected parameter read");
    }
    DamageParameter* receiver_parameter_zero() override {log.push_back("capture");return captured;}
    ReceiverDefence receiver_defence() override {log.push_back("defence");return defence;}
    const AttributeAffinityTables& affinity_tables() override {log.push_back("tables");return tables;}
    std::int32_t attacker_parameter_one_rate() override {++rate_reads;log.push_back("rate");return rate;}
    bool debug_enabled() override {log.push_back("debug");return debug;}
    void show_miss() override {miss=true;log.push_back("miss");}
    void after_damage(const DamageNumbers& numbers) override {
        ++notifications;result=numbers;log.push_back("after");if(callback)callback();
    }
};
int main() {
    {
        Host h;Draws r({90});r.log=&h.log;
        check(!apply_combat_damage(100,0,r,h)&&h.miss,"miss returns false");
        check(h.hp.value==200&&h.notifications==0&&r.cursor==1,"miss leaves state/draw suffix alone");
        check(h.log==std::vector<std::string>{"p9","p7","draw","miss"},"miss lookup order and short circuit");
    }
    {
        Host h;Draws r({0,5,99,3});r.log=&h.log;
        check(!apply_combat_damage(100,0,r,h),"ordinary hit not lethal");
        // 100% variation, strong affinity 90%, human factor 55% => trunc(49.5).
        check(h.result.amount==49&&h.hp.value==151&&h.result.affinity==0&&!h.result.critical,"ordered percentages");
        check(h.log==std::vector<std::string>{"p9","p7","draw","capture","defence","p8",
              "draw","tables","draw","rate","draw","debug","after"},"successful access/draw order");
    }
    {
        Host h;h.critical=200;Draws r({0,5,99,0});
        apply_combat_damage(100,0,r,h);
        check(h.result.amount==153&&h.rate_reads==0&&h.result.critical,"critical excludes human factor");
    }
    {
        Host h;h.attacker=CombatKind::enemy;h.receiver=CombatKind::other;h.defence={0,0,0};
        Draws r({0,5,99});apply_combat_damage(100,0,r,h);
        check(h.result.amount==44&&r.cursor==3,"matching attribute truncates, other skips floor");
    }
    {
        Host h;h.receiver=CombatKind::human;Draws r({0,5,99,2});
        apply_combat_damage(0,0,r,h);check(h.result.amount==3,"human floor inclusive maximum");
        Host sentinel;Draws s({0,5,99,124});apply_combat_damage(0,3,s,sentinel);
        check(sentinel.result.amount==1&&s.cursor==4,"sentinel affinity floor one still consumes draw");
        Host weak;weak.defence.attribute=1;Draws w({0,5,99,1});apply_combat_damage(0,0,w,weak);
        check(weak.result.affinity==2&&weak.result.amount==2,"weak affinity enemy floor");
    }
    {
        Host h;h.attacker=CombatKind::enemy;h.debug=true;h.hp.value=1;
        Draws r({0,5,99,0});check(!apply_combat_damage(100,0,r,h),"debug nonhuman skips subtraction");
        check(h.hp.value==1&&h.notifications==1&&r.cursor==4,"debug keeps effects and draws");
        Host human;human.debug=true;human.hp.value=1;Draws s({0,5,99,0});
        check(apply_combat_damage(100,0,s,human)&&human.hp.value==0,"debug does not protect from human");
    }
    {
        Host h;h.hp={1,200,1};Draws r({0,5,99,0});
        check(!apply_combat_damage(100,0,r,h)&&h.hp.value==0&&h.hp.extra==1,"death includes untouched extra");
        Host resurrect;resurrect.hp.value=1;
        resurrect.callback=[&]{check(resurrect.hp.value==0,"callback follows subtraction");
                              resurrect.hp.value=10;resurrect.captured=&resurrect.replacement;};
        Draws s({0,5,99,0});check(!apply_combat_damage(100,0,s,resurrect),"final test uses captured object after callback");
        Host kill;kill.callback=[&]{kill.hp.extra=-kill.hp.value;};Draws t({0,5,99,0});
        check(apply_combat_damage(100,0,t,kill),"callback can make hit lethal");
    }
    {
        Host h;h.receiver=CombatKind::other;h.attacker=CombatKind::other;
        h.defence={0,4,0};h.tables.rates[3]=100;h.hp.maximum=0;
        Draws r({0,5,99});apply_combat_damage(std::numeric_limits<std::int32_t>::max(),3,r,h);
        check(h.result.amount==-1&&h.hp.value==201,"multiply wraps before division; negative damage heals");
    }
    {
        Host h;h.tables.rates[0]=0;h.captured=nullptr;Draws r({0,5,99,0});
        bool failed=false;try{apply_combat_damage(100,0,r,h);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&r.cursor==4&&h.notifications==0,"null parameter faults at subtraction after draws");
        Host bad;Draws s({0,5,99,0});failed=false;
        try{apply_combat_damage(100,5,s,bad);}catch(const std::out_of_range&){failed=true;}
        check(failed&&s.cursor==2&&bad.hp.value==200,"invalid affinity faults after variation before critical");
    }
    {
        // Public seeded backend sequence: 1434747710,302596119,269548474,1122627734.
        Host h;LegacySystemRandom r(42);apply_combat_damage(100,0,r,h);
        // Variation = 95 + 302596119%11 = 96; trunc(96*90/100)=86; trunc(86*55/100)=47.
        check(h.result.amount==47&&h.hp.value==153,"seeded RNG through complete numeric hit");
        check(r.next()==361709742,"exactly four backend draws");
        Draws edge({99,0,std::numeric_limits<std::int32_t>::min()});
        check(random_hits(edge,105)&&!random_hits(edge,0)&&random_hits(edge,0),"unclamped thresholds and INT_MIN remainder");
    }
}
