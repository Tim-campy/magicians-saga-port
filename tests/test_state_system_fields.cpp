#include "core/gameplay/state_system_fields.hpp"
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <tuple>
#include <type_traits>

using namespace saga::core;
namespace {
void check_at(bool condition,int line){
    if(!condition){std::cerr<<"system state fields assertion at line "<<line<<'\n';std::abort();}
}
#define check(...) check_at((__VA_ARGS__),__LINE__)

template<class T,class F> void inspect(const T& state,std::string_view expected,F&& f){
    int calls=0;
    visit_state_fields(state,[&](const char* tag,const auto&... fields){
        ++calls;check(tag==expected);f(std::tie(fields...));
    });
    check(calls==1);
}
struct CountFields {
    template<class... T> void operator()(const char*,const T&...)const{}
};
// Explicit instantiation checks every overload body, including abstract public
// states which a snapshot writer receives by reference rather than constructs.
template<class T> [[maybe_unused]] void instantiate_schema(const T& state){visit_state_fields(state,CountFields{});}
#define INSTANTIATE(T) template void instantiate_schema<T>(const T&)
INSTANTIATE(StageQuest);INSTANTIATE(StagePlacementState);INSTANTIATE(AttackButtonState);
INSTANTIATE(StageHumanStart);INSTANTIATE(StageQuestForm);INSTANTIATE(StageRewards);
INSTANTIATE(DropCollectionState);INSTANTIATE(StageStartState);INSTANTIATE(StageForm);
INSTANTIATE(StageDialog);INSTANTIATE(VictoryForm);INSTANTIATE(MovementControl);
INSTANTIATE(PartyMembers);INSTANTIATE(MemberRoster);INSTANTIATE(PlayerWorldState);
INSTANTIATE(HumanFrameSystem);INSTANTIATE(EnemyDefeatCounters);INSTANTIATE(EnemyCreationSystem);
INSTANTIATE(RandomConfiguration);INSTANTIATE(WeightedLotteryState);INSTANTIATE(TownInitializationState);
INSTANTIATE(ProjectileSystemState);INSTANTIATE(CollisionSystemState);INSTANTIATE(BattleSystemState);
INSTANTIATE(DebugVisualizationState);INSTANTIATE(DebugPathVisualization);INSTANTIATE(SharedAnimationClip);
INSTANTIATE(AnimationResource);INSTANTIATE(SharedAnimationGates);INSTANTIATE(AnimationSystemState);
INSTANTIATE(RenderSystemState);INSTANTIATE(RenderQueue);INSTANTIATE(EffectSystemState);
INSTANTIATE(ModificationSystemState);INSTANTIATE(EmitterSystemState);INSTANTIATE(ParameterExperienceSystem);
INSTANTIATE(GaugeDrawState);INSTANTIATE(GaugeFrameState);INSTANTIATE(BubbleFrameState);
INSTANTIATE(ButtonFrameState);INSTANTIATE(PopupBar);INSTANTIATE(SlideBar);INSTANTIATE(InfoState);
INSTANTIATE(CursorPoint);INSTANTIATE(CursorFocusEffect);INSTANTIATE(CursorState);
INSTANTIATE(QuestCameraSystem);INSTANTIATE(QuestCameraForm);INSTANTIATE(CellSystemState);
INSTANTIATE(CellIndexState);INSTANTIATE(InputFrameState);INSTANTIATE(InputControl);
INSTANTIATE(InputTouch);INSTANTIATE(QuestFrameState);INSTANTIATE(QuestRecord);
INSTANTIATE(ItemStock);INSTANTIATE(RecruitmentState);INSTANTIATE(SavedCharacterProgression);
INSTANTIATE(CharacterNewGameState);INSTANTIATE(ShopNewGameState);INSTANTIATE(EquipmentNewGameState);
INSTANTIATE(QuestCharacterResult);INSTANTIATE(QuestRewardResult);INSTANTIATE(EventContext);
INSTANTIATE(DelayedEvent);INSTANTIATE(EventConditionRecord);INSTANTIATE(DecodedEventExecutionState);
INSTANTIATE(DiscoveryState);INSTANTIATE(EventProgression);INSTANTIATE(QuestConditionRecord);
INSTANTIATE(QuestConditionState);INSTANTIATE(CharacterConditionState);INSTANTIATE(DecodedEventConditionState);
INSTANTIATE(FormPushState);INSTANTIATE(InitialFormSelection);INSTANTIATE(TalkDefinition);
INSTANTIATE(PreparedTalkForm);INSTANTIATE(TextRevealState);INSTANTIATE(TextFontState);
INSTANTIATE(TextFitState);INSTANTIATE(TextBlockConfig);INSTANTIATE(LayoutTextBlock);INSTANTIATE(TextBlockLayout);
#undef INSTANTIATE

void stage_and_roster(){
    StageStartState stage;stage.quest=std::make_shared<StageQuest>();stage.quest->completed=2;
    stage.loop_counter=41;stage.min_z_rate=-19;stage.max_z_rate=101;stage.retried=true;stage.attack_attribute=3;
    stage.event_states=std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t>{1,0,2});
    AttackButtonState button{2,9};
    stage.attack_buttons=std::make_shared<std::vector<AttackButtonState*>>(std::initializer_list<AttackButtonState*>{&button,nullptr,&button});
    auto rewards=std::make_shared<StageRewards>();rewards->treasure_ids={7,7,2};stage.replace_rewards(rewards);
    inspect(stage,"StageStartState",[&](const auto& row){
        static_assert(std::tuple_size_v<std::decay_t<decltype(row)>> ==24);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(std::get<5>(row))>>);
        check(std::get<0>(row).quest==stage.quest);check(std::get<5>(row)==41);
        check(std::get<8>(row)==-19&&std::get<9>(row)==101&&std::get<13>(row));
        check(std::get<14>(row)==3);check(std::get<15>(row).rewards==rewards.get());
        check(std::get<19>(row)==stage.attack_buttons&&std::get<19>(row)->at(1)==nullptr);
        check(std::get<19>(row)->at(0)==std::get<19>(row)->at(2));
        check(std::get<20>(row)->at(2)==2&&std::get<23>(row)==rewards);
    });
    MemberRoster members;members.formation->at(0)=7;members.formation->at(1)=7;
    members.sub_members=members.members;
    inspect(members,"MemberRoster",[&](const auto& row){
        check(std::get<0>(row)==members.formation);check(std::get<0>(row)->at(1)==7);
        check(std::get<1>(row)==std::get<2>(row));
    });
    inspect(*members.members,"PartyMembers",[](const auto& row){
        check(std::get<0>(row).empty());check(std::get<1>(row)==0);
    });
    check(stage.rewards()==rewards&&stage.collection.rewards==rewards.get());
}
void queues_and_gates(){
    ComponentSlots first{0},second{0};
    auto pending=std::make_shared<std::vector<ComponentSlots*>>(std::initializer_list<ComponentSlots*>{&second,nullptr,&first,&second});
    BubbleFrameState bubble;bubble.seen=pending;bubble.pending=pending;
    inspect(bubble,"BubbleFrameState",[&](const auto& row){
        check(std::get<1>(row)==std::get<2>(row));
        const auto& values=*std::get<2>(row);check(values.size()==4&&values[0]==&second);
        check(values[1]==nullptr&&values[2]==&first&&values[3]==&second);
    });
    EmitterSystemState emitter;emitter.expired=std::make_shared<EmitterExpiryQueue>();
    emitter.expired->add(&second);emitter.expired->add(&first);emitter.loop_counter=-17;
    inspect(emitter,"EmitterSystemState",[&](const auto& row){
        check(std::get<1>(row)==emitter.expired&&std::get<2>(row)==-17);
        check(std::get<1>(row)->version()==2&&std::get<1>(row)->at(0)==&second);
        check(std::get<1>(row)->at(1)==&first);
    });
    EffectSystemState effect;effect.expired=emitter.expired;
    inspect(effect,"EffectSystemState",[&](const auto& row){check(std::get<1>(row)==emitter.expired);});
    ProjectileSystemState projectile;projectile.pass.emplace();
    inspect(projectile,"ProjectileSystemState",[](const auto& row){check(std::get<1>(row).has_value());});
    CollisionSystemState collision;
    inspect(collision,"CollisionSystemState",[](const auto& row){check(!std::get<1>(row));});
    SharedAnimationGates gates{false,true};
    inspect(gates,"SharedAnimationGates",[](const auto& row){check(!std::get<0>(row)&&std::get<1>(row));});
    DebugVisualizationState debug;debug.flags=456;
    inspect(debug,"DebugVisualizationState",[](const auto& row){check(std::get<1>(row)==456);});
    WeightedLotteryState lottery;lottery.table->at(255)=-1;
    inspect(lottery,"WeightedLotteryState",[&](const auto& row){
        check(std::get<0>(row)==lottery.table&&std::get<0>(row)->at(255)==-1);
    });
}
void actors_input_and_camera(){
    HumanFrameSystem human;human.loop_counter=-3;
    inspect(human,"HumanFrameSystem",[](const auto& row){check(std::get<3>(row)==-3);});
    EnemyCreationSystem enemy;enemy.defeated_enemies=12;enemy.remaining_enemies=2;
    enemy.created_enemies=14;enemy.loop_counter=71;
    enemy.maximum_by_section=std::make_shared<EventIntegers>(EventIntegers{4,5});
    enemy.created_by_section=std::make_shared<EventIntegers>(EventIntegers{1,2});
    inspect(enemy,"EnemyCreationSystem",[&](const auto& row){
        check(std::get<0>(row).defeated_enemies==12&&std::get<0>(row).remaining_enemies==2);
        check(std::get<6>(row)==14&&std::get<7>(row)==71);
        check(std::get<8>(row)==enemy.maximum_by_section&&std::get<9>(row)==enemy.created_by_section);
    });
    InputFrameState input;input.old_up=true;input.frame=47;input.last_down_frame=-9;
    inspect(input,"InputFrameState",[](const auto& row){
        check(std::get<1>(row)&&!std::get<2>(row)&&std::get<3>(row)==47&&std::get<4>(row)==-9);
    });
    QuestCameraSystem camera;camera.dirty=true;camera.dragging=true;camera.cursor_revision=true;camera.pinch_scale=77;
    camera.history=std::make_shared<std::vector<EventInstruction>>();camera.history->push_back(camera.scale_range);
    inspect(camera,"QuestCameraSystem",[&](const auto& row){
        check(std::get<1>(row)&&std::get<2>(row)&&std::get<3>(row)&&std::get<4>(row)==77);
        check(std::get<5>(row)->at(0)==std::get<6>(row));
    });
    StageQuestForm state{89,3};QuestCameraForm form{state};form.update_static_render=true;
    inspect(form,"QuestCameraForm",[&](const auto& row){
        check(&std::get<0>(row)==&state&&std::get<5>(row));
    });
    CellIndexState index;index.buckets=std::make_shared<CellIndexState::Buckets>();
    const auto list=std::make_shared<std::vector<ComponentSlots*>>();(*index.buckets)[-1]=list;
    inspect(index,"CellIndexState",[&](const auto& row){check(std::get<1>(row)->at(-1)==list);});
}
void events_and_text(){
    auto arguments=std::make_shared<EventIntegers>(EventIntegers{5,-4});
    DelayedEvent delay{9,4,8,-1,6,{std::make_shared<std::u16string>(u"synthetic"),arguments}};
    inspect(delay,"DelayedEvent",[&](const auto& row){
        check(std::get<0>(row)==9&&std::get<1>(row)==4&&std::get<2>(row)==8);
        check(std::get<3>(row)==-1&&std::get<4>(row)==6&&std::get<5>(row).argi==arguments);
    });
    int diagnostics=0;DecodedEventExecutionState execution;
    execution.normal_save_present=false;execution.quest_data_count=19;execution.print_enabled=true;
    execution.diagnostic=[&](EventDiagnostic,std::int32_t){++diagnostics;};
    execution.catalog=std::make_shared<EventCatalog>();
    auto record=std::make_shared<EventConditionRecord>();record->execution_count=21;
    record->effects=std::make_shared<std::vector<EventInstruction>>();record->effects->push_back(arguments);
    execution.catalog->push_back(record);execution.catalog->push_back(nullptr);execution.catalog->push_back(record);
    inspect(execution,"DecodedEventExecutionState",[&](const auto& row){
        check(std::get<0>(row)==execution.catalog&&std::get<0>(row)->at(0)==std::get<0>(row)->at(2));
        check(std::get<1>(row).has_value()&&!std::get<1>(row).value());
        check(std::get<3>(row)==19&&std::get<4>(row)==true&&static_cast<bool>(std::get<5>(row)));
    });
    inspect(*record,"EventConditionRecord",[&](const auto& row){
        check(std::get<0>(row)==21&&std::get<5>(row)==record->effects);
    });
    check(diagnostics==0);
    PreparedTalkForm talk{{0,1,2,3},4,{u"first",u"second"},1,-1,std::u16string(u"argument")};
    inspect(talk,"PreparedTalkForm",[](const auto& row){
        check(std::get<2>(row).at(1)==u"second"&&std::get<3>(row)==1&&std::get<4>(row)==-1);
        check(std::get<5>(row)==std::u16string(u"argument"));
    });
    TextRevealState reveal{50,7,1500,12345678901LL,-3,812};
    inspect(reveal,"TextRevealState",[](const auto& row){
        check(std::get<2>(row)==1500&&std::get<3>(row)==12345678901LL);
        check(std::get<4>(row)==-3&&std::get<5>(row)==812);
    });
    LayoutTextBlock block;block.text=u"a";block.x=-0.0F;block.source_begin=8;block.source_length=2;
    TextBlockLayout layout{u"source",{block},14.5F};
    inspect(layout,"TextBlockLayout",[](const auto& row){
        check(std::get<0>(row)==u"source"&&std::get<1>(row).at(0).source_begin==8&&std::get<2>(row)==14.5F);
    });
    QuestFrameState frame;frame.pinch_frame=9;frame.pinching=true;frame.fade_type=2;frame.fade_state=3;
    inspect(frame,"QuestFrameState",[](const auto& row){
        check(std::get<2>(row)==9&&std::get<3>(row)&&std::get<5>(row)==2&&std::get<6>(row)==3);
    });
}
}
int main(){stage_and_roster();queues_and_gates();actors_input_and_camera();events_and_text();}
