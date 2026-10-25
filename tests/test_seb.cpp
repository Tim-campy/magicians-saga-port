#include "assets/seb.hpp"
#include <iostream>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void short_value(Bytes& out, std::int16_t value) {
    const auto raw = static_cast<std::uint16_t>(value);
    out.push_back(static_cast<std::uint8_t>(raw >> 8U));
    out.push_back(static_cast<std::uint8_t>(raw));
}
void rejects(const Bytes& data, saga::assets::SebLimits limits = {}) {
    try { (void)saga::assets::parse_seb(data.data(), data.size(), limits); }
    catch (const saga::assets::SebError&) { return; }
    throw std::runtime_error("Malformed SEB accepted");
}
struct Case { std::int16_t discriminator; std::uint8_t control; bool extra; bool pair; bool flags; };
Bytes mode1(const Case& c) {
    Bytes out{255, 0, 1, 0, 1, 0, 1};
    for (std::int16_t value : {std::int16_t{-32768}, c.discriminator, std::int16_t{32767}, std::int16_t{-1}})
        short_value(out, value);
    out.push_back(c.control);
    if (c.extra) out.insert(out.end(), {255, 255, 255, 255});
    if (c.pair) { short_value(out, -32768); short_value(out, 32767); }
    short_value(out, -7); short_value(out, 8);
    if (c.flags) out.push_back(255);
    return out;
}
}

int main() {
    try {
        const std::array<std::int16_t, 10> values{{-32768,32767,-1,0,1,-2,3,4,5,6}};
        Bytes legacy{0,2,0,3, 0,1,0x80,0};
        for (auto value : values) short_value(legacy, value);
        legacy.insert(legacy.end(), {0,0,0x7f,0xff});
        auto parsed = saga::assets::parse_seb(legacy.data(), legacy.size());
        require(parsed.mode == 0 && parsed.header_value == 3 && parsed.groups.size() == 2, "Legacy header changed");
        require(parsed.records.size() == 1 && parsed.records[0].values == values, "Legacy signed values changed");
        const auto& record = parsed.records[0];
        require(record.value_count == 10 && !record.control && !record.extra_word && !record.pair && !record.tail_pair && !record.flags,
                "Legacy optional fields fabricated");
        require(parsed.groups[0].legacy_value == -32768 && parsed.groups[1].legacy_value == 32767, "Legacy group short changed");
        require(parsed.groups[1].record_count == 0 && parsed.groups[1].first_record == 1, "Empty legacy group lost");
        auto mutated = legacy; auto owns = saga::assets::parse_seb(mutated.data(), mutated.size());
        mutated[8] = 0;
        require(owns.records[0].values == values, "Output aliases input");
        const std::array<Case, 10> cases{{
            {-9,255,false,false,false}, {-8,255,false,true,false}, {-7,255,false,false,false},
            {-6,0,true,false,false}, {-2,0,true,false,false}, {-1,255,false,false,false},
            {0,0,false,true,true}, {0,1,true,true,true}, {32767,255,true,true,true}, {-32768,1,false,false,false}}};
        for (const auto& c : cases) {
            const auto wire = mode1(c);
            const auto seb = saga::assets::parse_seb(wire.data(), wire.size());
            const auto& r = seb.records.at(0);
            require(seb.mode == 1 && !seb.groups[0].legacy_value && r.value_count == 4, "Mode1 shape changed");
            require(r.values[0] == -32768 && r.values[1] == c.discriminator && r.values[2] == 32767 && r.values[3] == -1, "Mode1 values changed");
            require(r.control == c.control && r.extra_word.has_value() == c.extra && r.pair.has_value() == c.pair && r.flags.has_value() == c.flags, "Conditional presence differs");
            if (r.extra_word) require(*r.extra_word == 0xffffffffU, "Unsigned word lost bits");
            if (r.pair) require((*r.pair)[0] == -32768 && (*r.pair)[1] == 32767, "Pair signedness changed");
            require(r.tail_pair && (*r.tail_pair)[0] == -7 && (*r.tail_pair)[1] == 8, "Tail changed");
            if (r.flags) require(*r.flags == 255, "Flags truncated");
            for (std::size_t n = 0; n < wire.size(); ++n)
                rejects(Bytes(wire.begin(), wire.begin() + static_cast<std::ptrdiff_t>(n)));
            auto bad = wire; bad.push_back(0); rejects(bad);
        }
        for (std::size_t n = 0; n < legacy.size(); ++n)
            rejects(Bytes(legacy.begin(), legacy.begin() + static_cast<std::ptrdiff_t>(n)));
        auto bad = legacy; bad.push_back(0); rejects(bad);
        for (const auto& empty : {Bytes{0,0,0,0}, Bytes{255,0,0,0,0}})
            require(saga::assets::parse_seb(empty.data(), empty.size()).groups.empty(), "Empty file structure rejected");
        const Bytes empty_group{255,0,1,0,0,0,0};
        require(saga::assets::parse_seb(empty_group.data(), empty_group.size()).records.empty(), "Empty mode1 group rejected");
        rejects({254}); rejects({128});
        rejects({255,128,0,0,0}); rejects({0,0,128,0});
        rejects({0,1,0,0,255,255,0,0}); rejects({255,0,1,0,0,128,0});
        auto limits = saga::assets::SebLimits{};
        limits.max_bytes = legacy.size() - 1U; rejects(legacy, limits);
        limits = {}; limits.max_groups = 1; rejects(legacy, limits);
        limits = {}; limits.max_records = 0; rejects(legacy, limits);
        Bytes maximum_groups{0x7f,0xff,0,0}; maximum_groups.resize(4U + 32767U * 4U, 0);
        require(saga::assets::parse_seb(maximum_groups.data(), maximum_groups.size()).groups.size() == 32767, "Mode0 count width changed");
        Bytes maximum_records{0,1,0,0,0x7f,0xff,0,0}; maximum_records.resize(8U + 32767U * 20U, 0);
        require(saga::assets::parse_seb(maximum_records.data(), maximum_records.size()).records.size() == 32767, "Record count width changed");
        try { (void)saga::assets::parse_seb(nullptr, 4); throw std::runtime_error("Null accepted"); }
        catch (const saga::assets::SebError&) {}
        std::cout << "PASS: SEB modes0/1, conditional branches, signedness, unsigned word/flags, empty groups, truncations, unknown modes, limits\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
