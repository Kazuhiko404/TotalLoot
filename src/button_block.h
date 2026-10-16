#pragma once

#include "common.h"
#include "button_style.h"
#include "menu_slot.h"

namespace totalloot
{
struct BlockStyle
{
    int width;
    int height;
    int gapX;
    int gapY;
};

// Block style for a menu whose options list is `menuWidth` wide.
BlockStyle MakeBlockStyle(int menuWidth);

// Creates a hidden-by-default, styled (button_style.h) root button on the Popup
// layer, trying the preferred skin then the fallback. NULL if neither works.
MyGUI::Button* CreateContextButton(const char* name);

// Places `buttons` (top to bottom) as one block in a free slot beside the
// native menu and shows them. `ioSide` carries the previously chosen side in
// and the new one out (anti-flicker between mods' per-frame hooks).
// False when the geometry could not be applied.
bool PlaceButtonBlock(const MyGUI::IntCoord& menuCoord,
                      const std::vector<MyGUI::Button*>& buttons,
                      const BlockStyle& style,
                      menuslot::Side* ioSide);
} // namespace totalloot
