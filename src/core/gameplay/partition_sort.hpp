#pragma once
#include <cstdint>

namespace saga::core::detail {
// Shared by the recovered AppData and MeshManager sorts. Their equal-key
// partition order is observable, so std::sort is not a replacement contract.
inline std::int32_t sort_word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
template<class Key>std::int32_t native_pivot(std::int32_t left,std::int32_t right,Key& key){
    for(auto i=sort_word(static_cast<std::uint32_t>(left)+1U);i<=right;i=sort_word(static_cast<std::uint32_t>(i)+1U)){
        const auto first=key(left),other=key(i);if(first!=other)return first<other?i:left;
    }
    return -1;
}
template<class Key,class Swap>std::int32_t native_partition(std::int32_t left,std::int32_t right,std::int32_t pivot,Key& key,Swap& swap){
    if(left>right)return left;
    auto i=left,j=right;
    do{
        while(i<=right&&key(i)<pivot)i=sort_word(static_cast<std::uint32_t>(i)+1U);
        while(j>=left&&key(j)>=pivot)j=sort_word(static_cast<std::uint32_t>(j)-1U);
        if(i>j)return i;
        swap(i,j);i=sort_word(static_cast<std::uint32_t>(i)+1U);j=sort_word(static_cast<std::uint32_t>(j)-1U);
    }while(i<=j);
    return i;
}
template<class Key,class Swap>void native_quicksort(std::int32_t left,std::int32_t right,Key& key,Swap& swap){
    while(left!=right){
        const auto p=native_pivot(left,right,key);if(p==-1)return;
        const auto split=native_partition(left,right,key(p),key,swap);
        native_quicksort(left,sort_word(static_cast<std::uint32_t>(split)-1U),key,swap);left=split;
    }
}
} // namespace saga::core::detail
