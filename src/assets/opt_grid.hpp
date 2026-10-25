#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace saga::assets {

struct OptLimits {
    std::size_t max_bytes = 32U * 1024U * 1024U;
    std::uint32_t max_cells = 127U * 127U;
    std::uint32_t max_pieces = 127U * 127U * 127U;
};

struct OptPiece {
    std::array<std::int16_t, 7> values; // Preserve disk order without assuming renderer geometry.
};

struct OptCell {
    std::uint32_t first_piece;
    std::uint8_t piece_count;
};

struct OptGrid {
    std::uint8_t cell_width; // Header step, NOT full image/canvas dimensions.
    std::uint8_t cell_height;
    std::uint8_t columns;
    std::uint8_t rows;
    std::vector<OptCell> cells; // Row-major: row * columns + column; empty cells retained.
    std::vector<OptPiece> pieces;
};

class OptError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Image.LoadOptimize stream VA 0x1b8d2dc and GetOptimize VA 0x1b8e4c0;
// docs/18 and tools/inspect_opt.py prove this regular non-FF disk profile.
// Returned scalars own their storage; input lifetime is independent of output.
OptGrid parse_opt(const std::uint8_t* data, std::size_t size, OptLimits limits = {});

} // namespace saga::assets
