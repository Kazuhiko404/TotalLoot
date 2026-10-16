#pragma once

#include "common.h"

namespace totalloot
{
namespace pickpocket
{
// Engine side effects used by the pick-pocket session. SEH-guarded; each
// returns false if the engine faulted.

// Walks the actor to the target (custom move order, replaces current orders).
bool IssueApproachOrder(Character* actor, Character* target);

// Stops the actor and opens the native loot window on the target.
bool OpenLootWindow(Character* actor, Character* target);

// True while the loot window for `target` is on screen.
bool IsLootWindowVisible(const hand& target);

// Closes the loot window for `target` if open.
void CloseLootWindow(const hand& target);

// Short on-screen player message.
void ShowFeedback(const std::string& text);
} // namespace pickpocket
} // namespace totalloot
