#pragma once

#include "common.h"

namespace totalloot
{
// The "Pick pocket" context-menu button. Creates it on first use, decides
// whether it applies to `target` for the selected character, and shows nothing
// itself: the caller places it in the shared button block.
//
// Returns the button when pick pocket applies to this menu target (the caller
// must then show it), otherwise NULL and the button stays hidden.
MyGUI::Button* PreparePickPocketButton(Character* selected, RootObject* target);

// The button while Pick pocket is armed for the current menu (it was prepared
// and not yet clicked); NULL otherwise.
MyGUI::Button* ActivePickPocketButton();

// Hides the button and forgets the target.
void HidePickPocketButton();

// Unsubscribes and destroys the widget.
void DestroyPickPocketButton();
} // namespace totalloot
