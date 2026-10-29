#pragma once
#include "sprite_matrix.hpp"
#include <cstdint>

namespace saga::core {
using SharedSpriteMatrix=std::shared_ptr<SpriteMatrix>;
using MatrixPair=std::shared_ptr<std::vector<SharedSpriteMatrix>>;
struct RenderTransformState {
    SharedSpriteMatrix world=std::make_shared<SpriteMatrix>(),screen=std::make_shared<SpriteMatrix>(); // +0x1f8/+0x210.
    std::shared_ptr<std::vector<MatrixPair>> local=std::make_shared<std::vector<MatrixPair>>(); // +0x200.
    std::int32_t local_index=-1; // +0x208, increments before PushMatrix validation.
    std::int32_t saved_count=0,flag_count=0,local_coordinate_count=0; // +0x218/+0x228/+0x240.
    std::shared_ptr<std::vector<SharedSpriteMatrix>> saved=std::make_shared<std::vector<SharedSpriteMatrix>>(); // +0x220.
    std::shared_ptr<std::vector<std::uint8_t>> flags=std::make_shared<std::vector<std::uint8_t>>(); // +0x230.
    bool debug=false;
};
class RenderTransformHost {
public:
    virtual ~RenderTransformHost()=default;
    virtual void matrix_underflow()=0;
    virtual void begin_local_coordinates(float x,float y)=0;
    virtual void end_local_coordinates()=0;
};
// Graphics transform stack; standard containers replace managed storage.
void update_screen_matrix(RenderTransformState&); // 0x1b72110.
void set_graphics_matrix(RenderTransformState&,const SpriteMatrix*); // 0x1b72098, null means reset.
void push_graphics_matrix(RenderTransformState&,const SpriteMatrix*); // 0x1b65e34.
void pop_graphics_matrix(RenderTransformState&,RenderTransformHost&); // 0x1b72254.
// Begin zeroes x/y only after a nonzero translation was applied; End's saved_count
// gate and zero-position flag asymmetry are preserved, not normalized into RAII.
bool begin_render_matrix(RenderTransformState&,float& x,float& y,RenderTransformHost&); // 0x1b5e04c.
void end_render_matrix(RenderTransformState&,RenderTransformHost&); // 0x1b73e50.
} // namespace saga::core
