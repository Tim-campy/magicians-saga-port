#pragma once
#include "gameplay_catalog.hpp"
#include "core/ecs/dynamic_subsets.hpp"
#include "core/gameplay/animation_pass.hpp"
#include "core/gameplay/enemy_factory.hpp"
#include "core/gameplay/effect_factory.hpp"
#include "core/gameplay/drop_collection.hpp"
#include "core/gameplay/stage_cleanup.hpp"
#include "core/ecs/entity_events.hpp"
#include "core/gameplay/member_selection.hpp"
#include <deque>

namespace saga::desktop {
// Explicit decoded or selected scenario state. Missing saved values are not replaced by table initial_level.
struct MissionProgress {
    std::map<std::int32_t,core::SavedCharacterProgression> characters;
    std::map<std::int32_t,std::int32_t> shop_purchases;
    std::map<std::int32_t,std::int32_t> equipment_levels,hat_levels,cloth_levels;
};
// Fresh desktop entity/component ownership around the existing game factories.
// Native world teardown, pooling reuse, input updates and the complete system schedule remain separate.
class MissionEntities final:public core::HumanFactoryHost,public core::EnemyFactoryHost,
    public core::BulletFactoryHost,public core::EffectFactoryHost,public core::ComponentObserver,
    public core::AnimationInitializationHost,public core::QuestEntityHost,private core::EntityDeletionHost,
    public core::ProjectileLaunchHost {
public:
    using Resources=std::function<core::AnimationResource*(std::int32_t)>;
    using ResourceIds=std::function<std::int32_t(std::string_view)>;
    MissionEntities(const GameplayCatalog&,MissionProgress&,std::int32_t component_slots,core::StageGeometry*,Resources,ResourceIds);
    MissionEntities(const MissionEntities&)=delete;
    MissionEntities& operator=(const MissionEntities&)=delete;
    core::ComponentSlots& human(const core::HumanCreation&);
    core::ComponentSlots* enemy(const core::EnemyCreation&);
    core::ComponentSlots& bullet(const core::BulletCreation&);
    core::ComponentSlots& effect(core::ProjectilePoint,std::int32_t clip,std::int32_t image,bool loop);
    core::ComponentSlots& raw_entity(std::int32_t type);
    core::DropEffectWorld& effect_world(){return effect_world_;}
    core::ProjectileLink& acquire_projectile()override;
    core::ProjectileModification& acquire_modification()override;
    void return_component(std::int32_t,core::Component*)override;
    void add_animation(core::ComponentSlots&,std::int32_t rate,std::int32_t backup)override;
    std::int32_t id(const core::ComponentSlots&)const;
    core::QuestMemberId handle(const core::ComponentSlots&)const;
    bool destroyed(const core::ComponentSlots&)const;
    core::PlayerWorldState& player_world(){return player_world_;}
    core::EntityEvents& events(){return gameplay_events_;}
    void mark_player(core::ComponentSlots& e){e.add(30,&player_marker_);}
    void destroy(core::ComponentSlots&,bool remove_registration=true);
    core::QuestEntityLifetime& entity_lifetime(core::QuestMemberId)override;
    void delete_from_manager(core::QuestMemberId,bool)override;
    const core::EntityRegistry& registry()const{return registry_;}
    core::EntitySubset* subset(const std::vector<std::int32_t>&)override;
    core::AnimationResource* resource(std::int32_t)override;
    bool registered_id(std::int32_t)override;
    core::ComponentSlots* create(std::int32_t type,std::int32_t id)override;
    void register_entity(core::ComponentSlots&,bool temporary)override;
    void added(core::ComponentSlots&,std::int32_t,core::Component*)override;
    void removed(core::ComponentSlots&,std::int32_t,core::Component*)override;
    void changed(core::ComponentSlots&,std::int32_t,core::Component*,core::Component*)override;
    core::PartyCharacters character_table()override;
    std::int32_t weapon_image(std::int32_t)override;
    core::HumanHats hats()override{return hats_;}
    core::EventInstruction parameter_ids()override{return parameter_ids_;}
    std::int32_t maximum_level(std::int32_t)override;
    const core::ShopParameterUpgrade& shop_upgrade(std::int32_t)override;
    core::EnemyRecords enemy_records()override{return catalog_.enemies;}
    const core::ParameterMaximumLevels& enemy_maximum_levels()override{return enemy_maxima_;}
    core::EventInstruction enemy_parameter_ids()override{return enemy_parameter_ids_;}
    std::int32_t enemy_resource_id(std::string_view)override;
    core::BulletRecords bullet_records()override{return catalog_.bullets;}
    core::BulletFactoryWorld* bullet_global_world()override{return &bullet_world_;}
    core::StageGeometry* bullet_global_map()override{return map_;}
    std::int32_t clip_max_frame(std::int32_t,std::int32_t)override;
    void release_collision(core::Component*)override;
    core::ContactPosition& acquire_position()override;
    core::ProjectileSpeed& acquire_speed()override;
    core::EntitySprite& acquire_sprite()override;
    core::EntityDepth& acquire_depth()override;
    core::CellComponent& acquire_cell()override;
    core::EntityImage& acquire_image()override;
    core::EntityPlayback& acquire_playback()override;
    core::HumanRecordLink& acquire_human()override;
    core::CombatParameterLinks& acquire_parameters()override;
    core::EquipmentRecordLinks& acquire_equipment()override;
    core::ContactDirection& acquire_direction()override;
    core::HumanAI& acquire_ai()override;
    core::Component& acquire_input()override;
    core::ProjectileCollision& acquire_collision()override;
    core::EnemyRecordLink& acquire_enemy()override;
    core::HumanAI& acquire_enemy_ai()override{return acquire_ai();}
    core::CombatParameterLinks& acquire_enemy_parameters()override{return acquire_parameters();}
    core::ContactDirection& acquire_enemy_direction()override{return acquire_direction();}
    core::DefeatColor& acquire_enemy_color()override;
    core::BulletComponent& acquire_bullet()override;
    core::EmitterComponent& acquire_emitter()override;
    core::ProjectileCollision& acquire_bullet_collision()override{return acquire_collision();}
    core::ProjectileDamage& acquire_bullet_damage()override;
    core::DefeatColor& acquire_bullet_color()override{return acquire_enemy_color();}
    core::EffectComponent& acquire_effect()override;
    core::GarbageLifetime& acquire_garbage()override;
private:
    void remove_all_components(core::QuestMemberId)override;
    void dispose_entity_events(core::QuestMemberId)override;
    void erase_registration(core::QuestMemberId)override;
    struct Node {
        std::int32_t id;core::QuestMemberId handle;core::QuestEntityLifetime lifetime;
        core::ComponentEvents events;core::ComponentSlots components;
        Node(std::int32_t value,core::QuestMemberId key,std::int32_t slots):id(value),handle(key),components(slots,&events){}
    };
    const GameplayCatalog& catalog_;MissionProgress& progress_;std::int32_t slots_;core::StageGeometry* map_;
    Resources resources_;ResourceIds resource_ids_;
    std::deque<Node> nodes_;std::map<const core::ComponentSlots*,Node*> node_index_;
    std::vector<Node*> handles_; // Stable host identity, including multiple native temporary ID -1 entities.
    core::EntityRegistry registry_{*this};core::DynamicSubsets groups_;
    core::PlayerWorldState player_world_{nullptr,&groups_,0};core::Component player_marker_;
    core::EntityEvents gameplay_events_;
    core::EntityIdState ids_;core::BulletFactoryWorld bullet_world_;core::DropEffectWorld effect_world_;
    std::vector<std::optional<core::PartyCharacter>> party_rows_;core::PartyCharacters party_;
    std::vector<std::optional<core::HumanHat>> hat_rows_;core::HumanHats hats_;
    core::ParameterMaximumLevels maxima_=core::initial_parameter_maximum_levels(),enemy_maxima_=core::initial_enemy_parameter_maximum_levels();
    core::EventInstruction parameter_ids_=core::initial_character_parameter_ids(),enemy_parameter_ids_=core::initial_enemy_parameter_ids();
    core::ShopParameterTable shop_;
    std::deque<core::ContactPosition> positions_;std::deque<core::ProjectileSpeed> speeds_;
    std::deque<core::EntitySprite> sprites_;std::deque<core::EntityDepth> depths_;std::deque<core::CellComponent> cells_;
    std::deque<core::EntityImage> images_;std::deque<core::EntityPlayback> playback_;std::deque<core::HumanRecordLink> humans_;
    std::deque<core::CombatParameterLinks> parameters_;std::deque<core::EquipmentRecordLinks> equipment_;
    std::deque<core::ContactDirection> directions_;std::deque<core::HumanAI> ai_;std::deque<core::Component> inputs_;
    std::deque<core::ProjectileCollision> collisions_;std::deque<core::EnemyRecordLink> enemies_;std::deque<core::DefeatColor> colors_;
    std::deque<core::BulletComponent> bullets_;std::deque<core::EmitterComponent> emitters_;std::deque<core::ProjectileDamage> damages_;
    std::deque<core::EffectComponent> effects_;std::deque<core::GarbageLifetime> garbage_;
    std::deque<core::ProjectileLink> projectiles_;std::deque<core::ProjectileModification> modifications_;
    std::vector<std::pair<std::int32_t,core::Component*>> retired_; // Retain detached objects; pooling reuse remains unbound.
};
} // namespace saga::desktop
