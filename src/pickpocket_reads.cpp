#include "pickpocket_reads.h"

#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/AI/AITaskSystem.h>

namespace totalloot
{
namespace pickpocket
{
namespace
{
// NOTE: isUnconcious, getProneState, isAnimal, getPosition, getFloor and
// isIndoors are virtual and overridden in CharacterHuman / CharacterAnimal, so
// they are called normally (never through the _NV_ thunks).

bool ReadActorFlags(Character* actor, EligibilityInput* out)
{
    TL_TRY
    {
        out->actorIsPlayer = actor->isPlayerCharacter();
        out->actorInStealth = actor->isStealthMode();
        out->actorCanTakeOrders = actor->canTakePlayerOrdersAtThisTime();
        out->actorInCombat = actor->isInCombatMode(true, true);
        return true;
    }
    TL_CATCH { return false; }
}

bool ReadUiBlockers(EligibilityInput* out)
{
    TL_TRY
    {
        if (gui == NULL)
        {
            return false;
        }
        out->inDialogueOrModal = gui->inDialogue() || gui->hasModalMessage();
        return true;
    }
    TL_CATCH { return false; }
}

bool ReadTargetFlags(Character* target, EligibilityInput* out)
{
    TL_TRY
    {
        out->targetValid = target->isValid();
        out->targetIsPlayer = target->isPlayerCharacter();
        out->targetIsAnimal = (target->isAnimal() != NULL);
        out->targetDead = target->isDead();
        out->targetUnconscious = target->isUnconcious();
        out->targetDown = target->isDown();
        out->targetCarried = target->isBeingCarried();
        out->targetProneNormal = (target->getProneState() == PS_NORMAL);
        return true;
    }
    TL_CATCH { return false; }
}

bool SamePlace(Character* actor, Character* target, RangeInput* out)
{
    TL_TRY
    {
        out->sameFloor = (actor->getFloor() == target->getFloor());
        const hand& a = actor->isIndoors();
        const hand& b = target->isIndoors();
        out->sameBuilding = (a.isNull() && b.isNull()) || (a == b);
        return true;
    }
    TL_CATCH { return false; }
}

bool Offsets(Character* actor, Character* target, RangeInput* out,
             float* targetX, float* targetZ)
{
    TL_TRY
    {
        const Ogre::Vector3 from = actor->getPosition();
        const Ogre::Vector3 to = target->getPosition();
        out->dx = to.x - from.x;
        out->dz = to.z - from.z;
        *targetX = to.x;
        *targetZ = to.z;
        return true;
    }
    TL_CATCH { return false; }
}
} // namespace

bool ReadEligibilityInput(Character* actor, Character* target, EligibilityInput* out)
{
    *out = EligibilityInput();
    if (actor == NULL || target == NULL)
    {
        return false;
    }
    return ReadActorFlags(actor, out) &&
           ReadUiBlockers(out) &&
           ReadTargetFlags(target, out);
}

bool ReadRange(Character* actor, Character* target, RangeInput* out,
               float* targetX, float* targetZ)
{
    if (actor == NULL || target == NULL)
    {
        return false;
    }
    return Offsets(actor, target, out, targetX, targetZ) &&
           SamePlace(actor, target, out);
}

bool ReadActorState(Character* actor, bool* stealth, bool* combat)
{
    if (actor == NULL)
    {
        return false;
    }
    TL_TRY
    {
        *stealth = actor->isStealthMode();
        *combat = actor->isInCombatMode(true, true);
        return true;
    }
    TL_CATCH { return false; }
}

bool ReadTargetAlive(Character* target)
{
    if (target == NULL)
    {
        return false;
    }
    TL_TRY { return target->isValid() && !target->isDead(); }
    TL_CATCH { return false; }
}

Character* ResolveHand(const hand& h)
{
    TL_TRY { return h.getCharacter(); }
    TL_CATCH { return NULL; }
}
} // namespace pickpocket
} // namespace totalloot
