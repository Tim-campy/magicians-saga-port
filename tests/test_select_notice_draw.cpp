#include "core/gameplay/select_form_pass.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
template<class E,class F>void fails(F action){bool caught=false;try{action();}catch(const E&){caught=true;}check(caught,"expected exception");}
EventInstruction row(std::initializer_list<std::int32_t> values){return std::make_shared<EventIntegers>(values);}
EventScript rows(std::initializer_list<EventInstruction> values={}){return std::make_shared<std::vector<EventInstruction>>(values);}
struct Cursor final:SelectNoticeCursor {
    EventScript source;std::size_t index=0,count;int& disposed;
    Cursor(EventScript s,int& d):source(std::move(s)),count(source->size()),disposed(d){}
    std::optional<EventInstruction> next()override{
        if(source->size()!=count)throw std::runtime_error("list version");
        if(index==count)return std::nullopt;return source->at(index++);
    }
    void dispose()override{++disposed;}
};
struct Host final:SelectNoticeDrawingHost {
    int disposed=0;bool null_resource=false,fail_draw=false;
    std::vector<std::int32_t> resources;
    std::vector<std::array<std::int32_t,6>> draws;
    std::function<void()> after_resource,after_draw;
    std::int32_t current_resource=0;
    std::int32_t game_width()override{return 600;}
    std::int32_t game_height()override{return 320;}
    std::unique_ptr<SelectNoticeCursor> open_notice_cursor(EventScript source)override{return std::make_unique<Cursor>(std::move(source),disposed);}
    SelectBackgroundResource notice_resource(std::int32_t id)override{
        resources.push_back(id);current_resource=id;if(after_resource)after_resource();return null_resource?nullptr:this;}
    void notice_draw(SelectBackgroundResource r,std::int32_t clip,std::int32_t frame,std::int32_t layer,std::int32_t x,std::int32_t y)override{
        check(r==this,"captured notice resource");draws.push_back({current_resource,clip,frame,layer,x,y});
        if(after_draw)after_draw();if(fail_draw)throw std::runtime_error("draw");}
};
void order_and_owner(){
    auto first=row({11,100,200,5,2,1});auto second=row({2,200,210,6,3,0,-999});
    auto source=rows({first,second,row({0,9999,100})});auto current=source;auto replacement=rows({row({99})});Host host;
    host.after_resource=[&]{if(host.resources.size()==1){first->at(1)=120;first->at(3)=7;first->at(4)=8;first->at(5)=9;current=replacement;}};
    draw_select_notices(current,host);
    check(host.draws==std::vector<std::array<std::int32_t,6>>{{11,7,8,9,120,200},{2,6,3,0,200,210}},"insertion order, six-cell rows and post-resource field rereads; depth is unused");
    check(host.resources==std::vector<std::int32_t>{11,2},"culled three-cell row does not resolve a resource");
    check(host.disposed==1&&replacement->empty()&&source->size()==3,"success disposes captured enumeration then clears the current owner");
}
void failures(){
    {Host h;auto stack=rows({row({11,10})});fails<std::out_of_range>([&]{draw_select_notices(stack,h);});
     check(h.resources.empty()&&h.disposed==1&&stack->size()==1,"short row fails before resource and retains queue");}
    {Host h;h.null_resource=true;auto stack=rows({row({11,10,20,3,4})});fails<std::out_of_range>([&]{draw_select_notices(stack,h);});
     check(h.resources==std::vector<std::int32_t>{11}&&h.disposed==1,"resource query precedes six-cell validation and returned-null gate");}
    {Host h;h.null_resource=true;auto stack=rows({row({11,10,20,3,4,0})});fails<std::invalid_argument>([&]{draw_select_notices(stack,h);});
     check(h.disposed==1&&!stack->empty(),"missing resource retains queue");}
    {Host h;h.fail_draw=true;auto stack=rows({row({11,10,20,3,4,0})});fails<std::runtime_error>([&]{draw_select_notices(stack,h);});
     check(h.disposed==1&&!stack->empty(),"draw failure disposes without clearing");}
    {Host h;auto stack=rows({row({11,10,20,3,4,0})});h.after_draw=[&]{stack->push_back(row({11,20,30,4,5,0}));};
     fails<std::runtime_error>([&]{draw_select_notices(stack,h);});check(h.disposed==1&&stack->size()==2,"host enumeration invalidation stops a structurally changed list");}
    {Host h;auto stack=rows();draw_select_notices(stack,h);check(h.disposed==1&&h.draws.empty(),"empty queue still opens/disposes before clearing");}
}
}
int main(){order_and_owner();failures();}
