#pragma once

// Pure pick-pocket decisions: eligibility, range, and the approach session
// state machine. No Kenshi / MyGUI includes so it is unit-tested.

namespace totalloot
{
namespace pickpocket
{
// ---- Eligibility ------------------------------------------------------

struct EligibilityInput
{
    bool actorIsPlayer;
    bool actorInStealth;
    bool actorCanTakeOrders;
    bool actorInCombat;
    bool inDialogueOrModal;
    bool targetValid;
    bool targetIsPlayer;
    bool targetIsAnimal;
    bool targetDead;
    bool targetUnconscious;
    bool targetDown;
    bool targetCarried;
    bool targetProneNormal;

    EligibilityInput()
        : actorIsPlayer(false), actorInStealth(false), actorCanTakeOrders(false),
          actorInCombat(false), inDialogueOrModal(false), targetValid(false),
          targetIsPlayer(false), targetIsAnimal(false), targetDead(false),
          targetUnconscious(false), targetDown(false), targetCarried(false),
          targetProneNormal(false) {}
};

enum Eligibility
{
    ELIGIBLE = 0,
    NOT_PLAYER_ACTOR,
    NOT_IN_STEALTH,
    CANNOT_TAKE_ORDERS,
    IN_COMBAT,
    IN_DIALOGUE_OR_MODAL,
    TARGET_INVALID,
    TARGET_IS_PLAYER,
    TARGET_IS_ANIMAL,
    TARGET_DEAD,
    TARGET_UNCONSCIOUS,
    TARGET_DOWN,
    TARGET_CARRIED,
    TARGET_NOT_STANDING
};

// First failing condition wins; ELIGIBLE when the button may show.
Eligibility Evaluate(const EligibilityInput& in);

// Short English name for logs.
const char* EligibilityName(Eligibility reason);

// ---- Range ------------------------------------------------------------

// The engine lets items leave a victim's inventory only when the horizontal
// distance squared is below 3600 (60 units), on the same floor and in the
// same building. Height is not part of that rule, so it is not part of ours.
const float kEngineTradeRangeSq = 3600.0f;

// Approach stops a bit inside the engine limit so the window opens in range.
const float kApproachStopDistance = 40.0f;

struct RangeInput
{
    float dx;
    float dz;
    bool sameFloor;
    bool sameBuilding;

    RangeInput() : dx(0.0f), dz(0.0f), sameFloor(false), sameBuilding(false) {}
};

float HorizontalDistance(float dx, float dz);

// True while the engine would allow taking items (dist^2 < 3600 ...).
bool CanOpenWindow(const RangeInput& in);

// True once the approach is close enough to open the window (dist < 40 ...).
bool InApproachStopRange(const RangeInput& in);

// ---- Approach session -------------------------------------------------

const float kSessionTimeoutSeconds = 60.0f;
const float kNoProgressTimeoutSeconds = 15.0f;
const float kReissueIntervalSeconds = 1.0f;
const float kTargetMovedThreshold = 10.0f;
const float kWindowOpenGraceSeconds = 1.0f;  // time for the loot window to appear
const float kProgressEpsilon = 1.0f;         // distance gained that counts

enum Phase
{
    PHASE_IDLE = 0,
    PHASE_APPROACHING,
    PHASE_OPEN
};

enum CancelReason
{
    CANCEL_NONE = 0,
    CANCEL_STEALTH_LOST,
    CANCEL_PLAYER_REORDER,
    CANCEL_INVALID,
    CANCEL_TIMEOUT,
    CANCEL_NO_PROGRESS,
    CANCEL_COMBAT,
    CANCEL_OUT_OF_RANGE
};

enum Action
{
    ACT_NONE = 0,
    ACT_ISSUE_MOVE,     // (re)issue the approach order
    ACT_OPEN_WINDOW,    // clear orders and open the loot window
    ACT_CLOSE_WINDOW,   // close the window; session ended (see reason)
    ACT_CANCEL,         // session ended before the window opened (see reason)
    ACT_FINISH          // window was closed by the player; session ended
};

struct SessionState
{
    Phase phase;
    float startTime;
    float lastProgressTime;
    float bestDistance;
    float lastOrderTime;
    float openedTime;
    bool orderIssued;

    SessionState()
        : phase(PHASE_IDLE), startTime(0.0f), lastProgressTime(0.0f),
          bestDistance(0.0f), lastOrderTime(0.0f), openedTime(0.0f),
          orderIssued(false) {}
};

struct TickInput
{
    float now;              // monotonic seconds
    bool actorValid;
    bool targetValid;
    bool stealth;
    bool combat;
    // An order we did not issue reached the actor since the last tick (fed by
    // the Character::addOrder hook; the absence of OUR order proves nothing,
    // because the engine consumes it into an AI task while the actor walks).
    bool externalOrder;
    bool windowVisible;     // target's loot window is currently shown
    RangeInput range;
    float targetMoved;      // how far the target moved since the last move order

    TickInput()
        : now(0.0f), actorValid(false), targetValid(false), stealth(false),
          combat(false), externalOrder(false), windowVisible(false),
          targetMoved(0.0f) {}
};

struct TickResult
{
    Action action;
    CancelReason reason;

    TickResult() : action(ACT_NONE), reason(CANCEL_NONE) {}
};

void BeginSession(SessionState* state, float now);

// Advances the machine one frame. The caller performs the returned action.
// On ACT_CLOSE_WINDOW / ACT_CANCEL / ACT_FINISH the state is back to idle.
TickResult Step(SessionState* state, const TickInput& in);

// Stable machine-readable token for the log ("external_order", ...); "none" for
// CANCEL_NONE. Distinct per reason so a log line alone says why a session ended.
const char* ReasonName(CancelReason reason);

// Feedback line for the player; empty for CANCEL_NONE.
const char* CancelMessage(CancelReason reason);
} // namespace pickpocket
} // namespace totalloot
