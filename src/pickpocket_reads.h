#pragma once

#include "common.h"
#include "pickpocket_rules.h"

namespace totalloot
{
namespace pickpocket
{
// Engine reads feeding the pure rules. Every call is SEH-guarded and returns
// false (leaving the output unspecified) if the engine faults.

// All booleans for the Evaluate() decision, including dialogue / modal state.
bool ReadEligibilityInput(Character* actor, Character* target, EligibilityInput* out);

// Horizontal offset actor -> target plus the floor / building match, and the
// target's horizontal position (for "has the target moved" tracking).
bool ReadRange(Character* actor, Character* target, RangeInput* out,
               float* targetX, float* targetZ);

// Per-frame facts about the thief while a session runs.
bool ReadActorState(Character* actor, bool* stealth, bool* combat);

// Target still exists and is alive.
bool ReadTargetAlive(Character* target);

// Character behind a hand, or NULL.
Character* ResolveHand(const hand& h);
} // namespace pickpocket
} // namespace totalloot
