#pragma once
#include "assets/seb.hpp"
#include <array>
#include <memory>

namespace saga::core {
using SpriteValues=std::vector<std::int32_t>;
using SpriteArray=std::shared_ptr<SpriteValues>;
using SpriteRows=std::shared_ptr<std::vector<SpriteArray>>;
using SpriteCache=std::shared_ptr<std::vector<SpriteRows>>;
// Managed array lengths stay fixed; replacement references and mutable elements are distinct.
struct SebInstanceWork {SpriteRows sprites,list;}; // WorkValue +0x10 matrix, +0x18 retained GetSprites list.
struct SebSpriteState {
    std::shared_ptr<std::vector<std::int32_t>> groups; // Seb +0x20, first record indices.
    std::shared_ptr<std::vector<std::int16_t>> frames; // +0x28.
    std::shared_ptr<std::vector<std::uint8_t>> packed; // +0x30, eight bytes per record.
    std::shared_ptr<std::vector<std::int16_t>> extended; // +0x38, seven shorts per entry.
    std::uint8_t reverse=0; // +0x40: horizontal bit1, vertical bit0.
    SpriteCache cache; // +0x48: age columns -> layers -> twelve-cell sprites.
    std::shared_ptr<std::vector<std::uint8_t>> bounds; // +0x58, four signed bytes per frame; X=-128 marks extended.
    std::shared_ptr<std::vector<std::int16_t>> extended_bounds; // +0x60, four signed shorts; byte-sized entry indices.
    std::shared_ptr<std::vector<float>> draw_offset=std::make_shared<std::vector<float>>(2,0.0F); // +0x98, SetOffset/ClearOffset.
    std::shared_ptr<SebInstanceWork> work=std::make_shared<SebInstanceWork>(); // +0xa8.
    bool flip=false; // +0xb0, final ReverseU toggle; does not invalidate cache.
};
// Explicit per-thread caller-owned work; no runtime ThreadStatic reconstruction.
struct SebStaticWork {
    SpriteCache local; // WorkStaticValue +0x10: layer-count-indexed matrices.
    SpriteCache lists; // +0x18: layer-count-indexed result lists.
    SpriteArray left,right; // +0x30/+0x38, retained conversion scratch.
    SebStaticWork();
};
// Desktop preparation of _load's record normalization/packing slice 0x1baf850..0x1bb02c4.
// Input is the existing disk reader's output; prepare_seb_bounds supplies the separate load-time bounds.
SebSpriteState prepare_seb_sprites(const assets::Seb&);
// Sprite-evaluation projection of Seb(Seb) 0x1bab9fc: shared record arrays,
// fresh work, zero reversal/flip and no cache. Clock construction is separate.
SebSpriteState clone_seb_sprites(const SebSpriteState&);
void convert_seb_sprite(const SebSpriteState&,std::int32_t record,SpriteArray output,std::int32_t reverse); // 0x1bb2b5c.
SpriteArray seb_sprite(SebSpriteState&,SebStaticWork&,std::int32_t frame,std::int32_t layer); // 0x1bb1900.
SpriteRows seb_sprites(SebSpriteState&,SebStaticWork&,std::int32_t frame); // 0x1bb179c; retained instance list.
SpriteArray seb_sprite_local(SebSpriteState&,SebStaticWork&,std::int32_t frame,std::int32_t layer); // 0x1bb2290.
SpriteRows seb_sprites_local(SebSpriteState&,SebStaticWork&,std::int32_t frame); // 0x1bb0b90.
void set_seb_cache_size(SebSpriteState&,std::int32_t size); // 0x1babbfc.
void reverse_seb(SebSpriteState&,std::int32_t horizontal,std::int32_t vertical); // 0x1babb5c.
// _load 0x1bb02c4..0x1bb0854: snapshot each frame through GetSpritesLocal before resource options.
// Preserves compact/extended packing and local work/cache effects; aggregate bounds at +0x50 and
// optional trailing extension blocks remain outside this selected loader projection.
void prepare_seb_bounds(SebSpriteState&,SebStaticWork&,std::int32_t max_frame);
// GetBRect 0x1bb0d04 + GetBoundingRect 0x1ba913c; returns the four narrowed values by value,
// without reproducing the native thread-static short-array result alias.
std::array<std::int32_t,4> seb_bounding_rect(const SebSpriteState&,std::int32_t frame);
// ResourceManager.GetSebAnchorPosition 0x1ba8e38, value-returning adapter of the
// shared float scratch. Bounding coordinates/dimensions are signed int16.
std::array<float,2> seb_anchor_position(const SebSpriteState&,std::int32_t frame,float x,float y,std::int32_t anchor);
void set_seb_draw_offset(SebSpriteState&,float x,float y); // Seb.SetOffset 0x1ba8f44.
void clear_seb_draw_offset(SebSpriteState&); // Seb.ClearOffset 0x1bb8020.
} // namespace saga::core
