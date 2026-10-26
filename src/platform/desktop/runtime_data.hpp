#pragma once
#include "core/gameplay/hashtable_write.hpp"
#include <filesystem>
#include <functional>
namespace saga::desktop {
// Decode the local extractor's 72 little-endian int32 entries; no game table is embedded.
std::shared_ptr<const std::vector<std::int32_t>> decode_hashtable_primes(std::string_view);
std::shared_ptr<const std::vector<std::int32_t>> load_hashtable_primes(const std::filesystem::path& extracted_root);
// Portable keys must be u16string objects; clone dispatch remains an explicit runtime input.
class RuntimeCallContextHost : public core::CallContextHost {
public:
    RuntimeCallContextHost(std::shared_ptr<const std::vector<std::int32_t>> primes,
                          std::function<core::AsyncValue(core::AsyncValue)> clone);
    std::shared_ptr<core::CallDataStore> make_store() override;
    std::optional<std::u16string> string_key(core::AsyncValue) override;
    core::AsyncValue clone_correlation(core::AsyncValue) override;
private:
    std::shared_ptr<const std::vector<std::int32_t>> primes_;
    std::function<core::AsyncValue(core::AsyncValue)> clone_;
};
}
