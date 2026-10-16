#include "test_harness.h"
#include "pickpocket_rules.h"

#include <cstring>

using namespace totalloot::pickpocket;

namespace
{
const char* const kSuiteName = "pickpocket_rules";

EligibilityInput Good()
{
    EligibilityInput in;
    in.actorIsPlayer = true;
    in.actorInStealth = true;
    in.actorCanTakeOrders = true;
    in.targetValid = true;
    in.targetProneNormal = true;
    return in;
}

void EligibleHappyPath()
{
    CHECK(Evaluate(Good()) == ELIGIBLE);
}

void EligibilityRejections()
{
    EligibilityInput in = Good(); in.actorIsPlayer = false;
    CHECK(Evaluate(in) == NOT_PLAYER_ACTOR);
    in = Good(); in.actorInStealth = false;
    CHECK(Evaluate(in) == NOT_IN_STEALTH);
    in = Good(); in.actorCanTakeOrders = false;
    CHECK(Evaluate(in) == CANNOT_TAKE_ORDERS);
    in = Good(); in.actorInCombat = true;
    CHECK(Evaluate(in) == IN_COMBAT);
    in = Good(); in.inDialogueOrModal = true;
    CHECK(Evaluate(in) == IN_DIALOGUE_OR_MODAL);
    in = Good(); in.targetValid = false;
    CHECK(Evaluate(in) == TARGET_INVALID);
    in = Good(); in.targetIsPlayer = true;
    CHECK(Evaluate(in) == TARGET_IS_PLAYER);
    in = Good(); in.targetIsAnimal = true;
    CHECK(Evaluate(in) == TARGET_IS_ANIMAL);
    in = Good(); in.targetDead = true;
    CHECK(Evaluate(in) == TARGET_DEAD);
    in = Good(); in.targetUnconscious = true;
    CHECK(Evaluate(in) == TARGET_UNCONSCIOUS);
    in = Good(); in.targetDown = true;
    CHECK(Evaluate(in) == TARGET_DOWN);
    in = Good(); in.targetCarried = true;
    CHECK(Evaluate(in) == TARGET_CARRIED);
    in = Good(); in.targetProneNormal = false;
    CHECK(Evaluate(in) == TARGET_NOT_STANDING);
}

void EligibilityOrderAndNames()
{
    EligibilityInput in = Good();
    in.actorInStealth = false;
    in.targetDead = true;
    CHECK(Evaluate(in) == NOT_IN_STEALTH);      // actor checks come first
    CHECK(std::strcmp(EligibilityName(ELIGIBLE), "eligible") == 0);
    CHECK(std::strcmp(EligibilityName(NOT_IN_STEALTH), "not in stealth") == 0);
    CHECK(std::strlen(EligibilityName(TARGET_NOT_STANDING)) > 0);
}

RangeInput Range(float dx, float dz)
{
    RangeInput r;
    r.dx = dx;
    r.dz = dz;
    r.sameFloor = true;
    r.sameBuilding = true;
    return r;
}

void RangeRules()
{
    CHECK(HorizontalDistance(3.0f, 4.0f) > 4.99f && HorizontalDistance(3.0f, 4.0f) < 5.01f);
    CHECK(CanOpenWindow(Range(59.0f, 0.0f)));
    CHECK(!CanOpenWindow(Range(60.0f, 0.0f)));          // strict: 3600 is not < 3600
    CHECK(!CanOpenWindow(Range(43.0f, 43.0f)));         // 60.8 diagonal
    CHECK(CanOpenWindow(Range(40.0f, 40.0f)));          // 56.6 diagonal
    CHECK(InApproachStopRange(Range(39.0f, 0.0f)));
    CHECK(!InApproachStopRange(Range(40.0f, 0.0f)));
    CHECK(!InApproachStopRange(Range(30.0f, 30.0f)));   // 42.4 > 40
    // The stop distance sits strictly inside the engine limit.
    CHECK(kApproachStopDistance * kApproachStopDistance < kEngineTradeRangeSq);

    RangeInput r = Range(1.0f, 1.0f);
    r.sameFloor = false;
    CHECK(!CanOpenWindow(r));
    CHECK(!InApproachStopRange(r));
    r = Range(1.0f, 1.0f);
    r.sameBuilding = false;
    CHECK(!CanOpenWindow(r));
    CHECK(!InApproachStopRange(r));
}

TickInput Tick(float now, float dist)
{
    TickInput t;
    t.now = now;
    t.actorValid = true;
    t.targetValid = true;
    t.stealth = true;
    t.range = Range(dist, 0.0f);
    return t;
}

void FirstTickIssuesMove()
{
    SessionState s;
    BeginSession(&s, 100.0f);
    CHECK(s.phase == PHASE_APPROACHING);
    TickResult r = Step(&s, Tick(100.0f, 300.0f));
    CHECK(r.action == ACT_ISSUE_MOVE);
    CHECK(s.orderIssued);
    r = Step(&s, Tick(100.1f, 295.0f));
    CHECK(r.action == ACT_NONE);
}

void ReissueOnlyWhenTargetMovedAndIntervalElapsed()
{
    SessionState s;
    BeginSession(&s, 0.0f);
    Step(&s, Tick(0.0f, 300.0f));                 // order issued at t=0

    TickInput t = Tick(0.5f, 290.0f);
    t.targetMoved = 50.0f;
    CHECK(Step(&s, t).action == ACT_NONE);        // < 1 s since last order

    t = Tick(1.2f, 280.0f);
    t.targetMoved = 5.0f;
    CHECK(Step(&s, t).action == ACT_NONE);        // target barely moved

    t = Tick(1.3f, 275.0f);
    t.targetMoved = 11.0f;
    CHECK(Step(&s, t).action == ACT_ISSUE_MOVE);  // moved > 10 and > 1 s

    t = Tick(1.8f, 270.0f);
    t.targetMoved = 40.0f;
    CHECK(Step(&s, t).action == ACT_NONE);        // rate limited again
}

void ArrivalOpensWindow()
{
    SessionState s;
    BeginSession(&s, 0.0f);
    Step(&s, Tick(0.0f, 300.0f));
    TickResult r = Step(&s, Tick(5.0f, 39.0f));
    CHECK(r.action == ACT_OPEN_WINDOW);
    CHECK(s.phase == PHASE_OPEN);
}

void OpensImmediatelyWhenAlreadyInRange()
{
    SessionState s;
    BeginSession(&s, 0.0f);
    TickResult r = Step(&s, Tick(0.0f, 10.0f));
    CHECK(r.action == ACT_OPEN_WINDOW);
}

void CancelRules()
{
    SessionState s;
    TickInput t;

    BeginSession(&s, 0.0f);
    t = Tick(1.0f, 300.0f); t.stealth = false;
    TickResult r = Step(&s, t);
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_STEALTH_LOST);
    CHECK(s.phase == PHASE_IDLE);

