#pragma once

#include "common.h"
#include "menu_slot.h"

namespace totalloot
{
// Screen size from MyGUI's render manager. False when unavailable.
bool ReadScreenSize(int* outWidth, int* outHeight);

// Rects of visible foreign root widgets (other mods' context-menu buttons)
// that a button block must not cover. Scans every MyGUI root widget and also
// looks up the known overlay buttons by name. Skips our own "TL_" widgets and
// anything IsPlausibleObstacle rejects (the native menu, HUD roots).
void CollectObstacles(const menuslot::Rect& menu, std::vector<menuslot::Rect>* out);
} // namespace totalloot
