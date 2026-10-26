#include "core/gameplay/stage_cleanup.hpp"
#include "core/ecs/component_slots.hpp"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:StageCleanupHost,QuestEntityHost,EntityDeletionHost,ComponentObserver {
    struct Node {ComponentSlots slots;QuestEntityLifetime life;bool events=true;Node(ComponentObserver* o):slots(40,o){}};
    std::map<QuestMemberId,std::unique_ptr<Node>> nodes;
    std::map<std::int32_t,std::vector<std::optional<QuestMemberId>>> groups{{34,{1,2,std::nullopt}},{29,{1,3}},{26,{4}}};
    std::set<QuestMemberId> registry{1,2,3,4,5};std::optional<QuestMemberId> extra=5;
    std::vector<QuestMemberId> collecting{1,2};std::vector<std::string> calls;std::string fail;
    Component component;bool replace_extra=false;
    Host(){
        for(QuestMemberId id=1;id<=5;++id)nodes[id]=std::make_unique<Node>(this);
        nodes[1]->slots.add(34,&component);nodes[1]->slots.add(29,&component);
        nodes[2]->slots.add(34,&component);nodes[3]->slots.add(29,&component);
        nodes[4]->slots.add(26,&component);nodes[5]->slots.add(35,&component);
        nodes[4]->life.destroyed=true;
    }
    void call(std::string s){calls.push_back(s);if(s==fail)throw std::runtime_error(s);}
    std::vector<std::optional<QuestMemberId>> snapshot_group(std::int32_t slot)override{
        call("snapshot"+std::to_string(slot));return groups[slot];
    }
    QuestEntityHost& destruction_host()override{call("world");return *this;}
    std::optional<QuestMemberId> extra_entity()override{call("extra");return extra;}
    void clear_extra_entity()override{call("clear_extra");extra.reset();}
    void clear_collecting_drops()override{call("clear_collecting");collecting.clear();}
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{call("life"+std::to_string(id));return nodes.at(id)->life;}
    void delete_from_manager(QuestMemberId id,bool erase)override{delete_entity_from_manager(id,erase,*this);}
    void remove_all_components(QuestMemberId id)override{
        call("remove"+std::to_string(id));nodes.at(id)->slots.remove_all();
        // A callback can mutate the collection being cleaned; the current snapshot must survive.
        if(id==1)groups[34].clear();
    }
    void removed(ComponentSlots& slots,std::int32_t,Component*)override{
        for(auto& pair:nodes)if(&pair.second->slots==&slots){
            check(pair.second->events&&!pair.second->life.destroyed);
            call("notify"+std::to_string(pair.first));
            for(auto& group:groups){auto& values=group.second;
                values.erase(std::remove(values.begin(),values.end(),std::optional<QuestMemberId>(pair.first)),values.end());}
        }
    }
    void dispose_entity_events(QuestMemberId id)override{call("dispose"+std::to_string(id));nodes.at(id)->events=false;}
    void erase_registration(QuestMemberId id)override{
        call("erase"+std::to_string(id));registry.erase(id);if(replace_extra&&id==5)extra=3;
    }
};
int main(){
    {Host h;cleanup_quest_stage(h);
        check(h.registry==std::set<QuestMemberId>({4})&&!h.extra&&h.collecting.empty());
        check(h.nodes[1]->life.destroyed&&h.nodes[2]->life.destroyed&&h.nodes[3]->life.destroyed&&h.nodes[5]->life.destroyed);
        check(h.nodes[4]->slots.contains(26)&&h.nodes[4]->events); // Already destroyed: wrapper skips manager.
        check(h.calls==std::vector<std::string>({"snapshot34","world","life1","remove1","notify1","notify1","dispose1","erase1",
            "life2","remove2","notify2","dispose2","erase2","snapshot29","world","life3","remove3","notify3","dispose3","erase3",
            "snapshot26","world","life4","snapshot9","world","snapshot31","world","extra","world","life5",
            "remove5","notify5","dispose5","erase5","clear_extra","clear_collecting"}));}
    {Host h;h.fail="snapshot29";try{cleanup_quest_stage(h);check(false);}catch(const std::runtime_error&){}
        check(h.nodes[1]->life.destroyed&&h.nodes[2]->life.destroyed&&!h.nodes[3]->life.destroyed&&h.extra==5&&!h.collecting.empty());}
    {Host h;h.fail="notify1";try{cleanup_quest_stage(h);check(false);}catch(const std::runtime_error&){}
        check(!h.nodes[1]->life.destroyed&&h.nodes[1]->events&&!h.nodes[2]->life.destroyed);}
    {Host h;h.fail="erase5";try{cleanup_quest_stage(h);check(false);}catch(const std::runtime_error&){}
        check(!h.nodes[5]->life.destroyed&&!h.nodes[5]->events&&h.extra==5&&h.registry.count(5)==1&&!h.collecting.empty());}
    {Host h;h.extra=1;cleanup_quest_stage(h);check(h.registry.count(1)==0&&!h.extra);}
    {Host h;h.extra.reset();cleanup_quest_stage(h);check(h.registry.count(5)==1&&!h.nodes[5]->life.destroyed&&h.calls.back()=="clear_collecting");}
    {Host h;h.replace_extra=true;cleanup_quest_stage(h);check(!h.extra);}
}
