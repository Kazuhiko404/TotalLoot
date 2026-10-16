#pragma once

#include "common.h"

namespace totalloot
{
// What can be looted and where: SEH-guarded predicates over characters and
// containers, plus the area query used by Loot all and AUTO LOOT. Ported from
// TotalPredator so both mods agree on "lootable".

bool SafeIsPlayerOwned(Character* c);
bool SafeIsAnimal(Character* c);
bool SafeIsDead(Character* c);
bool SafeGetPosition(RootObject* obj, Ogre::Vector3* out);

// Dead, dying (bloodloss), playing dead, knocked out or ragdolled. Conscious
// crawling / crippled characters are NOT lootable.
bool SafeIsLootableCharacter(Character* c);

// A non-character object with items that is not owned by the player faction.
bool SafeIsLootableContainer(RootObject* obj);

// True when the object's inventory (or a character's worn backpack) holds items.
bool HasAnyLootableItems(RootObject* obj);

// Lootable, non-player bodies that still hold items within `radius` of `origin`,
// excluding `looter`, appended nearest-first from `sortFrom`. Returns the count
// added. Handles stay valid across frames; resolve them with ResolveBody.
int CollectBodiesInRadius(const Ogre::Vector3& origin, float radius, Character* looter,
                          const Ogre::Vector3& sortFrom, std::vector<hand>& out);

// The character a handle still points to, or NULL when it is gone.
Character* ResolveBody(const hand& handle);

// The body or container a handle still points to (any lootable object, not
// only characters), or NULL when it is gone.
RootObject* ResolveLootTarget(const hand& handle);
} // namespace totalloot
