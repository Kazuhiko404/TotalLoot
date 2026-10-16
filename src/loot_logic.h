#pragma once

#include "common.h"

namespace totalloot
{
// Check if a target object has lootable items
bool IsTargetLootable(Character* selectedChar, RootObject* targetObject, std::string* outReason = NULL);

// Loot all items possible from a single target to player character
int ExecuteLootSingle(Character* playerChar, RootObject* targetObject);

// Loot all items from all lootable targets in an area around the primary target
int ExecuteLootArea(Character* playerChar, RootObject* primaryTarget, float radiusMeters = 20.0f);
} // namespace totalloot
