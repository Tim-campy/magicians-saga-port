#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace saga::core {
class RandomSource {
public:
    virtual ~RandomSource()=default;
    virtual std::int32_t next()=0;
};
// Seeded System.Random route only. Public .NET Core 3.1 algorithm, checked against APK anchors.
class LegacySystemRandom final:public RandomSource {
public:
    explicit LegacySystemRandom(std::int32_t seed);
    std::int32_t next() override;
private:
    std::array<std::int32_t,56> seeds_{};
    std::int32_t index_=0,paired_index_=21;
};
struct RandomConfiguration {std::int32_t logic=0;bool fixed_seed=false;};
class RandomReplayHost {
public:
    virtual ~RandomReplayHost()=default;
    virtual bool replay_file_exists()=0;
    virtual std::string replay_record_preference()=0; // DEVCMD_REPLAY_RECORD, default "off".
};
void apply_replay_random_configuration(RandomConfiguration&,bool unity_editor,RandomReplayHost&);
std::int32_t random_seed_from_millis(std::int64_t,bool fixed_seed);
// JRandom.NextInt selects from three independent objects on every call.
class SelectedRandomSource final:public RandomSource {
public:
    SelectedRandomSource(const RandomConfiguration& config,RandomSource* system,
                         RandomSource* random2018,RandomSource* xorshift)
        :config_(config),sources_{{system,random2018,xorshift}}{}
    std::int32_t next() override;
private:
    const RandomConfiguration& config_;
    std::array<RandomSource*,3> sources_;
};
// Game-specific wrappers, not System.Random.Next(bound) distribution semantics.
std::int32_t random_bounded(RandomSource&,std::int32_t bound);
std::int32_t random_inclusive(RandomSource&,std::int32_t minimum,std::int32_t maximum);
std::int32_t jrandom_bounded(RandomSource&,std::int32_t bound);
bool random_hits1000(RandomSource&,std::int32_t threshold);
bool random_hits(RandomSource&,std::int32_t threshold);
// Random..cctor's 256-cell table is the mutable scratch space used by Lot.
// Array lengths stay fixed within int32; reference replacement is supported.
struct WeightedLotteryState {
    std::shared_ptr<std::vector<std::int32_t>> table=std::make_shared<std::vector<std::int32_t>>(256,0);
};
// Game-specific Random.Lot 0x1129064. Null/empty -> -1 with no draw;
// nonpositive-only weights retain the original retry/bounds-failure behaviour.
std::int32_t weighted_lottery(const std::vector<std::int32_t>*,WeightedLotteryState&,RandomSource&);
}
