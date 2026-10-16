#pragma once

#include "common.h"

namespace totalloot
{
// The Character behind a context-menu target, or NULL when the object is not
// a character (cages, items, buildings, ...). Never throws.
Character* CharacterFromRootObject(RootObject* object);

// The player's currently selected character, or NULL. Never throws.
Character* SelectedCharacter();
} // namespace totalloot
