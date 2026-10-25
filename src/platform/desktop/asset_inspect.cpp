#include "assets/jar_archive.hpp"
#include "assets/opt_grid.hpp"
#include "assets/seb.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
void json_string(std::ostream& out, std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (char raw : value) {
        const auto c = static_cast<unsigned char>(raw);
        if (c == '"' || c == '\\') out << '\\' << raw;
        else if (c < 0x20U) out << "\\u00" << hex[c >> 4U] << hex[c & 0xfU];
        else out << raw; // Parser has validated UTF-8.
    }
    out << '"';
}

void print_opt(const saga::assets::OptGrid& grid) {
    std::cout << "{\"cell_width\":" << static_cast<unsigned>(grid.cell_width)
              << ",\"cell_height\":" << static_cast<unsigned>(grid.cell_height)
              << ",\"columns\":" << static_cast<unsigned>(grid.columns)
              << ",\"rows\":" << static_cast<unsigned>(grid.rows) << ",\"cells\":[";
    bool first_cell = true;
    for (const auto& cell : grid.cells) {
        if (!first_cell) std::cout << ',';
        first_cell = false;
        std::cout << '[';
        for (std::uint32_t i = 0; i < cell.piece_count; ++i) {
            if (i) std::cout << ',';
            std::cout << '[';
            const auto& piece = grid.pieces[cell.first_piece + i];
            for (std::size_t j = 0; j < piece.values.size(); ++j) {
                if (j) std::cout << ',';
                std::cout << piece.values[j];
            }
            std::cout << ']';
        }
        std::cout << ']';
    }
    std::cout << "]}\n";
}

template<class T>
void print_scalar(const std::optional<T>& value) {
    if (value) std::cout << +*value;
    else std::cout << "null";
}
void print_pair(const std::optional<std::array<std::int16_t, 2>>& value) {
    if (value) std::cout << '[' << (*value)[0] << ',' << (*value)[1] << ']';
    else std::cout << "null";
}
void print_seb(const saga::assets::Seb& seb) {
    std::cout << "{\"mode\":" << static_cast<unsigned>(seb.mode)
              << ",\"header_value\":" << seb.header_value << ",\"groups\":[";
    bool first_group = true;
    for (const auto& group : seb.groups) {
        if (!first_group) std::cout << ',';
        first_group = false;
        std::cout << "{\"legacy_value\":";
        print_scalar(group.legacy_value);
        std::cout << ",\"records\":[";
        for (std::uint32_t i = 0; i < group.record_count; ++i) {
            if (i) std::cout << ',';
            const auto& record = seb.records[group.first_record + i];
            std::cout << "{\"values\":[";
            for (std::uint8_t k = 0; k < record.value_count; ++k) {
                if (k) std::cout << ',';
                std::cout << record.values[k];
            }
            std::cout << "],\"control\":"; print_scalar(record.control);
            std::cout << ",\"extra_word\":"; print_scalar(record.extra_word);
            std::cout << ",\"pair\":"; print_pair(record.pair);
            std::cout << ",\"tail_pair\":"; print_pair(record.tail_pair);
            std::cout << ",\"flags\":"; print_scalar(record.flags);
            std::cout << '}';
        }
        std::cout << "]}";
    }
    std::cout << "]}\n";
}
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
    try {
        const bool opt_mode = argc >= 2 && std::filesystem::path(argv[1]) == "--opt";
        const bool seb_mode = argc >= 2 && std::filesystem::path(argv[1]) == "--seb";
        const bool explicit_mode = opt_mode || seb_mode;
        if ((!explicit_mode && argc != 2) || (explicit_mode && argc != 3)) {
            std::cerr << "Usage: asset_inspect <decoded-container> | --opt <file> | --seb <file>\n";
            return 2;
        }
        const auto max_bytes = opt_mode ? saga::assets::OptLimits{}.max_bytes :
            seb_mode ? saga::assets::SebLimits{}.max_bytes : saga::assets::JarLimits{}.max_bytes;
        const std::filesystem::path path(argv[explicit_mode ? 2 : 1]);
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) throw saga::assets::JarError("Cannot open input file");
        const auto end = input.tellg();
        if (end < 0 || static_cast<std::uint64_t>(end) > max_bytes)
            throw saga::assets::JarError("Input size unavailable or exceeds limit");
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
        input.seekg(0);
        if (!bytes.empty() && !input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
            throw saga::assets::JarError("Could not read complete input");
        if (opt_mode) {
            print_opt(saga::assets::parse_opt(bytes.data(), bytes.size()));
            return std::cout ? 0 : 1;
        }
        if (seb_mode) {
            print_seb(saga::assets::parse_seb(bytes.data(), bytes.size()));
            return std::cout ? 0 : 1;
        }
        const auto archive = saga::assets::parse_jar(bytes.data(), bytes.size());
        std::cout << "{\"data_offset\":" << archive.data_offset << ",\"members\":[";
        bool first = true;
        for (const auto& member : archive.members) {
            if (!first) std::cout << ',';
            first = false;
            std::cout << "{\"name\":";
            json_string(std::cout, member.name);
            std::cout << ",\"offset\":" << member.offset << ",\"size\":" << member.size
                      << ",\"payload_offset\":" << member.payload_offset << '}';
        }
        std::cout << "]}\n";
        return std::cout ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "asset_inspect: " << error.what() << '\n';
        return 1;
    }
}
