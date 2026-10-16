#pragma once

#include "common.h"

namespace totalloot
{
// The one entry point for looting requests (the context-menu buttons call only
// this). CheatLoot=1 loots instantly from where the looter stands; CheatLoot=0
// hands the bodies to the walking loot (loot_walk_session). All of them return
// false when nothing was started or done.

// Loot: one body or container.
bool RequestLootSingle(Character* looter, RootObject* target);

// Loot all: the clicked target first, then every body within LootRadius of it.
bool RequestLootArea(Character* looter, RootObject* target);

// AUTO LOOT: walks the looter through the bodies around it, nearest first.
// Always walks, whatever CheatLoot says. False (with a short in-game message)
// when there is nothing to loot nearby.
bool RequestAutoLoot(Character* looter);

// AUTO LOOT button: stops the looter's auto loot when running, else starts it.
void ToggleAutoLoot(Character* looter);
} // namespace totalloot
