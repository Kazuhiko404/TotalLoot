#include "test_harness.h"
#include "loot_walk_rules.h"

#include <cstring>

using namespace totalloot::lootwalk;

namespace
{
const char* const kSuiteName = "loot_walk_rules";

// A healthy looter and a valid target with items at `distance`.
TickInput At(float now, float distance)
{
    TickInput in;
    in.now = now;
    in.actorValid = true;
    in.targetValid = true;
    in.targetStillHasItems = true;
    in.distance = distance;
    return in;
}

// The tick right after ACT_LOOT_CURRENT, carrying the transfer result.
TickInput Looted(float now, int moved, bool inventoryFull)
{
    TickInput in = At(now, 0.0f);
    in.lastTransferMoved = moved;
    in.lastTransferInventoryFull = inventoryFull;
    return in;
}

WalkState Begin(int queueSize, float now = 0.0f)
{
    WalkState s;
    BeginWalk(&s, now, queueSize);
    return s;
}

// Walks target `index` from far away to loot: returns the LOOT_CURRENT result.
TickResult WalkAndLoot(WalkState* s, float* now, float startDistance)
{
    Step(s, At(*now, startDistance));          // ISSUE_MOVE
    *now += 3.0f;
    return Step(s, At(*now, 2.0f));            // arrived
}

void BeginSetsUpTheWalk()
{
    const WalkState s = Begin(3, 4.0f);
    CHECK(s.phase == PHASE_APPROACHING);
    CHECK(s.queueSize == 3);
    CHECK(s.current == 0);
    CHECK(s.targetStartTime == 4.0f);
    CHECK(s.consecutiveEmpty == 0);
    CHECK(s.totalMoved == 0);
}

void IdleStateDoesNothing()
{
    WalkState s;
    const TickResult r = Step(&s, At(0.0f, 50.0f));
    CHECK(r.action == ACT_NONE);
    CHECK(s.phase == PHASE_IDLE);
}

void EmptyQueueFinishesAtOnce()
{
    WalkState s = Begin(0);
    const TickResult r = Step(&s, At(0.0f, 50.0f));
    CHECK(r.action == ACT_FINISH);
    CHECK(s.phase == PHASE_FINISHED);
    CHECK(Step(&s, At(1.0f, 50.0f)).action == ACT_NONE);
}

void SingleTargetWalkAndLoot()
{
    WalkState s = Begin(1);
    TickResult r = Step(&s, At(0.0f, 30.0f));
    CHECK(r.action == ACT_ISSUE_MOVE);
    CHECK(r.targetIndex == 0);
    CHECK(Step(&s, At(0.5f, 28.0f)).action == ACT_NONE);
    r = Step(&s, At(3.0f, 4.0f));
    CHECK(r.action == ACT_LOOT_CURRENT);
    CHECK(r.targetIndex == 0);
    CHECK(s.phase == PHASE_LOOTING);
    r = Step(&s, Looted(3.1f, 5, false));
    CHECK(r.action == ACT_FINISH);
    CHECK(s.phase == PHASE_FINISHED);
    CHECK(s.totalMoved == 5);
}

void ArrivesAtOnceWhenAlreadyInRange()
{
    WalkState s = Begin(1);
    const TickResult r = Step(&s, At(0.0f, 3.0f));
    CHECK(r.action == ACT_LOOT_CURRENT);
    CHECK(!s.moveIssued);
}

void ArrivalIsInclusiveOfTheThreshold()
{
    WalkState s = Begin(1);
    CHECK(Step(&s, At(0.0f, 5.0f)).action == ACT_LOOT_CURRENT);
    WalkState t = Begin(1);
    CHECK(Step(&t, At(0.0f, 5.01f)).action == ACT_ISSUE_MOVE);
}

void ArriveDistanceComesFromConfig()
{
    WalkState s = Begin(1);
    TickInput in = At(0.0f, 8.0f);
    in.arriveDistance = 10.0f;
    CHECK(Step(&s, in).action == ACT_LOOT_CURRENT);
}

void MultiTargetOrderingAndAdvance()
{
    WalkState s = Begin(3);
    float now = 0.0f;
    TickResult r = WalkAndLoot(&s, &now, 20.0f);
    CHECK(r.action == ACT_LOOT_CURRENT);
    CHECK(r.targetIndex == 0);

    r = Step(&s, Looted(now + 0.1f, 2, false));
    CHECK(r.action == ACT_ADVANCE);
    CHECK(r.targetIndex == 1);
    CHECK(s.current == 1);
    CHECK(s.phase == PHASE_APPROACHING);
    CHECK(!s.moveIssued);

    now += 1.0f;
    r = Step(&s, At(now, 15.0f));
    CHECK(r.action == ACT_ISSUE_MOVE);
    CHECK(r.targetIndex == 1);
    now += 3.0f;
    r = Step(&s, At(now, 1.0f));
    CHECK(r.action == ACT_LOOT_CURRENT);
    CHECK(r.targetIndex == 1);

    r = Step(&s, Looted(now + 0.1f, 1, false));
    CHECK(r.action == ACT_ADVANCE);
    CHECK(r.targetIndex == 2);

    now += 1.0f;
    CHECK(Step(&s, At(now, 2.0f)).action == ACT_LOOT_CURRENT);
    r = Step(&s, Looted(now + 0.1f, 4, false));
    CHECK(r.action == ACT_FINISH);
    CHECK(s.totalMoved == 7);
}

void InvalidTargetIsSkippedNotCancelled()
{
    WalkState s = Begin(3);
    TickInput in = At(0.0f, 20.0f);
    in.targetValid = false;
    TickResult r = Step(&s, in);
    CHECK(r.action == ACT_ADVANCE);
    CHECK(r.reason == CANCEL_NONE);
    CHECK(r.targetIndex == 1);
    CHECK(s.phase == PHASE_APPROACHING);
    CHECK(Step(&s, At(0.1f, 20.0f)).action == ACT_ISSUE_MOVE);
}

void EmptiedTargetIsSkipped()
{
    WalkState s = Begin(2);
    TickInput in = At(0.0f, 2.0f);
    in.targetStillHasItems = false;
    const TickResult r = Step(&s, in);
    CHECK(r.action == ACT_ADVANCE);
    CHECK(r.targetIndex == 1);
}

void SkippingTheLastTargetFinishes()
{
    WalkState s = Begin(1);
    TickInput in = At(0.0f, 20.0f);
    in.targetValid = false;
    CHECK(Step(&s, in).action == ACT_FINISH);
    CHECK(s.phase == PHASE_FINISHED);
}

void SkippedTargetDoesNotBreakTheEmptyStreak()
{
    WalkState s = Begin(5);
    float now = 0.0f;
    WalkAndLoot(&s, &now, 20.0f);
    Step(&s, Looted(now, 0, true));              // streak 1
    TickInput in = At(now + 1.0f, 20.0f);
    in.targetValid = false;
    Step(&s, in);                                // skip
    CHECK(s.consecutiveEmpty == 1);
}

void ReissuesMoveOnTheInterval()
{
    WalkState s = Begin(1);
    CHECK(Step(&s, At(0.0f, 40.0f)).action == ACT_ISSUE_MOVE);
    CHECK(Step(&s, At(1.0f, 38.0f)).action == ACT_NONE);
    CHECK(Step(&s, At(2.4f, 36.0f)).action == ACT_NONE);
    CHECK(Step(&s, At(2.5f, 34.0f)).action == ACT_ISSUE_MOVE);
    CHECK(Step(&s, At(4.0f, 32.0f)).action == ACT_NONE);
    CHECK(Step(&s, At(5.0f, 30.0f)).action == ACT_ISSUE_MOVE);
}

void NextTargetIssuesItsMoveImmediately()
{
    WalkState s = Begin(2);
    float now = 0.0f;
    WalkAndLoot(&s, &now, 20.0f);
    Step(&s, Looted(now, 1, false));
    // Well inside the re-issue interval of the previous target's last order.
    CHECK(Step(&s, At(now + 0.1f, 20.0f)).action == ACT_ISSUE_MOVE);
}

void TimesOutAfterTheTargetTimeout()
{
    WalkState s = Begin(1);
    float d = 1000.0f;
    float now = 0.0f;
    Step(&s, At(now, d));
    // Keeps making progress, so only the per-target timeout can fire.
    while (now < kTargetTimeoutSeconds)
    {
        now += 1.0f;
        d -= 3.0f;
        const TickResult r = Step(&s, At(now, d));
        CHECK(r.action != ACT_CANCEL);
    }
    now += 1.0f;
    d -= 3.0f;
    const TickResult r = Step(&s, At(now, d));
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_TIMEOUT);
    CHECK(s.phase == PHASE_IDLE);
}

