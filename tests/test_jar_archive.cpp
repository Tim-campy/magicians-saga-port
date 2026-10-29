#include "assets/jar_archive.hpp"
#include "jar_fixture.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void set32(Bytes& out, std::size_t pos, std::uint32_t n) {
    for (unsigned shift : {24U, 16U, 8U, 0U}) out.at(pos++) = static_cast<std::uint8_t>(n >> shift);
}
using saga::test::jar_fixture;
void rejects(const Bytes& data, saga::assets::JarLimits limits = {}) {
    try { (void)saga::assets::parse_jar(data.data(), data.size(), limits); }
    catch (const saga::assets::JarError&) { return; }
    throw std::runtime_error("Malformed input unexpectedly accepted");
}
}

int main() {
    try {
        const auto good = jar_fixture({{"first", {0, 1, 255}}, {"empty", {}}, {"first", {42}}});
        const auto parsed = saga::assets::parse_jar(good.data(), good.size());
        require(parsed.members.size() == 3, "Count mismatch");
        require(parsed.members[0].name == "first" && parsed.members[0].size == 3, "Name/size mismatch");
        require(parsed.members[1].offset == 7 && parsed.members[1].size == 0, "Empty payload mismatch");
        require(parsed.members[2].offset == 11 && good.at(parsed.members[2].payload_offset) == 42, "Payload offset mismatch");
        const auto zero = jar_fixture({});
        require(saga::assets::parse_jar(zero.data(), zero.size()).members.empty(), "Empty archive rejected");
        // No original game names or payload bytes are required for these fixtures.
        const std::string unusual = std::string("a\"\\\n\0", 5) + u8"é魔😀";
        const auto utf = jar_fixture({{unusual, {1}}});
        require(saga::assets::parse_jar(utf.data(), utf.size()).members[0].name == unusual, "UTF-8/control bytes changed");
        for (std::size_t length = 0; length < good.size(); ++length)
            rejects(Bytes(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(length)));
        auto bad = good; bad.push_back(0); rejects(bad);
        bad = good; set32(bad, 0, 0xffffffffU); rejects(bad);
        bad = good; set32(bad, 4, 0xffffffffU); rejects(bad);
        bad = good; set32(bad, 8, 0xffffffffU); rejects(bad);
        bad = good; set32(bad, 12, 0xffffffffU); rejects(bad);
        bad = good; bad[parsed.data_offset - 1U] = 1; rejects(bad);
        bad = good; set32(bad, parsed.data_offset, 2); rejects(bad);
        const std::size_t offsets = 12U + (4U + 5U) * 3U;
        bad = good; set32(bad, offsets, 1); rejects(bad);
        bad = good; set32(bad, offsets + 4U, 6); rejects(bad);
        bad = good; set32(bad, offsets + 12U, 0xffffffffU); rejects(bad);
        bad = good; bad.push_back(0); set32(bad, 4, 17); rejects(bad);
        for (const auto& name : {std::string("\xc0\xaf"), std::string("\xed\xa0\x80"),
                                 std::string("\xf4\x90\x80\x80"), std::string("\xe2\x82"), std::string("\x80")})
            rejects(jar_fixture({{name, {}}}));
        auto limits = saga::assets::JarLimits{};
        limits.max_bytes = good.size() - 1U; rejects(good, limits);
        limits = {}; limits.max_members = 2; rejects(good, limits);
        limits = {}; limits.max_name_bytes = 4; rejects(good, limits);
        try { (void)saga::assets::parse_jar(nullptr, 12); throw std::runtime_error("Null accepted"); }
        catch (const saga::assets::JarError&) {}
        std::cout << "PASS: valid/empty/duplicate/UTF8 names, payload views, all truncations, overflow, bounds, flags, contiguity, limits\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
