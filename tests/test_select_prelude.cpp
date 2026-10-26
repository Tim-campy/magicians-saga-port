#include "core/gameplay/select_prelude.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"prelude contract failure\n";std::exit(1);}}
template<class F> void fails(F f) {try{f();check(false);}catch(const std::exception&) {}}
struct Host:SelectPreludeHost {
    std::vector<int> calls;
    int orientation=-1;
    bool landscape=true,fail_save=false;
    std::shared_ptr<ImageAnimationValues> images=std::make_shared<ImageAnimationValues>();
    std::int32_t saved_orientation() override {calls.push_back(1);if(fail_save)throw std::runtime_error("save");return orientation;}
    bool enable_landscape() override {calls.push_back(2);return landscape;}
    void set_orientation(std::int32_t n) override {calls.push_back(10+n);}
    std::int32_t game_width() override {calls.push_back(3);return 240;}
    std::int32_t view_x() override {calls.push_back(4);return 123;}
    std::shared_ptr<ImageAnimationValues> image_animation_values() override {calls.push_back(5);return images;}
};
int main() {
    Host h;SelectPreludeState s{99,99,0,17};
    auto row=std::make_shared<EventIntegers>(EventIntegers{11,99,33});
    auto group=std::make_shared<ImageAnimationGroup>(ImageAnimationGroup{row,row});
    h.images->push_back(group);
    const auto result=update_select_prelude(s,0,h);
    check(result.camera_x==17 && result.view_x==123 && s.error_function==1 && s.error_line==0 && s.update_counter==1);
    check(*row==EventIntegers({11,2,33})); // Shared row increments twice, no deduplication.
    check(h.calls==std::vector<int>({1,2,9,3,4,5,5}));
    h.calls.clear();h.orientation=0;update_select_prelude(s,1,h);
    check(row->at(1)==4 && h.calls==std::vector<int>({1,3,4,5}));
    h.orientation=-1;h.landscape=false;h.calls.clear();update_my_form_base(s.update_counter,h);
    check(h.calls==std::vector<int>({1,2,10,3}));
    row->at(1)=std::numeric_limits<std::int32_t>::max();group->resize(1);
    advance_image_animations(*h.images);check(row->at(1)==std::numeric_limits<std::int32_t>::min());
    group->push_back(nullptr);fails([&]{reset_image_animations(*h.images);});check(row->at(1)==0);
    group->at(1)=std::make_shared<EventIntegers>(1,8);fails([&]{advance_image_animations(*h.images);});check(row->at(1)==1);
    h.fail_save=true;s.update_counter=std::numeric_limits<std::int32_t>::max();s.error_line=88;
    fails([&]{update_select_prelude(s,0,h);});check(s.update_counter==-1 && s.error_line==0);
    h.fail_save=false;h.images.reset();fails([&]{update_select_prelude(s,1,h);});
}
