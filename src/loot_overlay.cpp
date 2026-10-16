#include "common.h"
#include "loot_overlay.h"
#include "loot_dispatch.h"
#include "loot_logic.h"
#include "auto_loot_button.h"
#include "button_block.h"
#include "context_button_input.h"
#include "pickpocket_button.h"
#include "target_character.h"
#include "log.h"

namespace totalloot
{
namespace
{
const char* const kButtonLootWidgetName    = "TL_LootButton";
const char* const kButtonLootAllWidgetName = "TL_LootAllButton";

const char* const kLootCaption    = "Loot";
const char* const kLootAllCaption = "Loot all";

MyGUI::Button* g_buttonLoot = NULL;
MyGUI::Button* g_buttonLootAll = NULL;

RootObject* g_targetObject = NULL;
menuslot::Side g_previousSide = menuslot::SIDE_NONE;
bool g_lootActive = false;   // Loot / Loot all apply to the current menu target

void (*g_showContextMenu_orig)(ContextMenu*, bool, RootObject*) = NULL;
void (*g_contextMenuUpdate_orig)(ContextMenu*) = NULL;

void ApplyCaption(MyGUI::TextBox* widget, const char* text)
{
    widget->setCaption(text);
}

void SafeSetCaption(MyGUI::TextBox* widget, const char* text)
{
    TL_TRY { ApplyCaption(widget, text); }
    TL_CATCH { }
}

void ClearTarget()
{
    g_targetObject = NULL;
    g_lootActive = false;
    g_previousSide = menuslot::SIDE_NONE;
}

void HideButtons()
{
    if (g_buttonLoot != NULL)
    {
        TL_TRY { g_buttonLoot->setVisible(false); }
        TL_CATCH { }
    }
    if (g_buttonLootAll != NULL)
    {
        TL_TRY { g_buttonLootAll->setVisible(false); }
        TL_CATCH { }
    }
}

void OnLootClicked()
{
    Character* selected = SelectedCharacter();
    RootObject* target = g_targetObject;

    HideButtons();
    ClearTarget();

    if (selected == NULL || target == NULL)
    {
        LogInfo("Loot clicked but missing selected or target.");
        return;
    }

    LogInfo("Loot clicked (single target).");
    RequestLootSingle(selected, target);
}

void OnLootAllClicked()
{
    Character* selected = SelectedCharacter();
    RootObject* target = g_targetObject;

    HideButtons();
    ClearTarget();

    if (selected == NULL || target == NULL)
    {
        LogInfo("Loot all clicked but missing selected or target.");
        return;
    }

    LogInfo("Loot all clicked (area loot).");
    RequestLootArea(selected, target);
}

bool ConfigureButtons()
{
    return WireContextButton(g_buttonLoot, &OnLootClicked) &&
           WireContextButton(g_buttonLootAll, &OnLootAllClicked);
}

bool CreateButtons()
{
    if (g_buttonLoot != NULL && g_buttonLootAll != NULL)
    {
        return true;
    }

    if (g_buttonLoot == NULL)
    {
        g_buttonLoot = CreateContextButton(kButtonLootWidgetName);
    }
    if (g_buttonLootAll == NULL)
    {
        g_buttonLootAll = CreateContextButton(kButtonLootAllWidgetName);
    }

    if (g_buttonLoot == NULL || g_buttonLootAll == NULL)
    {
        LogError("Could not create Loot / Loot all buttons with candidate skins.");
        return false;
    }

    if (!ConfigureButtons())
    {
        LogError("Failed to configure Loot / Loot all buttons.");
        return false;
    }

    SafeSetCaption(g_buttonLoot, kLootCaption);
    SafeSetCaption(g_buttonLootAll, kLootAllCaption);
    return true;
}

void DestroyWidget(MyGUI::Widget* widget)
{
    MyGUI::Gui* instance = MyGUI::Gui::getInstancePtr();
    if (instance != NULL)
    {
        instance->destroyWidget(widget);
    }
}

void SafeDestroyWidget(MyGUI::Widget* widget)
{
    TL_TRY { DestroyWidget(widget); }
    TL_CATCH { }
}

void DestroyButtons()
{
    if (g_buttonLoot == NULL && g_buttonLootAll == NULL)
    {
        return;
    }

    UnwireContextButton(g_buttonLoot);
    UnwireContextButton(g_buttonLootAll);
    if (g_buttonLoot != NULL)
    {
        SafeDestroyWidget(g_buttonLoot);
        g_buttonLoot = NULL;
    }
    if (g_buttonLootAll != NULL)
    {
        SafeDestroyWidget(g_buttonLootAll);
        g_buttonLootAll = NULL;
    }
}

bool SafeReadOptionsCoord(ContextMenu* menu, MyGUI::IntCoord* out)
{
    const size_t kOptionsListOffset = 0xF8;

    TL_TRY
    {
        void* menuGUI = menu->menuGUI;
        if (menuGUI == NULL)
        {
            return false;
        }

        MyGUI::Widget* optionsList = *reinterpret_cast<MyGUI::Widget**>(
            reinterpret_cast<char*>(menuGUI) + kOptionsListOffset);
        if (optionsList == NULL)
        {
            return false;
        }
        *out = optionsList->getAbsoluteCoord();
        return true;
    }
    TL_CATCH { return false; }
}

// Places every active TotalLoot button (Loot, Loot all, Pick pocket, Auto loot) as one
// block in a free slot beside the native menu's options list (left, right, or
// stacked below foreign buttons). Which buttons are active is decided when the
// menu opens and never changes the Loot / Loot all rules.
bool AnchorToMenu(ContextMenu* menu)
{
    if (menu == NULL)
    {
        return false;
    }

    MyGUI::IntCoord optionsCoord;
    if (!SafeReadOptionsCoord(menu, &optionsCoord) || optionsCoord.width <= 0)
    {
        return false;
    }

    std::vector<MyGUI::Button*> block;
    if (g_lootActive && g_buttonLoot != NULL && g_buttonLootAll != NULL)
    {
        block.push_back(g_buttonLoot);
        block.push_back(g_buttonLootAll);
    }
    MyGUI::Button* pickPocket = ActivePickPocketButton();
    if (pickPocket != NULL)
    {
        block.push_back(pickPocket);
    }
    MyGUI::Button* autoLoot = ActiveAutoLootButton();
    if (autoLoot != NULL)
    {
        block.push_back(autoLoot);
    }
    if (block.empty())
    {
        return false;
    }

    return PlaceButtonBlock(optionsCoord, block,
                            MakeBlockStyle(optionsCoord.width), &g_previousSide);
}

void HideAll()
{
    HideButtons();
    HidePickPocketButton();
    HideAutoLootButton();
    ClearTarget();
}

void ContextMenu_showContextMenu_hook(ContextMenu* thisptr, bool on, RootObject* what)
{
    if (g_showContextMenu_orig != NULL)
    {
        g_showContextMenu_orig(thisptr, on, what);
    }

    if (!on || what == NULL)
    {
        HideAll();
        return;
    }

    Character* selected = SelectedCharacter();
    std::string reason;
    const bool lootable = IsTargetLootable(selected, what, &reason);
    MyGUI::Button* pickPocket = PreparePickPocketButton(selected, what);
    MyGUI::Button* autoLoot = PrepareAutoLootButton(what);

    HideButtons();
    ClearTarget();
    if (!lootable && pickPocket == NULL && autoLoot == NULL)
    {
        return;
    }

    g_targetObject = what;
    g_lootActive = lootable && CreateButtons();
    if (g_lootActive)
    {
        SafeSetCaption(g_buttonLoot, kLootCaption);
        SafeSetCaption(g_buttonLootAll, kLootAllCaption);
    }
    else if (pickPocket == NULL && autoLoot == NULL)
    {
        ClearTarget();
        return;
    }

    ArmContextButtons();
    AnchorToMenu(thisptr);
}

void ContextMenu_update_hook(ContextMenu* thisptr)
{
    if (g_contextMenuUpdate_orig != NULL)
    {
        g_contextMenuUpdate_orig(thisptr);
    }

    if (g_targetObject == NULL || thisptr == NULL)
    {
        return;
    }

    bool visible = false;
    TL_TRY { visible = thisptr->isVisible(); }
    TL_CATCH { visible = false; }

    if (!visible)
    {
        HideAll();
        return;
    }

    AnchorToMenu(thisptr);
}
} // namespace

bool InstallLootOverlay()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ContextMenu::showContextMenu),
            ContextMenu_showContextMenu_hook,
            &g_showContextMenu_orig))
    {
        LogError("Failed to hook ContextMenu::showContextMenu.");
        return false;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ContextMenu::update),
            ContextMenu_update_hook,
            &g_contextMenuUpdate_orig))
    {
        LogError("Failed to hook ContextMenu::update.");
        return false;
    }

    LogInfo("Loot overlay installed with Loot & Loot all buttons.");
    return true;
}

void ShutdownLootOverlay()
{
    ClearTarget();
    DestroyButtons();
    DestroyPickPocketButton();
    DestroyAutoLootButton();
}
} // namespace totalloot
