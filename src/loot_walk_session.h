#pragma once

#include "common.h"

namespace totalloot
{
// Engine glue for the walking loot: one session per looter (several looters can
// walk at once). A session snapshots its targets as handles, feeds the engine
// facts to lootwalk::Step each frame and performs the action it returns (move,
// loot, end). The frame tick comes from tick_dispatch.

// Registers the per-frame tick. Call once from startPlugin, together with
// InstallTickHook().
bool InstallLootWalkSession();

// Ends every session without touching the game.
void ShutdownLootWalkSession();

// Starts walking `looter` through `targets` in order (bodies or containers),
// looting each on arrival. Replaces any session the looter already has.
// `autoLoot` only labels the session (see IsAutoLooting). False when the looter
// is unusable or `targets` is empty.
bool StartWalkLoot(Character* looter, const std::vector<hand>& targets, bool autoLoot);

// Ends the looter's session and stops the walk it started. No-op without one.
void CancelWalkLoot(Character* looter);

// The looter has a running session (of any kind).
bool IsWalkLooting(Character* looter);

// The looter has a running AUTO LOOT session.
bool IsAutoLooting(Character* looter);
} // namespace totalloot