void TimeoutRestartsForEachTarget()
{
    WalkState s = Begin(2);
    float now = 0.0f;
    float d = 1000.0f;
    Step(&s, At(now, d));
    for (int i = 0; i < 50; ++i)
    {
        now += 1.0f;
        d -= 3.0f;
        Step(&s, At(now, d));
    }
    Step(&s, At(now, 2.0f));                     // arrive
    Step(&s, Looted(now, 1, false));             // advance to target 1
    now += 50.0f;                                // 100 s after the walk began
    d = 1000.0f;
    CHECK(Step(&s, At(now, d)).action == ACT_ISSUE_MOVE);
    now += 1.0f;
    d -= 3.0f;
    CHECK(Step(&s, At(now, d)).action == ACT_NONE);
}

void NoProgressCancels()
{
    WalkState s = Begin(1);
    Step(&s, At(0.0f, 30.0f));
    TickResult r;
    float now = 0.0f;
    while (now < kNoProgressTimeoutSeconds)
    {
        now += 1.0f;
        r = Step(&s, At(now, 30.0f));
        CHECK(r.action != ACT_CANCEL);
    }
    r = Step(&s, At(now + 1.0f, 30.0f));
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_NO_PROGRESS);
    CHECK(s.phase == PHASE_IDLE);
}

