#include "order_watch.h"
#include "log.h"

namespace totalloot
{
namespace
{
OrderWatchSet g_watch;

void (*g_addOrder_orig)(Character*, Building*, TaskType, RootObject*, bool, bool,
                        const Ogre::Vector3&) = NULL;

void Character_addOrder_hook(Character* thisptr, Building* dest, TaskType t,
                             RootObject* subject, bool shift, bool clear,
                             const Ogre::Vector3& location)
{
    g_watch.NoteOrder(thisptr, static_cast<int>(t), shift, clear);
    if (g_addOrder_orig != NULL)
    {
        g_addOrder_orig(thisptr, dest, t, subject, shift, clear, location);
    }
}
} // namespace

bool InstallOrderWatch()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Character::addOrder),
            Character_addOrder_hook,
            &g_addOrder_orig))
    {
        LogError("Failed to hook Character::addOrder.");
        return false;
    }
    LogInfo("Order watch hook installed (Character::addOrder).");
    return true;
}

void WatchOrdersOf(Character* actor)
{
    g_watch.Watch(actor);
}

void UnwatchOrdersOf(Character* actor)
{
    g_watch.Unwatch(actor);
}

bool TakeForeignOrder(Character* actor, ForeignOrder* out)
{
    return g_watch.Take(actor, out);
}

void BeginOwnOrder()
{
    g_watch.BeginOwn();
}

void EndOwnOrder()
{
    g_watch.EndOwn();
}
} // namespace totalloot
