#include "loot_transfer.h"
#include "log.h"
#include "loot_config.h"
#include "loot_inventory.h"
#include "loot_money.h"
#include "loot_stacking.h"
#include "loot_targets.h"
#include "target_character.h"

namespace totalloot
{
namespace
{
// Decides whether a looted item keeps, gains or loses its stolen mark.
void ApplyStolenFlagPolicy(Item* item, Character* victim, RootObject* target, bool clearStolenFlag)
{
    if (item == NULL) return;

    if (clearStolenFlag)
    {
        // Strip the stolen flag completely: launder the item so it is clean.
        TL_TRY { item->setProperOwner(hand()); } TL_CATCH { }
        return;
    }

    // Native Kenshi behaviour.
    if (victim != NULL)
    {
        if (!SafeIsDead(victim))
        {
            // Victim is alive (KO, unconscious, dying, playing dead): taking an
            // item is theft. Only mark it when it was not already stolen.
            bool alreadyStolen = false;
            TL_TRY { alreadyStolen = bool(item->getProperOwner()); } TL_CATCH { }

            if (!alreadyStolen)
            {
                TL_TRY { item->notifyTheftFrom(victim); } TL_CATCH { }

                bool nowStolen = false;
                TL_TRY { nowStolen = bool(item->getProperOwner()); } TL_CATCH { }
                if (!nowStolen)
                {
                    TL_TRY { item->setProperOwner(Item::findProperOwner(victim)); } TL_CATCH { }
                }
            }
        }
        // Dead victim: legal salvage, no stolen flag. An item stolen before
        // death keeps its proper owner as-is.
    }
    else if (target != NULL)
    {
        // Target is a container / object.
        bool alreadyStolen = false;
        TL_TRY { alreadyStolen = bool(item->getProperOwner()); } TL_CATCH { }
        if (!alreadyStolen)
        {
            TL_TRY { item->notifyTheftFrom(target); } TL_CATCH { }
        }
    }
}

int TransferCollectedItems(Character* looter, Inventory* looterInv, Inventory* looterBpInv,
                           Character* victim, Inventory* victimInv,
                           const std::vector<Item*>& items, RootObject* target)
{
    int itemsLooted = 0;
    const bool clearStolen = Config().clearStolenFlag;
    LogInfo("Starting transfer of " + IntToStr(static_cast<long long>(items.size())) +
            " items. Backpack available: " + (looterBpInv != NULL ? "yes" : "no") +
            ", clear_stolen_flag=" + IntToStr(clearStolen ? 1 : 0));

    // Consolidate existing stacks first so there is maximum room in partial stacks.
    if (looterBpInv != NULL) ConsolidateInventoryStacks(looterBpInv, clearStolen);
    if (looterInv != NULL) ConsolidateInventoryStacks(looterInv, clearStolen);

    for (size_t i = 0; i < items.size(); ++i)
    {
        Item* item = items[i];
        if (item == NULL) continue;

        const int originalQty = item->quantity > 0 ? item->quantity : 1;

        // 1. Money: cash it into the player wallet without occupying a slot.
        // Only this physical item is redeemed, never the victim's money pool.
        if (IsMoneyItem(item))
        {
            const int catsToAdd = GetMoneyItemValue(item);
            LogInfo("Redeeming money item (qty=" + IntToStr(originalQty) + ") for " +
                    IntToStr(catsToAdd) + " cats directly to player wallet.");

            SafeUnequip(victim, item);
            Item* removed = SafeRemoveItem(victimInv, item, originalQty);
            if (removed != NULL)
            {
                SafeDestroyItem(removed);
            }

            CreditPlayerCats(looter, catsToAdd);
            itemsLooted++;
            continue;
        }

        // 2. Unequip if worn on the victim, then remove it from them.
        SafeUnequip(victim, item);
        Item* removed = SafeRemoveItem(victimInv, item, originalQty);
        if (removed == NULL)
        {
            LogInfo("Item " + IntToStr(static_cast<long long>(i)) + ": failed to remove from victim inventory.");
            continue;
        }

        // Stolen policy before stacking or slotting.
        ApplyStolenFlagPolicy(removed, victim, target, clearStolen);

        int remainingQty = originalQty;

        // 3. Stack into existing items in the backpack, then the main inventory.
        if (looterBpInv != NULL)
        {
            TryStackItemIntoInventory(looterBpInv, removed, remainingQty, clearStolen);
        }
        if (remainingQty > 0 && looterInv != NULL)
        {
            TryStackItemIntoInventory(looterInv, removed, remainingQty, clearStolen);
        }

        // 4. Completely absorbed by existing stacks.
        if (remainingQty <= 0)
        {
            itemsLooted++;
            SafeDestroyItem(removed);
            continue;
        }

        // 5. Remainder goes into a new slot (backpack first, then main inventory).
        removed->quantity = remainingQty;
        bool addedNewSlot = false;
        if (looterBpInv != NULL)
        {
            addedNewSlot = SafeAddItem(looterBpInv, removed, remainingQty);
        }
        if (!addedNewSlot && looterInv != NULL)
        {
            addedNewSlot = SafeAddItem(looterInv, removed, remainingQty);
        }

        if (addedNewSlot)
        {
            itemsLooted++;
            continue;
        }

        // 6. Did not fit anywhere. A stacked portion still counts as looted.
        if (remainingQty < originalQty)
        {
            itemsLooted++;
        }
        LogInfo("Item " + IntToStr(static_cast<long long>(i)) + " could not fit (remainder " +
                IntToStr(remainingQty) + "). Rolling back to victim.");

        // Roll the remainder back to the victim, or drop it on the ground.
        if (!SafeAddItem(victimInv, removed, remainingQty))
        {
            SafeDropItem(victimInv, removed);
            LogInfo("Item " + IntToStr(static_cast<long long>(i)) + " dropped on ground (victim could not re-take).");
        }
    }

    // Final consolidation pass, then refresh the UI of every touched inventory.
    if (looterBpInv != NULL) ConsolidateInventoryStacks(looterBpInv, clearStolen);
    if (looterInv != NULL) ConsolidateInventoryStacks(looterInv, clearStolen);

    LogInfo("Transfer complete: " + IntToStr(itemsLooted) + " of " +
            IntToStr(static_cast<long long>(items.size())) + " items transferred.");

    SafeRefreshInventory(looterBpInv);
    SafeRefreshInventory(looterInv);
    SafeRefreshInventory(victimInv);

    return itemsLooted;
}
} // namespace

int TransferAllLoot(Character* looter, RootObject* source, bool* outSourceHasItemsLeft)
{
    if (outSourceHasItemsLeft != NULL) *outSourceHasItemsLeft = false;
    if (looter == NULL || source == NULL) return 0;

    Inventory* looterInv = SafeGetInventory(looter);
    Inventory* looterBpInv = SafeGetBackpackInventory(looter);
    if (looterInv == NULL && looterBpInv == NULL) return 0;

    Character* victim = CharacterFromRootObject(source);
    Inventory* victimInv = SafeGetInventory(source);
    if (victimInv == NULL) return 0;

    // Snapshot every item first so we never mutate the engine collection while
    // iterating it; a worn backpack's contents are collected too.
    std::vector<Item*> items;
    if (!CollectLootableItems(victimInv, items))
    {
        LogError("Exception collecting items to transfer from target");
        return 0;
    }
    if (victim != NULL)
    {
        CollectLootableItems(SafeGetBackpackInventory(victim), items);
    }
    if (items.empty()) return 0;

    const int moved = TransferCollectedItems(looter, looterInv, looterBpInv, victim, victimInv,
                                             items, source);
    if (outSourceHasItemsLeft != NULL)
    {
        *outSourceHasItemsLeft = HasAnyLootableItems(source);
    }
    return moved;
}
} // namespace totalloot
