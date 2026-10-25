#include "assets/opt_grid.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
using Piece = std::array<std::int16_t, 7>;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
Bytes fixture(std::uint8_t width, std::uint8_t height, std::uint8_t columns, std::uint8_t rows,
              const std::vector<std::vector<Piece>>& cells) {
    Bytes data{width, height, columns, rows};
    for (const auto& cell : cells) {
        data.push_back(static_cast<std::uint8_t>(cell.size()));
        for (const auto& piece : cell) {
            for (auto value : piece) {
                const auto raw = static_cast<std::uint16_t>(value);
                data.push_back(static_cast<std::uint8_t>(raw >> 8U));
                data.push_back(static_cast<std::uint8_t>(raw));
            }
        }
    }
    return data;
}
void rejects(const Bytes& data, saga::assets::OptLimits limits = {}) {
    try { (void)saga::assets::parse_opt(data.data(), data.size(), limits); }
    catch (const saga::assets::OptError&) { return; }
    throw std::runtime_error("Malformed OPT accepted");
}
}

int main() {
    try {
        const Piece first{{-32768, 32767, -1, 0, 1, -1234, 2345}};
        const Piece second{{7, 6, 5, 4, 3, 2, 1}};
        auto data = fixture(254, 255, 2, 2, {{first, second}, {}, {second}, {first}});
        auto grid = saga::assets::parse_opt(data.data(), data.size());
        require(grid.cell_width == 254 && grid.cell_height == 255, "Unsigned steps changed");
        require(grid.columns == 2 && grid.rows == 2 && grid.cells.size() == 4, "Grid changed");
        require(grid.pieces.size() == 4 && grid.pieces[0].values == first, "Signed BE16 values changed");
        require(grid.pieces[1].values == second && grid.pieces[3].values == first, "Piece order changed");
        require(grid.cells[0].first_piece == 0 && grid.cells[0].piece_count == 2, "First cell changed");
        require(grid.cells[1].first_piece == 2 && grid.cells[1].piece_count == 0, "Empty cell lost");
        require(grid.cells[2].first_piece == 2 && grid.cells[3].first_piece == 3, "Row-major cell order changed");
        // Output owns all scalar storage independently of input bytes.
        data[5] = 0;
        require(grid.pieces[0].values == first, "Output references mutable input");

        const auto good = fixture(0, 0, 2, 2, {{first}, {}, {second}, {first, second}});
        const auto zero_step = saga::assets::parse_opt(good.data(), good.size());
        require(zero_step.cell_width == 0 && zero_step.cell_height == 0, "Zero step normalized");
        for (const auto& zero : {Bytes{0, 0, 0, 127}, Bytes{0, 255, 127, 0}, Bytes{0, 0, 0, 0}})
            require(saga::assets::parse_opt(zero.data(), zero.size()).cells.empty(), "Zero grid rejected");
        for (std::size_t n = 0; n < good.size(); ++n)
            rejects(Bytes(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(n)));
        auto bad = good; bad.push_back(0); rejects(bad);
        rejects({255}); rejects({255, 0, 0, 0});
        bad = good; bad[2] = 128; rejects(bad);
        bad = good; bad[3] = 255; rejects(bad);
        bad = good; bad[4] = 128; rejects(bad);
        bad = good; bad[4] = 255; rejects(bad);
        bad = good; bad[4] = 127; rejects(bad);
        auto limits = saga::assets::OptLimits{};
        limits.max_bytes = good.size() - 1U; rejects(good, limits);
        limits = {}; limits.max_cells = 3; rejects(good, limits);
        limits = {}; limits.max_pieces = 3; rejects(good, limits);
        const auto max_cell = fixture(1, 1, 1, 1, {std::vector<Piece>(127, first)});
        require(saga::assets::parse_opt(max_cell.data(), max_cell.size()).pieces.size() == 127, "127 pieces rejected");
        const auto max_grid = fixture(1, 1, 127, 127, std::vector<std::vector<Piece>>(127U * 127U));
        require(saga::assets::parse_opt(max_grid.data(), max_grid.size()).cells.size() == 16129, "Maximum regular grid rejected");
        try { (void)saga::assets::parse_opt(nullptr, 4); throw std::runtime_error("Null accepted"); }
        catch (const saga::assets::OptError&) {}
        std::cout << "PASS: regular OPT signed/unsigned widths, BE16 extremes, row-major/empty cells, ownership, truncations, FF, negative counts, limits\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
