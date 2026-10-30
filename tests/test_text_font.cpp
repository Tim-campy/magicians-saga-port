#include "core/gameplay/text_font.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    check(text_gui_pixel_size(12,150)==18,"GUI font size uses scaled integer plus native bias");
    check(text_gui_pixel_size(-12,150)==-17,"GUI font conversion truncates after adding positive bias");
    check(text_gui_pixel_size(12,std::numeric_limits<float>::quiet_NaN())==0,"GUI size maps NaN to zero");
    check(text_gui_pixel_size(12,std::numeric_limits<float>::infinity())==std::numeric_limits<std::int32_t>::min(),"GUI size retains explicit positive infinity quirk");
    check(text_run_logical_width(13.5F,150)==9.0F,"logical run width multiplies then divides by captured ratio");
    check(std::isinf(text_run_logical_width(1,0)),"native float division keeps zero-ratio infinity");
    const std::vector<std::int32_t> presets{8,12,16};
    TextFontState font{0,8,true,8,1,150};
    set_text_font_size(font,1,true,&presets);
    check(font.requested_size==1 && font.size==18 && font.adjusted_size==18,
          "preset is scaled while request remains an index");
    set_text_font_size(font,1,false,&presets);
    check(font.size==1 && !font.use_preset, "literal size truncates after scaling");
    update_text_font_scale(font,200,&presets);
    check(font.size==24 && font.use_preset && font.requested_size==1,
          "scale change reinterprets the retained request with presets enabled");
    update_text_font_scale(font,200,nullptr);
    check(font.size==24, "unchanged scale does not access the table");
    bool failed=false;
    try { set_text_font_size(font,-1,true,&presets); }
    catch (const std::out_of_range&) { failed=true; }
    check(failed && font.requested_size==-1 && font.size==-1 &&
          font.use_preset && font.adjusted_size==24,
          "negative preset fails after partial stores without replacing adjusted size");
    failed=false;
    try { set_text_font_size(font,7,false,nullptr); }
    catch (const std::runtime_error&) { failed=true; }
    check(failed && font.size==7 && font.requested_size==7 && !font.use_preset &&
          font.adjusted_size==24, "missing table fails even with presets disabled");
    set_text_font_size(font,3,true,&presets);
    check(font.size==6, "request equal to table length is a literal size");
    set_text_font_size(font,-3,false,&presets);
    check(font.size==-6, "negative literal bypasses preset indexing");
    font.font_scale=100;
    set_text_font_size(font,21474837,false,&presets);
    check(font.size==-21474835, "multiplication wraps before signed division");
    font={1,12,false,99,1,100};
    failed=false;
    try { update_text_font_scale(font,150,nullptr); }
    catch (const std::runtime_error&) { failed=true; }
    check(failed && font.font_scale==150 && font.size==1 && font.use_preset &&
          font.adjusted_size==99, "scale update is retained when setter fails");
    font={10,10,false,99,1,100};
    adjust_text_font_size(font,150);
    check(font.size==10 && font.requested_size==10 && font.adjusted_size==10 &&
          font.scale==1.5F, "adjustment keeps effective size distinct from scale");
    adjust_text_font_size(font,0);
    check(font.size==10 && font.adjusted_size==0 && font.scale==0,
          "zero ratio is not algebraically cancelled");
    adjust_text_font_size(font,-0.0F);
    check(font.adjusted_size==0 && std::signbit(font.scale), "negative zero preserved");
    adjust_text_font_size(font,std::numeric_limits<float>::infinity());
    check(font.adjusted_size==0 && std::isinf(font.scale), "infinite ratio yields NaN quotient");
    adjust_text_font_size(font,std::numeric_limits<float>::quiet_NaN());
    check(font.adjusted_size==0 && std::isnan(font.scale), "NaN converts without undefined cast");
    font.size=std::numeric_limits<std::int32_t>::max();
    adjust_text_font_size(font,100);
    check(font.adjusted_size==std::numeric_limits<std::int32_t>::max(),
          "rounded positive integer boundary saturates");
    adjust_text_font_size(font,std::numeric_limits<float>::max());
    check(font.adjusted_size==std::numeric_limits<std::int32_t>::min(),
          "positive intermediate infinity follows explicit original override");
    font.size=std::numeric_limits<std::int32_t>::min();
    adjust_text_font_size(font,100);
    check(font.adjusted_size==font.size, "negative integer boundary remains representable");
}
