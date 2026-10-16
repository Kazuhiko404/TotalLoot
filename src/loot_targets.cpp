#include "loot_targets.h"
#include "loot_inventory.h"
#include "target_character.h"

#include <algorithm>

namespace totalloot
{
namespace
{
struct BodyCandidate
{
    float distSq;
    hand handle;
};

struct NearestFirst
{
    bool operator()(const BodyCandidate& a, const BodyCandidate& b) const
    {
        return a.distSq < b.distSq;
    }
};

void RawCollectWorld(std::vector<Character*>* out)
{
    const ogre_unordered_set<Character*>::type& updateList = ou->getCharacterUpdateList();
    for (ogre_unordered_set<Character*>::type::const_iterator it = updateList.begin();
         it != updateList.end(); ++it)
    {
        if (*it != NULL) out->push_back(*it);
    }
    for (ogre_unordered_map<hand, Character*>::type::const_iterator it = ou->deathParade.begin();
         it != ou->deathParade.end(); ++it)
    {
        if (it->second != NULL) out->push_back(it->second);
    }
}

// Living characters plus the death parade (bodies that left the update list).
bool SafeCollectWorld(std::vector<Character*>* out)
{
    if (ou == NULL) return false;
    TL_TRY
    {
        RawCollectWorld(out);
        return true;
    }
    TL_CATCH { return false; }
}

bool SafeGetHandle(Character* c, hand* out)
{
    TL_TRY
    {
        *out = c->getHandle();
        return true;
    }
    TL_CATCH { return false; }
}
} // namespace

bool SafeIsPlayerOwned(Character* c)
{
    if (c == NULL || ou == NULL || ou->player == NULL) return false;
    TL_TRY
    {
        const lektor<Character*>& roster = ou->player->getAllPlayerCharacters();
        for (unsigned int i = 0; i < roster.size(); ++i)
        {
            if (roster[i] == c) return true;
        }
    }
    TL_CATCH { return false; }
    return false;
}

bool SafeIsAnimal(Character* c)
{
    if (c == NULL) return false;
    TL_TRY
    {
        return c->isAnimal() != NULL;
    }
    TL_CATCH { return false; }
}

bool SafeIsDead(Character* c)
{
    if (c == NULL) return false;
    TL_TRY
    {
        if (c->isDead()) return true;
        MedicalSystem* med = c->getMedical();
        return med != NULL && med->isDead();
    }
    TL_CATCH { return false; }
}

bool SafeGetPosition(RootObject* obj, Ogre::Vector3* out)
{
    if (obj == NULL) return false;
    TL_TRY
    {
        *out = obj->getPosition();
        return true;
    }
    TL_CATCH { return false; }
}

bool SafeIsLootableCharacter(Character* c)
{
    if (c == NULL) return false;
    TL_TRY
    {
        if (c->isDestroyed()) return false;

        MedicalSystem* med = c->getMedical();

        // 1. Dead (humanoid or animal)
        if (c->isDead() || (med != NULL && med->isDead()))
        {
            return true;
        }

        // 2. Dying or severe trauma (bloodloss coma)
        if (med != NULL && (med->isProbablyDying() || med->isInBloodlossTrauma()))
        {
            return true;
        }

        // 3. Playing dead (feigning death allows looting in Kenshi)
        if (c->getProneState() == PS_PLAYING_DEAD)
        {
            return true;
        }

        // 4. Knocked out / unconscious
        if (c->getProneState() == PS_KO)
        {
            return true;
        }

        if (c->isLiterallyUnconciousNotPretending())
        {
            return true;
        }

        if (c->isUnconcious() || (med != NULL && med->isUnconcious()))
        {
            return true;
        }

        if (med != NULL && (med->knockoutTimer > 0.0f || med->isHungerKO()))
        {
            return true;
        }

        // 5. Ragdoll (picked up, tossed, in physics ragdoll)
        if (c->isRagdoll())
        {
            return true;
        }

        // Conscious crawling / crippled enemies (PS_CRIPPLED) are NOT lootable.
        return false;
    }
    TL_CATCH { return false; }
}

bool SafeIsLootableContainer(RootObject* obj)
{
    if (obj == NULL) return false;
    TL_TRY
    {
        if (CharacterFromRootObject(obj) != NULL) return false;

        Faction* fac = obj->getFaction();
        if (fac != NULL && fac->isThePlayer())
        {
            return false;
        }

        Inventory* inv = obj->getInventory();
        if (inv == NULL) return false;

        return CountItemsInInventory(inv) > 0;
    }
    TL_CATCH { return false; }
}

bool HasAnyLootableItems(RootObject* obj)
{
    if (obj == NULL) return false;
    if (CountItemsInInventory(SafeGetInventory(obj)) > 0) return true;
    Character* c = CharacterFromRootObject(obj);
    return c != NULL && CountItemsInInventory(SafeGetBackpackInventory(c)) > 0;
}

int CollectBodiesInRadius(const Ogre::Vector3& origin, float radius, Character* looter,
                          const Ogre::Vector3& sortFrom, std::vector<hand>& out)
{
    std::vector<Character*> world;
    if (!SafeCollectWorld(&world)) return 0;

    // Update list and death parade can overlap.
    std::sort(world.begin(), world.end());
    world.erase(std::unique(world.begin(), world.end()), world.end());

    const float radiusSq = radius * radius;
    std::vector<BodyCandidate> found;
    for (size_t i = 0; i < world.size(); ++i)
    {
        Character* c = world[i];
        if (c == NULL || c == looter) continue;
        if (SafeIsPlayerOwned(c)) continue;
        if (!SafeIsLootableCharacter(c)) continue;
        if (!HasAnyLootableItems(c)) continue;

        Ogre::Vector3 pos;
        if (!SafeGetPosition(c, &pos)) continue;
        if (origin.squaredDistance(pos) > radiusSq) continue;

        BodyCandidate cand;
        cand.distSq = sortFrom.squaredDistance(pos);
        if (!SafeGetHandle(c, &cand.handle)) continue;
        found.push_back(cand);
    }

    std::sort(found.begin(), found.end(), NearestFirst());
    for (size_t i = 0; i < found.size(); ++i)
    {
        out.push_back(found[i].handle);
    }
    return static_cast<int>(found.size());
}

Character* ResolveBody(const hand& handle)
{
    TL_TRY { return handle.getCharacter(); }
    TL_CATCH { return NULL; }
}

RootObject* ResolveLootTarget(const hand& handle)
{
    TL_TRY
    {
        if (handle.type == CHARACTER || handle.type == HUMAN_CHARACTER ||
            handle.type == ANIMAL_CHARACTER || handle.type == WORLDMAP_CHARACTER)
        {
            return handle.getCharacter();
        }
        return handle.getRootObject();
    }
    TL_CATCH { return NULL; }
}
} // namespace totalloot
