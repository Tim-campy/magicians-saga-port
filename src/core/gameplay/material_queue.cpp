#include "material_queue.hpp"
#include "render_colors.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t next(std::int32_t n){return n==std::numeric_limits<std::int32_t>::max()?std::numeric_limits<std::int32_t>::min():n+1;}
std::int32_t previous(std::int32_t n){return n==std::numeric_limits<std::int32_t>::min()?std::numeric_limits<std::int32_t>::max():n-1;}
std::vector<std::int32_t>& required(const std::shared_ptr<std::vector<std::int32_t>>& p){
    if(!p)throw std::invalid_argument("missing graphics queue stack");
    return *p;
}
}
std::int32_t add_queue_offset(std::int32_t a,std::int32_t b){
    const auto word=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(word<0x80000000U?static_cast<std::int64_t>(word):static_cast<std::int64_t>(word)-0x100000000LL);
}
void save_graphics_queue(GraphicsQueueState& s){
    auto& values=required(s.saved);
    if(s.count>=static_cast<std::int64_t>(values.size()))values.push_back(0);
    values.at(static_cast<std::size_t>(s.count))=s.value;s.count=next(s.count);
}
void push_graphics_queue(GraphicsQueueState& s,std::int32_t value){save_graphics_queue(s);s.value=value;}
void pop_graphics_queue(GraphicsQueueState& s){
    const auto index=previous(s.count);if(index<0)return;
    const auto value=required(s.saved).at(static_cast<std::size_t>(index));
    s.value=value;s.count=previous(s.count);
}
void set_material_queue_offset(MaterialQueueState& s,std::int32_t offset,MaterialQueueHost& host){
    s.offset=offset;const auto base=host.shader_queue();s.absolute=add_queue_offset(s.offset,base);
}
std::int32_t sprite_shader_queue(std::uint32_t key){
    (void)sprite_pixel_pipeline(key); // Same reached mesh-texture boundary.
    return (key&0x200U)?3000:((key&0x400U)?2450:2000);
}
} // namespace saga::core
