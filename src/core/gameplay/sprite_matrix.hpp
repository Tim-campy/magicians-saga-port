#pragma once
#include <memory>
#include <vector>

namespace saga::core {
using MatrixValues=std::shared_ptr<std::vector<float>>;
struct SpriteMatrix {
    MatrixValues values=std::make_shared<std::vector<float>>(std::initializer_list<float>{1,0,0,0,1,0,0,0,1});
    MatrixValues concat_work,rotation_work; // Original Matrix +0x18/+0x20, retained after allocation.
};
// Narrow KairoLibrary column-major adapter; MapPoints deliberately ignores the
// perspective row. Each product/addition rounds to binary32, without FMA.
SpriteMatrix& reset_matrix(SpriteMatrix&); // 0x1ba39d0.
SpriteMatrix& set_matrix_values(SpriteMatrix&,MatrixValues); // 0x1ba3afc, copies supplied length.
SpriteMatrix& copy_matrix(SpriteMatrix&,const SpriteMatrix*); // 0x1ba3ac0, copies destination length.
SpriteMatrix& post_concat_matrix(SpriteMatrix&,MatrixValues); // 0x1ba3c3c: other * this.
SpriteMatrix& pre_concat_matrix(SpriteMatrix&,MatrixValues); // 0x1ba3dbc: this * other.
SpriteMatrix& set_concat_matrix(SpriteMatrix&,const SpriteMatrix*,const SpriteMatrix*); // 0x1ba3fb8 / 0x1ba3f40.
SpriteMatrix& post_translate_matrix(SpriteMatrix&,float x,float y); // 0x1ba4180.
SpriteMatrix& pre_translate_matrix(SpriteMatrix&,float x,float y); // 0x1ba4414.
SpriteMatrix& post_scale_matrix(SpriteMatrix&,float x,float y,float center_x,float center_y); // 0x1ba45c8.
SpriteMatrix& pre_scale_matrix(SpriteMatrix&,float x,float y,float center_x,float center_y); // 0x1ba4710.
SpriteMatrix& set_scale_matrix(SpriteMatrix&,float x,float y,float center_x,float center_y); // 0x1ba4858.
SpriteMatrix& set_translate_matrix(SpriteMatrix&,float x,float y); // 0x1ba4924.
SpriteMatrix& post_rotate_matrix(SpriteMatrix&,float angle,float center_x,float center_y); // 0x1ba3fd8.
SpriteMatrix& pre_rotate_matrix(SpriteMatrix&,float angle,float center_x,float center_y); // 0x1ba426c.
SpriteMatrix& set_rotate_matrix(SpriteMatrix&,float angle,float center_x,float center_y); // 0x1ba450c.
void map_matrix_point(const SpriteMatrix&,float& x,float& y); // 0x1ba4a3c, affine XY only.
// Game adapters around public double-precision libm; do not reconstruct libm.
float degree_sine(float);
float degree_cosine(float);
float degree_tangent(float);
} // namespace saga::core
