#pragma once
#include "contact_collision.hpp"

namespace saga::core {
// CollisionSystem.CheckHit 0x114698c: inclusive 3D spheres, wrapping int32 radius arithmetic.
// First null faults; a missing first Collision returns false without inspecting second.
bool check_collision(ComponentSlots* first,ComponentSlots* second);
}
