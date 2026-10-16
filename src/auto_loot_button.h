#pragma once

#include "common.h"

namespace totalloot
{
// The "Auto loot" / "Stop auto loot" context-menu button for player-owned
// animals. Creates it on first use and decides whether it applies to the
// right-clicked `target`; like the other TotalLoot buttons it shows nothing
// itself, the caller places it in the shared button block.
//
// Returns the button when it applies (the caller must then show it), otherwise
// NULL and the button stays hidden.
MyGUI::Button* PrepareAutoLootButton(RootObject* target);

// The button while armed for the current menu (prepared and not yet clicked);
// NULL otherwise.
MyGUI::Button* ActiveAutoLootButton();

// Hides the button and forgets the target.
void HideAutoLootButton();

// Unsubscribes and destroys the widget.
void DestroyAutoLootButton();
} // namespace totalloot
