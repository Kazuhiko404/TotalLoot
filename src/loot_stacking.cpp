#include "loot_stacking.h"
#include "loot_inventory.h"

namespace totalloot
{
namespace
{
bool AreOwnersEqual(Item* a, Item* b)
{
    if (a == NULL || b == NULL) return false;
    TL_TRY
    {
        return a->getProperOwner() == b->getProperOwner();
    }
    TL_CATCH
    {
        return true;
    }
}
} // namespace

bool TryStackItemIntoInventory(Inventory* inv, Item* item, int& inOutRemainingQty,
                               bool clearStolenFlag)
{
    if (inv == NULL || item == NULL || inOutRemainingQty <= 0) return false;

    bool anyStacked = false;
    TL_TRY
    {
        lektor<InventorySection*>& sections = inv->getAllSections();
        for (unsigned int s = 0; s < sections.size() && inOutRemainingQty > 0; ++s)
        {
            InventorySection* sec = sections[s];
            if (sec == NULL || !sec->getEnabled()) continue;

            int maxStack = 1;
            TL_TRY { maxStack = item->isStackable(sec); } TL_CATCH { maxStack = 1; }
            if (maxStack <= 1) continue;

            const Ogre::vector<InventorySection::SectionItem>::type& sItems = sec->getItems();
            for (size_t i = 0; i < sItems.size() && inOutRemainingQty > 0; ++i)
            {
                Item* existing = sItems[i].item;
                if (existing == NULL || existing == item) continue;

                bool canStack = false;
                TL_TRY { canStack = existing->canStackWith(item); } TL_CATCH { canStack = false; }
                if (!canStack) continue;

                if (!clearStolenFlag && !AreOwnersEqual(existing, item)) continue;

                int currentQty = existing->quantity > 0 ? existing->quantity : 1;
                if (currentQty < maxStack)
                {
                    int space = maxStack - currentQty;
                    int toAdd = (inOutRemainingQty < space) ? inOutRemainingQty : space;

                    existing->quantity += toAdd;
                    inOutRemainingQty -= toAdd;
                    anyStacked = true;

                    TL_TRY
                    {
                        sec->recalculateTotalWeight();
                        sec->notifyModified();
                        inv->notifyModified();
                    }
                    TL_CATCH { }
                }
            }
        }
    }
    TL_CATCH
    {
        return anyStacked;
    }
    return anyStacked;
}

void ConsolidateInventoryStacks(Inventory* inv, bool clearStolenFlag)
{
    if (inv == NULL) return;
    TL_TRY
    {
        lektor<InventorySection*>& sections = inv->getAllSections();
        for (unsigned int s = 0; s < sections.size(); ++s)
        {
            InventorySection* sec = sections[s];
            if (sec == NULL || !sec->getEnabled()) continue;

            const Ogre::vector<InventorySection::SectionItem>::type& sItems = sec->getItems();
            for (size_t i = 0; i < sItems.size(); ++i)
            {
                Item* baseItem = sItems[i].item;
                if (baseItem == NULL) continue;

                int maxStack = 1;
                TL_TRY { maxStack = baseItem->isStackable(sec); } TL_CATCH { maxStack = 1; }
                if (maxStack <= 1) continue;

                for (size_t j = i + 1; j < sItems.size() && baseItem->quantity < maxStack;)
                {
                    Item* otherItem = sItems[j].item;
                    if (otherItem != NULL && otherItem != baseItem)
                    {
                        bool canStack = false;
                        TL_TRY { canStack = baseItem->canStackWith(otherItem); } TL_CATCH { canStack = false; }
                        if (canStack)
                        {
                            if (!clearStolenFlag && !AreOwnersEqual(baseItem, otherItem))
                            {
                                ++j;
                                continue;
                            }

                            int space = maxStack - baseItem->quantity;
                            int otherQty = otherItem->quantity > 0 ? otherItem->quantity : 1;
                            int transfer = (otherQty < space) ? otherQty : space;

                            baseItem->quantity += transfer;
                            otherQty -= transfer;

                            if (otherQty <= 0)
                            {
                                inv->removeItemDontDestroy_returnsItem(otherItem, otherItem->quantity, false);
                                SafeDestroyItem(otherItem);
                                break;
                            }
                            else
                            {
                                otherItem->quantity = otherQty;
                                break;
                            }
                        }
                        else
                        {
                            ++j;
                        }
                    }
                    else
                    {
                        ++j;
                    }
                }
            }
            TL_TRY
            {
                sec->recalculateTotalWeight();
                sec->notifyModified();
            }
            TL_CATCH { }
        }
        inv->notifyModified();
    }
    TL_CATCH { }
}
} // namespace totalloot
