#pragma once

// Pure walking-loot decisions: the queue state machine that walks a looter to
// each body in turn and loots it on arrival. No Kenshi / MyGUI includes so it
// is unit-tested. Targets are opaque: the caller keeps its own list and this
// machine only tracks an index into it.

namespace totalloot
{
namespace lootwalk
{
// ---- Tuning -----------------------------------------------------------

const float kReissueIntervalSeconds = 2.5f;      // approach order refresh (as TotalPredator)
const float kTargetTimeoutSeconds = 60.0f;       // per body, as pickpocket's session
const float kNoProgressTimeoutSeconds = 15.0f;   // as pickpocket's session
const float kProgressEpsilon = 1.0f;             // distance gained that counts
const int kMaxConsecutiveEmptyBodies = 3;        // bodies in a row that yield nothing

// ---- Types ------------------------------------------------------------

enum Phase
{
    PHASE_IDLE = 0,     // not started, or cancelled
    PHASE_APPROACHING,  // walking to the current target
    PHASE_LOOTING,      // loot requested; the next tick carries its result
    PHASE_FINISHED      // queue exhausted
};

enum CancelReason
{
    CANCEL_NONE = 0,
    CANCEL_PLAYER_REORDER,
    CANCEL_COMBAT,
    CANCEL_ALLY_ATTACKED,
    CANCEL_INVALID_ACTOR,
    CANCEL_TIMEOUT,
    CANCEL_NO_PROGRESS,
    CANCEL_INVENTORY_FULL
};

enum Action
{
    ACT_NONE = 0,
    ACT_ISSUE_MOVE,     // (re)issue the move order to the current target
    ACT_LOOT_CURRENT,   // looter arrived: loot the current target now
    ACT_ADVANCE,        // moved on to the next target (state already updated)
    ACT_FINISH,         // nothing left to loot; the walk is over
    ACT_CANCEL          // the walk ended early (see reason)
};

struct WalkState
{
    Phase phase;
    int queueSize;              // number of targets in the caller's list
    int current;                // index of the current target
    float targetStartTime;      // when the approach to `current` began
    float lastProgressTime;
    float lastMoveTime;
    float bestDistance;         // < 0 until the first measurement
    bool moveIssued;            // a move order was issued for `current`
    int consecutiveEmpty;       // bodies in a row: nothing moved, items remain
    int totalMoved;             // items moved over the whole walk

    WalkState()
        : phase(PHASE_IDLE), queueSize(0), current(0), targetStartTime(0.0f),
          lastProgressTime(0.0f), lastMoveTime(0.0f), bestDistance(-1.0f),
          moveIssued(false), consecutiveEmpty(0), totalMoved(0) {}
};

struct TickInput
{
    float now;                      // monotonic seconds
    bool actorValid;
    bool actorInCombat;
    bool allyAttacked;              // a nearby ally is under attack
    // An order we did not issue reached the actor since the last tick.
    bool foreignOrder;

    // The current target (ignored once the queue is exhausted).
    bool targetValid;
    bool targetStillHasItems;
    float distance;                 // horizontal looter-to-target distance

    // Result of the loot the previous tick requested (read in PHASE_LOOTING).
    int lastTransferMoved;
    bool lastTransferInventoryFull; // target still had items after the transfer

    // Configuration.
    float arriveDistance;
    bool cancelOnCombat;
    bool cancelOnAllyAttacked;

    TickInput()
        : now(0.0f), actorValid(false), actorInCombat(false), allyAttacked(false),
          foreignOrder(false), targetValid(false), targetStillHasItems(false),
          distance(0.0f), lastTransferMoved(0), lastTransferInventoryFull(false),
          arriveDistance(5.0f), cancelOnCombat(true), cancelOnAllyAttacked(true) {}
};

struct TickResult
{
    Action action;
    CancelReason reason;
    int targetIndex;    // target the action refers to (the new one after ACT_ADVANCE)

    TickResult() : action(ACT_NONE), reason(CANCEL_NONE), targetIndex(0) {}
};

// ---- Machine ----------------------------------------------------------

// Starts a walk over `queueSize` targets. The first tick evaluates target 0.
void BeginWalk(WalkState* state, float now, int queueSize);

// Advances the machine one frame. The caller performs the returned action. On
// ACT_CANCEL the state is back to idle; on ACT_FINISH it is finished.
TickResult Step(WalkState* state, const TickInput& in);

// Stable machine-readable token for the log ("player_reorder", ...); "none" for
// CANCEL_NONE.
const char* ReasonName(CancelReason reason);

// Feedback line for the player; empty for CANCEL_NONE.
const char* CancelMessage(CancelReason reason);
} // namespace lootwalk
} // namespace totalloot
