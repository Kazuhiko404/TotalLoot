#include "loot_filter.h"
#include "better_looting/better_looting.h"
#include "log.h"
#include "loot_inventory.h"
#include "loot_money.h"
#include "target_character.h"

namespace totalloot
{
namespace
{
// SEH boundary around BetterLooting's rule evaluation (no objects with
// destructors may live in a __try function, hence the separate wrapper).
bool RunBetterLootingRules(Inventory* sourceInv, Inventory* sourceBpInv,
                           const std::vector<Item*>& in, std::vector<Item*>& out)
{
    TL_TRY { return BetterLootingSelectLootItems(sourceInv, sourceBpInv, in, out); }
    TL_CATCH { return false; }
}

// Narrows `items` to what BetterLooting's rules accept, money first and never
// filtered. On a fault the list is left unfiltered.
void ApplyBetterLootingRules(Inventory* sourceInv, Inventory* sourceBpInv,
                             std::vector<Item*>& items)
{
    std::vector<Item*> money;
    std::vector<Item*> others;
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (items[i] == NULL) continue;
        if (IsMoneyItem(items[i])) money.push_back(items[i]);
        else others.push_back(items[i]);
    }

    std::vector<Item*> accepted;
    if (!RunBetterLootingRules(sourceInv, sourceBpInv, others, accepted))
    {
        LogError("BetterLooting rules failed; keeping items unfiltered");
        return;
    }

    money.insert(money.end(), accepted.begin(), accepted.end());
    items.swap(money);
}
} // namespace

bool CollectWantedLoot(RootObject* source, std::vector<Item*>& outItems)
{
    Inventory* inv = SafeGetInventory(source);
    if (inv == NULL) return false;

    // Snapshot first so the engine collection is never mutated while iterated.
    if (!CollectLootableItems(inv, outItems)) return false;

    Character* victim = CharacterFromRootObject(source);
    Inventory* backpack = (victim != NULL) ? SafeGetBackpackInventory(victim) : NULL;
    if (backpack != NULL)
    {
        CollectLootableItems(backpack, outItems);
    }
    if (outItems.empty()) return true;

    ApplyBetterLootingRules(inv, backpack, outItems);
    return true;
}

bool HasWantedLoot(RootObject* source)
{
    if (source == NULL) return false;
    std::vector<Item*> items;
    return CollectWantedLoot(source, items) && !items.empty();
}
} // namespace totalloot
