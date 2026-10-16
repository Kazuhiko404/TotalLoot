#include "loot_inventory.h"

namespace totalloot
{
Inventory* SafeGetInventory(RootObject* obj)
{
    if (obj == NULL) return NULL;
    TL_TRY { return obj->getInventory(); }
    TL_CATCH { return NULL; }
}

Inventory* SafeGetBackpackInventory(Character* character)
{
    if (character == NULL) return NULL;
    TL_TRY
    {
        ContainerItem* bp = character->hasABackpackOn();
        if (bp != NULL)
        {
            Inventory* inv = bp->inventory;
            if (inv != NULL) return inv;
            return bp->getInventory();
        }
    }
    TL_CATCH { }
    return NULL;
}

int CountItemsInInventory(Inventory* inv)
{
    if (inv == NULL) return 0;
    int count = 0;
    TL_TRY
    {
        lektor<InventorySection*>& sections = inv->getAllSections();
        for (unsigned int s = 0; s < sections.size(); ++s)
        {
            InventorySection* sec = sections[s];
            if (sec != NULL)
            {
                count += sec->getNumItems();
            }
        }
    }
    TL_CATCH { return 0; }
    return count;
}

// Mirrors the engine collection into a separate vector so we never mutate it
// while iterating.
bool CollectLootableItems(Inventory* inv, std::vector<Item*>& outItems)
{
    if (inv == NULL) return false;
    TL_TRY
    {
        lektor<InventorySection*>& sections = inv->getAllSections();
        for (unsigned int s = 0; s < sections.size(); ++s)
        {
            InventorySection* sec = sections[s];
            if (sec == NULL) continue;

            const Ogre::vector<InventorySection::SectionItem>::type& sItems = sec->getItems();
            for (size_t i = 0; i < sItems.size(); ++i)
            {
                Item* item = sItems[i].item;
                if (item != NULL)
                {
                    outItems.push_back(item);
                }
            }
        }
        return true;
    }
    TL_CATCH { return false; }
}

void SafeUnequip(Character* c, Item* item)
{
    if (c == NULL || item == NULL) return;
    TL_TRY
    {
        if (item->isEquipped)
        {
            c->unequipItem(item->inventorySection, item);
        }
    }
    TL_CATCH { }
}

Item* SafeRemoveItem(Inventory* inv, Item* item, int qty)
{
    if (inv == NULL || item == NULL) return NULL;
    TL_TRY
    {
        return inv->removeItemDontDestroy_returnsItem(item, qty, false);
    }
    TL_CATCH { return NULL; }
}

bool SafeAddItem(Inventory* inv, Item* item, int qty)
{
    if (inv == NULL || item == NULL) return false;
    TL_TRY
    {
        return inv->addItem(item, qty, false, false);
    }
    TL_CATCH { return false; }
}

void SafeDropItem(Inventory* inv, Item* item)
{
    if (inv == NULL || item == NULL) return;
    TL_TRY { inv->dropItem(item); }
    TL_CATCH { }
}

void SafeDestroyItem(Item* item)
{
    if (item == NULL) return;
    TL_TRY
    {
        if (ou != NULL)
        {
            ou->destroy(item, false, "totalloot_stack_absorbed");
        }
    }
    TL_CATCH { }
}

void SafeRefreshInventory(Inventory* inv)
{
    if (inv == NULL) return;
    TL_TRY
    {
        inv->notifyModified();
        inv->refreshGui();
    }
    TL_CATCH { }
}
} // namespace totalloot