    BeginSession(&s, 0.0f);
    t = Tick(1.0f, 300.0f); t.combat = true;
    r = Step(&s, t);
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_COMBAT);

    BeginSession(&s, 0.0f);
    t = Tick(1.0f, 300.0f); t.targetValid = false;
    r = Step(&s, t);
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_INVALID);

    BeginSession(&s, 0.0f);
    t = Tick(1.0f, 300.0f); t.actorValid = false;
    r = Step(&s, t);
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_INVALID);
}

void ExternalOrderCancelsApproach()
{
    SessionState s;
    BeginSession(&s, 0.0f);
    Step(&s, Tick(0.0f, 300.0f));                 // our order issued

    TickInput t = Tick(0.3f, 298.0f);
    t.externalOrder = true;                       // hook saw a foreign addOrder
    TickResult r = Step(&s, t);
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_PLAYER_REORDER);
    CHECK(s.phase == PHASE_IDLE);
}

void ExternalOrderBeforeFirstOrderStillCancels()
{
    SessionState s;
    BeginSession(&s, 0.0f);
    TickInput t = Tick(0.0f, 300.0f);
    t.externalOrder = true;
    TickResult r = Step(&s, t);
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_PLAYER_REORDER);
}

void OurOwnOrderDisappearingNeverCancels()
{
    // Regression (RE_Kenshi_log 2256.133): the engine consumes our move order
    // into an AI task, so "order still queued" goes false while the actor is
    // walking. Only a reported foreign order may cancel, never the absence of
    // ours: a long uneventful approach keeps going.
    SessionState s;
    BeginSession(&s, 0.0f);
    float dist = 300.0f;
    for (float now = 0.0f; now < 10.0f; now += 0.25f)
    {
        TickResult r = Step(&s, Tick(now, dist));
        CHECK(r.action == ACT_NONE || r.action == ACT_ISSUE_MOVE);
        dist -= 5.0f;
    }
    CHECK(s.phase == PHASE_APPROACHING);
}

