#pragma once
#include "startup_fixture.hpp"
#include "core/gameplay/stage_update.hpp"
#include "core/gameplay/stage_recovery.hpp"
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
struct UpdateFixture:StartupFixture,StageUpdateHost,StageRecoveryHost,BossNotificationHost,StageFormStack,
    QuestRewardHost,QuestExperienceHost,QuestReturnHost,MemberFinishHost {
    std::vector<std::string> log;std::string fail;
    std::function<void(std::int32_t)> on_event;std::function<void()> on_result,on_retry,on_make_ad,on_make_notice,on_interval;
    std::function<void(std::size_t)> on_upgrade;
    std::function<void()> on_boss,on_boss_bgm,on_release_modifier,on_effect_world,on_recovery_world,on_recovery_animation;
    std::function<void(std::size_t)> on_duration;
    std::vector<Component*> returned_modifiers;
    std::size_t duration_queries=0;std::int32_t duration=0;
    bool missing_destroy_world=false,missing_object_world=false,missing_boss=false;
    ComponentSlots* made_boss=nullptr;
    StageObjectWorld object_world{ids,static_cast<DropFactoryHost&>(*this)};
    std::vector<std::int32_t> events,keys;std::int32_t pulse=0,money=0,crystals=0,retries=0;
    bool event_result=false,affordable=false,null_stack=false,null_retry=false,null_exit=false;
    StageFormStack* selected_stack=this;
    std::array<std::int32_t,4> levels{4,11,7,9};
    std::vector<std::pair<EquipmentSlot,std::int32_t>> upgrades;
    std::vector<std::shared_ptr<StageForm>> forms;std::vector<StageForm*> pushed;
    QuestRecord record;QuestRewardResult result;std::function<void()> transition;TownReturnEvent returned{-1,0,0};
    PartyCursor experience_cursor,finish_cursor,sub_cursor;std::size_t treasure_cursor=0;
    UpdateFixture(){human_system.transitions=this;state.quest->events=scripts(std::make_shared<std::vector<EventInstruction>>());
        duration=initial_human_invincible_time(fps);}
    void call(std::string s){log.push_back(s);if(fail==s)throw std::runtime_error(s);}
    StageGeometry* stage_map()override{return StartupFixture::stage_map();}
    DropPlacementWorld* drop_world()override{return StartupFixture::drop_world();}
    DropCollectionHost& collection_host()override{call("collection");return *this;}
    BossNotificationHost& movement_host()override{return *this;}
    StageQuestForm* update_quest_form()override{call("form");return no_form?nullptr:&form;}
    bool key_pulse(std::int32_t mask)override{keys.push_back(mask);return pulse==mask;}
    void pause_systems(bool value)override{call(value?"pause":"resume");paused=value;}
    bool execute_stage_event(std::int32_t id)override{events.push_back(id);if(on_event)on_event(id);return event_result;}
    StageRecoveryHost& recovery_host()override{if(state.dialog&&state.dialog->kind==98){call("retry");++retries;if(on_retry)on_retry();}return *this;}
    StageStartHost& recovery_start_host()override{return *this;}
    PlayerWorldState* recovery_world()override{if(on_recovery_world)on_recovery_world();return current_world();}
    EntitySubset* recovery_effect_subset()override{return &groups.get(std::make_shared<ComponentGroupFilter>(EventIntegers{26}));}
    QuestEntityHost* recovery_destruction_world()override{return missing_destroy_world?nullptr:static_cast<Fixture*>(this);}
    QuestMemberId recovery_entity_id(ComponentSlots& e)override{return entity_id(e);}
    PartyCharacter* recovery_character(HumanRecordLink& h)override{return Fixture::character(h);}
    void recovery_change_animation(ComponentSlots& e,std::int32_t clip,bool reset)override{
        if(on_recovery_animation)on_recovery_animation();Fixture::change_animation(e,clip,reset);}
    DropEffectWorld* recovery_effect_world()override{if(on_effect_world)on_effect_world();return &effects;}
    std::int32_t invincible_time()override{++duration_queries;if(on_duration)on_duration(duration_queries);return duration;}
    ProjectileModification& acquire_modification()override{return StartupFixture::acquire_modification();}
    void release_recovery_modification(Component* p)override{returned_modifiers.push_back(p);if(on_release_modifier)on_release_modifier();}
    ComponentSlots* create_stage_boss(StageEnemyStart& receiver)override{
        check(&receiver==state.enemies);call("boss");if(on_boss)on_boss();if(missing_boss)return nullptr;
        made_boss=&create_entity(ids,0,false,static_cast<DropFactoryHost&>(*this));return made_boss;}
    StageObjectWorld* stage_object_world()override{return missing_object_world?nullptr:&object_world;}
    void play_boss_bgm()override{call("boss-bgm");if(on_boss_bgm)on_boss_bgm();}
    void play_boss_sound()override{call("boss-sound");}
    std::int32_t clip_max_frame(std::int32_t resource,std::int32_t clip)override{
        if(resource==20&&clip==102)return 18;return StartupFixture::clip_max_frame(resource,clip);}
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
