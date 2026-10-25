#include "core/animation/animation_transitions.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>

using namespace saga::core;

void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
EntityAnimation fixture() {
    return {SpriteAnimation{7, 8, 9}, AnimationPlayback{2, -1}, 3, false, false, false};
}
template<class F> void missing(F operation) {
    try { operation(); }
    catch (const MissingAnimationComponent&) { return; }
    check(false, "missing component must fail");
}

int main() {
    auto e = fixture();
    change_animation(e, 12, false);
    check(e.sprite->clip == 12 && e.sprite->frame == 9, "plain change preserves frame");
    e.human = true;
    change_animation(e, 12, true);
    check(e.sprite->clip == 15 && e.sprite->frame == 0, "human direction and reset");
    check(e.sprite->resource == 7 && e.playback->rate == 2 && e.playback->return_clip == -1,
          "change preserves unrelated state");

    // Predicate difference must survive refactoring: monster != enemy.
    e = fixture(); e.monster = true;
    cut_in_animation(e, 20, 11);
    check(e.sprite->clip == 20, "monster alone has no cut-in direction");
    restore_animation(e);
    check(e.sprite->clip == 11 && e.playback->return_clip == -1, "monster restoration aligns group then adds3");
    e = fixture(); e.enemy = true;
    cut_in_animation(e, 20, 10);
    check(e.sprite->clip == 23, "enemy cut-in adds direction");
    restore_animation(e);
    check(e.sprite->clip == 10 && e.sprite->frame == 0, "enemy alone restores literal ID");

    // Second cut-in replaces the pending return value; it is not a stack.
    e = fixture();
    cut_in_animation(e, 20, 8);
    cut_in_animation(e, 30, 12);
    restore_animation(e);
    check(e.sprite->clip == 12, "latest explicit return wins");
    e.sprite->frame = 17;
    restore_animation(e);
    check(e.sprite->frame == 17, "-1 restore is a no-op");
    e.sprite.reset();
    restore_animation(e); // Sentinel return must precede any sprite access.
    e.playback.reset(); e.sprite.reset();
    cut_in_animation(e, 20, 8); // Missing animation returns before requiring seb.
    missing([&] { restore_animation(e); });

    // Signed group alignment truncates toward zero, unlike a simple mask.
    e = fixture(); e.human = true;
    e.playback->return_clip = -5;
    restore_animation(e);
    check(e.sprite->clip == -1, "negative ID uses adjusted four-way group");
    constexpr auto hi = std::numeric_limits<std::int32_t>::max();
    constexpr auto lo = std::numeric_limits<std::int32_t>::min();
    e.playback->return_clip = -3;
    restore_animation(e);
    check(e.sprite->clip == 3, "negative near-zero group truncates toward zero");
    e.playback->return_clip = lo;
    restore_animation(e);
    check(e.sprite->clip == lo + 3, "minimum signed ID retains its group");
    e.direction = 1;
    change_animation(e, hi, false);
    check(e.sprite->clip == lo, "direction addition wraps32");
    e.playback->return_clip = hi; e.direction = hi;
    restore_animation(e);
    check(e.sprite->clip == -5, "restoration addition wraps32");

    e = fixture(); e.human = true; e.direction.reset();
    missing([&] { change_animation(e, 20, true); });
    check(e.sprite->clip == 8 && e.sprite->frame == 9, "change fails before mutations");
    cut_in_animation(e, 20, 12);
    check(e.sprite->clip == 20, "cut-in tolerates absent direction");
    e.sprite->frame = 9;
    missing([&] { restore_animation(e); });
    check(e.sprite->clip == 12 && e.sprite->frame == 0 && e.playback->return_clip == 12,
          "restore retains earlier mutations on missing direction");
    e = fixture(); e.sprite.reset();
    missing([&] { cut_in_animation(e, 20, 33); });
    check(e.playback->return_clip == 33, "cut-in writes backup before requiring seb");
    std::cout << "animation transition contracts passed\n";
}
