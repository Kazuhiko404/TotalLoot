#include "pickpocket_rules.h"

#include <cmath>

namespace totalloot
{
namespace pickpocket
{
namespace
{
float SquaredHorizontal(const RangeInput& in)
{
    return in.dx * in.dx + in.dz * in.dz;
}

TickResult Result(Action action, CancelReason reason)
{
    TickResult r;
    r.action = action;
    r.reason = reason;
    return r;
}

// Conditions that end a session in either phase, in priority order.
bool CommonCancel(const TickInput& in, CancelReason* reason)
{
    if (!in.actorValid || !in.targetValid)
    {
        *reason = CANCEL_INVALID;
        return true;
    }
    if (!in.stealth)
    {
        *reason = CANCEL_STEALTH_LOST;
        return true;
    }
    if (in.combat)
    {
        *reason = CANCEL_COMBAT;
        return true;
    }
    return false;
}

TickResult StepApproaching(SessionState* state, const TickInput& in)
{
    CancelReason reason = CANCEL_NONE;
    if (CommonCancel(in, &reason))
    {
        state->phase = PHASE_IDLE;
        return Result(ACT_CANCEL, reason);
    }

    // Someone else gave the character an order: never fight it.
    if (in.externalOrder)
    {
        state->phase = PHASE_IDLE;
        return Result(ACT_CANCEL, CANCEL_PLAYER_REORDER);
    }

    if (in.now - state->startTime > kSessionTimeoutSeconds)
    {
        state->phase = PHASE_IDLE;
        return Result(ACT_CANCEL, CANCEL_TIMEOUT);
    }

    if (InApproachStopRange(in.range))
    {
        state->phase = PHASE_OPEN;
        state->openedTime = in.now;
        return Result(ACT_OPEN_WINDOW, CANCEL_NONE);
    }

    const float distance = HorizontalDistance(in.range.dx, in.range.dz);
    if (state->bestDistance < 0.0f || distance < state->bestDistance - kProgressEpsilon)
    {
        state->bestDistance = distance;
        state->lastProgressTime = in.now;
    }
    else if (in.now - state->lastProgressTime > kNoProgressTimeoutSeconds)
    {
        state->phase = PHASE_IDLE;
        return Result(ACT_CANCEL, CANCEL_NO_PROGRESS);
    }

    const bool firstOrder = !state->orderIssued;
    const bool retarget = in.targetMoved > kTargetMovedThreshold &&
                          in.now - state->lastOrderTime >= kReissueIntervalSeconds;
    if (firstOrder || retarget)
    {
        state->orderIssued = true;
        state->lastOrderTime = in.now;
        return Result(ACT_ISSUE_MOVE, CANCEL_NONE);
    }
    return Result(ACT_NONE, CANCEL_NONE);
}

TickResult StepOpen(SessionState* state, const TickInput& in)
{
    CancelReason reason = CANCEL_NONE;
    if (CommonCancel(in, &reason))
    {
        state->phase = PHASE_IDLE;
        return Result(ACT_CLOSE_WINDOW, reason);
    }
    if (!CanOpenWindow(in.range))
    {
        state->phase = PHASE_IDLE;
        return Result(ACT_CLOSE_WINDOW, CANCEL_OUT_OF_RANGE);
    }
    if (!in.windowVisible && in.now - state->openedTime >= kWindowOpenGraceSeconds)
    {
        state->phase = PHASE_IDLE;
        return Result(ACT_FINISH, CANCEL_NONE);
    }
    return Result(ACT_NONE, CANCEL_NONE);
}
} // namespace

Eligibility Evaluate(const EligibilityInput& in)
{
    if (!in.actorIsPlayer) return NOT_PLAYER_ACTOR;
    if (!in.actorInStealth) return NOT_IN_STEALTH;
    if (!in.actorCanTakeOrders) return CANNOT_TAKE_ORDERS;
    if (in.actorInCombat) return IN_COMBAT;
    if (in.inDialogueOrModal) return IN_DIALOGUE_OR_MODAL;
    if (!in.targetValid) return TARGET_INVALID;
    if (in.targetIsPlayer) return TARGET_IS_PLAYER;
    if (in.targetIsAnimal) return TARGET_IS_ANIMAL;
    if (in.targetDead) return TARGET_DEAD;
    if (in.targetUnconscious) return TARGET_UNCONSCIOUS;
    if (in.targetDown) return TARGET_DOWN;
    if (in.targetCarried) return TARGET_CARRIED;
    if (!in.targetProneNormal) return TARGET_NOT_STANDING;
    return ELIGIBLE;
}

const char* EligibilityName(Eligibility reason)
{
    switch (reason)
    {
    case ELIGIBLE:              return "eligible";
    case NOT_PLAYER_ACTOR:      return "actor is not a player character";
    case NOT_IN_STEALTH:        return "not in stealth";
    case CANNOT_TAKE_ORDERS:    return "cannot take orders";
    case IN_COMBAT:             return "in combat";
    case IN_DIALOGUE_OR_MODAL:  return "dialogue or modal message open";
    case TARGET_INVALID:        return "target invalid";
    case TARGET_IS_PLAYER:      return "target is a player character";
    case TARGET_IS_ANIMAL:      return "target is an animal";
    case TARGET_DEAD:           return "target is dead";
    case TARGET_UNCONSCIOUS:    return "target is unconscious";
    case TARGET_DOWN:           return "target is down";
    case TARGET_CARRIED:        return "target is being carried";
    case TARGET_NOT_STANDING:   return "target is not standing";
    }
    return "unknown";
}

float HorizontalDistance(float dx, float dz)
{
    return std::sqrt(dx * dx + dz * dz);
}

bool CanOpenWindow(const RangeInput& in)
{
    return in.sameFloor && in.sameBuilding &&
           SquaredHorizontal(in) < kEngineTradeRangeSq;
}

bool InApproachStopRange(const RangeInput& in)
{
    return in.sameFloor && in.sameBuilding &&
           SquaredHorizontal(in) < kApproachStopDistance * kApproachStopDistance;
}

void BeginSession(SessionState* state, float now)
{
    *state = SessionState();
    state->phase = PHASE_APPROACHING;
    state->startTime = now;
    state->lastProgressTime = now;
    state->bestDistance = -1.0f;        // unset until the first tick
    state->lastOrderTime = now;
}

TickResult Step(SessionState* state, const TickInput& in)
{
    switch (state->phase)
    {
    case PHASE_APPROACHING: return StepApproaching(state, in);
    case PHASE_OPEN:        return StepOpen(state, in);
    case PHASE_IDLE:        break;
    }
    return Result(ACT_NONE, CANCEL_NONE);
}

const char* ReasonName(CancelReason reason)
{
    switch (reason)
    {
    case CANCEL_NONE:          return "none";
    case CANCEL_STEALTH_LOST:  return "stealth_lost";
    case CANCEL_PLAYER_REORDER:return "external_order";
    case CANCEL_INVALID:       return "actor_or_target_invalid";
    case CANCEL_TIMEOUT:       return "timeout";
    case CANCEL_NO_PROGRESS:   return "no_progress";
    case CANCEL_COMBAT:        return "combat";
    case CANCEL_OUT_OF_RANGE:  return "out_of_range";
    }
    return "unknown";
}

const char* CancelMessage(CancelReason reason)
{
    switch (reason)
    {
    case CANCEL_NONE:          return "";
    case CANCEL_STEALTH_LOST:  return "Pick pocket cancelled: you are no longer sneaking.";
    case CANCEL_PLAYER_REORDER:return "Pick pocket cancelled: new order given.";
    case CANCEL_INVALID:       return "Pick pocket cancelled: target lost.";
    case CANCEL_TIMEOUT:       return "Pick pocket cancelled: took too long to reach the target.";
    case CANCEL_NO_PROGRESS:   return "Pick pocket cancelled: cannot reach the target.";
    case CANCEL_COMBAT:        return "Pick pocket cancelled: combat started.";
    case CANCEL_OUT_OF_RANGE:  return "Pick pocket window closed: target moved out of reach.";
    }
    return "";
}
} // namespace pickpocket
} // namespace totalloot