void ProgressBelowEpsilonDoesNotCount()
{
    WalkState s = Begin(1);
    Step(&s, At(0.0f, 30.0f));
    float now = 0.0f;
    float d = 30.0f;
    TickResult r;
    for (int i = 0; i < 20; ++i)
    {
        now += 1.0f;
        d -= 0.01f;                              // creeping, under the epsilon
        r = Step(&s, At(now, d));
        if (r.action == ACT_CANCEL) break;
    }
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_NO_PROGRESS);
}

void RealProgressResetsTheNoProgressClock()
{
    WalkState s = Begin(1);
    Step(&s, At(0.0f, 100.0f));
    float now = 0.0f;
    float d = 100.0f;
    for (int i = 0; i < 40; ++i)
    {
        now += 1.0f;
        d -= 2.0f;                               // steady real progress
        CHECK(Step(&s, At(now, d)).action != ACT_CANCEL);
    }
}

void PlayerOrderCancelsInEveryPhase()
{
    WalkState s = Begin(2);
    TickInput in = At(0.0f, 20.0f);
    in.foreignOrder = true;
    TickResult r = Step(&s, in);
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_PLAYER_REORDER);
    CHECK(s.phase == PHASE_IDLE);

    WalkState t = Begin(2);
    Step(&t, At(0.0f, 2.0f));                    // LOOT_CURRENT, now PHASE_LOOTING
    in = Looted(1.0f, 3, false);
    in.foreignOrder = true;
    r = Step(&t, in);
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_PLAYER_REORDER);
}

