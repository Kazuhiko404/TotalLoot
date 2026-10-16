#pragma once

#include "common.h"

namespace totalloot
{
// Hooks the per-frame update that drives approach and auto-close.
bool InstallPickPocketSession();

// Drops any running session (does not touch the game).
void ShutdownPickPocketSession();

// Re-checks eligibility, then walks `actor` to `target` and opens the native
// loot window once in range. False when it could not start.
bool StartPickPocket(Character* actor, Character* target);
} // namespace totalloot
