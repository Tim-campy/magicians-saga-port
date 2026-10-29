#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace saga::core {
struct GraphicsQueueState {
    std::int32_t value=0,count=0;
    std::shared_ptr<std::vector<std::int32_t>> saved=std::make_shared<std::vector<std::int32_t>>();
};
void save_graphics_queue(GraphicsQueueState&); // Graphics.PushRenderQueue(), 0x1b74290.
void push_graphics_queue(GraphicsQueueState&,std::int32_t); // 0x1b62ccc.
void pop_graphics_queue(GraphicsQueueState&); // 0x1b632b8.
std::int32_t add_queue_offset(std::int32_t,std::int32_t);

struct MaterialQueueState {std::int32_t offset=0,absolute=0;};
class MaterialQueueHost {
public:
    virtual ~MaterialQueueHost()=default;
    virtual std::int32_t shader_queue()=0; // Public Shader.renderQueue contract.
};
// Material.set_RenderQueueOffset, 0x1c9ff14: offset stored before shader query.
void set_material_queue_offset(MaterialQueueState&,std::int32_t,MaterialQueueHost&);
// ShaderBuilder.GetCode's reached mesh-texture Queue tag, interpreted through
// public Unity Queue values. Reject unsupported special shader states.
std::int32_t sprite_shader_queue(std::uint32_t key);
} // namespace saga::core