void CombatCancelFollowsTheConfigToggle()
{
    WalkState s = Begin(1);
    TickInput in = At(0.0f, 20.0f);
    in.actorInCombat = true;
    in.cancelOnCombat = true;
    TickResult r = Step(&s, in);
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_COMBAT);

    WalkState t = Begin(1);
    in.cancelOnCombat = false;
    CHECK(Step(&t, in).action == ACT_ISSUE_MOVE);
}

void AllyAttackedCancelFollowsTheConfigToggle()
{
    WalkState s = Begin(1);
    TickInput in = At(0.0f, 20.0f);
    in.allyAttacked = true;
    in.cancelOnAllyAttacked = true;
    TickResult r = Step(&s, in);
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_ALLY_ATTACKED);

    WalkState t = Begin(1);
    in.cancelOnAllyAttacked = false;
    CHECK(Step(&t, in).action == ACT_ISSUE_MOVE);
}

void InvalidActorCancels()
{
    WalkState s = Begin(1);
    TickInput in = At(0.0f, 20.0f);
    in.actorValid = false;
    const TickResult r = Step(&s, in);
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_INVALID_ACTOR);
}

void CancelPriorityIsStable()
{
    TickInput in = At(0.0f, 20.0f);
    in.actorValid = false;
    in.foreignOrder = true;
    in.actorInCombat = true;
    in.allyAttacked = true;
    WalkState s = Begin(1);
    CHECK(Step(&s, in).reason == CANCEL_INVALID_ACTOR);

    in.actorValid = true;
    s = Begin(1);
    CHECK(Step(&s, in).reason == CANCEL_PLAYER_REORDER);

    in.foreignOrder = false;
    s = Begin(1);
    CHECK(Step(&s, in).reason == CANCEL_COMBAT);

    in.actorInCombat = false;
    s = Begin(1);
    CHECK(Step(&s, in).reason == CANCEL_ALLY_ATTACKED);
}

void CancelBeatsFinishOnEmptyQueue()
{
    WalkState s = Begin(0);
    TickInput in = At(0.0f, 0.0f);
    in.foreignOrder = true;
    CHECK(Step(&s, in).action == ACT_CANCEL);
}

void InventoryFullAfterThreeEmptyBodies()
{
    WalkState s = Begin(5);
    float now = 0.0f;
    TickResult r;
    for (int i = 0; i < 2; ++i)
    {
        r = WalkAndLoot(&s, &now, 20.0f);
        CHECK(r.action == ACT_LOOT_CURRENT);
        r = Step(&s, Looted(now, 0, true));
        CHECK(r.action == ACT_ADVANCE);
        now += 1.0f;
    }
    CHECK(s.consecutiveEmpty == 2);
    r = WalkAndLoot(&s, &now, 20.0f);
    CHECK(r.action == ACT_LOOT_CURRENT);
    r = Step(&s, Looted(now, 0, true));
    CHECK(r.action == ACT_CANCEL);
    CHECK(r.reason == CANCEL_INVENTORY_FULL);
    CHECK(s.phase == PHASE_IDLE);
}

void SuccessResetsTheEmptyStreak()
{
    WalkState s = Begin(6);
    float now = 0.0f;
    WalkAndLoot(&s, &now, 20.0f);
    Step(&s, Looted(now, 0, true));
    now += 1.0f;
    WalkAndLoot(&s, &now, 20.0f);
    Step(&s, Looted(now, 0, true));
    CHECK(s.consecutiveEmpty == 2);
    now += 1.0f;
    WalkAndLoot(&s, &now, 20.0f);
    TickResult r = Step(&s, Looted(now, 2, true));  // took something: not "full"
    CHECK(r.action == ACT_ADVANCE);
    CHECK(s.consecutiveEmpty == 0);
}

