#pragma once

#include "common.h"

namespace totalloot
{
// Facts about the looter and its squad that end a walking loot. SEH-guarded
// reads; ported from TotalPredator so both mods agree.

// Distance within which an attacked squadmate cancels a loot walk (m), the
// same default as TotalPredator's ally_defense_radius.
const float kAllyDefenseRadius = 100.0f;

// Alive, valid and able to act (not dead, down, unconscious or ragdolled).
bool IsActorOperational(Character* actor);

// Being attacked or in melee / ranged combat mode.
bool IsInCombat(Character* actor);

// A squadmate other than `looter` within `radius` of `pos` is in combat or
// under attack.
bool NearbyAllyAttacked(Character* looter, const Ogre::Vector3& pos, float radius);
} // namespace totalloot
