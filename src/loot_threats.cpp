#include "loot_threats.h"
#include "loot_targets.h"

namespace totalloot
{
namespace
{
void RawCollectRoster(std::vector<Character*>* out)
{
    const lektor<Character*>& roster = ou->player->getAllPlayerCharacters();
    for (unsigned int i = 0; i < roster.size(); ++i)
    {
        if (roster[i] != NULL) out->push_back(roster[i]);
    }
}

bool SafeCollectRoster(std::vector<Character*>* out)
{
    if (ou == NULL || ou->player == NULL) return false;
    TL_TRY
    {
        RawCollectRoster(out);
        return true;
    }
    TL_CATCH { return false; }
}

// False when the engine faulted or the ally is dead (a dead ally is no threat
// signal).
bool ReadAlly(Character* ally, Ogre::Vector3* pos, bool* threatened)
{
    TL_TRY
    {
        if (ally->isDead()) return false;
        *pos = ally->getPosition();
        *threatened = ally->getAllAttackersCount() > 0 || ally->isInCombatMode(true, true);
        return true;
    }
    TL_CATCH { return false; }
}
} // namespace

bool IsActorOperational(Character* actor)
{
    if (actor == NULL) return false;
    TL_TRY
    {
        return actor->isValid() && !actor->isDead() && !actor->isDown() &&
               !actor->isUnconcious() && !actor->isRagdoll() &&
               actor->getProneState() != PS_KO;
    }
    TL_CATCH { return false; }
}

bool IsInCombat(Character* actor)
{
    if (actor == NULL) return false;
    TL_TRY
    {
        return actor->getAllAttackersCount() > 0 || actor->isInCombatMode(true, true);
    }
    TL_CATCH { return false; }
}

bool NearbyAllyAttacked(Character* looter, const Ogre::Vector3& pos, float radius)
{
    std::vector<Character*> roster;
    if (!SafeCollectRoster(&roster)) return false;

    const float radiusSq = radius * radius;
    for (size_t i = 0; i < roster.size(); ++i)
    {
        Character* ally = roster[i];
        if (ally == looter) continue;

        Ogre::Vector3 allyPos;
        bool threatened = false;
        if (!ReadAlly(ally, &allyPos, &threatened)) continue;
        if (allyPos.squaredDistance(pos) > radiusSq) continue;
        if (threatened) return true;
    }
    return false;
}
} // namespace totalloot