void EmptyResultWithNothingLeftIsNotInventoryFull()
{
    WalkState s = Begin(4);
    float now = 0.0f;
    WalkAndLoot(&s, &now, 20.0f);
    Step(&s, Looted(now, 0, true));
    CHECK(s.consecutiveEmpty == 1);
    now += 1.0f;
    WalkAndLoot(&s, &now, 20.0f);
    const TickResult r = Step(&s, Looted(now, 0, false)); // someone emptied it
    CHECK(r.action == ACT_ADVANCE);
    CHECK(s.consecutiveEmpty == 0);
}

void SingleFullTargetJustFinishes()
{
    // One full body is not enough to call the looter full.
    WalkState s = Begin(1);
    float now = 0.0f;
    WalkAndLoot(&s, &now, 20.0f);
    const TickResult r = Step(&s, Looted(now, 0, true));
    CHECK(r.action == ACT_FINISH);
}

void StepAfterTheWalkEndsDoesNothing()
{
    WalkState s = Begin(1);
    TickInput in = At(0.0f, 20.0f);
    in.foreignOrder = true;
    Step(&s, in);
    CHECK(Step(&s, At(1.0f, 20.0f)).action == ACT_NONE);

    WalkState f = Begin(0);
    Step(&f, At(0.0f, 0.0f));
    CHECK(Step(&f, At(1.0f, 0.0f)).action == ACT_NONE);
}

void NamesAndMessagesAreDistinct()
{
    const CancelReason all[] =
    {
        CANCEL_PLAYER_REORDER, CANCEL_COMBAT, CANCEL_ALLY_ATTACKED,
        CANCEL_INVALID_ACTOR, CANCEL_TIMEOUT, CANCEL_NO_PROGRESS, CANCEL_INVENTORY_FULL
    };
    const int count = sizeof(all) / sizeof(all[0]);
    CHECK(std::strcmp(ReasonName(CANCEL_NONE), "none") == 0);
    CHECK(CancelMessage(CANCEL_NONE)[0] == '\0');
    for (int i = 0; i < count; ++i)
    {
        CHECK(ReasonName(all[i])[0] != '\0');
        CHECK(CancelMessage(all[i])[0] != '\0');
        for (int j = i + 1; j < count; ++j)
        {
            CHECK(std::strcmp(ReasonName(all[i]), ReasonName(all[j])) != 0);
            CHECK(std::strcmp(CancelMessage(all[i]), CancelMessage(all[j])) != 0);
        }
    }
}
} // namespace

void RunLootWalkRulesTests()
{
    BeginSetsUpTheWalk();
    IdleStateDoesNothing();
    EmptyQueueFinishesAtOnce();
    SingleTargetWalkAndLoot();
    ArrivesAtOnceWhenAlreadyInRange();
    ArrivalIsInclusiveOfTheThreshold();
    ArriveDistanceComesFromConfig();
    MultiTargetOrderingAndAdvance();
    InvalidTargetIsSkippedNotCancelled();
    EmptiedTargetIsSkipped();
    SkippingTheLastTargetFinishes();
    SkippedTargetDoesNotBreakTheEmptyStreak();
    ReissuesMoveOnTheInterval();
    NextTargetIssuesItsMoveImmediately();
    TimesOutAfterTheTargetTimeout();
    TimeoutRestartsForEachTarget();
    NoProgressCancels();
    ProgressBelowEpsilonDoesNotCount();
    RealProgressResetsTheNoProgressClock();
    PlayerOrderCancelsInEveryPhase();
    CombatCancelFollowsTheConfigToggle();
    AllyAttackedCancelFollowsTheConfigToggle();
    InvalidActorCancels();
    CancelPriorityIsStable();
    CancelBeatsFinishOnEmptyQueue();
    InventoryFullAfterThreeEmptyBodies();
    SuccessResetsTheEmptyStreak();
    EmptyResultWithNothingLeftIsNotInventoryFull();
    SingleFullTargetJustFinishes();
    StepAfterTheWalkEndsDoesNothing();
    NamesAndMessagesAreDistinct();
}
