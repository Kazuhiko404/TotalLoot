#include "common.h"
#include "loot_config.h"
#include "log.h"
#include "tick_dispatch.h"

#include <fstream>
#include <sstream>

namespace totalloot
{
extern HMODULE g_module;

namespace
{
const char* const kIniFileName = "TotalLoot.ini";

LootConfig g_config;
volatile LONG g_reloadRequested = 0;  // set by the settings window thread

// Frame tick (game thread): the only place a requested reload writes g_config.
void ApplyPendingConfigReload()
{
    if (InterlockedExchange(&g_reloadRequested, 0) != 0)
    {
        ReloadConfig();
    }
}

std::string GetIniPath()
{
    char buffer[MAX_PATH] = { 0 };
    const DWORD len = GetModuleFileNameA(g_module, buffer, MAX_PATH);
    std::string path(buffer, len);
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) return kIniFileName;
    return path.substr(0, slash + 1) + kIniFileName;
}

std::string Describe(const LootConfig& c)
{
    std::ostringstream oss;
    oss << "cheat_loot=" << (c.cheatLoot ? "1" : "0")
        << " loot_radius=" << c.lootRadius
        << " arrive_distance=" << c.arriveDistance
        << " clear_stolen_flag=" << (c.clearStolenFlag ? "1" : "0")
        << " cancel_on_combat=" << (c.cancelOnCombat ? "1" : "0")
        << " cancel_when_nearby_ally_attacked=" << (c.cancelWhenNearbyAllyAttacked ? "1" : "0");
    return oss.str();
}
} // namespace

LootConfig ReadConfigFile()
{
    const std::string path = GetIniPath();
    std::ifstream file(path.c_str());
    if (file.is_open())
    {
        return ParseLootConfig(file);
    }
    return LootConfig();
}

void RequestConfigReload()
{
    InterlockedExchange(&g_reloadRequested, 1);
}

void LoadConfig()
{
    ReloadConfig();
    RegisterTick(&ApplyPendingConfigReload);
}

void ReloadConfig()
{
    const std::string path = GetIniPath();
    std::ifstream file(path.c_str());
    if (file.is_open())
    {
        g_config = ParseLootConfig(file);
        LogInfo("Config loaded from '" + path + "': " + Describe(g_config));
    }
    else
    {
        g_config = LootConfig();
        LogError("Config not found at '" + path + "'; using defaults: " + Describe(g_config));
    }
}

const LootConfig& Config()
{
    return g_config;
}
} // namespace totalloot
