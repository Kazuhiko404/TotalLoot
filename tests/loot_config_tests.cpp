#include "test_harness.h"
#include "loot_config.h"

#include <sstream>
#include <string>

using namespace totalloot;

namespace
{
const char* const kSuiteName = "loot_config";

LootConfig Parse(const char* text)
{
    std::istringstream in((std::string(text)));
    return ParseLootConfig(in);
}

void EmptyInputGivesDefaults()
{
    const LootConfig c = Parse("");
    CHECK(!c.cheatLoot);
    CHECK(c.lootRadius == 20.0f);
    CHECK(c.arriveDistance == 5.0f);
    CHECK(!c.clearStolenFlag);
    CHECK(c.cancelOnCombat);
    CHECK(c.cancelWhenNearbyAllyAttacked);
}

void ReadsEveryKey()
{
    const LootConfig c = Parse(
        "[General]\n"
        "CheatLoot=1\n"
        "LootRadius=35.5\n"
        "ArriveDistance=8\n"
        "[Looting]\n"
        "clear_stolen_flag=1\n"
        "cancel_on_combat=0\n"
        "cancel_when_nearby_ally_attacked=0\n");
    CHECK(c.cheatLoot);
    CHECK(c.lootRadius == 35.5f);
    CHECK(c.arriveDistance == 8.0f);
    CHECK(c.clearStolenFlag);
    CHECK(!c.cancelOnCombat);
    CHECK(!c.cancelWhenNearbyAllyAttacked);
}

void KeysAndSectionsAreCaseInsensitive()
{
    const LootConfig a = Parse("[General]\nCheatLoot=1\n");
    const LootConfig b = Parse("[GENERAL]\ncheatloot=1\n");
    const LootConfig c = Parse("[general]\nCHEATLOOT = 1\n");
    CHECK(a.cheatLoot);
    CHECK(b.cheatLoot);
    CHECK(c.cheatLoot);
    const LootConfig d = Parse("[LOOTING]\nCLEAR_STOLEN_FLAG=true\n");
    CHECK(d.clearStolenFlag);
}

void CommentsAndBlankLinesAreIgnored()
{
    const LootConfig c = Parse(
        "; leading comment\n"
        "# hash comment\n"
        "\n"
        "[General]\n"
        "; CheatLoot=1\n"
        "# CheatLoot=1\n"
        "LootRadius=30\n");
    CHECK(!c.cheatLoot);
    CHECK(c.lootRadius == 30.0f);
}

void BoolSpellings()
{
    CHECK(Parse("[General]\nCheatLoot=true\n").cheatLoot);
    CHECK(Parse("[General]\nCheatLoot=yes\n").cheatLoot);
    CHECK(Parse("[General]\nCheatLoot=on\n").cheatLoot);
    CHECK(Parse("[General]\nCheatLoot=TRUE\n").cheatLoot);
    CHECK(!Parse("[General]\nCheatLoot=false\n").cheatLoot);
    CHECK(!Parse("[General]\nCheatLoot=no\n").cheatLoot);
    CHECK(!Parse("[General]\nCheatLoot=off\n").cheatLoot);
    CHECK(!Parse("[General]\nCheatLoot=0\n").cheatLoot);
    // Garbage keeps the default (false here, true for cancel_on_combat).
    CHECK(!Parse("[General]\nCheatLoot=maybe\n").cheatLoot);
    CHECK(Parse("[Looting]\ncancel_on_combat=maybe\n").cancelOnCombat);
}

void InvalidRadiusFallsBackToDefault()
{
    CHECK(Parse("[General]\nLootRadius=0\n").lootRadius == 20.0f);
    CHECK(Parse("[General]\nLootRadius=-5\n").lootRadius == 20.0f);
    CHECK(Parse("[General]\nLootRadius=abc\n").lootRadius == 20.0f);
    CHECK(Parse("[General]\nLootRadius=\n").lootRadius == 20.0f);
    CHECK(Parse("[General]\nArriveDistance=0\n").arriveDistance == 5.0f);
    CHECK(Parse("[General]\nArriveDistance=-2\n").arriveDistance == 5.0f);
    CHECK(Parse("[General]\nArriveDistance=nan\n").arriveDistance == 5.0f);
}

void HugeValuesAreClamped()
{
    CHECK(Parse("[General]\nLootRadius=100000\n").lootRadius == kMaxLootRadius);
    CHECK(Parse("[General]\nArriveDistance=100000\n").arriveDistance == kMaxArriveDistance);
}

void KeysOutsideTheirSectionAreIgnored()
{
    const LootConfig c = Parse("CheatLoot=1\n[Looting]\nCheatLoot=1\n");
    CHECK(!c.cheatLoot);
}

void UnknownKeysAndOtherSectionsAreIgnored()
{
    const LootConfig c = Parse("[Other]\nfoo=bar\n[General]\nunknown=1\nLootRadius=25\n");
    CHECK(c.lootRadius == 25.0f);
    CHECK(!c.cheatLoot);
}

void WindowsLineEndingsAndPaddingWork()
{
    const LootConfig c = Parse("[General]\r\n  CheatLoot  =  1  \r\nLootRadius=12\r\n");
    CHECK(c.cheatLoot);
    CHECK(c.lootRadius == 12.0f);
}
} // namespace

void RunLootConfigTests()
{
    EmptyInputGivesDefaults();
    ReadsEveryKey();
    KeysAndSectionsAreCaseInsensitive();
    CommentsAndBlankLinesAreIgnored();
    BoolSpellings();
    InvalidRadiusFallsBackToDefault();
    HugeValuesAreClamped();
    KeysOutsideTheirSectionAreIgnored();
    UnknownKeysAndOtherSectionsAreIgnored();
    WindowsLineEndingsAndPaddingWork();
}
