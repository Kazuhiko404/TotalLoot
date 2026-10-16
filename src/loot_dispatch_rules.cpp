#include "loot_dispatch_rules.h"

namespace totalloot
{
namespace lootdispatch
{
Route RouteFor(bool cheatLoot)
{
    return cheatLoot ? ROUTE_INSTANT : ROUTE_WALK;
}
} // namespace lootdispatch
} // namespace totalloot
