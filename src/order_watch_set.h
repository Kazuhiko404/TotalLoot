#pragma once

// Pure bookkeeping behind order_watch: which actors are watched, which of them
// received an order TotalLoot did not issue, and the marker that excludes our
// own orders. No Kenshi includes so it is unit-tested; actors are opaque.

#include <map>

namespace totalloot
{
struct ForeignOrder
{
    int taskType;
    bool shift;
    bool clear;
};

class OrderWatchSet
{
public:
    OrderWatchSet();

    // Starts watching `actor`. Watching is counted: pick pocket and a walking
    // loot can both watch one actor, which stays watched until every Watch has
    // its Unwatch. The first Watch drops anything pending; later ones keep it.
    // NULL is ignored.
    void Watch(const void* actor);

    // Releases one Watch; at zero the actor and its pending order are
    // forgotten. Unbalanced calls are harmless.
    void Unwatch(const void* actor);

    bool IsWatched(const void* actor) const;

    // Called for every addOrder. Recorded only for a watched actor while no own
    // order is in flight.
    void NoteOrder(const void* actor, int taskType, bool shift, bool clear);

    // True once per foreign order for `actor` since the last call; fills `out`
    // when non-NULL.
    bool Take(const void* actor, ForeignOrder* out);

    // Brackets an addOrder that TotalLoot itself issues.
    void BeginOwn();
    void EndOwn();

private:
    struct Entry
    {
        int watchers;
        bool pending;
        ForeignOrder last;
    };

    std::map<const void*, Entry> m_actors;
    int m_ownDepth;
};
} // namespace totalloot
