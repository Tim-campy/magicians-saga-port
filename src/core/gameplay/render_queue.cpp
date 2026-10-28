#include "render_queue.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing render queue data");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
EventInstruction& row(RenderRows& rows,std::int32_t index){return required(rows.get()).at(static_cast<std::size_t>(index));}
std::int32_t depth(RenderRows& rows,std::int32_t index){return required(row(rows,index).get()).at(render_depth);}
}
RenderQueue make_render_queue(){
    RenderQueue result;result.counts=std::make_shared<EventIntegers>(2,0);
    result.buffers=std::make_shared<std::vector<RenderRows>>(2);
    for(auto& rows:*result.buffers)rows=std::make_shared<std::vector<EventInstruction>>();return result;
}
void clear_render_queue(RenderQueue& state){
    auto& counts=required(state.counts.get());std::fill(counts.begin(),counts.end(),0);
}
void insert_render_command(RenderQueue& state,const RenderCommand& command){
    const auto type=static_cast<std::size_t>(command.type);
    auto& outer=required(state.buffers.get());static_cast<void>(outer.at(type));
    const auto count=required(state.counts.get()).at(type);auto rows=outer.at(type);auto& old=required(rows.get());
    if(old.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("render buffer exceeds int32");
    const auto capacity=static_cast<std::int32_t>(old.size());
    if(count>=capacity){
        const auto next=word(static_cast<std::uint32_t>(capacity)+128U);
        if(next<0)throw std::length_error("negative render buffer capacity");
        auto grown=std::make_shared<std::vector<EventInstruction>>(static_cast<std::size_t>(next));
        std::copy(old.begin(),old.end(),grown->begin());
        for(auto i=capacity;i<next;++i)grown->at(static_cast<std::size_t>(i))=std::make_shared<EventIntegers>(19,0);
        required(state.buffers.get()).at(type)=grown;rows=std::move(grown);
    }
    auto record=row(rows,count);auto& cells=required(record.get());
    for(std::size_t i=0;i<command.values.size();++i)cells.at(i)=command.values[i];
    auto& fresh=required(state.counts.get()).at(type);fresh=word(static_cast<std::uint32_t>(fresh)+1U);
}
std::int32_t render_pivot(RenderRows rows,std::int32_t left,std::int32_t right){
    for(auto index=word(static_cast<std::uint32_t>(left)+1U);index<=right;index=word(static_cast<std::uint32_t>(index)+1U)){
        const auto first=depth(rows,left),other=depth(rows,index);
        if(first!=other)return first<other?index:left;
    }
    return -1;
}
std::int32_t partition_render_rows(RenderRows rows,std::int32_t left,std::int32_t right,std::int32_t pivot){
    if(left>right)return left;
    auto index=left,last=right;
    do{
        while(index<=right&&depth(rows,index)<pivot)index=word(static_cast<std::uint32_t>(index)+1U);
        while(last>=left&&depth(rows,last)>=pivot)last=word(static_cast<std::uint32_t>(last)-1U);
        if(index>last)return index;
        std::swap(row(rows,index),row(rows,last));index=word(static_cast<std::uint32_t>(index)+1U);last=word(static_cast<std::uint32_t>(last)-1U);
    }while(index<=last);
    return index;
}
void quicksort_render_rows(RenderRows rows,std::int32_t left,std::int32_t right){
    while(left!=right){
        const auto pivot_index=render_pivot(rows,left,right);if(pivot_index==-1)return;
        const auto split=partition_render_rows(rows,left,right,depth(rows,pivot_index));
        quicksort_render_rows(rows,left,word(static_cast<std::uint32_t>(split)-1U));left=split;
    }
}
void sort_render_queue(RenderQueue& state,std::int32_t type){
    const auto end=word(static_cast<std::uint32_t>(required(state.counts.get()).at(static_cast<std::size_t>(type)))-1U);
    if(end<0)return;
    auto rows=required(state.buffers.get()).at(static_cast<std::size_t>(type));quicksort_render_rows(rows,0,end);
}
void sort_render_queue(RenderQueue& state){sort_render_queue(state,0);sort_render_queue(state,1);}
}
