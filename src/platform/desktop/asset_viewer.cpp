// Technical PNG/OPT and explicitly bound SEB previews; no playable mission yet.
#include "assets/opt_grid.hpp"
#include "assets/jar_archive.hpp"
#include "resource_archive.hpp"
#include "sprite_preview.hpp"
#include "sdl_bitmap.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

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
void json_string(std::string_view value) {
    constexpr char hex[]="0123456789abcdef";
    std::cout << '"';
    for (const char raw:value) {
        const auto c=static_cast<unsigned char>(raw);
        if (c=='"'||c=='\\') std::cout << '\\' << raw;
        else if (c<32) std::cout << "\\u00" << hex[c>>4U] << hex[c&15U];
        else std::cout << raw;
    }
    std::cout << '"';
}
struct ArchivePreview {saga::desktop::ResourceProfile profile;std::int32_t clip;};
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

int view(const std::string& filename, bool headless, const std::string& capture, const std::string& opt_file,
         std::optional<saga::desktop::SpritePreviewOptions> sprite_options,
         const std::optional<ArchivePreview>& archive_options) {
    if (!capture.empty()) {
        const auto target = std::filesystem::u8path(capture);
        if (std::filesystem::exists(target)) throw std::runtime_error("Capture destination already exists");
    }
    std::optional<saga::desktop::ArchiveSprite> archive_sprite;
    std::vector<stbi_uc> encoded;
    if(archive_options){
        saga::desktop::ResourceArchive archive(read_file(filename,saga::assets::JarLimits{}.max_bytes,8),archive_options->profile);
        archive_sprite=archive.sprite(archive_options->clip,sprite_options->frame,sprite_options->layer);
        encoded=std::move(archive_sprite->png);
        if(encoded.size()<8||encoded.size()>max_encoded_bytes)throw std::invalid_argument("archive PNG size outside preview limits");
        sprite_options->texture=archive_sprite->texture;
        sprite_options->standard_width=archive_sprite->standard_width;sprite_options->standard_height=archive_sprite->standard_height;
        sprite_options->resource_reverse_u=archive_sprite->reverse_u;sprite_options->resource_reverse_v=archive_sprite->reverse_v;
        sprite_options->resource_cache_size=archive_sprite->cache_size;sprite_options->image_render_queue=archive_sprite->image_render_queue;
    }else encoded=read_file(filename, max_encoded_bytes, 8);
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
    const bool sprite_mode=sprite_options.has_value();
    const bool opt_mode = !opt_file.empty()&&!sprite_mode;
    const auto composition = opt_mode ? compose_geometry(opt_file, width, height) : Composition{};
    const auto target_width = sprite_mode?sprite_options->width:(opt_mode ? composition.width : width);
    const auto target_height = sprite_mode?sprite_options->height:(opt_mode ? composition.height : height);
    saga::desktop::SpritePreview preview;
    std::int32_t image_alpha=0;
    if(sprite_mode){
        const auto bytes=archive_sprite?std::vector<stbi_uc>{}:read_file(sprite_options->seb,saga::assets::SebLimits{}.max_bytes,1);
        const auto file=archive_sprite?archive_sprite->animation:saga::assets::parse_seb(bytes.data(),bytes.size());
        std::optional<saga::assets::OptGrid> opt;
        if(archive_sprite)opt=archive_sprite->opt;
        else if(!opt_file.empty()){
            const auto opt_bytes=read_file(opt_file,saga::assets::OptLimits{}.max_bytes,1);
            opt=saga::assets::parse_opt(opt_bytes.data(),opt_bytes.size());
        }
        saga::core::ImageAlphaClassifier classifier;
        const auto count=static_cast<std::size_t>(width)*static_cast<std::size_t>(height);
        for(std::size_t i=0;i<count;++i)classifier.add(static_cast<float>(pixels.get()[i*4+3])/255.0F);
        image_alpha=classifier.finish(static_cast<std::int32_t>(count));
        preview=saga::desktop::prepare_sprite_preview(*sprite_options,file,opt?&*opt:nullptr,width,height,image_alpha);
    }

    // SDL's software QueueGeometry truncates source UV texels before interpolation
    // in 3.4.18. Selected native inset geometry requires a floating-UV renderer;
    // headless sprite capture therefore uses an invisible GPU-backed window.
    const bool software_capture=headless&&!sprite_mode;
    require(SDL_Init(software_capture ? 0U : SDL_INIT_VIDEO), "SDL_Init");
    QuitSDL quit;
    Surface source(SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels.get(), width * 4), SDL_DestroySurface);
    require(source != nullptr, "SDL_CreateSurfaceFrom");
    Surface target(software_capture ? SDL_CreateSurface(target_width, target_height, SDL_PIXELFORMAT_RGBA32) : nullptr, SDL_DestroySurface);
    if (software_capture) require(target != nullptr, "SDL_CreateSurface");
    Window window(software_capture ? nullptr : SDL_CreateWindow("Magician's Saga - technical asset preview", target_width, target_height, headless?SDL_WINDOW_HIDDEN:SDL_WINDOW_RESIZABLE), SDL_DestroyWindow);
    if (!software_capture) require(window != nullptr, "SDL_CreateWindow");
    Renderer renderer(software_capture ? SDL_CreateSoftwareRenderer(target.get()) : SDL_CreateRenderer(window.get(), nullptr), SDL_DestroyRenderer);
    require(renderer != nullptr, "SDL_CreateRenderer");
    if(sprite_mode&&std::string(SDL_GetRendererName(renderer.get()))=="software")
        throw std::runtime_error("selected sprite UVs require a floating-UV graphics renderer; software fallback refused");
    Texture texture(SDL_CreateTextureFromSurface(renderer.get(), source.get()), SDL_DestroyTexture);
    require(texture != nullptr, "SDL_CreateTextureFromSurface");
    require(SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST), "SDL_SetTextureScaleMode");
    require(SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE), "SDL_SetTextureBlendMode");
    Texture canvas(sprite_mode?SDL_CreateTexture(renderer.get(),SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_TARGET,target_width,target_height):nullptr,SDL_DestroyTexture);
    if(sprite_mode){require(canvas!=nullptr,"SDL_CreateTexture RGBA canvas");require(SDL_SetTextureBlendMode(canvas.get(),SDL_BLENDMODE_NONE),"SDL_SetTextureBlendMode canvas");}
    struct MaskTexture {saga::core::SpriteAlphaTest test;float alpha;Texture texture;};
    std::vector<MaskTexture> masks;
    std::vector<SDL_Texture*> quad_textures;
    for(const auto& quad:preview.quads){
        auto* selected=texture.get();const auto test=quad.pipeline.alpha_test;
        if(test!=saga::core::SpriteAlphaTest::none){
            if(quad.pipeline.blend==saga::core::SpriteBlend::replace)
                throw std::runtime_error("alpha-discard with replace blending requires a shader backend");
            auto found=std::find_if(masks.begin(),masks.end(),[&](const auto& m){return m.test==test&&m.alpha==quad.color.a;});
            if(found==masks.end()){
                const auto bytes=static_cast<std::size_t>(width)*static_cast<std::size_t>(height)*4;
                std::vector<stbi_uc> masked(pixels.get(),pixels.get()+bytes);
                for(std::size_t i=3;i<bytes;i+=4){
                    const float alpha=(static_cast<float>(masked[i])/255.0F)*quad.color.a;
                    if(!saga::core::accepts_sprite_alpha(test,alpha))masked[i]=0;
                }
                Surface mask_surface(SDL_CreateSurfaceFrom(width,height,SDL_PIXELFORMAT_RGBA32,masked.data(),width*4),SDL_DestroySurface);
                require(mask_surface!=nullptr,"SDL_CreateSurfaceFrom alpha mask");
                Texture variant(SDL_CreateTextureFromSurface(renderer.get(),mask_surface.get()),SDL_DestroyTexture);
                require(variant!=nullptr,"SDL_CreateTextureFromSurface alpha mask");
                require(SDL_SetTextureScaleMode(variant.get(),SDL_SCALEMODE_NEAREST),"SDL_SetTextureScaleMode alpha mask");
                masks.push_back({test,quad.color.a,std::move(variant)});found=std::prev(masks.end());
            }
            selected=found->texture.get();
        }
        quad_textures.push_back(selected);
    }
    if (!headless) require(SDL_SetRenderLogicalPresentation(renderer.get(), target_width, target_height, SDL_LOGICAL_PRESENTATION_LETTERBOX), "SDL_SetRenderLogicalPresentation");
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) running = false;
        }
        if(sprite_mode)require(SDL_SetRenderTarget(renderer.get(),canvas.get()),"SDL_SetRenderTarget canvas");
        const auto background=sprite_mode?sprite_options->background:std::array<std::uint8_t,4>{0,0,0,0};
        require(SDL_SetRenderDrawColor(renderer.get(), background[0],background[1],background[2],background[3]), "SDL_SetRenderDrawColor");
        require(SDL_RenderClear(renderer.get()), "SDL_RenderClear");
        if(sprite_mode){
            for(std::size_t i=0;i<preview.quads.size();++i)
                require(saga::desktop::render_preview_quad(renderer.get(),quad_textures[i],preview.quads[i],static_cast<float>(target_height)),"SDL_RenderGeometry sprite");
        } else if (opt_mode) {
            for (const auto& piece : composition.pieces) {
                const SDL_FRect from{static_cast<float>(piece.u), static_cast<float>(piece.v),
                    static_cast<float>(piece.width), static_cast<float>(piece.height)};
                const SDL_FRect to{static_cast<float>(piece.x - composition.x0), static_cast<float>(piece.y - composition.y0),
                    static_cast<float>(piece.width), static_cast<float>(piece.height)};
                require(SDL_RenderTexture(renderer.get(), texture.get(), &from, &to), "SDL_RenderTexture OPT piece");
            }
        } else require(SDL_RenderTexture(renderer.get(), texture.get(), nullptr, nullptr), "SDL_RenderTexture");
        Surface readback(headless&&sprite_mode?SDL_RenderReadPixels(renderer.get(),nullptr):nullptr,SDL_DestroySurface);
        if(headless&&sprite_mode)require(readback!=nullptr,"SDL_RenderReadPixels");
        if(sprite_mode){
            require(SDL_SetRenderTarget(renderer.get(),nullptr),"SDL_SetRenderTarget window");
            require(SDL_SetRenderDrawColor(renderer.get(),0,0,0,0),"SDL_SetRenderDrawColor window");require(SDL_RenderClear(renderer.get()),"SDL_RenderClear window");
            require(SDL_RenderTexture(renderer.get(),canvas.get(),nullptr,nullptr),"SDL_RenderTexture canvas");
        }
        require(SDL_RenderPresent(renderer.get()), "SDL_RenderPresent");
        if (headless) {
            // Readback format is backend-selected; explicitly retain four channels.
            Surface rgba(sprite_mode?SDL_ConvertSurface(readback.get(),SDL_PIXELFORMAT_RGBA32):nullptr,SDL_DestroySurface);
            if(sprite_mode)require(rgba!=nullptr,"SDL_ConvertSurface RGBA");
            require(SDL_SaveBMP(sprite_mode?rgba.get():target.get(), capture.c_str()), "SDL_SaveBMP");
            running = false;
        } else SDL_Delay(16);
    }
    if(sprite_mode){
        std::cout<<"{\"canvas_kind\":\"explicit_selected_sprite\",\"canvas_size\":["<<target_width<<','<<target_height
                 <<"],\"frame\":"<<sprite_options->frame<<",\"layer\":"<<sprite_options->layer<<",\"texture_slot\":"<<sprite_options->texture
                 <<",\"quad_count\":"<<preview.quads.size()<<",\"sprite\":";
        if(preview.sprite){std::cout<<'[';for(std::size_t i=0;i<12;++i){if(i)std::cout<<',';std::cout<<preview.sprite->at(i);}std::cout<<']';}
        else std::cout<<"null";
        std::cout<<",\"angle\":"<<sprite_options->angle<<",\"skew\":"<<(sprite_options->skew?"true":"false")
                 <<",\"image_alpha_type\":"<<image_alpha<<",\"raw_rgba\":"<<(sprite_options->raw_rgba?"true":"false")
                 <<",\"encoded_alpha\":"<<sprite_options->alpha<<",\"color\":"<<sprite_options->color<<",\"shader_keys\":[";
        for(std::size_t i=0;i<preview.quads.size();++i){if(i)std::cout<<',';std::cout<<preview.quads[i].shader_key;}
        std::cout<<']';
        std::cout<<",\"render_queues\":[";
        for(std::size_t i=0;i<preview.quads.size();++i){if(i)std::cout<<',';std::cout<<preview.quads[i].render_queue;}
        std::cout<<']';
        if(archive_sprite){
            std::cout<<",\"resource_clip\":"<<archive_options->clip<<",\"animation_member\":";json_string(archive_sprite->animation_member);
            std::cout<<",\"image_member\":";json_string(archive_sprite->image_member);
            std::cout<<",\"opt_member\":";json_string(archive_sprite->opt_member);
            std::cout<<",\"image_catalog\":";json_string(archive_sprite->image_catalog);
            std::cout<<",\"animation_catalog\":";json_string(archive_sprite->animation_catalog);
            std::cout<<",\"resource_dpi\":"<<archive_options->profile.dpi<<",\"resource_language\":";json_string(archive_options->profile.language_folder);
            std::cout<<",\"resource_text_profile\":\"ASCII/invariant, Config selector 2, no language-pack overrides\"";
            std::cout<<",\"resource_reverse\":["<<archive_sprite->reverse_u<<','<<archive_sprite->reverse_v<<"],\"image_queue_offset\":"<<archive_sprite->image_render_queue;
        }
        std::cout<<",\"profile\":\"identity world, native local transforms, no atlas, zero expansion, explicit canvas and mesh color state, no UI overlays\"}\n";
    } else if (opt_mode) {
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
        std::optional<saga::desktop::SpritePreviewOptions> sprite;
        std::optional<std::array<std::int32_t,2>> canvas,origin,standard;
        std::optional<std::int32_t> angle,alpha,color;bool skew=false,raw_rgba=false;
        std::optional<std::int32_t> resource_clip,dpi;
        std::optional<std::string> language;
        std::optional<std::array<std::uint8_t,4>> background;
        const auto number=[](const char* text){
            const std::string value(text);std::int32_t result=0;
            const auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
            if(parsed.ec!=std::errc{}||parsed.ptr!=value.data()+value.size())throw std::invalid_argument("invalid int32 preview argument");
            return result;
        };
        bool headless = false;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--headless" && !headless) headless = true;
            else if (argument == "--capture" && capture.empty() && i + 1 < argc) capture = argv[++i];
            else if (argument == "--opt" && opt_file.empty() && i + 1 < argc) opt_file = argv[++i];
            else if(argument=="--sprite"&&!sprite&&i+4<argc){
                sprite.emplace();sprite->seb=argv[++i];sprite->frame=number(argv[++i]);sprite->layer=number(argv[++i]);sprite->texture=number(argv[++i]);
            }
            else if(argument=="--resource"&&!sprite&&i+3<argc){
                sprite.emplace();resource_clip=number(argv[++i]);sprite->frame=number(argv[++i]);sprite->layer=number(argv[++i]);
            }
            else if(argument=="--language"&&!language&&i+1<argc)language=argv[++i];
            else if(argument=="--dpi"&&!dpi&&i+1<argc)dpi=number(argv[++i]);
            else if(argument=="--canvas"&&!canvas&&i+2<argc){const auto w=number(argv[++i]),h=number(argv[++i]);canvas=std::array<std::int32_t,2>{w,h};}
            else if(argument=="--origin"&&!origin&&i+2<argc){const auto x=number(argv[++i]),y=number(argv[++i]);origin=std::array<std::int32_t,2>{x,y};}
            else if(argument=="--standard"&&!standard&&i+2<argc){const auto w=number(argv[++i]),h=number(argv[++i]);standard=std::array<std::int32_t,2>{w,h};}
            else if(argument=="--angle"&&!angle&&i+1<argc)angle=number(argv[++i]);
            else if(argument=="--skew"&&!skew)skew=true;
            else if(argument=="--alpha"&&!alpha&&i+1<argc)alpha=number(argv[++i]);
            else if(argument=="--color"&&!color&&i+1<argc)color=number(argv[++i]);
            else if(argument=="--raw-rgba"&&!raw_rgba)raw_rgba=true;
            else if(argument=="--background"&&!background&&i+4<argc){
                std::array<std::uint8_t,4> rgba;
                for(auto& channel:rgba){const auto value=number(argv[++i]);if(value<0||value>255)throw std::invalid_argument("background channels must be 0..255");channel=static_cast<std::uint8_t>(value);}
                background=rgba;
            }
            else if (filename.empty() && argument.rfind("--", 0) != 0) filename = argument;
            else throw std::runtime_error("Invalid or duplicate argument");
        }
        if (filename.empty() || headless != !capture.empty()) {
            std::cerr << "Usage: asset_viewer image.png [--opt file.opt] [--sprite file.seb frame layer texture-slot --canvas width height [--origin x y] [--standard width height] [--angle degrees [--skew]] [--alpha encoded] [--color packed-int32] [--background r g b a] [--raw-rgba]] [--headless --capture output.bmp]\n"
                         "Archive: asset_viewer decoded.bin --resource clip frame layer --language folder --dpi index --canvas width height [preview options] [--headless --capture output.bmp]\n";
            return 2;
        }
        std::optional<ArchivePreview> archive;
        if(resource_clip){
            if(!language||!dpi||!opt_file.empty()||standard)throw std::invalid_argument("--resource requires --language and --dpi; image/OPT/standard dimensions come from catalogs");
            archive=ArchivePreview{{*language,*dpi},*resource_clip};
        }else if(language||dpi)throw std::invalid_argument("--language/--dpi require --resource");
        if(sprite){
            if(!canvas||(*canvas)[0]<=0||(*canvas)[1]<=0||(*canvas)[0]>8192||(*canvas)[1]>8192||
               static_cast<std::uint64_t>((*canvas)[0])*static_cast<std::uint64_t>((*canvas)[1])>max_pixels)
                throw std::invalid_argument("sprite requires explicit canvas within 8192/axis, 16 Mi pixels");
            sprite->width=(*canvas)[0];sprite->height=(*canvas)[1];
            if(origin){sprite->x=(*origin)[0];sprite->y=(*origin)[1];}
            if(standard){sprite->standard_width=(*standard)[0];sprite->standard_height=(*standard)[1];}
            if(skew&&!angle)throw std::invalid_argument("--skew requires --angle");
            sprite->angle=angle.value_or(0);sprite->skew=skew;
            if(raw_rgba&&(alpha||color))throw std::invalid_argument("alpha/color arguments require native color passes");
            sprite->alpha=alpha.value_or(255);sprite->color=color.value_or(-1);sprite->raw_rgba=raw_rgba;if(background)sprite->background=*background;
        }else if(canvas||origin||standard||angle||skew||alpha||color||raw_rgba||background)throw std::invalid_argument("preview arguments require --sprite");
        return view(filename, headless, capture, opt_file,sprite,archive);
    } catch (const std::exception& error) {
        std::cerr << "asset_viewer: " << error.what() << '\n';
        return 1;
    }
}
