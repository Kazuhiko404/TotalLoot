#include "loot_move.h"
#include "loot_targets.h"
#include "order_watch.h"

#include <kenshi/AI/AITaskSystem.h>

namespace totalloot
{
namespace
{
bool AddMoveOrder(Character* actor, RootObject* target)
{
    TL_TRY
    {
        Building* building = target->isIndoors().getBuilding();
        const Ogre::Vector3 destination = target->getPosition();
        actor->addOrder(building, MOVE_CUS_ORDERED, NULL, false, true, destination);
        return true;
    }
    TL_CATCH { return false; }
}

// Same extra steps as TotalPredator's auto-loot for animals; harmless to skip
// when the engine refuses them.
void KickAnimal(Character* actor)
{
    TL_TRY
    {
        actor->addGoal(MOVE_CUS_ORDERED, NULL);
        CharBody* body = actor->getBody();
        if (body != NULL)
        {
            body->_NV_setCurrentAction(MOVE_CUS_ORDERED, NULL);
        }
    }
    TL_CATCH { }
}
} // namespace

bool IssueMoveTo(Character* actor, RootObject* target)
{
    if (actor == NULL || target == NULL)
    {
        return false;
    }
    BeginOwnOrder();
    const bool issued = AddMoveOrder(actor, target);
    if (issued && SafeIsAnimal(actor))
    {
        KickAnimal(actor);
    }
    EndOwnOrder();
    return issued;
}

bool StopActor(Character* actor)
{
    if (actor == NULL)
    {
        return false;
    }
    TL_TRY
    {
        OrdersReceiver* orders = actor->getOrdersReciever();
        if (orders != NULL)
        {
            orders->clearOrders();
        }
        actor->reThinkCurrentAIAction();
        return true;
    }
    TL_CATCH { return false; }
}
} // namespace totalloot
