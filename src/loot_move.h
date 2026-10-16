#pragma once

#include "common.h"

namespace totalloot
{
// Movement orders for the walking loot. SEH-guarded; each returns false if the
// engine faulted. The orders are marked as ours (order_watch) so they never
// count as a player order.

// Walks `actor` to `target` (body or container), replacing its current orders.
// Animals get the extra goal / action kick that TotalPredator used, because a
// plain order alone does not always wake an idle animal.
bool IssueMoveTo(Character* actor, RootObject* target);

// Clears the actor's orders and re-thinks its current action.
bool StopActor(Character* actor);
} // namespace totalloot
