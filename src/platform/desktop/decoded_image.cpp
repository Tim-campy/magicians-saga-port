#include "decoded_image.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <string>

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

namespace saga::desktop {
DecodedImage decode_png(const std::vector<std::uint8_t>& bytes){
    constexpr std::array<std::uint8_t,8> signature{{137,80,78,71,13,10,26,10}};
    if(bytes.size()<signature.size()||bytes.size()>64U*1024U*1024U)throw std::invalid_argument("PNG encoded size outside desktop limits");
    if(!std::equal(signature.begin(),signature.end(),bytes.begin()))throw std::invalid_argument("Input is not PNG");
    const auto count=static_cast<int>(bytes.size());
    if(stbi_is_16_bit_from_memory(bytes.data(),count))throw std::invalid_argument("16-bit PNG is outside this RGBA8 viewer profile");
    int width=0,height=0,channels=0;
    if(!stbi_info_from_memory(bytes.data(),count,&width,&height,&channels)||width<=0||height<=0||width>8192||height>8192||
       static_cast<std::uint64_t>(width)*static_cast<std::uint64_t>(height)>16U*1024U*1024U)
        throw std::invalid_argument("Invalid PNG dimensions or limit exceeded (8192/axis, 16 Mi pixels)");
    std::unique_ptr<stbi_uc,decltype(&stbi_image_free)> pixels(stbi_load_from_memory(bytes.data(),count,&width,&height,&channels,4),stbi_image_free);
    if(!pixels)throw std::runtime_error(std::string("PNG decode failed: ")+stbi_failure_reason());
    const auto length=static_cast<std::size_t>(width)*static_cast<std::size_t>(height)*4U;
    return {width,height,{pixels.get(),pixels.get()+length}};
}
} // namespace saga::desktop
