// Technical PNG/OPT inspection only: no scene layout, game loop or gameplay model.
#include "assets/opt_grid.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 8192
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <stb_image.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace {
constexpr std::uint64_t max_encoded_bytes = 64U * 1024U * 1024U;
constexpr std::uint64_t max_pixels = 16U * 1024U * 1024U;
void require(bool ok, const char* operation) {
    if (!ok) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
struct QuitSDL { ~QuitSDL() { SDL_Quit(); } };
using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
using Renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

std::vector<stbi_uc> read_file(const std::string& filename, std::uint64_t limit, std::size_t minimum) {
    const auto path = std::filesystem::u8path(filename); // SDL_main supplies UTF-8 on Windows.
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Cannot open input file");
    const auto length = input.tellg();
    if (length < 0 || static_cast<std::uint64_t>(length) < minimum || static_cast<std::uint64_t>(length) > limit)
        throw std::runtime_error("Input file size outside supported bounds");
    std::vector<stbi_uc> encoded(static_cast<std::size_t>(length));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(encoded.data()), static_cast<std::streamsize>(encoded.size())))
        throw std::runtime_error("Incomplete input read");
    return encoded;
}

struct Piece {
    std::int64_t x, y, width, height, u, v;
};
struct Composition {
    std::vector<Piece> pieces;
    std::int64_t x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    int width = 0, height = 0;
    std::uint32_t grid_width = 0, grid_height = 0;
    std::uint32_t outside_grid = 0;
    std::uint64_t assigned_pixels = 0;
};

Composition compose_geometry(const std::string& filename, int png_width, int png_height) {
    saga::assets::OptLimits limits;
    limits.max_pieces = 65536U; // Viewer work budget, not a claim about original format limits.
    const auto bytes = read_file(filename, limits.max_bytes, 1);
    const auto grid = saga::assets::parse_opt(bytes.data(), bytes.size(), limits);
    Composition result;
    result.grid_width = std::uint32_t{grid.columns} * grid.cell_width;
    result.grid_height = std::uint32_t{grid.rows} * grid.cell_height;
    result.pieces.reserve(grid.pieces.size());
    for (std::size_t index = 0; index < grid.cells.size(); ++index) {
        const auto& cell = grid.cells[index];
        // Promotion precedes all coordinate arithmetic; header/short operands fit int64.
        const auto cell_x = static_cast<std::int64_t>(index % grid.columns) * grid.cell_width;
        const auto cell_y = static_cast<std::int64_t>(index / grid.columns) * grid.cell_height;
        for (std::uint32_t n = 0; n < cell.piece_count; ++n) {
            const auto& v = grid.pieces[cell.first_piece + n].values;
            if (v[0] != -1) throw std::runtime_error("OPT external image selector unsupported");
            const Piece piece{cell_x + v[1], cell_y + v[2], v[5], v[6], v[3], v[4]};
            if (piece.u < 0 || piece.v < 0 || piece.width < 0 || piece.height < 0 ||
                piece.u + piece.width > png_width || piece.v + piece.height > png_height)
                throw std::runtime_error("OPT crop outside packed PNG");
            if (piece.width == 0 || piece.height == 0) continue;
            const auto right = piece.x + piece.width, bottom = piece.y + piece.height;
            if (piece.x < std::numeric_limits<std::int32_t>::min() || piece.y < std::numeric_limits<std::int32_t>::min() ||
                right > std::numeric_limits<std::int32_t>::max() || bottom > std::numeric_limits<std::int32_t>::max())
                throw std::runtime_error("OPT coordinate overflow");
            if (result.pieces.empty()) { result.x0 = piece.x; result.y0 = piece.y; result.x1 = right; result.y1 = bottom; }
            else {
                result.x0 = std::min(result.x0, piece.x); result.y0 = std::min(result.y0, piece.y);
                result.x1 = std::max(result.x1, right); result.y1 = std::max(result.y1, bottom);
            }
            if (piece.x < 0 || piece.y < 0 || right > result.grid_width || bottom > result.grid_height) ++result.outside_grid;
            result.pieces.push_back(piece);
        }
    }
    if (result.pieces.empty()) throw std::runtime_error("OPT has no nonempty pieces to display");
    const auto width = result.x1 - result.x0, height = result.y1 - result.y0;
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192 ||
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) > max_pixels)
        throw std::runtime_error("OPT occupied canvas exceeds viewer dimensions/pixel budget");
    result.width = static_cast<int>(width); result.height = static_cast<int>(height);
    const auto area = static_cast<std::size_t>(width * height);
    std::vector<std::uint8_t> occupied(area, 0);
    for (const auto& piece : result.pieces) {
        const auto piece_area = static_cast<std::uint64_t>(piece.width * piece.height);
        if (piece_area > area - result.assigned_pixels) throw std::runtime_error("OPT overlapping destinations exceed canvas area");
        for (std::int64_t y = 0; y < piece.height; ++y) {
            const auto start = static_cast<std::size_t>((piece.y - result.y0 + y) * width + piece.x - result.x0);
            for (std::size_t x = 0; x < static_cast<std::size_t>(piece.width); ++x) {
                if (occupied[start + x]) throw std::runtime_error("OPT overlapping destinations: no blending rule inferred");
                occupied[start + x] = 1;
            }
        }
        result.assigned_pixels += piece_area;
    }
    // Occupancy work <= canvas pixels before success or first overlap; no O(piece_count^2).
    return result;
}

