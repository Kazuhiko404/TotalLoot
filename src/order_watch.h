#pragma once

#include "common.h"
#include "order_watch_set.h"

namespace totalloot
{
// Detects orders given to watched characters that TotalLoot did not issue.
//
// Hooks Character::addOrder (non-virtual, exported by KenshiLib): the path the
// engine takes when the player right-click-moves a character or issues any
// other order. Our own orders are excluded with a re-entrancy marker around our
// call, so only genuinely new orders count. Any number of actors can be watched
// at once (pick pocket plus every walking looter); ForeignOrder lives in
// order_watch_set.h.
bool InstallOrderWatch();

// Starts watching `actor` (counted: see OrderWatchSet::Watch). NULL is ignored.
void WatchOrdersOf(Character* actor);

// Releases one watch of `actor`.
void UnwatchOrdersOf(Character* actor);

// True once per foreign order for `actor` since the last call; fills `out` when
// non-NULL.
bool TakeForeignOrder(Character* actor, ForeignOrder* out);

// Brackets an addOrder that TotalLoot itself issues.
void BeginOwnOrder();
void EndOwnOrder();
} // namespace totalloot
