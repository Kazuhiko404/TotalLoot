#pragma once

// Pure decisions behind loot_dispatch: how a Loot request is carried out and in
// what order an area walk visits its bodies. No Kenshi includes so it is
// unit-tested; targets are an opaque type compared with operator==.

#include <cstddef>
#include <vector>

namespace totalloot
{
namespace lootdispatch
{
enum Route
{
    ROUTE_INSTANT = 0,  // CheatLoot=1: transfer from where the looter stands
    ROUTE_WALK          // CheatLoot=0: walk to each body and loot on arrival
};

// Loot and Loot all follow CheatLoot. AUTO LOOT never calls this: it always
// walks.
Route RouteFor(bool cheatLoot);

// The walk queue of an area loot: `primary` first, then `others` in their given
// order without any copy of `primary`.
template <typename T>
std::vector<T> ComposeAreaQueue(const T& primary, const std::vector<T>& others)
{
    std::vector<T> queue;
    queue.push_back(primary);
    for (size_t i = 0; i < others.size(); ++i)
    {
        if (!(others[i] == primary))
        {
            queue.push_back(others[i]);
        }
    }
    return queue;
}
} // namespace lootdispatch
} // namespace totalloot
