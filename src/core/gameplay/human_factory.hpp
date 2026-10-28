#pragma once
#include "party_start.hpp"
#include "entity_visual.hpp"
#include "character_parameter_set.hpp"

namespace saga::core {
struct HumanHat {std::int32_t flags=0,image=0;};
using HumanHats=std::shared_ptr<std::vector<HumanHat*>>;
// Only CacheArray construction is needed here: new empty List<T>, zero count_.
// Growth, script execution and other CacheArray operations are not implemented.
struct HumanScriptCache {
    std::shared_ptr<std::vector<std::shared_ptr<void>>> array=std::make_shared<std::vector<std::shared_ptr<void>>>();
    std::int32_t count=0;
};
struct HumanAI final:Component {
    std::shared_ptr<HumanScriptCache> scripts;
    std::int32_t state=0; // Only these fields are written by AddAI.
};
class HumanFactoryHost:public EntityVisualHost,public PlaybackPool,public PartyCollisionPool,
                       public CharacterParameterHost {
public:
    virtual PartyCharacters character_table()=0;
    virtual std::int32_t weapon_image(std::int32_t weapon)=0; // Data.equipmentData[id].img_.
    virtual HumanHats hats()=0;
    virtual EventInstruction parameter_ids()=0;
    virtual HumanRecordLink& acquire_human()=0;
    virtual CombatParameterLinks& acquire_parameters()=0;
    virtual EquipmentRecordLinks& acquire_equipment()=0;
    virtual ContactDirection& acquire_direction()=0;
    virtual HumanAI& acquire_ai()=0;
    virtual Component& acquire_input()=0;
};
// World.CreateHuman 0x1133a14: complete creation order and component payloads.
ComponentSlots& create_human(const HumanCreation&,EntityIdState&,HumanFactoryHost&);
// Bridge for the World.world_ singleton selected by CreateSubChara.
class HumanFactory final:public PartyHumanFactory {
public:
    HumanFactory(EntityIdState& ids,HumanFactoryHost& host):ids_(ids),host_(host){}
    ComponentSlots* create_human(const HumanCreation& args)override{return &saga::core::create_human(args,ids_,host_);}
private:
    EntityIdState& ids_;HumanFactoryHost& host_;
};
} // namespace saga::core
