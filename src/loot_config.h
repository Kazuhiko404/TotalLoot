#pragma once

#include <istream>

// Looting settings read from TotalLoot.ini. The struct and the parser are pure
// (no Windows / Kenshi includes) so they are unit-tested; the file IO lives in
// loot_config_io.cpp.

namespace totalloot
{
const float kDefaultLootRadius = 20.0f;
const float kMaxLootRadius = 500.0f;
const float kDefaultArriveDistance = 5.0f;
const float kMaxArriveDistance = 100.0f;

struct LootConfig
{
    bool cheatLoot;                     // true = instant remote loot
    float lootRadius;                   // Loot all / AUTO LOOT search radius (m)
    float arriveDistance;               // looter counts as arrived inside this (m)
    bool clearStolenFlag;               // strip the stolen flag from looted items
    bool cancelOnCombat;                // abort a walking loot when combat starts
    bool cancelWhenNearbyAllyAttacked;  // abort when a nearby ally is attacked

    LootConfig()
        : cheatLoot(false), lootRadius(kDefaultLootRadius),
          arriveDistance(kDefaultArriveDistance), clearStolenFlag(false),
          cancelOnCombat(true), cancelWhenNearbyAllyAttacked(true) {}
};

// Parses INI text. Sections and keys are case-insensitive; `;` and `#` start a
// comment line. Unknown keys and malformed values keep the default; non-positive
// radii fall back to the default and large ones are clamped.
LootConfig ParseLootConfig(std::istream& in);

// Loads TotalLoot.ini from next to the DLL (defaults when missing) and logs the
// result. Call once from startPlugin.
void LoadConfig();

// The active config (defaults until LoadConfig runs).
const LootConfig& Config();
} // namespace totalloot
