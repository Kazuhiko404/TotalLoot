#pragma once

#include "common.h"

namespace totalloot
{
// Cats are redeemed straight into the player wallet instead of taking a slot.

// True for a physical money item ("cats").
bool IsMoneyItem(Item* item);

// Cats the item is worth (quantity * single value, at least 1 each).
int GetMoneyItemValue(Item* item);

// Adds `amount` to the looter's wallet, falling back to the player platoon.
bool CreditPlayerCats(Character* looter, int amount);
} // namespace totalloot
