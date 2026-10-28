#pragma once
#include "event_flow.hpp"
#include <array>

namespace saga::core {
// AppData.InsertZSortObject's nineteen int32 cells, in native argument order.
enum RenderField:std::size_t {
    render_texture,render_resource,render_depth,render_clip,render_frame,render_x,render_y,
    render_width,render_height,render_u,render_v,render_source_width,render_source_height,
    render_layer,render_alpha,render_color,render_angle,render_image_resource,render_skew
};
struct RenderCommand {std::array<std::int32_t,19> values{};std::int32_t type=0;};
using RenderRows=std::shared_ptr<std::vector<EventInstruction>>;
struct RenderQueue {
    EventInstruction counts; // AppData +0x3d0.
    std::shared_ptr<std::vector<RenderRows>> buffers; // +0x3d8, fixed outer length.
};
// Explicit host preparation of two empty buffers, not AppData's full constructor.
RenderQueue make_render_queue();
void clear_render_queue(RenderQueue&); // ClearZSortObjects 0x1115824, counts only.
void insert_render_command(RenderQueue&,const RenderCommand&); // 0x111586c; grow by 128, reuse rows.
// Game-specific equal-depth partition order: not std::sort/stable_sort's contract.
std::int32_t render_pivot(RenderRows,std::int32_t left,std::int32_t right); // 0x1115cc4.
std::int32_t partition_render_rows(RenderRows,std::int32_t left,std::int32_t right,std::int32_t pivot); // 0x1115d50.
void quicksort_render_rows(RenderRows,std::int32_t left,std::int32_t right); // 0x1115c10.
void sort_render_queue(RenderQueue&,std::int32_t type); // Zsort(int) 0x1115bac.
void sort_render_queue(RenderQueue&); // Zsort() 0x1115b8c, type0 before type1.
}
