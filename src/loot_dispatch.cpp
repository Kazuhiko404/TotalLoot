#include "loot_dispatch.h"
#include "loot_dispatch_rules.h"
#include "loot_config.h"
#include "loot_logic.h"
#include "loot_targets.h"
#include "loot_walk_session.h"
#include "pickpocket_actions.h"
#include "log.h"

namespace totalloot
{
namespace
{
bool SafeCaptureHandle(RootObject* object, hand* out)
{
    TL_TRY
    {
        *out = object->getHandle();
        return true;
    }
    TL_CATCH { return false; }
}

bool StartWalk(Character* looter, const std::vector<hand>& queue)
{
    if (!StartWalkLoot(looter, queue, false))
    {
        LogInfo("Loot: the walk could not start (looter unusable).");
        return false;
    }
    return true;
}
} // namespace

bool RequestLootSingle(Character* looter, RootObject* target)
{
    if (looter == NULL || target == NULL)
    {
        return false;
    }
    if (lootdispatch::RouteFor(Config().cheatLoot) == lootdispatch::ROUTE_INSTANT)
    {
        return ExecuteLootSingle(looter, target) >= 0;
    }

    hand targetHandle;
    if (!SafeCaptureHandle(target, &targetHandle))
    {
        return false;
    }
    std::vector<hand> queue;
    queue.push_back(targetHandle);
    return StartWalk(looter, queue);
}

bool RequestLootArea(Character* looter, RootObject* target)
{
    if (looter == NULL || target == NULL)
    {
        return false;
    }
    const LootConfig& cfg = Config();
    if (lootdispatch::RouteFor(cfg.cheatLoot) == lootdispatch::ROUTE_INSTANT)
    {
        return ExecuteLootArea(looter, target, cfg.lootRadius) >= 0;
    }

    hand targetHandle;
    if (!SafeCaptureHandle(target, &targetHandle))
    {
        return false;
    }

    // The radius is measured from the clicked target; the rest are ordered
    // nearest-first from where the looter stands.
    std::vector<hand> others;
    Ogre::Vector3 targetPos;
    Ogre::Vector3 looterPos;
    if (SafeGetPosition(target, &targetPos) && SafeGetPosition(looter, &looterPos))
    {
        CollectBodiesInRadius(targetPos, cfg.lootRadius, looter, looterPos, others);
    }
    return StartWalk(looter, lootdispatch::ComposeAreaQueue(targetHandle, others));
}

bool RequestAutoLoot(Character* looter)
{
    Ogre::Vector3 pos;
    if (looter == NULL || !SafeGetPosition(looter, &pos))
    {
        return false;
    }

    std::vector<hand> bodies;
    CollectBodiesInRadius(pos, Config().lootRadius, looter, pos, bodies);
    if (bodies.empty())
    {
        LogInfo("Auto loot: no bodies with loot within range.");
        pickpocket::ShowFeedback("Auto loot: nothing to loot nearby.");
        return false;
    }
    return StartWalkLoot(looter, bodies, true);
}

void ToggleAutoLoot(Character* looter)
{
    if (looter == NULL)
    {
        return;
    }
    if (IsAutoLooting(looter))
    {
        CancelWalkLoot(looter);
        return;
    }
    RequestAutoLoot(looter);
}
} // namespace totalloot
