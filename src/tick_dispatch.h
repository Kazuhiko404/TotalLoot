#pragma once

#include "common.h"

namespace totalloot
{
// The one hook on the per-frame game update (GameWorld::mainLoop). Several
// modules need a frame tick (pick pocket, walking loot); hooking the same
// function once per module would chain hooks on one address, so they register
// here instead and the hook runs them after the original update.

typedef void (*TickCallback)();

// Hooks the game update. Call once from startPlugin, before or after the
// registrations; ticks only run once the hook is installed.
bool InstallTickHook();

// Adds a callback (ignored when NULL or already registered). Callbacks run in
// registration order, once per frame.
void RegisterTick(TickCallback callback);

// Monotonic seconds since the first call; the shared clock of the walking loot.
float TickNowSeconds();
} // namespace totalloot
