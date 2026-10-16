#include "tick_dispatch.h"
#include "log.h"

#include <algorithm>

namespace totalloot
{
namespace
{
std::vector<TickCallback>* g_callbacks = NULL;  // heap: no work at static init
ULONGLONG g_clockBase = 0;

void (*g_mainLoop_orig)(GameWorld*, float) = NULL;

void GameWorld_mainLoop_hook(GameWorld* thisptr, float time)
{
    if (g_mainLoop_orig != NULL)
    {
        g_mainLoop_orig(thisptr, time);
    }
    if (g_callbacks == NULL)
    {
        return;
    }
    // Index loop: a callback may register another one while it runs.
    for (size_t i = 0; i < g_callbacks->size(); ++i)
    {
        (*g_callbacks)[i]();
    }
}
} // namespace

bool InstallTickHook()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff),
            GameWorld_mainLoop_hook,
            &g_mainLoop_orig))
    {
        LogError("Failed to hook GameWorld::mainLoop_GPUSensitiveStuff.");
        return false;
    }
    LogInfo("Tick hook installed (GameWorld::mainLoop_GPUSensitiveStuff).");
    return true;
}

void RegisterTick(TickCallback callback)
{
    if (callback == NULL)
    {
        return;
    }
    if (g_callbacks == NULL)
    {
        g_callbacks = new std::vector<TickCallback>();
    }
    if (std::find(g_callbacks->begin(), g_callbacks->end(), callback) == g_callbacks->end())
    {
        g_callbacks->push_back(callback);
    }
}

float TickNowSeconds()
{
    const ULONGLONG ticks = GetTickCount64();
    if (g_clockBase == 0)
    {
        g_clockBase = ticks;
    }
    return static_cast<float>(ticks - g_clockBase) / 1000.0f;
}
} // namespace totalloot