void ExternalOrderIgnoredOnceWindowIsOpen()
{
    // Range / stealth / window state own the open phase.
    SessionState s;
    BeginSession(&s, 0.0f);
    Step(&s, Tick(0.0f, 10.0f));                  // in range: window opens
    TickInput t = Tick(0.5f, 10.0f);
    t.externalOrder = true;
    t.windowVisible = true;
    CHECK(Step(&s, t).action == ACT_NONE);
    CHECK(s.phase == PHASE_OPEN);
}

void TimeoutAndNoProgress()
{
    SessionState s;
    BeginSession(&s, 0.0f);
    Step(&s, Tick(0.0f, 300.0f));
    // Distance keeps shrinking, so only the 60 s cap applies.
    float dist = 300.0f;
    TickResult r;
    for (float now = 1.0f; now <= 59.0f; now += 1.0f)
    {
        dist -= 2.0f;
        r = Step(&s, Tick(now, dist));
        if (r.action != ACT_NONE) break;
    }
    CHECK(r.action == ACT_NONE);
    r = Step(&s, Tick(60.5f, dist - 2.0f));
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_TIMEOUT);

    BeginSession(&s, 0.0f);
    Step(&s, Tick(0.0f, 300.0f));
    CHECK(Step(&s, Tick(14.0f, 300.0f)).action == ACT_NONE);
    r = Step(&s, Tick(15.5f, 300.0f));
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_NO_PROGRESS);

    // Progress resets the no-progress clock.
    BeginSession(&s, 0.0f);
    Step(&s, Tick(0.0f, 300.0f));
    Step(&s, Tick(10.0f, 250.0f));
    CHECK(Step(&s, Tick(20.0f, 250.0f)).action == ACT_NONE);
    r = Step(&s, Tick(25.5f, 250.0f));
    CHECK(r.action == ACT_CANCEL && r.reason == CANCEL_NO_PROGRESS);
}

SessionState OpenSession()
{
    SessionState s;
    BeginSession(&s, 0.0f);
    Step(&s, Tick(0.0f, 300.0f));
    Step(&s, Tick(5.0f, 30.0f));    // opens at t=5
    return s;
}

void OpenPhaseClosesOnLoss()
{
    SessionState s = OpenSession();
    TickInput t = Tick(5.5f, 30.0f);
    t.windowVisible = true;
    CHECK(Step(&s, t).action == ACT_NONE);

    t.stealth = false;
    TickResult r = Step(&s, t);
    CHECK(r.action == ACT_CLOSE_WINDOW && r.reason == CANCEL_STEALTH_LOST);
    CHECK(s.phase == PHASE_IDLE);

    s = OpenSession();
    t = Tick(5.5f, 70.0f);          // beyond 60 units
    t.windowVisible = true;
    r = Step(&s, t);
    CHECK(r.action == ACT_CLOSE_WINDOW && r.reason == CANCEL_OUT_OF_RANGE);

    s = OpenSession();
    t = Tick(5.5f, 30.0f); t.windowVisible = true; t.targetValid = false;
    r = Step(&s, t);
    CHECK(r.action == ACT_CLOSE_WINDOW && r.reason == CANCEL_INVALID);

    s = OpenSession();
    t = Tick(5.5f, 30.0f); t.windowVisible = true; t.combat = true;
    r = Step(&s, t);
    CHECK(r.action == ACT_CLOSE_WINDOW && r.reason == CANCEL_COMBAT);
}

