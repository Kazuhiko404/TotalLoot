#include "common.h"
#include "log.h"

namespace totalloot
{
namespace
{
const char* const kLogPrefix = "[TotalLoot] ";
}

void LogInfo(const std::string& message)
{
    DebugLog(std::string(kLogPrefix) + message);
}

void LogError(const std::string& message)
{
    ErrorLog(std::string(kLogPrefix) + message);
}
} // namespace totalloot
