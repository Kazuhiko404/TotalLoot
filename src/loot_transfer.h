#pragma once

#include "common.h"

namespace totalloot
{
// Moves everything the looter can carry out of `source` (a character's
// inventory and worn backpack, or a container). Behaviour ported from
// TotalPredator: backpack first, items that do not fit are skipped (not the
// end of the transfer), cats go straight to the wallet, stacks are merged and
// the stolen-flag policy follows Config().clearStolenFlag.
//
// Returns the number of items moved. `outSourceHasItemsLeft` (optional) is set
// to whether `source` still holds items afterwards; together with a result of 0
// it means the looter has no room left.
int TransferAllLoot(Character* looter, RootObject* source, bool* outSourceHasItemsLeft = NULL);
} // namespace totalloot
