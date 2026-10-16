#include "test_harness.h"
#include "auto_loot_rules.h"

#include <cstring>

using namespace totalloot::autoloot;

namespace
{
const char* const kSuiteName = "auto_loot_rules";

// A living, able, player-owned animal that is not auto looting.
ButtonInput Animal()
{
    ButtonInput in;
    in.isAnimal = true;
    in.isPlayerOwned = true;
    return in;
}

void LivingPlayerAnimalShowsStart()
{
    const ButtonDecision d = DecideButton(Animal());
    CHECK(d.visible);
    CHECK(!d.stop);
}

void RunningAutoLootShowsStop()
{
    ButtonInput in = Animal();
    in.autoLooting = true;
    const ButtonDecision d = DecideButton(in);
    CHECK(d.visible);
    CHECK(d.stop);
}

void HumanoidsGetNoButton()
{
    ButtonInput in = Animal();
    in.isAnimal = false;
    CHECK(!DecideButton(in).visible);
}

void WildAnimalsGetNoButton()
{
    ButtonInput in = Animal();
    in.isPlayerOwned = false;
    CHECK(!DecideButton(in).visible);
}

void DownOrDeadAnimalsGetNoButton()
{
    ButtonInput in = Animal();
    in.isDownOrDead = true;
    CHECK(!DecideButton(in).visible);
    in.autoLooting = true;
    const ButtonDecision d = DecideButton(in);
    CHECK(!d.visible);
    CHECK(!d.stop);
}

void CaptionsDistinguishStartFromStop()
{
    CHECK(std::strcmp(Caption(false), "Auto loot") == 0);
    CHECK(std::strcmp(Caption(true), "Stop auto loot") == 0);
}
} // namespace

void RunAutoLootRulesTests()
{
    LivingPlayerAnimalShowsStart();
    RunningAutoLootShowsStop();
    HumanoidsGetNoButton();
    WildAnimalsGetNoButton();
    DownOrDeadAnimalsGetNoButton();
    CaptionsDistinguishStartFromStop();
}
