#include "game_random.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t wrap(std::int64_t value){
    const auto bits=static_cast<std::uint32_t>(value);
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):
        static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t remainder(std::int32_t value,std::int32_t divisor){
    // ARM64 SDIV/MSUB: zero quotient for divisor 0; INT_MIN / -1 produces INT_MIN.
    if(divisor==0)return value;
    if(divisor==-1&&value==std::numeric_limits<std::int32_t>::min())return 0;
    return value%divisor;
}
}
void apply_replay_random_configuration(RandomConfiguration& state,bool editor,RandomReplayHost& host){
    // IApplication.Awake 0x1b7ebe0..0x1b7ec80: set only, no reset when the gate is false.
    if(editor&&(host.replay_file_exists()||host.replay_record_preference()=="on"))state.fixed_seed=true;
}
std::int32_t random_seed_from_millis(std::int64_t millis,bool fixed){return fixed?0:wrap(millis);}
// JRandom.NextInt 0x1b04d9c.
std::int32_t SelectedRandomSource::next(){
    const auto index=config_.logic==2?2U:config_.logic==1?1U:0U;
    auto* source=sources_[index];if(!source)throw std::invalid_argument("selected random backend is unavailable");
    return source->next();
}
// kairo.unity.math.Random.Rand(bound) 0x1128e00.
std::int32_t random_bounded(RandomSource& source,std::int32_t bound){
    const auto value=source.next();return remainder(value<0?wrap(-static_cast<std::int64_t>(value)):value,bound);
}
// Random.Rand(min,max) 0x1128ea8.
std::int32_t random_inclusive(RandomSource& source,std::int32_t minimum,std::int32_t maximum){
    const auto range=wrap(static_cast<std::int64_t>(maximum)-minimum+1);
    return wrap(static_cast<std::int64_t>(minimum)+remainder(source.next(),range));
}
// JRandom.NextInt(bound) 0x1b04e20.
std::int32_t jrandom_bounded(RandomSource& source,std::int32_t bound){return remainder(source.next(),bound);}
// Random.Hits1000 0x1128f94.
bool random_hits1000(RandomSource& source,std::int32_t threshold){return random_bounded(source,1000)<threshold;}
// Random.Hits 0x1128f34: the draw occurs even outside the probability range.
bool random_hits(RandomSource& source,std::int32_t threshold){return random_bounded(source,100)<threshold;}
}
