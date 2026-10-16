#pragma once

#include "common.h"

namespace totalloot
{
// Mouse input shared by every TotalLoot context-menu button, behaving like
// TotalSparring's (src/spar/ui/spar_overlay.cpp:307-417): the button listens to
// mouse pressed AND released, so the native-menu gesture (hold right button,
// slide onto the entry, release) works as well as an ordinary click. One
// gesture acts exactly once: the event that acts consumes it and the paired
// event is an echo (see click_gesture.h).

typedef void (*ContextButtonAction)();

// Makes `button` mouse-focusable, hides it, and routes its left/right
// press and release to `action`. False when the widget could not be wired.
bool WireContextButton(MyGUI::Button* button, ContextButtonAction action);

// Forgets `button` (call before destroying it).
void UnwireContextButton(MyGUI::Button* button);

// A menu with TotalLoot buttons just opened: the next press or release over a
// button is a fresh gesture. Never fires by itself, so the click that opened
// the menu cannot trigger a button unless it is released over one.
void ArmContextButtons();
} // namespace totalloot
