#include "core/gameplay/select_camera.hpp"
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"camera contract failure\n";std::exit(1);}}
struct Host final:SelectCameraHost {
    std::vector<std::string> calls;
    std::vector<std::int32_t> widths{320,320},heights{240,240},game_heights{240,240};
    std::size_t wi=0,hi=0,ghi=0;
    std::function<void()> width_hook;
    bool fail_height=false;
    std::int32_t game_width() override {calls.push_back("gw");if(width_hook)width_hook();return 320;}
    std::int32_t game_height() override {calls.push_back("gh");if(fail_height)throw std::runtime_error("height");return game_heights.at(ghi++);}
    std::int32_t view_width() override {calls.push_back("vw");return widths.at(wi++);}
    std::int32_t view_height() override {calls.push_back("vh");return heights.at(hi++);}
};
struct Fixture {
    std::int32_t x=-10,y=0;
    SelectCameraState state{false,0,0,x,y,0,1000};
    Host host;
    void move(){move_select_camera(state,host);}
};
int main() {
    check(camera_clamp(5,10,0)==10);
    check(camera_clamp(-20,-10,10)==-10);
    check(camera_clamp(20,-10,10)==10);
    check(scaled_view_dimension(101,200.0F)==50);
    check(scaled_view_dimension(-101,200.0F)==-50);
    check(scaled_view_dimension(1,0.0F)==std::numeric_limits<std::int32_t>::min());
    check(scaled_view_dimension(0,0.0F)==0);
    check(scaled_select_dimension(321,200)==160);
    check(scaled_select_dimension(321,0)==0);
    check(scaled_select_dimension(0x20000000,-1)==std::numeric_limits<std::int32_t>::min());
    {Fixture f;f.state.velocity_x=0.9F;f.state.velocity_y=-0.8F;f.move();check(f.host.calls.empty() && f.state.velocity_x==0.9F && f.state.velocity_y==-0.8F);}
    {Fixture f;f.state.dragging=true;f.state.velocity_x=100.0F;f.move();check(f.host.calls.empty() && f.state.velocity_x==100.0F);}
    {Fixture f;f.state.velocity_x=2.0F;f.state.velocity_y=-0.25F;f.move();check(f.state.velocity_x==1.8F && f.state.velocity_y==0.0F && !std::signbit(f.state.velocity_y));check(f.x==-9 && f.y==0);check((f.host.calls==std::vector<std::string>{"gw","vw","vw","gh","vh","gh","vh"}));}
    {Fixture f;f.state.velocity_x=std::numeric_limits<float>::quiet_NaN();f.move();check(f.host.calls.empty() && std::isnan(f.state.velocity_x));}
    {Fixture f;f.state.velocity_x=std::numeric_limits<float>::infinity();f.move();check(f.x==0 && std::isinf(f.state.velocity_x));} // Wrapped -10 + INT32_MIN is positive, then upper clamp.
    {Fixture f;f.host.widths={320,330};f.host.game_heights={240,260};f.host.heights={240,250};set_select_camera_position(f.state,-999,999,f.host);check(f.x==-670 && f.y==60);}
    {Fixture f;f.host.width_hook=[&]{f.state.right_extent=2000;f.state.left_extent=20;};set_select_camera_position(f.state,-999,0,f.host);check(f.x==-680);} // Right bound captured before the callback.
    {Fixture f;f.host.fail_height=true;f.state.velocity_x=2.0F;try{f.move();check(false);}catch(const std::runtime_error&){}check(f.x==-9 && f.y==0 && f.state.velocity_x==1.8F);}
}
