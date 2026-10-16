#include "test_harness.h"
#include "loot_dispatch_rules.h"

using namespace totalloot::lootdispatch;

namespace
{
const char* const kSuiteName = "loot_dispatch_rules";

void CheatLootTransfersInstantly()
{
    CHECK(RouteFor(true) == ROUTE_INSTANT);
}

void LegitLootWalks()
{
    CHECK(RouteFor(false) == ROUTE_WALK);
}

std::vector<int> Others(int a, int b, int c)
{
    std::vector<int> v;
    v.push_back(a);
    v.push_back(b);
    v.push_back(c);
    return v;
}

void PrimaryComesFirstThenOthersInOrder()
{
    const std::vector<int> q = ComposeAreaQueue(9, Others(1, 2, 3));
    CHECK(q.size() == 4);
    if (q.size() != 4) return;
    CHECK(q[0] == 9);
    CHECK(q[1] == 1);
    CHECK(q[2] == 2);
    CHECK(q[3] == 3);
}

void PrimaryIsNotVisitedTwice()
{
    const std::vector<int> q = ComposeAreaQueue(2, Others(1, 2, 3));
    CHECK(q.size() == 3);
    if (q.size() != 3) return;
    CHECK(q[0] == 2);
    CHECK(q[1] == 1);
    CHECK(q[2] == 3);
}

void NoOthersLeavesJustThePrimary()
{
    const std::vector<int> q = ComposeAreaQueue(5, std::vector<int>());
    CHECK(q.size() == 1);
    if (q.size() != 1) return;
    CHECK(q[0] == 5);
}

void EveryCopyOfThePrimaryIsDropped()
{
    std::vector<int> others = Others(7, 1, 7);
    const std::vector<int> q = ComposeAreaQueue(7, others);
    CHECK(q.size() == 2);
    if (q.size() != 2) return;
    CHECK(q[0] == 7);
    CHECK(q[1] == 1);
}
} // namespace

void RunLootDispatchRulesTests()
{
    CheatLootTransfersInstantly();
    LegitLootWalks();
    PrimaryComesFirstThenOthersInOrder();
    PrimaryIsNotVisitedTwice();
    NoOthersLeavesJustThePrimary();
    EveryCopyOfThePrimaryIsDropped();
}
