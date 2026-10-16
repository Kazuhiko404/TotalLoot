#include "loot_walk_rules.h"

namespace totalloot
{
namespace lootwalk
{
namespace
{
TickResult Result(Action action, CancelReason reason, int targetIndex)
{
    TickResult r;
    r.action = action;
    r.reason = reason;
    r.targetIndex = targetIndex;
    return r;
}

TickResult Cancel(WalkState* state, CancelReason reason)
{
    state->phase = PHASE_IDLE;
    return Result(ACT_CANCEL, reason, state->current);
}

// Conditions that end a walk in any active phase, in priority order.
bool CommonCancel(const TickInput& in, CancelReason* reason)
{
    if (!in.actorValid)
    {
        *reason = CANCEL_INVALID_ACTOR;
        return true;
    }
    // Someone else gave the character an order: never fight it.
    if (in.foreignOrder)
    {
        *reason = CANCEL_PLAYER_REORDER;
        return true;
    }
    if (in.cancelOnCombat && in.actorInCombat)
    {
        *reason = CANCEL_COMBAT;
        return true;
    }
    if (in.cancelOnAllyAttacked && in.allyAttacked)
    {
        *reason = CANCEL_ALLY_ATTACKED;
        return true;
    }
    return false;
}

// Moves to the next target (restarting the per-target clocks), or finishes
// when that was the last one.
TickResult AdvanceOrFinish(WalkState* state, float now)
{
    ++state->current;
    if (state->current >= state->queueSize)
    {
        state->phase = PHASE_FINISHED;
        return Result(ACT_FINISH, CANCEL_NONE, state->current);
    }
    state->phase = PHASE_APPROACHING;
    state->targetStartTime = now;
    state->lastProgressTime = now;
    state->lastMoveTime = now;
    state->bestDistance = -1.0f;
    state->moveIssued = false;
    return Result(ACT_ADVANCE, CANCEL_NONE, state->current);
}

TickResult StepApproaching(WalkState* state, const TickInput& in)
{
    if (state->current >= state->queueSize)
    {
        state->phase = PHASE_FINISHED;
        return Result(ACT_FINISH, CANCEL_NONE, state->current);
    }

    // Gone or already emptied (by the player, another looter, decay): skip it.
    if (!in.targetValid || !in.targetStillHasItems)
    {
        return AdvanceOrFinish(state, in.now);
    }

    if (in.now - state->targetStartTime > kTargetTimeoutSeconds)
    {
        return Cancel(state, CANCEL_TIMEOUT);
    }

    if (in.distance <= in.arriveDistance)
    {
        state->phase = PHASE_LOOTING;
        return Result(ACT_LOOT_CURRENT, CANCEL_NONE, state->current);
    }

    if (state->bestDistance < 0.0f || in.distance < state->bestDistance - kProgressEpsilon)
    {
        state->bestDistance = in.distance;
        state->lastProgressTime = in.now;
    }
    else if (in.now - state->lastProgressTime > kNoProgressTimeoutSeconds)
    {
        return Cancel(state, CANCEL_NO_PROGRESS);
    }

    const bool first = !state->moveIssued;
    const bool refresh = in.now - state->lastMoveTime >= kReissueIntervalSeconds;
    if (first || refresh)
    {
        state->moveIssued = true;
        state->lastMoveTime = in.now;
        return Result(ACT_ISSUE_MOVE, CANCEL_NONE, state->current);
    }
    return Result(ACT_NONE, CANCEL_NONE, state->current);
}

// The tick after ACT_LOOT_CURRENT: fold in what the transfer achieved.
TickResult StepLooting(WalkState* state, const TickInput& in)
{
    state->totalMoved += in.lastTransferMoved;

    if (in.lastTransferMoved == 0 && in.lastTransferInventoryFull)
    {
        ++state->consecutiveEmpty;
        if (state->consecutiveEmpty >= kMaxConsecutiveEmptyBodies)
        {
            return Cancel(state, CANCEL_INVENTORY_FULL);
        }
    }
    else
    {
        state->consecutiveEmpty = 0;
    }
    return AdvanceOrFinish(state, in.now);
}
} // namespace

void BeginWalk(WalkState* state, float now, int queueSize)
{
    *state = WalkState();
    state->phase = PHASE_APPROACHING;
    state->queueSize = queueSize < 0 ? 0 : queueSize;
    state->targetStartTime = now;
    state->lastProgressTime = now;
    state->lastMoveTime = now;
}

TickResult Step(WalkState* state, const TickInput& in)
{
    if (state->phase != PHASE_APPROACHING && state->phase != PHASE_LOOTING)
    {
        return Result(ACT_NONE, CANCEL_NONE, state->current);
    }

    CancelReason reason = CANCEL_NONE;
    if (CommonCancel(in, &reason))
    {
        return Cancel(state, reason);
    }

    if (state->phase == PHASE_LOOTING)
    {
        return StepLooting(state, in);
    }
    return StepApproaching(state, in);
}

const char* ReasonName(CancelReason reason)
{
    switch (reason)
    {
    case CANCEL_NONE:           return "none";
    case CANCEL_PLAYER_REORDER: return "player_reorder";
    case CANCEL_COMBAT:         return "combat";
    case CANCEL_ALLY_ATTACKED:  return "ally_attacked";
    case CANCEL_INVALID_ACTOR:  return "invalid_actor";
    case CANCEL_TIMEOUT:        return "timeout";
    case CANCEL_NO_PROGRESS:    return "no_progress";
    case CANCEL_INVENTORY_FULL: return "inventory_full";
    }
    return "unknown";
}

const char* CancelMessage(CancelReason reason)
{
    switch (reason)
    {
    case CANCEL_NONE:           return "";
    case CANCEL_PLAYER_REORDER: return "Looting cancelled: new order given.";
    case CANCEL_COMBAT:         return "Looting cancelled: combat started.";
    case CANCEL_ALLY_ATTACKED:  return "Looting cancelled: an ally is under attack.";
    case CANCEL_INVALID_ACTOR:  return "Looting cancelled: looter lost.";
    case CANCEL_TIMEOUT:        return "Looting cancelled: took too long to reach the body.";
    case CANCEL_NO_PROGRESS:    return "Looting cancelled: cannot reach the body.";
    case CANCEL_INVENTORY_FULL: return "Looting stopped: inventory is full.";
    }
    return "";
}
} // namespace lootwalk
} // namespace totalloot
