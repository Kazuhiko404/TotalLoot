#include "loot_walk_session.h"
#include "loot_walk_rules.h"
#include "loot_config.h"
#include "loot_filter.h"
#include "loot_targets.h"
#include "loot_transfer.h"
#include "loot_move.h"
#include "loot_threats.h"
#include "target_character.h"
#include "order_watch.h"
#include "pickpocket_actions.h"
#include "tick_dispatch.h"
#include "log.h"

#include <cmath>
#include <cstdio>

namespace totalloot
{
namespace
{
// Sessions are heap-held: hand's constructor calls into the engine, which must
// not run during this DLL's static initialisation.
struct Session
{
    hand looter;
    Character* watched;             // the actor order_watch tracks for us
    std::vector<hand> targets;
    lootwalk::WalkState state;
    int pendingMoved;               // result of the loot the last tick requested
    bool pendingLeft;               // target still held items after it
    bool autoLoot;
    bool moveActive;                // a move order of ours may still be running
    int loggedMoveFor;              // target index whose first move was logged
    int bodiesLooted;
    float startTime;
    float lastAllyCheck;
    bool allyAttacked;
    ForeignOrder lastForeign;

    Session()
        : watched(NULL), pendingMoved(0), pendingLeft(false), autoLoot(false),
          moveActive(false), loggedMoveFor(-1), bodiesLooted(0), startTime(0.0f),
          lastAllyCheck(-100.0f), allyAttacked(false)
    {
        lastForeign.taskType = 0;
        lastForeign.shift = false;
        lastForeign.clear = false;
    }
};

std::vector<Session*>* g_sessions = NULL;

// The ally scan walks the whole squad, so it runs at this interval, not every
// frame.
const float kAllyCheckIntervalSeconds = 0.5f;

std::string Num(float value)
{
    char buffer[32];
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%.1f", value);
    return buffer;
}

std::string Int(int value)
{
    return IntToStr(value);
}

const char* WalkName(const Session* s)
{
    return s->autoLoot ? "Auto loot" : "Walk loot";
}

const hand* SafeHandleOf(Character* character)
{
    TL_TRY { return &character->getHandle(); }
    TL_CATCH { return NULL; }
}

// Horizontal looter-to-target distance; false when a position is unreadable.
bool SafeHorizontalDistance(Character* looter, RootObject* target, float* out)
{
    TL_TRY
    {
        const Ogre::Vector3 from = looter->getPosition();
        const Ogre::Vector3 to = target->getPosition();
        const float dx = to.x - from.x;
        const float dz = to.z - from.z;
        *out = std::sqrt(dx * dx + dz * dz);
        return true;
    }
    TL_CATCH { return false; }
}

std::vector<Session*>& Sessions()
{
    if (g_sessions == NULL)
    {
        g_sessions = new std::vector<Session*>();
    }
    return *g_sessions;
}

// Index of the looter's session, or -1.
int FindSessionIndex(Character* looter)
{
    if (g_sessions == NULL || looter == NULL)
    {
        return -1;
    }
    const hand* handle = SafeHandleOf(looter);
    if (handle == NULL)
    {
        return -1;
    }
    for (size_t i = 0; i < g_sessions->size(); ++i)
    {
        if ((*g_sessions)[i]->looter == *handle)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Removes session `index`, releasing its watch. Stops the actor only when asked
// (we never stop a character the player just re-ordered).
void EndSession(size_t index, bool stopActor)
{
    Session* session = (*g_sessions)[index];
    if (stopActor && session->moveActive)
    {
        StopActor(ResolveBody(session->looter));
    }
    UnwatchOrdersOf(session->watched);
    delete session;
    g_sessions->erase(g_sessions->begin() + index);
}

// "Valid target" and "still holds items" for the current body or container.
void ReadTarget(RootObject* target, bool* valid, bool* hasItems)
{
    Character* body = CharacterFromRootObject(target);
    *valid = (body != NULL) ? SafeIsLootableCharacter(body)
                            : SafeIsLootableContainer(target);
    *hasItems = *valid && HasWantedLoot(target);
}

bool PollAllyAttacked(Session* s, Character* looter, const Ogre::Vector3& pos, float now)
{
    if (now - s->lastAllyCheck >= kAllyCheckIntervalSeconds)
    {
        s->lastAllyCheck = now;
        s->allyAttacked = NearbyAllyAttacked(looter, pos, kAllyDefenseRadius);
    }
    return s->allyAttacked;
}

// Fills `in` from the engine. Fields stay at their "invalid / lost" defaults
// when a read fails, so the state machine cancels or skips instead of acting
// blind.
void GatherTickInput(Session* s, Character* looter, lootwalk::TickInput* in)
{
    const LootConfig& cfg = Config();
    in->now = TickNowSeconds();
    in->arriveDistance = cfg.arriveDistance;
    in->cancelOnCombat = cfg.cancelOnCombat;
    in->cancelOnAllyAttacked = cfg.cancelWhenNearbyAllyAttacked;
    in->lastTransferMoved = s->pendingMoved;
    in->lastTransferInventoryFull = s->pendingLeft;

    // Taken first so a foreign order is never lost to an early return below.
    if (TakeForeignOrder(looter, &s->lastForeign))
    {
        in->foreignOrder = true;
    }

    in->actorValid = IsActorOperational(looter);
    if (!in->actorValid)
    {
        return;
    }
    in->actorInCombat = IsInCombat(looter);

    Ogre::Vector3 looterPos;
    if (!SafeGetPosition(looter, &looterPos))
    {
        in->actorValid = false;
        return;
    }
    if (cfg.cancelWhenNearbyAllyAttacked)
    {
        in->allyAttacked = PollAllyAttacked(s, looter, looterPos, in->now);
    }

    const int index = s->state.current;
    if (index < 0 || index >= static_cast<int>(s->targets.size()))
    {
        return; // queue exhausted: the machine finishes without a target
    }
    RootObject* target = ResolveLootTarget(s->targets[index]);
    if (target == NULL)
    {
        return;
    }
    ReadTarget(target, &in->targetValid, &in->targetStillHasItems);
    if (!SafeHorizontalDistance(looter, target, &in->distance))
    {
        in->targetValid = false;
    }
}

void PerformMove(Session* s, Character* looter, int index)
{
    RootObject* target = ResolveLootTarget(s->targets[index]);
    if (target == NULL || !IssueMoveTo(looter, target))
    {
        LogError("Walk loot: could not issue the move order.");
        return;
    }
    s->moveActive = true;
    // The machine re-issues the order every few seconds: log the first only.
    if (s->loggedMoveFor != index)
    {
        s->loggedMoveFor = index;
        float distance = 0.0f;
        SafeHorizontalDistance(looter, target, &distance);
        LogInfo(std::string(WalkName(s)) + ": walking to body " + Int(index + 1) + "/" +
                Int(static_cast<int>(s->targets.size())) + ", distance " + Num(distance) + ".");
    }
}

void PerformLoot(Session* s, Character* looter, int index)
{
    RootObject* target = ResolveLootTarget(s->targets[index]);
    s->pendingMoved = 0;
    s->pendingLeft = false;
    StopActor(looter);
    s->moveActive = false;
    if (target == NULL)
    {
        return;
    }
    bool left = false;
    s->pendingMoved = TransferAllLoot(looter, target, &left);
    s->pendingLeft = left;
    if (s->pendingMoved > 0)
    {
        ++s->bodiesLooted;
    }
    LogInfo(std::string(WalkName(s)) + ": arrived at body " + Int(index + 1) + "/" +
            Int(static_cast<int>(s->targets.size())) + ", moved " + Int(s->pendingMoved) +
            " items" + (left ? " (items left)" : "") + ".");
}

// True when the session ended.
bool Perform(size_t sessionIndex, const lootwalk::TickResult& result, Character* looter)
{
    using namespace lootwalk;
    Session* s = (*g_sessions)[sessionIndex];
    switch (result.action)
    {
    case ACT_ISSUE_MOVE:
        PerformMove(s, looter, result.targetIndex);
        return false;

    case ACT_LOOT_CURRENT:
        PerformLoot(s, looter, result.targetIndex);
        return false;

    case ACT_FINISH:
        LogInfo(std::string(WalkName(s)) + " finished: " + Int(s->state.totalMoved) +
                " items from " + Int(s->bodiesLooted) + " bodies in " +
                Num(TickNowSeconds() - s->startTime) + "s.");
        EndSession(sessionIndex, true);
        return true;

    case ACT_CANCEL:
    {
        LogInfo(std::string(WalkName(s)) + " cancelled, reason=" + ReasonName(result.reason) +
                " (" + Int(s->state.totalMoved) + " items so far).");
        const char* message = CancelMessage(result.reason);
        if (message[0] != '\0')
        {
            pickpocket::ShowFeedback(message);
        }
        // Never stop a character the player just re-ordered or one we lost.
        const bool stop = result.reason != CANCEL_PLAYER_REORDER &&
                          result.reason != CANCEL_INVALID_ACTOR;
        EndSession(sessionIndex, stop);
        return true;
    }

    case ACT_ADVANCE:
    case ACT_NONE:
        break;
    }
    return false;
}

void Tick()
{
    if (g_sessions == NULL)
    {
        return;
    }
    for (size_t i = 0; i < g_sessions->size();)
    {
        Session* s = (*g_sessions)[i];
        Character* looter = ResolveBody(s->looter);

        lootwalk::TickInput input;
        GatherTickInput(s, looter, &input);

        const lootwalk::TickResult result = lootwalk::Step(&s->state, input);
        // The transfer result is consumed by the tick that carried it.
        if (s->state.phase != lootwalk::PHASE_LOOTING)
        {
            s->pendingMoved = 0;
            s->pendingLeft = false;
        }
        if (!Perform(i, result, looter))
        {
            ++i;
        }
    }
}
} // namespace

bool InstallLootWalkSession()
{
    RegisterTick(&Tick);
    LogInfo("Walk loot session tick registered.");
    return true;
}

void ShutdownLootWalkSession()
{
    while (g_sessions != NULL && !g_sessions->empty())
    {
        EndSession(g_sessions->size() - 1, false);
    }
}

bool StartWalkLoot(Character* looter, const std::vector<hand>& targets, bool autoLoot)
{
    if (looter == NULL || targets.empty() || !IsActorOperational(looter))
    {
        return false;
    }
    const hand* handle = SafeHandleOf(looter);
    if (handle == NULL)
    {
        return false;
    }

    // Re-clicking Loot replaces the looter's walk; the new one issues its own
    // move, so the old one is not stopped.
    const int existing = FindSessionIndex(looter);
    if (existing >= 0)
    {
        EndSession(static_cast<size_t>(existing), false);
    }

    Session* s = new Session();
    s->looter = *handle;
    s->watched = looter;
    s->targets = targets;
    s->autoLoot = autoLoot;
    s->startTime = TickNowSeconds();
    lootwalk::BeginWalk(&s->state, s->startTime, static_cast<int>(targets.size()));
    WatchOrdersOf(looter);
    Sessions().push_back(s);
    LogInfo(std::string(WalkName(s)) + " started: " +
            Int(static_cast<int>(targets.size())) + " target(s).");
    return true;
}

void CancelWalkLoot(Character* looter)
{
    const int index = FindSessionIndex(looter);
    if (index < 0)
    {
        return;
    }
    LogInfo(std::string(WalkName((*g_sessions)[index])) + " stopped by request.");
    EndSession(static_cast<size_t>(index), true);
}

bool IsWalkLooting(Character* looter)
{
    return FindSessionIndex(looter) >= 0;
}

bool IsAutoLooting(Character* looter)
{
    const int index = FindSessionIndex(looter);
    return index >= 0 && (*g_sessions)[index]->autoLoot;
}
} // namespace totalloot
