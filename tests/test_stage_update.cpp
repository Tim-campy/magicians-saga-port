#include "startup_fixture.hpp"
#include "core/gameplay/stage_update.hpp"
#include <algorithm>
#include <limits>

struct PartyCursor {
    std::shared_ptr<PartyMembers> list;std::uint32_t version=0;std::size_t index=0;
    void reset(std::shared_ptr<PartyMembers> p){list=std::move(p);version=list->version();index=0;}
    std::optional<QuestMemberId> next(Fixture& f){
        if(list->version()!=version)throw std::runtime_error("changed cursor");
        if(index>=list->entities().size())return std::nullopt;
        auto* e=list->entities()[index++];if(!e)throw std::invalid_argument("null member");return f.entity_id(*e);
    }
};
struct UpdateFixture final:StartupFixture,StageUpdateHost,BossNotificationHost,StageFormStack,
    QuestRewardHost,QuestExperienceHost,QuestReturnHost,MemberFinishHost {
    std::vector<std::string> log;std::string fail;
    std::function<void(std::int32_t)> on_event;std::function<void()> on_result,on_retry,on_make_ad,on_make_notice,on_interval;
    std::function<void(std::size_t)> on_upgrade;
    std::vector<std::int32_t> events,keys;std::int32_t pulse=0,money=0,crystals=0,retries=0;
    bool event_result=false,affordable=false,null_stack=false,null_retry=false,null_exit=false;
    StageFormStack* selected_stack=this;
    std::array<std::int32_t,4> levels{4,11,7,9};
    std::vector<std::pair<EquipmentSlot,std::int32_t>> upgrades;
    std::vector<std::shared_ptr<StageForm>> forms;std::vector<StageForm*> pushed;
    QuestRecord record;QuestRewardResult result;std::function<void()> transition;TownReturnEvent returned{-1,0,0};
    PartyCursor experience_cursor,finish_cursor,sub_cursor;std::size_t treasure_cursor=0;
    UpdateFixture(){human_system.transitions=this;state.quest->events=scripts(std::make_shared<std::vector<EventInstruction>>());}
    void call(std::string s){log.push_back(s);if(fail==s)throw std::runtime_error(s);}
    StageGeometry* stage_map()override{return StartupFixture::stage_map();}
    DropPlacementWorld* drop_world()override{return StartupFixture::drop_world();}
    DropCollectionHost& collection_host()override{call("collection");return *this;}
    BossNotificationHost& movement_host()override{return *this;}
    StageQuestForm* update_quest_form()override{call("form");return no_form?nullptr:&form;}
    bool key_pulse(std::int32_t mask)override{keys.push_back(mask);return pulse==mask;}
    void pause_systems(bool value)override{call(value?"pause":"resume");paused=value;}
    bool execute_stage_event(std::int32_t id)override{events.push_back(id);if(on_event)on_event(id);return event_result;}
    void start_boss_battle(StageStartState& s)override{call("boss");s.boss_started=true;}
    void retry_stage(StageStartState& s)override{call("retry");++retries;if(on_retry)on_retry();s.retried=true;}
    bool check_crystals(std::int32_t n)override{check(n==100);call("check");return affordable;}
    void subtract_crystals(std::int32_t n)override{check(n==100);call("subtract");crystals-=n;}
    StageFormStack* stage_form_stack()override{call("stack");return null_stack?nullptr:selected_stack;}
    std::shared_ptr<StageDialog> make_retry_form(std::int32_t cost)override{check(cost==100);call("retry-form");
        auto p=null_retry?nullptr:std::make_shared<StageDialog>();forms.push_back(p);return p;}
    std::shared_ptr<StageDialog> make_exit_form()override{call("exit-form");
        auto p=null_exit?nullptr:std::make_shared<StageDialog>();forms.push_back(p);return p;}
    std::shared_ptr<StageForm> make_insufficient_crystals_form()override{call("notice");if(on_make_notice)on_make_notice();
        auto p=std::make_shared<StageForm>();forms.push_back(p);return p;}
    std::shared_ptr<StageForm> make_interstitial_ad()override{call("ad");if(on_make_ad)on_make_ad();
        auto p=std::make_shared<StageForm>();forms.push_back(p);return p;}
    std::shared_ptr<StageForm> make_defeat_form(std::int32_t reason)override{check(reason==1||reason==2);call("defeat");
        auto p=std::make_shared<StageForm>();forms.push_back(p);return p;}
    void stop_stage_bgm()override{call("stop-bgm");}
    void push(StageForm* p)override{call("push");pushed.push_back(p);}
    void key_up(std::int32_t mask)override{keys.push_back(mask);}
    VictoryFormStack* form_stack()override{throw std::runtime_error("unexpected victory form");}
    VictoryForm* make_victory(std::int32_t,bool)override{throw std::runtime_error("unexpected victory form");}
    MovementControl* movement_control(std::int32_t group,std::int32_t)override{check(group==35);return nullptr;}
    void send_touch_release(MovementControl&,std::int32_t,std::int32_t)override{throw std::runtime_error("unexpected control");}
    void show_quest_result()override{call("result");if(on_result)on_result();treasure_cursor=0;experience_cursor.reset(roster.members);
        result=grant_quest_rewards(*this);}
    PlayerWorldState* update_world()override{return current_world();}
    std::int32_t* equipment_level(EquipmentSlot slot,std::int32_t id)override{
        call("equipment");upgrades.emplace_back(slot,id);if(on_upgrade)on_upgrade(upgrades.size());return &levels.at(static_cast<std::size_t>(slot));}
    std::int32_t attack_interval(ComponentSlots&)override{if(on_interval)on_interval();return 10;}
    QuestReturnHost& return_host()override{return *this;}
    StageRewards& drops()override{return *state.rewards();}
    std::optional<std::int32_t> next_treasure()override{const auto& v=state.rewards()->treasure_ids;
        return treasure_cursor<v.size()?std::optional<std::int32_t>{v[treasure_cursor++]}:std::nullopt;}
    const TreasureRecord& treasure(std::int32_t id)override{return StartupFixture::treasure(id);}
    ItemStock& item(RewardItem,std::int32_t)override{throw std::runtime_error("unexpected stock");}
    RecruitmentState& character(std::int32_t)override{throw std::runtime_error("unexpected recruitment");}
    void add_money(std::int32_t n)override{money+=n;}
    void add_crystals(std::int32_t n)override{crystals+=n;}
    std::int32_t& rescued_prisoners()override{return state.quest->rescued_prisoners;}
    QuestRecord& quest_record()override{return record;}
    QuestRecord& return_record()override{return record;}
    std::int32_t defeated_enemies()override{return 2;}
    std::int32_t collected_prisoners()override{return state.rewards()->prisoners;}
    std::int32_t total_damage()override{return state.total_damage;}
    QuestExperienceHost& members()override{return *this;}
    std::optional<QuestMemberId> next_member()override{return experience_cursor.next(*this);}
    MemberExperience& human(QuestMemberId id)override{return nodes.at(static_cast<std::size_t>(id)).human;}
    std::int32_t parameter_zero(QuestMemberId id)override{return nodes.at(static_cast<std::size_t>(id)).hp.value;}
    std::int32_t table_count()override{return 13;}
    std::int32_t next_experience(MemberExperience&,std::int32_t)override{return 10;}
    void cleanup_stage()override{call("cleanup");cleanup_quest_stage(*this);}
    MemberFinishHost& finish_members_host()override{finish_cursor.reset(roster.members);sub_cursor.reset(roster.sub_members);return *this;}
    std::optional<QuestMemberId> next_finish_member(FinishMemberList list)override{return (list==FinishMemberList::members?finish_cursor:sub_cursor).next(*this);}
    MemberExperience& finish_human(QuestMemberId id)override{return human(id);}
    SavedCharacterProgression* saved_character(MemberExperience& value)override{
        for(auto& n:nodes)if(static_cast<MemberExperience*>(&n.human)==&value)return &records.at(static_cast<std::size_t>(n.human.character_id)).saved;
        throw std::invalid_argument("human");}
    void clear_finish_members(FinishMemberList list)override{(list==FinishMemberList::members?roster.members:roster.sub_members)->clear();}
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{return Fixture::entity_lifetime(id);}
    void delete_from_manager(QuestMemberId id,bool remove)override{Fixture::delete_from_manager(id,remove);}
    void make_transition(std::int32_t mode,std::function<void()> cb,bool enabled,std::uint32_t color)override{
        check(mode==1&&enabled&&color==0xff000000U);transition=std::move(cb);}
    std::int32_t return_area_id()override{return 12;}
    std::int32_t return_stage()override{return state.stage;}
    void change_to_town_form(bool value)override{check(!value);call("town");}
    void send_town_return(std::int32_t id,const TownReturnEvent& e)override{check(id==31);returned=e;}
    void begin(){start_stage(state,nullptr,state.quest,3,random,*this);log.clear();trace.clear();}
    void dialog(std::int32_t kind,std::int32_t answer,std::int32_t reason=1){
        state.dialog=std::make_shared<StageDialog>();state.dialog->kind=kind;state.dialog->answer=answer;state.dialog->reason=reason;}
};
int main()try{
    // Real Start -> intro -> collection -> boss gate -> celebration -> grants -> town.
    {UpdateFixture f;f.treasures[0]->flags=1;f.treasures[0]->experience=15;f.treasures[0]->money=7;
     f.begin();auto& s=f.state;auto& hero=f.nodes.front();
     f.form.fade_frame=19;update_stage(s,f);check(s.state==0&&s.loop_counter==1);
     f.form.fade_frame=20;update_stage(s,f);check(s.state==1&&s.frame==0);
     hero.position.z=12;update_stage(s,f);check(s.boss_started&&!s.collection.reward_box_picked_up&&s.remaining_time==599);
     s.collection.boss_defeated=true;update_stage(s,f);
     check(s.state==3&&s.collection.reward_box_picked_up&&s.rewards()->treasure_ids==EventIntegers{0}&&hero.human.state==6);
     check(s.remaining_time==599&&f.jingles==1);
     update_stage(s,f);check(s.state==3&&s.frame==1&&f.money==0);
     f.pulse=0x100000;update_stage(s,f);
     check(s.state==4&&s.frame==0&&f.money==7&&hero.human.level==2&&hero.human.experience==5);
     check(f.upgrades==std::vector<std::pair<EquipmentSlot,std::int32_t>>({{EquipmentSlot::weapon,3},{EquipmentSlot::hat,0},{EquipmentSlot::cloth,0}}));
     check(f.levels==std::array<std::int32_t,4>{5,11,8,10}&&s.quest->completed==0);
     update_stage(s,f);check(s.quest->completed==1&&f.transition&&f.roster.members->entities().empty());
     check(f.records[0].saved.level==2&&f.records[0].saved.experience==5&&f.record.clears==1);
     f.transition();check(f.returned.result==0&&f.returned.area_id==12&&f.returned.stage==3);}
    // All due events run in order; flags are live and a null event result still consumes its row.
    {UpdateFixture f;f.begin();f.state.quest->events=scripts(std::make_shared<std::vector<EventInstruction>>());
     for(auto row:{EventIntegers{8,0},EventIntegers{9,0}})f.state.quest->events->at(0)->push_back(std::make_shared<EventIntegers>(row));
     f.state.event_states=std::make_shared<std::vector<std::uint8_t>>(2,std::uint8_t{0});f.event_result=true;f.state.state=1;
     const auto time=f.state.remaining_time;update_stage(f.state,f);
     check(f.events==EventIntegers{8,9}&&f.state.remaining_time==time&&f.keys==EventIntegers{0x20000,0x80000});
     check(*f.state.event_states==std::vector<std::uint8_t>({1,1}));
     update_stage(f.state,f);check(f.events.size()==2&&f.state.remaining_time==time-1);}
    {UpdateFixture f;f.begin();f.state.quest->events=scripts(std::make_shared<std::vector<EventInstruction>>(1,std::make_shared<EventIntegers>(EventIntegers{7,0})));
     f.state.event_states=std::make_shared<std::vector<std::uint8_t>>(1,std::uint8_t{0});auto old=f.state.event_states;
     f.on_event=[&](std::int32_t){f.state.event_states=std::make_shared<std::vector<std::uint8_t>>(1,std::uint8_t{0});};
     check(!execute_stage_events(f.state,f)&&old->at(0)==0&&f.state.event_states->at(0)==1);
     f.state.event_states->at(0)=0;f.on_event=[](std::int32_t){throw std::runtime_error("event");};
     fails([&]{execute_stage_events(f.state,f);});check(f.state.event_states->at(0)==0);
     f.state.quest->completed=1;check(!execute_stage_events(f.state,f));}
    // Pause/resume priority, exit dialog creation, cancellation and voluntary return.
    {UpdateFixture f;f.begin();f.state.state=1;f.pulse=0x2000000;update_stage(f.state,f);check(f.state.state==2&&f.paused);
     f.pulse=0x4000000;update_stage(f.state,f);check(f.state.state==1&&!f.paused);
     f.state.state=2;f.state.frame=11;f.pulse=0x8000000;update_stage(f.state,f);
     check(f.state.state==2&&f.state.frame==11&&f.state.dialog->kind==99);
     f.state.dialog->answer=1;f.pulse=0;update_stage(f.state,f);check(!f.state.dialog&&f.state.state==2);
     f.dialog(99,0);const auto counter=f.state.loop_counter;update_stage(f.state,f);
     check(!f.state.dialog&&f.transition&&f.state.loop_counter==counter);f.transition();check(f.returned.result==3);}
    // Timeout creates retry + defeat forms. Refusing retry chooses the reason-specific exit.
    {UpdateFixture f;f.begin();f.state.state=1;f.state.remaining_time=1;update_stage(f.state,f);
     check(f.state.remaining_time==0&&f.state.dialog->kind==98&&f.state.dialog->reason==2&&f.pushed.size()==2);
     f.state.dialog->answer=2;auto counter=f.state.loop_counter;update_stage(f.state,f);check(f.state.state==6&&f.state.loop_counter==counter);
     update_stage(f.state,f);f.transition();check(f.returned.result==2);}
    {UpdateFixture f;f.begin();f.state.retried=true;f.state.frame=77;stage_game_over(f.state,1,f);
     check(f.state.state==5&&f.state.frame==0&&!f.state.dialog&&f.pushed.size()==1);
     update_stage(f.state,f);f.transition();check(f.returned.result==1);}
    // Crystal and ad paths call the required retry service only after the preceding operation succeeds.
    {UpdateFixture f;f.dialog(98,0);f.affordable=true;f.crystals=150;update_stage(f.state,f);
     check(f.crystals==50&&f.retries==1&&!f.state.dialog&&f.state.loop_counter==0);
     check(f.log==std::vector<std::string>({"check","subtract","retry"}));}
    {UpdateFixture f;f.dialog(98,0,2);auto old=f.state.dialog;update_stage(f.state,f);
     check(f.state.dialog!=old&&f.state.dialog->kind==98&&f.state.dialog->reason==2&&f.pushed.size()==2&&f.retries==0);}
    {UpdateFixture f;f.dialog(98,1);f.on_make_ad=[&]{f.selected_stack=nullptr;};update_stage(f.state,f);
     check(f.retries==1&&f.pushed.size()==1&&!f.state.dialog);}
    {UpdateFixture f;f.dialog(98,1);f.fail="push";fails([&]{update_stage(f.state,f);});check(f.retries==0&&f.state.dialog);}
    {UpdateFixture f;f.dialog(98,0);f.affordable=true;f.crystals=150;f.fail="retry";
     fails([&]{update_stage(f.state,f);});check(f.crystals==50&&f.state.dialog&&f.state.loop_counter==0);}
    {UpdateFixture f;f.dialog(98,0);f.null_retry=true;fails([&]{update_stage(f.state,f);});check(f.state.dialog&&f.pushed.empty());}
    // Counter wrap/remainder and signed cooldown arithmetic, including unknown states.
    {UpdateFixture f;f.state.state=-1;f.state.frame=9;f.state.loop_counter=std::numeric_limits<std::int32_t>::max()-1;
     update_stage(f.state,f);check(f.state.loop_counter==0&&f.state.frame==9);
     f.state.loop_counter=std::numeric_limits<std::int32_t>::max();update_stage(f.state,f);check(f.state.loop_counter==-1);
     f.buttons[0].frame=std::numeric_limits<std::int32_t>::max();f.buttons[1].frame=9;f.buttons[2].frame=-2;
     update_stage_attack_buttons(f.state);check(f.buttons[0].frame==std::numeric_limits<std::int32_t>::min()&&f.buttons[1].frame==4&&f.buttons[2].frame==-1);}
    {UpdateFixture f;auto& host=static_cast<StageUpdateHost&>(f);
     check(stage_progress_rate(f.state,12,host)==100&&stage_progress_rate(f.state,0,host)==116);
     check(stage_progress_rate(f.state,1000,host)==0);f.state.quest->distance=1;check(stage_progress_rate(f.state,0,host)==0);
     f.state.quest->distance=4;check(stage_progress_rate(f.state,std::numeric_limits<float>::quiet_NaN(),host)==116);
     check(stage_progress_rate(f.state,std::numeric_limits<float>::infinity(),host)==0);}
    // Alive excludes exactly states 4 and 5; zero HP is not consulted.
    {HumanRecordLink h;for(std::int32_t i=-2;i<=8;++i){h.state=i;check(stage_human_alive(h)==(i!=4&&i!=5));}
     std::int32_t n=100;raise_stage_equipment_level(n);check(n==100);n=std::numeric_limits<std::int32_t>::max();
     raise_stage_equipment_level(n);check(n==std::numeric_limits<std::int32_t>::min());}
    {UpdateFixture f;f.begin();f.state.state=3;f.pulse=0x100000;f.on_result=[&]{f.nodes.front().entity.remove(8);};
     fails([&]{update_stage(f.state,f);});check(f.state.state==3&&f.state.frame==1&&f.upgrades.empty()&&f.record.clears==1);}
    {UpdateFixture f;f.begin();f.state.state=1;f.state.collection.boss_defeated=true;f.state.collection.reward_box_picked_up=true;
     f.state.humans=nullptr;fails([&]{update_stage(f.state,f);});check(f.state.state==1&&f.state.loop_counter==1);}
    {UpdateFixture f;f.begin();f.state.state=1;f.state.collection.boss_defeated=true;f.state.collection.reward_box_picked_up=true;
     f.nodes.front().human.state=2;f.on_interval=[&]{f.roster.members->append(&f.nodes.front().entity);};
     fails([&]{update_stage(f.state,f);});check(f.state.state==1&&f.nodes.front().human.state==6&&f.nodes.front().human.frame==0);}
    {UpdateFixture f;f.begin();f.state.state=3;f.pulse=0x100000;
     f.on_upgrade=[&](std::size_t i){if(i==1)f.nodes.front().equipment.ids->at(2)=6;if(i==2)throw std::runtime_error("hat");};
     fails([&]{update_stage(f.state,f);});check(f.state.state==3&&f.levels[0]==5&&f.levels[2]==7&&f.money==0&&f.record.clears==1);
     check(f.upgrades[1].second==6&&f.upgrades.size()==2);}
    {UpdateFixture f;f.begin();f.state.state=1;f.state.remaining_time=std::numeric_limits<std::int32_t>::min();
     update_stage(f.state,f);check(f.state.remaining_time==std::numeric_limits<std::int32_t>::max());}
}catch(const std::exception& e){std::cerr<<"uncaught stage update error: "<<e.what()<<'\n';return 1;}
