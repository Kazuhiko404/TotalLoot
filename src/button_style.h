#pragma once

#include "common.h"

namespace totalloot
{
// The look of every TotalLoot context-menu button, copied from TotalSparring's
// action buttons (src/spar/ui/spar_overlay.cpp:27-37 and :464-485) so the two
// mods' buttons read as one set. The skin is the stock Kenshi_Button1 (the game
// ships it; TotalSparring defines no skin of its own for these buttons), with
// the stock "Button" skin as fallback, so no resource file is needed.
const char* const kButtonLayer   = "Popup";
const char* const kPreferredSkin = "Kenshi_Button1";
const char* const kFallbackSkin  = "Button";

const int kButtonHeight   = 44;
const int kButtonGapX     = 2;
const int kButtonGapY     = 2;
const int kMinButtonWidth = 160;
const int kButtonFontHeight = 18;

// Applies font, text colour, shadow and tint, and subscribes the hover swap
// (amber fill with dark text while the cursor is over the button). False when
// the widget could not be styled.
bool StyleContextButton(MyGUI::Button* button);
} // namespace totalloot
