#pragma once

#include "common.h"

namespace totalloot
{
// SEH-guarded primitives over engine inventories. Every function tolerates NULL
// and a dead pointer; mutators return success instead of throwing.

// Inventory of any object (character, container). NULL on failure.
Inventory* SafeGetInventory(RootObject* obj);

// Inventory of the backpack the character wears, or NULL.
Inventory* SafeGetBackpackInventory(Character* character);

// Total number of item entries across all sections.
int CountItemsInInventory(Inventory* inv);

// Snapshots every item into `outItems` (so the engine collection is never
// mutated while iterating). False when the engine access faulted.
bool CollectLootableItems(Inventory* inv, std::vector<Item*>& outItems);

void SafeUnequip(Character* c, Item* item);
Item* SafeRemoveItem(Inventory* inv, Item* item, int qty);
bool SafeAddItem(Inventory* inv, Item* item, int qty);
void SafeDropItem(Inventory* inv, Item* item);
void SafeDestroyItem(Item* item);
void SafeRefreshInventory(Inventory* inv);
} // namespace totalloot
