#include "pickpocket_session.h"
#include "pickpocket_rules.h"
#include "pickpocket_reads.h"
#include "pickpocket_actions.h"
#include "order_watch.h"
#include "tick_dispatch.h"
#include "log.h"

#include <cstdio>

namespace totalloot
{
namespace
{
using namespace pickpocket;

// Handles are heap-held: hand's constructor calls into the engine, which must
// not run during this DLL's static initialisation.
hand* g_actorHand = NULL;
hand* g_targetHand = NULL;
Character* g_watchedActor = NULL;  // the actor whose orders order_watch tracks
SessionState g_state;
float g_lastOrderTargetX = 0.0f;
float g_lastOrderTargetZ = 0.0f;
ULONGLONG g_clockBase = 0;

// Diagnostics only: what the next in-game test needs to read from the log.
float g_startTime = 0.0f;
float g_lastDistance = 0.0f;
float g_lastIssueLogTime = 0.0f;
int g_issueCount = 0;
bool g_windowSeenLogged = false;
ForeignOrder g_lastForeign = { 0, false, false };
const float kIssueLogIntervalSeconds = 5.0f;

float NowSeconds()
{
    const ULONGLONG ticks = GetTickCount64();
    if (g_clockBase == 0)
    {
        g_clockBase = ticks;
    }
    return static_cast<float>(ticks - g_clockBase) / 1000.0f;
}

void ReleaseHands()
{
    delete g_actorHand;
    delete g_targetHand;
    g_actorHand = NULL;
    g_targetHand = NULL;
}

void EndSession()
{
    g_state = SessionState();
    UnwatchOrdersOf(g_watchedActor);
    g_watchedActor = NULL;
    ReleaseHands();
}

// "1.23" style text for log lines (no SEH in this frame).
std::string Num(float value)
{
    char buffer[32];
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%.1f", value);
    return buffer;
}

std::string Int(int value)
{
    char buffer[16];
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    return buffer;
}

std::string Since(float now)
{
    return Num(now - g_startTime) + "s after start";
}

const hand* SafeHandleOf(Character* character)
{
    TL_TRY { return &character->getHandle(); }
    TL_CATCH { return NULL; }
}

bool CaptureHands(Character* actor, Character* target)
{
    const hand* actorHandle = SafeHandleOf(actor);
    const hand* targetHandle = SafeHandleOf(target);
    if (actorHandle == NULL || targetHandle == NULL)
    {
        return false;
    }
    g_actorHand = new hand(*actorHandle);
    g_targetHand = new hand(*targetHandle);
    return true;
}

void RememberOrderTarget(float x, float z)
{
    g_lastOrderTargetX = x;
    g_lastOrderTargetZ = z;
}

// Fills `in` from the engine. Fields stay at their "invalid / lost" defaults
// when a read fails, so the state machine cancels instead of acting blind.
void GatherTickInput(Character* actor, Character* target, TickInput* in)
{
    in->now = NowSeconds();
    // Taken first so a foreign order is never lost to an early return below.
    if (TakeForeignOrder(actor, &g_lastForeign))
    {
        in->externalOrder = true;
    }
    in->actorValid = (actor != NULL);
    in->targetValid = ReadTargetAlive(target);
    if (!in->actorValid || !in->targetValid)
    {
        return;
    }

    bool stealth = false;
    bool combat = false;
    if (!ReadActorState(actor, &stealth, &combat))
    {
        in->actorValid = false;
        return;
    }
    in->stealth = stealth;
    in->combat = combat;

    float targetX = 0.0f;
    float targetZ = 0.0f;
    if (!ReadRange(actor, target, &in->range, &targetX, &targetZ))
    {
        in->targetValid = false;
        return;
    }
    g_lastDistance = HorizontalDistance(in->range.dx, in->range.dz);
    in->targetMoved = HorizontalDistance(targetX - g_lastOrderTargetX,
                                         targetZ - g_lastOrderTargetZ);
    in->windowVisible = IsLootWindowVisible(*g_targetHand);
}

// Throttled: the first order always, re-targeting orders at most every 5 s.
void LogMoveOrder(float now)
{
    ++g_issueCount;
    if (g_issueCount > 1 && now - g_lastIssueLogTime < kIssueLogIntervalSeconds)
    {
        return;
    }
    g_lastIssueLogTime = now;
    LogInfo("Pick pocket: approach move order #" + Int(g_issueCount) +
            " issued, distance " + Num(g_lastDistance) + ", " + Since(now) + ".");
}

void LogWindowOpened(float now)
{
    const bool visible = IsLootWindowVisible(*g_targetHand);
    LogInfo(std::string("Pick pocket: loot window opened (") +
            (g_issueCount == 0 ? "target already in range, no approach"
                               : "arrived after approach") +
            ", distance " + Num(g_lastDistance) + ", " + Since(now) +
            "); getInventoryWindow visible=" + (visible ? "yes" : "no") + ".");
    g_windowSeenLogged = visible;
}

void LogSessionEnd(const TickResult& result, float now)
{
    std::string line = std::string("Pick pocket: ") +
        (result.action == ACT_FINISH ? "loot window closed by the player"
         : result.action == ACT_CLOSE_WINDOW ? "loot window closed by TotalLoot"
                                              : "session cancelled") +
        ", reason=" + ReasonName(result.reason) + ", " + Since(now);
    if (result.reason == CANCEL_PLAYER_REORDER)
    {
        line += std::string("; foreign Character::addOrder type=") +
                Int(g_lastForeign.taskType) +
                (g_lastForeign.shift ? " shift" : "") +
                (g_lastForeign.clear ? " clear" : "");
    }
    LogInfo(line + ".");
}

void Perform(const TickResult& result, Character* actor, Character* target)
{
    switch (result.action)
    {
    case ACT_ISSUE_MOVE:
        IssueApproachOrder(actor, target);
        LogMoveOrder(NowSeconds());
        {
            RangeInput range;
            float x = 0.0f;
            float z = 0.0f;
            if (ReadRange(actor, target, &range, &x, &z))
            {
                RememberOrderTarget(x, z);
            }
        }
        return;

    case ACT_OPEN_WINDOW:
        if (!OpenLootWindow(actor, target))
        {
            LogError("Pick pocket: could not open the loot window.");
            EndSession();
            return;
        }
        LogWindowOpened(NowSeconds());
        return;

    case ACT_CLOSE_WINDOW:
        CloseLootWindow(*g_targetHand);
        break;

    case ACT_CANCEL:
    case ACT_FINISH:
    case ACT_NONE:
        break;
    }

    if (result.action == ACT_NONE)
    {
        return;
    }

    LogSessionEnd(result, NowSeconds());
    const char* message = CancelMessage(result.reason);
    if (message[0] != '\0')
    {
        ShowFeedback(message);
    }
    EndSession();
}

void Tick()
{
    if (g_state.phase == PHASE_IDLE || g_actorHand == NULL || g_targetHand == NULL)
    {
        return;
    }

    Character* actor = ResolveHand(*g_actorHand);
    Character* target = ResolveHand(*g_targetHand);

    TickInput input;
    GatherTickInput(actor, target, &input);

    const TickResult result = Step(&g_state, input);
    Perform(result, actor, target);

    if (g_state.phase == PHASE_OPEN && input.windowVisible && !g_windowSeenLogged)
    {
        g_windowSeenLogged = true;
        LogInfo("Pick pocket: loot window is now visible (getInventoryWindow), " +
                Since(input.now) + ".");
    }
}
} // namespace

bool InstallPickPocketSession()
{
    // The frame hook itself is shared (tick_dispatch.h).
    RegisterTick(&Tick);
    LogInfo("Pick pocket session tick registered.");
    return true;
}

void ShutdownPickPocketSession()
{
    EndSession();
}

bool StartPickPocket(Character* actor, Character* target)
{
    if (actor == NULL || target == NULL)
    {
        return false;
    }
    if (g_state.phase != PHASE_IDLE)
    {
        return false;
    }

    EligibilityInput eligibility;
    if (!ReadEligibilityInput(actor, target, &eligibility))
    {
        return false;
    }
    const Eligibility verdict = Evaluate(eligibility);
    if (verdict != ELIGIBLE)
    {
        LogInfo(std::string("Pick pocket refused: ") + EligibilityName(verdict));
        return false;
    }

    if (!CaptureHands(actor, target))
    {
        ReleaseHands();
        return false;
    }

    RangeInput range;
    float x = 0.0f;
    float z = 0.0f;
    if (ReadRange(actor, target, &range, &x, &z))
    {
        RememberOrderTarget(x, z);
    }
    g_startTime = NowSeconds();
    g_issueCount = 0;
    g_lastIssueLogTime = 0.0f;
    g_windowSeenLogged = false;
    g_lastDistance = HorizontalDistance(range.dx, range.dz);
    g_watchedActor = actor;
    WatchOrdersOf(actor);
    BeginSession(&g_state, g_startTime);
    LogInfo(std::string("Pick pocket session started: distance ") + Num(g_lastDistance) +
            (InApproachStopRange(range) ? ", already in range (will open directly)"
                                        : ", out of range (will approach)") + ".");
    return true;
}
} // namespace totalloot
