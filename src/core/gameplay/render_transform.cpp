#include "render_transform.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing render transform input");return *p;}
template<class T>T& at(const std::shared_ptr<std::vector<T>>& p,std::int32_t i){return required(p.get()).at(static_cast<std::size_t>(i));}
std::int32_t add(std::int32_t a,std::int32_t b){
    const auto value=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(value<0x80000000U?static_cast<std::int64_t>(value):static_cast<std::int64_t>(value)-0x100000000LL);
}
SpriteMatrix& matrix(const SharedSpriteMatrix& m){return required(m.get());}
}
void update_screen_matrix(RenderTransformState& s){
    auto screen=s.screen;
    if(s.local_index>=0){const auto pair=at(s.local,s.local_index);auto left=at(pair,0),right=at(pair,1);set_concat_matrix(matrix(screen),left.get(),right.get());}
    else reset_matrix(matrix(screen));
    auto world=s.world;post_concat_matrix(matrix(s.screen),matrix(world).values);
}
void set_graphics_matrix(RenderTransformState& s,const SpriteMatrix* input){
    SharedSpriteMatrix destination;
    if(s.local_index==-1)destination=s.world;else destination=at(at(s.local,s.local_index),0);
    if(input)copy_matrix(matrix(destination),input);else reset_matrix(matrix(destination));
    update_screen_matrix(s);
}
void push_graphics_matrix(RenderTransformState& s,const SpriteMatrix* input){
    const auto previous=s.local_index;s.local_index=add(previous,1);auto entries=s.local;
    if(s.local_index>=static_cast<std::int64_t>(required(entries.get()).size())){
        const auto size=add(previous,2);if(size<0)throw std::length_error("negative matrix stack size");
        auto grown=std::make_shared<std::vector<MatrixPair>>(static_cast<std::size_t>(size));
        if(entries->size()>grown->size())throw std::out_of_range("matrix stack copy range");
        std::copy(entries->begin(),entries->end(),grown->begin());
        for(std::size_t n=grown->size();n>0;--n){auto& pair=grown->at(n-1);if(pair)break;
            pair=std::make_shared<std::vector<SharedSpriteMatrix>>();pair->push_back(std::make_shared<SpriteMatrix>());pair->push_back(std::make_shared<SpriteMatrix>());}
        s.local=grown;entries=grown;
    }
    auto destination=at(at(entries,s.local_index),1);
    if(s.local_index==0)reset_matrix(matrix(destination));
    else {auto pair=at(entries,add(s.local_index,-1));auto left=at(pair,0),right=at(pair,1);set_concat_matrix(matrix(destination),left.get(),right.get());}
    set_graphics_matrix(s,input);
}
void pop_graphics_matrix(RenderTransformState& s,RenderTransformHost& host){
    s.local_index=add(s.local_index,-1);
    if(s.local_index<=-2){s.local_index=-1;if(s.debug)host.matrix_underflow();}
    update_screen_matrix(s);
}
bool begin_render_matrix(RenderTransformState& s,float& x,float& y,RenderTransformHost& host){
    if(s.local_index<0&&s.local_coordinate_count<1)return false;
    auto flags=s.flags;
    if(s.flag_count>=static_cast<std::int64_t>(required(flags.get()).size()))flags->push_back(1);
    const auto index=s.flag_count;s.flag_count=add(index,1);
    if(x==0.0F&&y==0.0F){at(s.flags,index)=0;return true;}
    at(s.flags,index)=1;
    auto saved=s.saved;
    if(s.saved_count>=static_cast<std::int64_t>(required(saved.get()).size()))saved->push_back(std::make_shared<SpriteMatrix>());
    auto destination=at(s.saved,s.saved_count);auto world=s.world;copy_matrix(matrix(destination),world.get());
    s.saved_count=add(s.saved_count,1);
    if(s.local_coordinate_count!=0)host.begin_local_coordinates(x,y);
    else {pre_translate_matrix(matrix(s.world),x,y);update_screen_matrix(s);}
    y=0.0F;x=0.0F;return true;
}
void end_render_matrix(RenderTransformState& s,RenderTransformHost& host){
    if(s.saved_count<1)return;
    s.flag_count=add(s.flag_count,-1);
    if((at(s.flags,s.flag_count)&1U)==0)return;
    auto world=s.world,source=at(s.saved,add(s.saved_count,-1));copy_matrix(matrix(world),source.get());
    s.saved_count=add(s.saved_count,-1);
    if(s.local_coordinate_count!=0)host.end_local_coordinates();else update_screen_matrix(s);
}
} // namespace saga::core
