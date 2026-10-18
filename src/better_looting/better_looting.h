#pragma once

#include <vector>

class Inventory;
class Item;

// Installs the copied BetterLooting hooks and UI thread. Returns true when started.
bool InstallBetterLooting();

// Stops the settings-window thread and destroys its window (waits up to 2 s).
void ShutdownBetterLooting();

// Applies BetterLooting's item rules (category, AllowStolen, MinValue,
// MinValuePerKg) to `in`, taken from `sourceInventory` (and, optionally, the
// source's worn backpack `extraInventory`, used only for the resource set).
// Items the rules accept are appended to `out`, ordered best-first when
// BestItemsFirst is on. Returns false when nothing could be evaluated.
bool BetterLootingSelectLootItems(
    Inventory* sourceInventory,
    Inventory* extraInventory,
    const std::vector<Item*>& in,
    std::vector<Item*>& out
);

// BetterLooting's PreferBackpack switch, read the way BetterLooting reads it.
bool BetterLootingPreferBackpack();
