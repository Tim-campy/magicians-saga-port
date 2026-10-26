#pragma once
#include "contact_collision.hpp"

namespace saga::core {
// Per-entity normal path of MoveSystem.Update 0x115faac. Required slots: Position 0, Speed 1.
// Caller preserves live subset order; this follows the projectile completion pass.
void move_entity(ComponentSlots&);
}
