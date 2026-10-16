#include "test_harness.h"
#include "order_watch_set.h"

using namespace totalloot;

namespace
{
const char* const kSuiteName = "order_watch_set";

int a, b, c; // distinct addresses stand in for actors

void UnwatchedActorRecordsNothing()
{
    OrderWatchSet w;
    w.NoteOrder(&a, 1, false, false);
    CHECK(!w.IsWatched(&a));
    CHECK(!w.Take(&a, NULL));
}

void WatchedActorReportsForeignOrderOnce()
{
    OrderWatchSet w;
    w.Watch(&a);
    CHECK(w.IsWatched(&a));
    w.NoteOrder(&a, 7, true, false);
    ForeignOrder out = { 0, false, false };
    CHECK(w.Take(&a, &out));
    CHECK(out.taskType == 7);
    CHECK(out.shift);
    CHECK(!out.clear);
    CHECK(!w.Take(&a, &out));
}

void TakeWithoutOutputPointerStillConsumes()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.NoteOrder(&a, 1, false, true);
    CHECK(w.Take(&a, NULL));
    CHECK(!w.Take(&a, NULL));
}

void ActorsAreIndependent()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.Watch(&b);
    w.NoteOrder(&a, 3, false, false);
    CHECK(!w.Take(&b, NULL));
    ForeignOrder out = { 0, false, false };
    CHECK(w.Take(&a, &out));
    CHECK(out.taskType == 3);
}

void LatestOrderWinsPerActor()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.NoteOrder(&a, 1, false, false);
    w.NoteOrder(&a, 2, true, true);
    ForeignOrder out = { 0, false, false };
    CHECK(w.Take(&a, &out));
    CHECK(out.taskType == 2);
    CHECK(out.shift);
    CHECK(out.clear);
}

void OwnOrdersAreExcluded()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.BeginOwn();
    w.NoteOrder(&a, 1, false, false);
    w.EndOwn();
    CHECK(!w.Take(&a, NULL));
    w.NoteOrder(&a, 1, false, false);
    CHECK(w.Take(&a, NULL));
}

void OwnOrdersNest()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.BeginOwn();
    w.BeginOwn();
    w.EndOwn();
    w.NoteOrder(&a, 1, false, false);
    CHECK(!w.Take(&a, NULL));
    w.EndOwn();
    w.NoteOrder(&a, 1, false, false);
    CHECK(w.Take(&a, NULL));
}

void UnbalancedEndOwnIsHarmless()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.EndOwn();
    w.NoteOrder(&a, 1, false, false);
    CHECK(w.Take(&a, NULL));
}

void UnwatchForgetsActorAndPending()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.Watch(&b);
    w.NoteOrder(&a, 1, false, false);
    w.Unwatch(&a);
    CHECK(!w.IsWatched(&a));
    CHECK(w.IsWatched(&b));
    CHECK(!w.Take(&a, NULL));
    w.NoteOrder(&a, 1, false, false);
    CHECK(!w.Take(&a, NULL));
}

void FreshWatchDropsStalePendingOrder()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.NoteOrder(&a, 1, false, false);
    w.Unwatch(&a);
    w.Watch(&a);
    CHECK(!w.Take(&a, NULL));
}

// Pick pocket and a walking loot can both watch one actor: the actor stays
// watched until every owner has let go.
void SharedWatchSurvivesOneUnwatch()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.Watch(&a);
    w.Unwatch(&a);
    CHECK(w.IsWatched(&a));
    w.NoteOrder(&a, 4, false, false);
    CHECK(w.Take(&a, NULL));
    w.Unwatch(&a);
    CHECK(!w.IsWatched(&a));
}

void SecondWatcherKeepsPendingOrderOfTheFirst()
{
    OrderWatchSet w;
    w.Watch(&a);
    w.NoteOrder(&a, 1, false, false);
    w.Watch(&a);
    CHECK(w.Take(&a, NULL));
}

void ExtraUnwatchIsHarmless()
{
    OrderWatchSet w;
    w.Unwatch(&a);
    w.Watch(&a);
    w.Unwatch(&a);
    w.Unwatch(&a);
    CHECK(!w.IsWatched(&a));
    w.Watch(&a);
    CHECK(w.IsWatched(&a));
    w.Unwatch(&a);
    CHECK(!w.IsWatched(&a));
}

void NullActorIsIgnored()
{
    OrderWatchSet w;
    w.Watch(NULL);
    CHECK(!w.IsWatched(NULL));
    w.NoteOrder(NULL, 1, false, false);
    CHECK(!w.Take(NULL, NULL));
    w.Unwatch(NULL);
    w.Watch(&c);
    CHECK(w.IsWatched(&c));
}
} // namespace

void RunOrderWatchSetTests()
{
    UnwatchedActorRecordsNothing();
    WatchedActorReportsForeignOrderOnce();
    TakeWithoutOutputPointerStillConsumes();
    ActorsAreIndependent();
    LatestOrderWinsPerActor();
    OwnOrdersAreExcluded();
    OwnOrdersNest();
    UnbalancedEndOwnIsHarmless();
    UnwatchForgetsActorAndPending();
    FreshWatchDropsStalePendingOrder();
    SharedWatchSurvivesOneUnwatch();
    SecondWatcherKeepsPendingOrderOfTheFirst();
    ExtraUnwatchIsHarmless();
    NullActorIsIgnored();
}
