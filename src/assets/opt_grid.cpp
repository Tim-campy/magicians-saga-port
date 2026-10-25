#include "assets/opt_grid.hpp"

namespace saga::assets {
OptGrid parse_opt(const std::uint8_t* data, std::size_t size, OptLimits limits) {
    if (size > limits.max_bytes) throw OptError("OPT input exceeds byte limit");
    if (!data || size == 0) throw OptError("Missing OPT header");
    if (data[0] == 255U) throw OptError("Unsupported FF OPT variant");
    if (size < 4U) throw OptError("Truncated OPT header");
    if (data[2] > 127U || data[3] > 127U) throw OptError("Negative signed OPT grid count");
    const std::uint32_t cell_count = std::uint32_t{data[2]} * data[3];
    if (cell_count > limits.max_cells) throw OptError("OPT cell limit exceeded");
    if (cell_count > size - 4U) throw OptError("Missing OPT cell counts");

    // Validate before allocation. The first pass counts exact storage required.
    std::size_t pos = 4U;
    std::uint32_t piece_count = 0;
    for (std::uint32_t i = 0; i < cell_count; ++i) {
        if (pos >= size) throw OptError("Missing OPT cell count");
        const auto count = data[pos++];
        if (count > 127U) throw OptError("Negative signed OPT piece count");
        if (count > limits.max_pieces - piece_count) throw OptError("OPT piece limit exceeded");
        piece_count += count;
        const auto byte_count = std::size_t{count} * 14U;
        if (byte_count > size - pos) throw OptError("Truncated OPT piece data");
        pos += byte_count;
    }
    if (pos != size) throw OptError("Unconsumed OPT bytes");

    OptGrid result{data[0], data[1], data[2], data[3], {}, {}};
    result.cells.reserve(cell_count);
    result.pieces.reserve(piece_count);
    pos = 4U;
    for (std::uint32_t i = 0; i < cell_count; ++i) {
        const auto count = data[pos++];
        result.cells.push_back({static_cast<std::uint32_t>(result.pieces.size()), count});
        for (std::uint8_t j = 0; j < count; ++j) {
            OptPiece piece{};
            for (auto& value : piece.values) {
                const auto raw = (std::uint32_t{data[pos]} << 8U) | data[pos + 1U];
                // Arithmetic conversion avoids implementation-defined unsigned-to-signed narrowing.
                const auto signed_value = raw <= 32767U ? static_cast<std::int32_t>(raw)
                    : static_cast<std::int32_t>(raw) - 65536;
                value = static_cast<std::int16_t>(signed_value);
                pos += 2U;
            }
            result.pieces.push_back(piece);
        }
    }
    return result;
}
} // namespace saga::assets
