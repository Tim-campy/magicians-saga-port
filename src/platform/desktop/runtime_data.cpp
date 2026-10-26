#include "runtime_data.hpp"
#include <fstream>
namespace saga::desktop {
std::shared_ptr<const std::vector<std::int32_t>> decode_hashtable_primes(std::string_view bytes){
    if(bytes.size()!=288)throw std::invalid_argument("Hashtable prime table must have 288 bytes");
    auto values=std::make_shared<std::vector<std::int32_t>>();values->reserve(72);
    std::uint32_t previous=1;
    for(std::size_t i=0;i<bytes.size();i+=4){
        std::uint32_t value=0;
        for(unsigned j=0;j<4;++j)value|=std::uint32_t(static_cast<unsigned char>(bytes[i+j]))<<(j*8U);
        if(value<=previous||value>0x7fffffffU||!core::hashtable_is_prime(static_cast<std::int32_t>(value)))
            throw std::invalid_argument("Hashtable prime entries must be positive, increasing primes");
        values->push_back(static_cast<std::int32_t>(value));previous=value;
    }
    return values;
}
std::shared_ptr<const std::vector<std::int32_t>> load_hashtable_primes(const std::filesystem::path& root){
    std::ifstream input(root/"runtime"/"hashtable-primes.bin",std::ios::binary);
    if(!input)throw std::runtime_error("Cannot open runtime Hashtable prime table");
    std::string bytes(288,'\0');input.read(bytes.data(),288);
    if(input.gcount()!=288||input.peek()!=std::char_traits<char>::eof()||input.bad())
        throw std::invalid_argument("Invalid runtime Hashtable prime table length/read");
    return decode_hashtable_primes(bytes);
}
RuntimeCallContextHost::RuntimeCallContextHost(std::shared_ptr<const std::vector<std::int32_t>> primes,
    std::function<core::AsyncValue(core::AsyncValue)> clone):primes_(std::move(primes)),clone_(std::move(clone)){
    if(!primes_||!clone_)throw std::invalid_argument("Runtime call host requires primes and clone dispatch");
}
std::shared_ptr<core::CallDataStore> RuntimeCallContextHost::make_store(){
    return std::make_shared<core::StringCallDataStore>(core::make_hashtable(0,1.0F,*primes_),primes_);
}
std::optional<std::u16string> RuntimeCallContextHost::string_key(core::AsyncValue key){
    if(!key)return std::nullopt;
    return *std::static_pointer_cast<std::u16string>(key);
}
core::AsyncValue RuntimeCallContextHost::clone_correlation(core::AsyncValue value){return clone_(value);}
}
