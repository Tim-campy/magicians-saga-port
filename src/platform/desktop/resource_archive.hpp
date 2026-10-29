#pragma once
#include "assets/seb.hpp"
#include "assets/opt_grid.hpp"
#include <memory>
#include <optional>
#include <string>

namespace saga::desktop {
struct ResourceProfile {
    std::string language_folder;
    std::int32_t dpi = 0;
};
enum class ArchiveContents {images_and_animation,data_only};
struct ArchiveData {std::string member;std::vector<std::uint8_t> bytes;};
struct ArchiveCatalog {
    std::string member;
    std::vector<std::optional<std::string>> slots;
};
struct ArchiveSprite {
    assets::Seb animation;
    std::optional<assets::OptGrid> opt;
    std::vector<std::uint8_t> png;
    std::string animation_member, image_member, opt_member, image_catalog, animation_catalog;
    std::int32_t texture = -1, standard_width = 0, standard_height = 0;
    std::int32_t reverse_u=0,reverse_v=0,cache_size=0,image_render_queue=0;
    std::vector<std::string> unrecognized_image_options;
};
struct ArchiveAnimation {
    assets::Seb animation;
    std::string filename,member;
    std::int32_t reverse_u=0,reverse_v=0,cache_size=0;
};
struct ArchiveImage {
    std::optional<assets::OptGrid> opt;
    std::vector<std::uint8_t> png;
    std::string member,opt_member;
    std::int32_t standard_width=0,standard_height=0,render_queue=0;
    std::vector<std::string> unrecognized_options; // Native unknown-option PRINT diagnostic, not an atlas request.
};
// Synchronous preparation from one decoded archive. Explicit ASCII/invariant
// resource profile, matching the shipped names/catalogs; not Android boot or
// the asynchronous ResourceManager. Recognized unbound services fail; unknown
// image tokens are retained for the original conditional PRINT diagnostic.
class ResourceArchive {
public:
    ResourceArchive(std::vector<std::uint8_t>, ResourceProfile,ArchiveContents=ArchiveContents::images_and_animation);
    ~ResourceArchive();
    ResourceArchive(const ResourceArchive&) = delete;
    ResourceArchive& operator=(const ResourceArchive&) = delete;
    const ArchiveCatalog& images() const;
    const ArchiveCatalog& animations() const;
    std::optional<ArchiveData> data(std::string_view name); // Same native candidate/first-match lookup; no image catalog required.
    // Successful selections are cached by catalog ID; references remain stable.
    const ArchiveAnimation& animation(std::int32_t clip);
    const ArchiveImage& image(std::int32_t texture);
    ArchiveSprite sprite(std::int32_t clip, std::int32_t frame, std::int32_t layer);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace saga::desktop
