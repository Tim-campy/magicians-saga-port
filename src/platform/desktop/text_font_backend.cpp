#ifdef _MSC_VER
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif
#include "text_font_backend.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <codecvt>
#include <fstream>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

namespace saga::desktop {
namespace {
void require(FT_Error error,const char* operation){
    if(error)throw std::runtime_error(std::string(operation)+": FreeType error "+std::to_string(error));
}
std::u32string codepoints(std::u16string_view text){
    if(text.empty())return {};
    // Reuse C++17 conversion facets rather than duplicating a Unicode decoder.
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> utf16;
    std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> utf32;
    return utf32.from_bytes(utf16.to_bytes(text.data(),text.data()+text.size()));
}
}
struct DesktopTextFont::Impl {
    FT_Library library=nullptr;
    FT_Face face=nullptr;
    std::vector<std::uint8_t> bytes;
    ~Impl(){if(face)FT_Done_Face(face);if(library)FT_Done_FreeType(library);}
};
DesktopTextFont::DesktopTextFont(const std::filesystem::path& path):impl_(std::make_unique<Impl>()){
    std::ifstream input(path,std::ios::binary|std::ios::ate);if(!input)throw std::invalid_argument("cannot read explicit text font");
    const auto length=input.tellg();
    if(length<=0||length>64*1024*1024)throw std::length_error("text font outside64MiB host limit");
    impl_->bytes.resize(static_cast<std::size_t>(length));input.seekg(0);
    input.read(reinterpret_cast<char*>(impl_->bytes.data()),length);if(!input)throw std::runtime_error("incomplete text font read");
    require(FT_Init_FreeType(&impl_->library),"initialize text font service");
    require(FT_New_Memory_Face(impl_->library,impl_->bytes.data(),static_cast<FT_Long>(impl_->bytes.size()),0,&impl_->face),"load supplied font face");
    require(FT_Select_Charmap(impl_->face,FT_ENCODING_UNICODE),"select Unicode charmap");
}
DesktopTextFont::~DesktopTextFont()=default;
float DesktopTextFont::run_width(std::u16string_view text,std::int32_t pixel_size,std::int32_t load_flags,std::int32_t kerning_mode){
    if(pixel_size<1||pixel_size>4096)throw std::invalid_argument("text font pixel size outside1..4096");
    // UNSCALED returns font units, outside this provider's pixel-advance contract.
    if(kerning_mode< -1||kerning_mode>1)throw std::invalid_argument("text kerning mode requires -1(disabled) or FreeType0..1");
    // Horizontal scalable outline runs only; flags that replace that contract fail explicitly.
    constexpr FT_Int32 incompatible=FT_LOAD_NO_SCALE|FT_LOAD_VERTICAL_LAYOUT|FT_LOAD_RENDER|FT_LOAD_IGNORE_TRANSFORM;
    if((load_flags&incompatible)!=0)throw std::invalid_argument("font flags change the selected horizontal run contract");
    require(FT_Set_Pixel_Sizes(impl_->face,0,static_cast<FT_UInt>(pixel_size)),"select explicit pixel size");
    std::int64_t advance=0;FT_UInt previous=0;
    for(const auto point:codepoints(text)){
        const auto glyph=FT_Get_Char_Index(impl_->face,static_cast<FT_ULong>(point));
        if(!glyph)throw std::invalid_argument("explicit font lacks a requested code point");
        if(previous&&kerning_mode>=0&&FT_HAS_KERNING(impl_->face)){
            FT_Vector delta{};
            require(FT_Get_Kerning(impl_->face,previous,glyph,static_cast<FT_UInt>(kerning_mode),&delta),"measure run kerning");
            advance+=static_cast<std::int64_t>(delta.x);
        }
        require(FT_Load_Glyph(impl_->face,glyph,load_flags),"measure supplied font glyph");
        advance+=static_cast<std::int64_t>(impl_->face->glyph->advance.x);previous=glyph;
    }
    return static_cast<float>(advance)/64.0F;
}
}