void OpenPhaseKeepsWindowBetween40And60()
{
    SessionState s = OpenSession();
    TickInput t = Tick(5.5f, 55.0f);
    t.windowVisible = true;
    CHECK(Step(&s, t).action == ACT_NONE);
}

void OpenPhaseFinishesWhenWindowGone()
{
    SessionState s = OpenSession();
    TickInput t = Tick(5.4f, 30.0f);
    t.windowVisible = false;                       // not shown yet: grace
    CHECK(Step(&s, t).action == ACT_NONE);

    t = Tick(6.2f, 30.0f);
    t.windowVisible = true;
    CHECK(Step(&s, t).action == ACT_NONE);

    t = Tick(9.0f, 30.0f);
    t.windowVisible = false;                       // player closed it
    TickResult r = Step(&s, t);
    CHECK(r.action == ACT_FINISH && r.reason == CANCEL_NONE);
    CHECK(s.phase == PHASE_IDLE);
}

void IdleStepDoesNothing()
{
    SessionState s;
    CHECK(Step(&s, Tick(1.0f, 10.0f)).action == ACT_NONE);
    CHECK(s.phase == PHASE_IDLE);
}

void ReasonNamesAreDistinctTokens()
{
    const CancelReason all[] = { CANCEL_NONE, CANCEL_STEALTH_LOST, CANCEL_PLAYER_REORDER,
                                 CANCEL_INVALID, CANCEL_TIMEOUT, CANCEL_NO_PROGRESS,
                                 CANCEL_COMBAT, CANCEL_OUT_OF_RANGE };
    const int count = sizeof(all) / sizeof(all[0]);
    for (int i = 0; i < count; ++i)
    {
        CHECK(std::strlen(ReasonName(all[i])) > 0);
        for (int j = i + 1; j < count; ++j)
        {
            CHECK(std::strcmp(ReasonName(all[i]), ReasonName(all[j])) != 0);
        }
    }
    CHECK(std::strcmp(ReasonName(CANCEL_PLAYER_REORDER), "external_order") == 0);
}

void CancelMessages()
{
    CHECK(std::strlen(CancelMessage(CANCEL_NONE)) == 0);
    CHECK(std::strlen(CancelMessage(CANCEL_STEALTH_LOST)) > 0);
    CHECK(std::strlen(CancelMessage(CANCEL_PLAYER_REORDER)) > 0);
    CHECK(std::strlen(CancelMessage(CANCEL_INVALID)) > 0);
    CHECK(std::strlen(CancelMessage(CANCEL_TIMEOUT)) > 0);
    CHECK(std::strlen(CancelMessage(CANCEL_NO_PROGRESS)) > 0);
    CHECK(std::strlen(CancelMessage(CANCEL_COMBAT)) > 0);
    CHECK(std::strlen(CancelMessage(CANCEL_OUT_OF_RANGE)) > 0);
}
} // namespace

void RunPickpocketRulesTests()
{
    EligibleHappyPath();
    EligibilityRejections();
    EligibilityOrderAndNames();
    RangeRules();
    FirstTickIssuesMove();
    ReissueOnlyWhenTargetMovedAndIntervalElapsed();
    ArrivalOpensWindow();
    OpensImmediatelyWhenAlreadyInRange();
    CancelRules();
    ExternalOrderCancelsApproach();
    ExternalOrderBeforeFirstOrderStillCancels();
    OurOwnOrderDisappearingNeverCancels();
    ExternalOrderIgnoredOnceWindowIsOpen();
    TimeoutAndNoProgress();
    OpenPhaseClosesOnLoss();
    OpenPhaseKeepsWindowBetween40And60();
    OpenPhaseFinishesWhenWindowGone();
    IdleStepDoesNothing();
    CancelMessages();
    ReasonNamesAreDistinctTokens();
}
