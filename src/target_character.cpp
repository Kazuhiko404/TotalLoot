#include "target_character.h"

namespace totalloot
{
namespace
{
bool SafeDataType(RootObject* object, itemType* out)
{
    TL_TRY
    {
        *out = object->getDataType();
        return true;
    }
    TL_CATCH { return false; }
}
} // namespace

Character* SelectedCharacter()
{
    TL_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return NULL;
        }
        return ou->player->selectedCharacter.getCharacter();
    }
    TL_CATCH { return NULL; }
}

Character* CharacterFromRootObject(RootObject* object)
{
    if (object == NULL)
    {
        return NULL;
    }

    itemType type = NULL_ITEM;
    if (!SafeDataType(object, &type))
    {
        return NULL;
    }

    if (type == CHARACTER || type == HUMAN_CHARACTER ||
        type == ANIMAL_CHARACTER || type == WORLDMAP_CHARACTER)
    {
        return static_cast<Character*>(object);
    }
    return NULL;
}
} // namespace totalloot
