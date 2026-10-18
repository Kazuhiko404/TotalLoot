#include "common.h"
#include "log.h"
#include "loot_config.h"
#include "loot_overlay.h"
#include "loot_walk_session.h"
#include "order_watch.h"
#include "pickpocket_session.h"
#include "tick_dispatch.h"
#include "better_looting/better_looting.h"

namespace totalloot
{
HMODULE g_module = NULL;
} // namespace totalloot

__declspec(dllexport) void startPlugin()
{
    using namespace totalloot;
    LogInfo("startPlugin called.");
    LoadConfig();

    KenshiLib::BinaryVersion versionInfo = KenshiLib::GetKenshiVersion();
    const std::string version = versionInfo.GetVersion();
    LogInfo("Detected Kenshi version: '" + version + "' (" +
            versionInfo.GetPlatformStr() + ").");

    if (version != kSupportedGameVersion)
    {
        LogError("Version is not " + std::string(kSupportedGameVersion) +
                 "; hooks may resolve to the wrong functions. Continuing anyway.");
    }

    const bool overlayOk = InstallLootOverlay();
    LogInfo(overlayOk ? "Context-menu Loot all overlay installed."
                      : "Context-menu Loot all overlay FAILED.");

    const bool watchOk = InstallOrderWatch();
    LogInfo(watchOk ? "Order watch installed."
                    : "Order watch FAILED (pick pocket cannot detect new orders).");

    // One frame hook shared by every session; they register their ticks on it.
    const bool sessionOk = InstallPickPocketSession();
    LogInfo(sessionOk ? "Pick pocket session installed."
                      : "Pick pocket session FAILED.");

    const bool walkOk = InstallLootWalkSession();
    LogInfo(walkOk ? "Walk loot session installed."
                   : "Walk loot session FAILED.");

    const bool tickOk = InstallTickHook();
    LogInfo(tickOk ? "Frame tick hook installed."
                   : "Frame tick hook FAILED (pick pocket and walking loot will not run).");

    const bool blOk = InstallBetterLooting();
    LogInfo(blOk ? "BetterLooting module installed."
                 : "BetterLooting module FAILED.");

    LogInfo("startPlugin complete.");
}

__declspec(dllexport) void stopPlugin()
{
    using namespace totalloot;
    LogInfo("stopPlugin called.");
    ShutdownLootWalkSession();
    ShutdownPickPocketSession();
    ShutdownLootOverlay();
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID /*reserved*/)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        totalloot::g_module = module;
        DisableThreadLibraryCalls(module);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
