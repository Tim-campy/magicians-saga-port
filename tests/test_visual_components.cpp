#include "core/gameplay/visual_components.hpp"
#include <cstdlib>
#include <functional>
#include <limits>

using namespace saga::core;
void check(bool value) { if (!value) std::abort(); }
ImageArray array(std::initializer_list<std::int32_t> values) {
    return std::make_shared<std::vector<std::int32_t>>(values);
}
template<class F> void fails(F f) {
    bool caught = false;
    try { f(); } catch (const std::exception&) { caught = true; }
    check(caught);
}
struct Pool final : ImagePool, PlaybackPool {
    EntityImage image;
    EntityPlayback playback;
    int images = 0, playbacks = 0;
    bool fail = false;
    std::function<void()> on_image;
    EntityImage& acquire_image() override {
        ++images;
        if (fail) throw std::runtime_error("image pool");
        if (on_image) on_image();
        return image;
    }
    EntityPlayback& acquire_playback() override {
        ++playbacks;
        if (fail) throw std::runtime_error("playback pool");
        return playback;
    }
};
struct Observer final : ComponentObserver {
    int calls = 0;
    bool fail = false;
    void added(ComponentSlots& e, std::int32_t slot, Component* c) override {
        check(e.get(slot) == c);
        if (slot == 7) {
            auto& a = dynamic_cast<EntityImage&>(*c).arrays;
            check(a.textures && a.resources && a.u && a.v && a.width && a.height);
        } else check(slot == 10);
        ++calls;
        if (fail) throw std::runtime_error("notification");
    }
};
int main() {
    { Pool p; Observer o; ComponentSlots e(35, &o);
      auto textures = array({3, 4, 5}), resources = array({8}), empty = array({});
      check(&add_image_arrays(e, {textures, resources, empty, nullptr, nullptr, nullptr}, p) == &e);
      auto& a = p.image.arrays;
      check(p.images == 1 && o.calls == 1 && e.get(7) == &p.image);
      check(a.textures == textures && a.resources == resources && empty->empty() && a.u != empty);
      check(*a.u == std::vector<std::int32_t>{-1} && *a.v == *a.u && *a.width == *a.u && *a.height == *a.u);
      check(a.u != a.v && a.u != a.width && a.u != a.height && a.v != a.width && a.v != a.height && a.width != a.height);
      (*textures)[1] = -20; check(a.textures->at(1) == -20); // References, no clone or length matching.
      auto old = a.textures;
      add_image_component(e, std::array<std::int32_t, 6>{9, 7, 1, 2, 3, 4}, p);
      check(p.images == 2 && o.calls == 1 && a.textures != old && old == textures);
      check(*a.textures == std::vector<std::int32_t>{9} && *a.resources == std::vector<std::int32_t>{7});
      check(*a.u == std::vector<std::int32_t>{1} && *a.v == std::vector<std::int32_t>{2});
      check(*a.width == std::vector<std::int32_t>{3} && *a.height == std::vector<std::int32_t>{4});
      check(a.textures != a.resources && a.u != a.v && a.width != a.height);
      Pool other; add_image_component(e, textures, resources, other);
      check(e.get(7) == &p.image && other.image.arrays.textures == textures && o.calls == 1);
      p.fail = true; auto retained = a.textures;
      fails([&] { add_image_arrays(e, {}, p); }); check(a.textures == retained);
    }
    { Pool p; ComponentSlots e(35); auto shared = array({-99});
      ImageArrays args{shared, shared, shared, shared, shared, shared};
      p.on_image = [&] { args = {}; (*shared)[0] = 42; };
      add_image_arrays(e, args, p);
      auto& a = p.image.arrays;
      check(a.textures == shared && a.resources == shared && a.u == shared && a.v == shared && a.width == shared && a.height == shared);
      check(a.textures->at(0) == 42); // Captured references survive argument reassignment during acquisition.
    }
    { Pool p; Observer o; ComponentSlots e(35, &o); o.fail = true;
      fails([&] { add_image_component(e, array({}), nullptr, p); });
      check(e.get(7) == &p.image && *p.image.arrays.textures == std::vector<std::int32_t>{-1});
      check(*p.image.arrays.resources == std::vector<std::int32_t>{-1} && o.calls == 1);
    }
    { Pool p; Observer o; ComponentSlots e(35, &o);
      check(&add_animation_component(e, -3, std::numeric_limits<std::int32_t>::max(), p) == &e);
      check(e.get(10) == &p.playback && p.playback.state.rate == -3 && p.playback.state.return_clip == std::numeric_limits<std::int32_t>::max());
      add_animation_component(e, 1, -1, p);
      check(p.playbacks == 2 && o.calls == 1 && p.playback.state.rate == 1 && p.playback.state.return_clip == -1);
      Pool other; add_animation_component(e, 0, -20, other);
      check(e.get(10) == &p.playback && other.playback.state.return_clip == -20 && o.calls == 1);
      p.fail = true; fails([&] { add_animation_component(e, 99, 77, p); }); check(p.playback.state.rate == 1);
      p.fail = false; e.remove(10); o.fail = true;
      fails([&] { add_animation_component(e, 7, 8, p); });
      check(e.get(10) == &p.playback && p.playback.state.rate == 7 && p.playback.state.return_clip == 8);
    }
}
