#include "core/script_payload.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool ok, const char* why) {
    if (!ok) { std::cerr << why << '\n'; std::exit(1); }
}
template<class E, class F> void rejects(F fn) {
    bool failed = false;
    try { fn(); } catch (const E&) { failed = true; }
    check(failed, "missing expected rejection");
}
int main() {
    const ScriptRows rows{{}, {10, -1, std::numeric_limits<std::int32_t>::min(),
                              std::numeric_limits<std::int32_t>::max()}, {0x12345678}};
    const std::vector<std::uint8_t> fixture{
        0,0,0,3, 0,0,0,0, 0,0,0,4,
        0,0,0,10, 255,255,255,255, 128,0,0,0, 127,255,255,255,
        0,0,0,1, 0x12,0x34,0x56,0x78};
    check(encode_script_payload(rows) == fixture, "independent big-endian fixture");
    const auto decoded = decode_script_payload(fixture);
    check(decoded.rows == rows && decoded.consumed == fixture.size(), "fixture decode");
    for (std::size_t n = 0; n < fixture.size(); ++n) {
        const std::vector<std::uint8_t> prefix(fixture.begin(), fixture.begin() + n);
        rejects<std::out_of_range>([&] { decode_script_payload(prefix); });
    }
    auto suffix = fixture; suffix.push_back(99);
    check(decode_script_payload(suffix).consumed == fixture.size(), "prefix consumption");
    check(encode_script_payload({}) == std::vector<std::uint8_t>({0,0,0,0}), "empty writer");
    for (const auto bytes : {std::vector<std::uint8_t>{0,0,0,0,99},
                            std::vector<std::uint8_t>{255,255,255,255,99},
                            std::vector<std::uint8_t>{128,0,0,0,99}}) {
        const auto empty = decode_script_payload(bytes);
        check(empty.rows.empty() && empty.consumed == 4, "nonpositive outer count");
    }
    rejects<std::length_error>([] { decode_script_payload({0,0,0,1,255,255,255,255}); });
    rejects<std::out_of_range>([] { decode_script_payload({127,255,255,255}); });
    rejects<std::out_of_range>([] { decode_script_payload({0,0,0,1,127,255,255,255}); });
    ScriptRows patterns(1);
    for (unsigned shift = 0; shift < 31; ++shift) {
        patterns[0].push_back(static_cast<std::int32_t>(1U << shift));
        patterns[0].push_back(-static_cast<std::int32_t>(1U << shift));
    }
    check(decode_script_payload(encode_script_payload(patterns)).rows == patterns,
          "signed bit patterns");
}
