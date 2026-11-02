#include "state_archive.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace saga::core {
void StateArchive::integer(std::uint64_t value,std::size_t width){
    for(std::size_t i=0;i<width;++i)data_.push_back(static_cast<std::uint8_t>(value>>(i*8)));
}
void StateArchive::number(bool value){integer(value?1U:0U,1);}
void StateArchive::number(std::int8_t value){number(static_cast<std::uint8_t>(value));}
void StateArchive::number(std::uint8_t value){integer(value,1);}
void StateArchive::number(std::int16_t value){number(static_cast<std::uint16_t>(value));}
void StateArchive::number(std::uint16_t value){integer(value,2);}
void StateArchive::number(std::int32_t value){number(static_cast<std::uint32_t>(value));}
void StateArchive::number(std::uint32_t value){integer(value,4);}
void StateArchive::number(std::int64_t value){number(static_cast<std::uint64_t>(value));}
void StateArchive::number(std::uint64_t value){integer(value,8);}
void StateArchive::number(float value){
    static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"checkpoint requires IEEE binary32");
    std::uint32_t bits;std::memcpy(&bits,&value,sizeof bits);number(bits);
}
void StateArchive::count(std::size_t value){
    static_assert(sizeof(std::size_t)<=sizeof(std::uint64_t),"checkpoint length exceeds uint64");
    number(static_cast<std::uint64_t>(value));
}
void StateArchive::text(std::string_view value){bytes(value.data(),value.size());}
void StateArchive::text(std::u16string_view value){
    count(value.size());for(const auto unit:value)number(static_cast<std::uint16_t>(unit));
}
void StateArchive::bytes(const void* source,std::size_t size){
    if(size&&!source)throw std::invalid_argument("null checkpoint bytes");
    count(size);if(!size)return;
    const auto* first=static_cast<const std::uint8_t*>(source);data_.insert(data_.end(),first,first+size);
}
std::uint64_t StateArchive::reference(std::string_view domain,const void* object){
    return typed_reference(domain,typeid(void),object);
}
std::uint64_t StateArchive::typed_reference(std::string_view domain,std::type_index type,const void* object){
    std::uint64_t id=0;
    if(object){
        auto& objects=references_[std::string(domain)][type];const auto found=objects.find(object);
        if(found!=objects.end())id=found->second;
        else{
            if(!next_reference_)throw std::overflow_error("checkpoint reference IDs exhausted");
            id=next_reference_;objects.emplace(object,id);++next_reference_;
        }
    }
    text(domain);number(id);return id;
}
}
