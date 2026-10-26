#include "core/gameplay/garbage_lifetime.hpp"
#include "core/gameplay/stage_cleanup.hpp"
#include <array>
#include <cstdlib>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:GarbageUpdateHost,GarbagePool,EntityDeletionHost,ComponentObserver {
    ComponentSlots a{35,this},b{35,this};GarbageLifetime ca,cb,pooled;
    std::array<QuestEntityLifetime,2> life{};std::vector<GarbageEntity> entries;
    std::vector<std::string> calls;std::function<void()> on_delete;bool fail_next=false;
    struct Cursor final:GarbageCursor {
        Host& h;std::size_t index=0;explicit Cursor(Host& v):h(v){}
        ~Cursor()override{h.calls.push_back("dispose");}
        std::optional<GarbageEntity> next()override{
            if(h.fail_next&&index==1)throw std::runtime_error("invalidated subset");
            if(index==h.entries.size())return std::nullopt;
            return h.entries[index++];}
    };
    Host(){a.add(22,&ca);b.add(22,&cb);entries={{0,&a},{1,&b}};calls.clear();}
    GarbageLifetime& acquire_garbage()override{calls.push_back("acquire");return pooled;}
    std::unique_ptr<GarbageCursor> open_garbage_cursor()override{calls.push_back("open");return std::make_unique<Cursor>(*this);}
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{return life.at(static_cast<std::size_t>(id));}
    void delete_from_manager(QuestMemberId id,bool remove)override{
        check(remove);calls.push_back("delete"+std::to_string(id));if(on_delete)on_delete();delete_entity_from_manager(id,remove,*this);}
    void remove_all_components(QuestMemberId id)override{(id==0?a:b).remove_all();calls.push_back("components");}
    void dispose_entity_events(QuestMemberId)override{calls.push_back("events");}
    void erase_registration(QuestMemberId)override{calls.push_back("registry");}
    void added(ComponentSlots&,std::int32_t slot,Component* c)override{if(slot==22){check(dynamic_cast<GarbageLifetime*>(c)!=nullptr);calls.push_back("add");}}
};
int main(){
    for(const auto input:{2,1,0,-1,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()}){
        Host h;h.ca.frames=input;const bool expired=advance_garbage_lifetime(h.a);
        const auto expected=input==std::numeric_limits<int>::min()?std::numeric_limits<int>::max():input-1;
        check(h.ca.frames==expected&&expired==(expected<=0));}
    {Host h;h.a.remove(22);auto& result=add_garbage_lifetime(h.a,2,h);check(&result==&h.a&&h.pooled.frames==2&&h.calls==std::vector<std::string>{"acquire","add"});}
    {Host h;h.ca.frames=7;add_garbage_lifetime(h.a,2,h);check(h.ca.frames==7&&h.pooled.frames==2&&h.a.get(22)==&h.ca&&h.calls==std::vector<std::string>{"acquire"});}
    {Host h;GarbagePass pass;h.ca.frames=1;h.cb.frames=2;h.on_delete=[&]{check(h.cb.frames==1&&h.calls[1]=="dispose");};pass.update(h);
        check(h.life[0].destroyed&&!h.life[1].destroyed&&!h.a.contains(22)&&h.b.contains(22));
        check(h.calls==std::vector<std::string>{"open","dispose","delete0","components","events","registry"});
        h.entries={{1,&h.b}};h.on_delete=nullptr;pass.update(h);check(h.life[1].destroyed&&!h.b.contains(22));}
    {Host h;GarbagePass pass;h.ca.frames=h.cb.frames=1;h.on_delete=[&]{h.cb.frames=100;};pass.update(h);check(h.life[1].destroyed);} // Queue does not recheck lifetime.
    {Host h;GarbagePass pass;h.ca.frames=1;h.cb.frames=1;h.fail_next=true;bool failed=false;
        try{pass.update(h);}catch(const std::runtime_error&){failed=true;}check(failed&&h.ca.frames==0&&h.cb.frames==1&&!h.life[0].destroyed&&h.calls.back()=="dispose");
        h.fail_next=false;h.entries.clear();pass.update(h);check(!h.life[0].destroyed);} // Failed queue is not replayed.
    {Host h;GarbagePass pass;h.ca.frames=h.cb.frames=1;h.on_delete=[] {throw std::runtime_error("deletion");};bool failed=false;
        try{pass.update(h);}catch(const std::runtime_error&){failed=true;}check(failed&&h.ca.frames==0&&h.cb.frames==0&&!h.life[0].destroyed&&!h.life[1].destroyed);}
    {Host h;GarbagePass pass;h.entries={{0,nullptr}};bool failed=false;try{pass.update(h);}catch(const std::invalid_argument&){failed=true;}check(failed&&h.calls.back()=="dispose");}
    {Host h;GarbagePass pass;h.life[0].destroyed=true;h.ca.frames=1;h.entries={{0,&h.a}};pass.update(h);check(h.ca.frames==0&&h.a.contains(22)&&h.calls.size()==2);}
    {Host h;GarbagePass pass;h.ca.frames=h.cb.frames=1;bool nested=false;
        h.on_delete=[&]{if(!nested){nested=true;h.entries.clear();pass.update(h);}};
        pass.update(h);check(h.life[0].destroyed&&h.life[1].destroyed);}
}
