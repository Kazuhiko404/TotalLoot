#pragma once

#include "common.h"

#include <vector>

namespace totalloot
{
// TotalLoot's view of the copied BetterLooting item rules. Snapshots what
// `source` holds (its inventory and, for a character, the worn backpack) and
// keeps only what the rules accept, best-first when that option is on. Money
// is never filtered. On a rule fault the snapshot is kept unfiltered. Returns
// false when the inventory could not be read.
bool CollectWantedLoot(RootObject* source, std::vector<Item*>& outItems);

// True when `source` holds at least one item CollectWantedLoot would keep:
// bodies holding only filtered-out items are not worth walking to. Silent, so
// it can run every tick.
bool HasWantedLoot(RootObject* source);
} // namespace totalloot
