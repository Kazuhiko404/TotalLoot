#pragma once

#include "common.h"

namespace totalloot
{
// Stacking rules for looted items. With `clearStolenFlag` false, items with
// different proper owners never merge, so theft identity is preserved.

// Adds as much of `inOutRemainingQty` of `item` as fits into existing stacks of
// `inv`; decrements the remainder. True when anything was stacked.
bool TryStackItemIntoInventory(Inventory* inv, Item* item, int& inOutRemainingQty,
                               bool clearStolenFlag);

// Merges partially filled stacks inside `inv` to free slots.
void ConsolidateInventoryStacks(Inventory* inv, bool clearStolenFlag);
} // namespace totalloot
