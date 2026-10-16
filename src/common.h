#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <sstream>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Kenshi.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
#include <kenshi/CharacterAnimal.h>
#include <kenshi/CharBody.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/Faction.h>
#include <kenshi/Platoon.h>
#include <kenshi/GameData.h>
#include <kenshi/RootObject.h>
#include <kenshi/Enums.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Macros.h>
#include <mygui/MyGUI_MouseButton.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>

#include <Windows.h>

namespace totalloot
{
const char* const kSupportedGameVersion = "1.0.65";

inline std::string IntToStr(long long val)
{
    std::ostringstream ss;
    ss << val;
    return ss.str();
}
} // namespace totalloot

#define TL_TRY __try
#define TL_CATCH __except (EXCEPTION_EXECUTE_HANDLER)
