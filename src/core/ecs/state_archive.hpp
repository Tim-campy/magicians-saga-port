#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace saga::core {
// Canonical checkpoint bytes, independent of object layout and host byte order.
// Callers select fields and sequence order; this does not serialize arbitrary objects.
class StateArchive {
public:
    const std::vector<std::uint8_t>& data()const noexcept{return data_;}
    void number(bool);
    void number(std::int8_t);void number(std::uint8_t);
    void number(std::int16_t);void number(std::uint16_t);
    void number(std::int32_t);void number(std::uint32_t);
    void number(std::int64_t);void number(std::uint64_t);
    void number(float); // IEEE binary32 bits, including signed zero and NaN payloads.
    void number(double)=delete;
    void count(std::size_t); // uint64 sequence length.
    void text(std::string_view); // uint64 byte count followed by the exact bytes.
    void text(std::u16string_view); // uint64 unit count followed by little-endian UTF16 units.
    void bytes(const void*,std::size_t); // Null is accepted only with length 0.
    void tag(std::string_view value){text(value);}
    // Emit domain text and uint64 ID; return that ID. Null is 0 in every domain.
    // IDs are global, assigned by first encounter of each typed (domain,object).
    // Type information is a lookup discriminator only; no compiler names enter bytes.
    // Referenced objects must retain their identities throughout archive construction.
    std::uint64_t reference(std::string_view domain,const void* object);
    template<class T>std::uint64_t reference(std::string_view domain,const T* object){return typed_reference(domain,typeid(T),object);}
private:
    std::uint64_t typed_reference(std::string_view,std::type_index,const void*);
    void integer(std::uint64_t,std::size_t width);
    std::vector<std::uint8_t> data_;
    std::unordered_map<std::string,std::unordered_map<std::type_index,std::unordered_map<const void*,std::uint64_t>>> references_;
    std::uint64_t next_reference_=1;
};
}
