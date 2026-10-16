#include "auto_loot_rules.h"

namespace totalloot
{
namespace autoloot
{
ButtonDecision DecideButton(const ButtonInput& input)
{
    ButtonDecision decision;
    decision.visible = input.isAnimal && input.isPlayerOwned && !input.isDownOrDead;
    decision.stop = decision.visible && input.autoLooting;
    return decision;
}

const char* Caption(bool stop)
{
    return stop ? "Stop auto loot" : "Auto loot";
}
} // namespace autoloot
} // namespace totalloot
