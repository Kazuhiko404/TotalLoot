#include "common.h"
#include "loot_logic.h"
#include "log.h"
#include "loot_targets.h"
#include "loot_transfer.h"
#include "target_character.h"

namespace totalloot
{
namespace
{
// Bodies in a row that yield nothing while still holding items: the looter has
// no room left, so the area loot stops.
const int kMaxConsecutiveEmptyBodies = 3;
} // namespace

bool IsTargetLootable(Character* selectedChar, RootObject* targetObject, std::string* outReason)
{
    if (selectedChar == NULL)
    {
        if (outReason) *outReason = "No character selected";
        return false;
    }

    if (targetObject == NULL)
    {
        if (outReason) *outReason = "No target object";
        return false;
    }

    if (targetObject == selectedChar)
    {
        if (outReason) *outReason = "Cannot loot self";
        return false;
    }

    Character* targetChar = CharacterFromRootObject(targetObject);
    if (targetChar != NULL)
    {
        if (!SafeIsLootableCharacter(targetChar))
        {
            if (outReason)
            {
                *outReason = SafeIsPlayerOwned(targetChar) ? "Cannot loot conscious squadmate"
                                                           : "Target is alive and standing";
            }
            return false;
        }
    }
    else if (!SafeIsLootableContainer(targetObject))
    {
        if (outReason) *outReason = "Target is not a lootable container";
        return false;
    }

    if (!HasAnyLootableItems(targetObject))
    {
        if (outReason) *outReason = "Target inventory is empty";
        return false;
    }

    return true;
}

int ExecuteLootSingle(Character* playerChar, RootObject* targetObject)
{
    return TransferAllLoot(playerChar, targetObject, NULL);
}

int ExecuteLootArea(Character* playerChar, RootObject* primaryTarget, float radiusMeters)
{
    if (playerChar == NULL || primaryTarget == NULL) return 0;

    int totalTransferred = 0;
    int consecutiveEmpty = 0;

    // 1. Loot the primary target first.
    bool stillHasItems = false;
    int transferred = TransferAllLoot(playerChar, primaryTarget, &stillHasItems);
    totalTransferred += transferred;
    if (transferred == 0 && stillHasItems) ++consecutiveEmpty;

    // 2. Everything else within the radius of the primary target, nearest first.
    Ogre::Vector3 center;
    if (consecutiveEmpty < kMaxConsecutiveEmptyBodies && SafeGetPosition(primaryTarget, &center))
    {
        std::vector<hand> bodies;
        CollectBodiesInRadius(center, radiusMeters, playerChar, center, bodies);
        for (size_t i = 0; i < bodies.size() && consecutiveEmpty < kMaxConsecutiveEmptyBodies; ++i)
        {
            Character* body = ResolveBody(bodies[i]);
            if (body == NULL || body == primaryTarget) continue;

            transferred = TransferAllLoot(playerChar, body, &stillHasItems);
            totalTransferred += transferred;
            if (transferred == 0 && stillHasItems) ++consecutiveEmpty;
            else consecutiveEmpty = 0;
        }
    }

    LogInfo("ExecuteLootArea: looted " + IntToStr(totalTransferred) + " items across area.");
    return totalTransferred;
}

} // namespace totalloot
