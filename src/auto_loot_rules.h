#pragma once

// Pure decision behind the AUTO LOOT context-menu button: whether it shows for
// the right-clicked character and which caption it carries. No Kenshi includes
// so it is unit-tested.

namespace totalloot
{
namespace autoloot
{
struct ButtonInput
{
    bool isAnimal;
    bool isPlayerOwned;
    bool isDownOrDead;      // dead, down, unconscious or ragdolled
    bool autoLooting;       // an AUTO LOOT session is already running for it

    ButtonInput()
        : isAnimal(false), isPlayerOwned(false), isDownOrDead(false), autoLooting(false) {}
};

struct ButtonDecision
{
    bool visible;
    bool stop;              // the click stops the running AUTO LOOT

    ButtonDecision() : visible(false), stop(false) {}
};

// Shown for a living, able, player-owned animal; "stop" while it auto loots.
ButtonDecision DecideButton(const ButtonInput& input);

// "Auto loot" to start, "Stop auto loot" while running.
const char* Caption(bool stop);
} // namespace autoloot
} // namespace totalloot