int view(const std::string& filename, bool headless, const std::string& capture, const std::string& opt_file) {
    if (!capture.empty()) {
        const auto target = std::filesystem::u8path(capture);
        if (std::filesystem::exists(target)) throw std::runtime_error("Capture destination already exists");
    }
    const auto encoded = read_file(filename, max_encoded_bytes, 8);
    constexpr std::array<stbi_uc, 8> signature{{137, 80, 78, 71, 13, 10, 26, 10}};
    if (!std::equal(signature.begin(), signature.end(), encoded.begin())) throw std::runtime_error("Input is not PNG");
    int width = 0, height = 0, channels = 0;
    const auto byte_count = static_cast<int>(encoded.size());
    if (stbi_is_16_bit_from_memory(encoded.data(), byte_count))
        throw std::runtime_error("16-bit PNG is outside this RGBA8 viewer profile");
    if (!stbi_info_from_memory(encoded.data(), byte_count, &width, &height, &channels) ||
        width <= 0 || height <= 0 || width > 8192 || height > 8192 ||
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) > max_pixels)
        throw std::runtime_error("Invalid PNG dimensions or limit exceeded (8192/axis, 16 Mi pixels)");
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
        stbi_load_from_memory(encoded.data(), byte_count, &width, &height, &channels, 4), stbi_image_free);
    if (!pixels) throw std::runtime_error(std::string("PNG decode failed: ") + stbi_failure_reason());
    const bool opt_mode = !opt_file.empty();
    const auto composition = opt_mode ? compose_geometry(opt_file, width, height) : Composition{};
    const auto target_width = opt_mode ? composition.width : width;
    const auto target_height = opt_mode ? composition.height : height;

    require(SDL_Init(headless ? 0U : SDL_INIT_VIDEO), "SDL_Init");
    QuitSDL quit;
    Surface source(SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels.get(), width * 4), SDL_DestroySurface);
    require(source != nullptr, "SDL_CreateSurfaceFrom");
    Surface target(headless ? SDL_CreateSurface(target_width, target_height, SDL_PIXELFORMAT_RGBA32) : nullptr, SDL_DestroySurface);
    if (headless) require(target != nullptr, "SDL_CreateSurface");
    Window window(headless ? nullptr : SDL_CreateWindow("Magician's Saga - research occupied bounds / PNG", target_width, target_height, SDL_WINDOW_RESIZABLE), SDL_DestroyWindow);
    if (!headless) require(window != nullptr, "SDL_CreateWindow");
    Renderer renderer(headless ? SDL_CreateSoftwareRenderer(target.get()) : SDL_CreateRenderer(window.get(), nullptr), SDL_DestroyRenderer);
    require(renderer != nullptr, "SDL_CreateRenderer");
    Texture texture(SDL_CreateTextureFromSurface(renderer.get(), source.get()), SDL_DestroyTexture);
    require(texture != nullptr, "SDL_CreateTextureFromSurface");
    require(SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST), "SDL_SetTextureScaleMode");
    require(SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE), "SDL_SetTextureBlendMode");
    if (!headless) require(SDL_SetRenderLogicalPresentation(renderer.get(), target_width, target_height, SDL_LOGICAL_PRESENTATION_LETTERBOX), "SDL_SetRenderLogicalPresentation");
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) running = false;
        }
        require(SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 0), "SDL_SetRenderDrawColor");
        require(SDL_RenderClear(renderer.get()), "SDL_RenderClear");
        if (opt_mode) {
            for (const auto& piece : composition.pieces) {
                const SDL_FRect from{static_cast<float>(piece.u), static_cast<float>(piece.v),
                    static_cast<float>(piece.width), static_cast<float>(piece.height)};
                const SDL_FRect to{static_cast<float>(piece.x - composition.x0), static_cast<float>(piece.y - composition.y0),
                    static_cast<float>(piece.width), static_cast<float>(piece.height)};
                require(SDL_RenderTexture(renderer.get(), texture.get(), &from, &to), "SDL_RenderTexture OPT piece");
            }
        } else require(SDL_RenderTexture(renderer.get(), texture.get(), nullptr, nullptr), "SDL_RenderTexture");
        require(SDL_RenderPresent(renderer.get()), "SDL_RenderPresent");
        if (headless) {
            require(SDL_SaveBMP(target.get(), capture.c_str()), "SDL_SaveBMP");
            running = false;
        } else SDL_Delay(16);
    }
    if (opt_mode) {
        std::cout << "{\"canvas_kind\":\"occupied_bounds_not_original_dimensions\",\"occupied_bounds\":["
                  << composition.x0 << ',' << composition.y0 << ',' << composition.x1 << ',' << composition.y1
                  << "],\"canvas_size\":[" << target_width << ',' << target_height
                  << "],\"grid_domain\":[0,0," << composition.grid_width << ',' << composition.grid_height
                  << "],\"piece_count\":" << composition.pieces.size() << ",\"pieces_outside_grid\":" << composition.outside_grid
                  << ",\"assigned_pixels\":" << composition.assigned_pixels
                  << ",\"padding_convention\":\"RGBA zero for research display only\"}\n";
    } else std::cout << "PNG " << width << 'x' << height << (headless ? " captured 1:1 RGBA\n" : " closed\n");
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        std::string filename, capture, opt_file;
        bool headless = false;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--headless" && !headless) headless = true;
            else if (argument == "--capture" && capture.empty() && i + 1 < argc) capture = argv[++i];
            else if (argument == "--opt" && opt_file.empty() && i + 1 < argc) opt_file = argv[++i];
            else if (filename.empty() && argument.rfind("--", 0) != 0) filename = argument;
            else throw std::runtime_error("Invalid or duplicate argument");
        }
        if (filename.empty() || headless != !capture.empty()) {
            std::cerr << "Usage: asset_viewer image.png [--opt file.opt] [--headless --capture output.bmp]\n";
            return 2;
        }
        return view(filename, headless, capture, opt_file);
    } catch (const std::exception& error) {
        std::cerr << "asset_viewer: " << error.what() << '\n';
        return 1;
    }
}
