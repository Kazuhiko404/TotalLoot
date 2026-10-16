#include "loot_config.h"

#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

namespace totalloot
{
namespace
{
typedef std::map<std::string, std::string> KeyValues;

std::string ToLower(const std::string& s)
{
    std::string out(s);
    for (size_t i = 0; i < out.size(); ++i)
    {
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

std::string Trim(const std::string& s)
{
    size_t begin = 0;
    while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
    size_t end = s.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(begin, end - begin);
}

bool ParseBool(const std::string& value, bool fallback)
{
    const std::string v = ToLower(Trim(value));
    if (v == "true" || v == "1" || v == "yes" || v == "on") return true;
    if (v == "false" || v == "0" || v == "no" || v == "off") return false;
    return fallback;
}

void ReadBool(const KeyValues& kv, const char* key, bool& out)
{
    KeyValues::const_iterator it = kv.find(key);
    if (it != kv.end()) out = ParseBool(it->second, out);
}

// Keeps `out` (the default) unless the value is a number above zero; values
// above `maxValue` are clamped. `!(v > 0)` also rejects NaN.
void ReadPositiveFloat(const KeyValues& kv, const char* key, float maxValue, float& out)
{
    KeyValues::const_iterator it = kv.find(key);
    if (it == kv.end() || it->second.empty()) return;
    char* end = NULL;
    const double v = std::strtod(it->second.c_str(), &end);
    if (end == it->second.c_str() || !(v > 0.0)) return;
    out = v > maxValue ? maxValue : static_cast<float>(v);
}

// Reads `[section] key=value` pairs into per-section maps (lowercased names).
std::map<std::string, KeyValues> ReadSections(std::istream& in)
{
    std::map<std::string, KeyValues> sections;
    KeyValues* current = NULL;
    std::string line;
    while (std::getline(in, line))
    {
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;

        if (trimmed[0] == '[')
        {
            const size_t close = trimmed.find(']');
            const std::string name = ToLower(Trim(
                trimmed.substr(1, close == std::string::npos ? std::string::npos : close - 1)));
            current = &sections[name];
            continue;
        }

        const size_t eq = trimmed.find('=');
        if (eq == std::string::npos || current == NULL) continue;
        const std::string key = ToLower(Trim(trimmed.substr(0, eq)));
        if (!key.empty()) (*current)[key] = Trim(trimmed.substr(eq + 1));
    }
    return sections;
}
} // namespace

LootConfig ParseLootConfig(std::istream& in)
{
    LootConfig c;
    const std::map<std::string, KeyValues> sections = ReadSections(in);

    std::map<std::string, KeyValues>::const_iterator general = sections.find("general");
    if (general != sections.end())
    {
        ReadBool(general->second, "cheatloot", c.cheatLoot);
        ReadPositiveFloat(general->second, "lootradius", kMaxLootRadius, c.lootRadius);
        ReadPositiveFloat(general->second, "arrivedistance", kMaxArriveDistance, c.arriveDistance);
    }

    std::map<std::string, KeyValues>::const_iterator looting = sections.find("looting");
    if (looting != sections.end())
    {
        ReadBool(looting->second, "clear_stolen_flag", c.clearStolenFlag);
        ReadBool(looting->second, "cancel_on_combat", c.cancelOnCombat);
        ReadBool(looting->second, "cancel_when_nearby_ally_attacked", c.cancelWhenNearbyAllyAttacked);
    }
    return c;
}
} // namespace totalloot
