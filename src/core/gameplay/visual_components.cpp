#include "visual_components.hpp"

namespace saga::core {
namespace {
ImageArray singleton(std::int32_t value) {
    return std::make_shared<std::vector<std::int32_t>>(1, value);
}
ImageArray normalized(ImageArray array) {
    return array && !array->empty() ? array : singleton(-1);
}
}
ComponentSlots& add_image_arrays(ComponentSlots& entity, ImageArrays args, ImagePool& pool) {
    auto& image = pool.acquire_image();
    image.arrays.textures = normalized(args.textures);
    image.arrays.resources = normalized(args.resources);
    image.arrays.u = normalized(args.u);
    image.arrays.v = normalized(args.v);
    image.arrays.width = normalized(args.width);
    image.arrays.height = normalized(args.height);
    return entity.add(7, &image);
}
ComponentSlots& add_image_component(ComponentSlots& entity, std::array<std::int32_t, 6> values, ImagePool& pool) {
    ImageArrays arrays;
    arrays.textures = singleton(values[0]);
    arrays.resources = singleton(values[1]);
    arrays.u = singleton(values[2]);
    arrays.v = singleton(values[3]);
    arrays.width = singleton(values[4]);
    arrays.height = singleton(values[5]);
    return add_image_arrays(entity, arrays, pool);
}
ComponentSlots& add_image_component(ComponentSlots& entity, ImageArray textures, ImageArray resources, ImagePool& pool) {
    return add_image_arrays(entity, {textures, resources, nullptr, nullptr, nullptr, nullptr}, pool);
}
ComponentSlots& add_animation_component(ComponentSlots& entity, std::int32_t rate, std::int32_t backup, PlaybackPool& pool) {
    auto& playback = pool.acquire_playback();
    playback.state.rate = rate;
    playback.state.return_clip = backup;
    return entity.add(10, &playback);
}
} // namespace saga::core
