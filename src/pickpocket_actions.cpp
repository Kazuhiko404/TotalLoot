#include "pickpocket_actions.h"
#include "order_watch.h"

#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/InventoryGUI.h>
#include <kenshi/AI/AITaskSystem.h>

namespace totalloot
{
namespace pickpocket
{
namespace
{
bool StopActor(Character* actor)
{
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

bool ShowTrade(Character* actor, Character* target)
{
    TL_TRY
    {
        if (gui == NULL)
        {
            return false;
        }
        // ForgottenGUI::_showTradeWindow is public in KenshiLib's header (the
        // "private" in the RVA dump comment is only an annotation), so it is
        // called directly. TW_LOOTING opens the same window as looting.
        gui->_showTradeWindow(actor, target, TW_LOOTING);
        return true;
    }
    TL_CATCH { return false; }
}

bool AddMoveOrder(Character* actor, Character* target)
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

bool SendMessageToPlayer(const std::string& text)
{
    TL_TRY
    {
        if (ou == NULL)
        {
            return false;
        }
        ou->showPlayerAMessage(text, false);
        return true;
    }
    TL_CATCH { return false; }
}
} // namespace

bool IssueApproachOrder(Character* actor, Character* target)
{
    if (actor == NULL || target == NULL)
    {
        return false;
    }
    // Marked as our own so order_watch does not mistake it for a player order.
    BeginOwnOrder();
    const bool issued = AddMoveOrder(actor, target);
    EndOwnOrder();
    return issued;
}

bool OpenLootWindow(Character* actor, Character* target)
{
    if (actor == NULL || target == NULL)
    {
        return false;
    }
    StopActor(actor);
    return ShowTrade(actor, target);
}

bool IsLootWindowVisible(const hand& target)
{
    TL_TRY
    {
        if (gui == NULL)
        {
            return false;
        }
        InventoryGUI* window = gui->getInventoryWindow(target);
        return window != NULL && window->isVisible();
    }
    TL_CATCH { return false; }
}

void CloseLootWindow(const hand& target)
{
    TL_TRY
    {
        if (gui != NULL)
        {
            gui->closeInventory(target);
        }
    }
    TL_CATCH { }
}

void ShowFeedback(const std::string& text)
{
    SendMessageToPlayer(text);
}
} // namespace pickpocket
} // namespace totalloot
