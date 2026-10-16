#include "pickpocket_button.h"
#include "pickpocket_rules.h"
#include "pickpocket_reads.h"
#include "pickpocket_session.h"
#include "button_block.h"
#include "context_button_input.h"
#include "target_character.h"
#include "log.h"

namespace totalloot
{
namespace
{
const char* const kWidgetName = "TL_PickPocketButton";
const char* const kCaption    = "Pick pocket";

MyGUI::Button* g_button = NULL;
Character* g_target = NULL;

// setCaption builds a temporary UString, which cannot share a frame with __try.
void SetCaption()
{
    g_button->setCaption(kCaption);
}

void ApplyCaption()
{
    TL_TRY { SetCaption(); }
    TL_CATCH { }
}

void SetVisible(bool visible)
{
    if (g_button == NULL)
    {
        return;
    }
    TL_TRY { g_button->setVisible(visible); }
    TL_CATCH { }
}

// Runs once per gesture (click, or hold-and-release of the menu's own button),
// routed by context_button_input.
void OnActivated()
{
    Character* actor = SelectedCharacter();
    Character* target = g_target;
    HidePickPocketButton();

    if (actor == NULL || target == NULL)
    {
        LogInfo("Pick pocket clicked but missing selected or target.");
        return;
    }
    StartPickPocket(actor, target);
}

bool EnsureButton()
{
    if (g_button != NULL)
    {
        return true;
    }
    g_button = CreateContextButton(kWidgetName);
    if (g_button == NULL)
    {
        LogError("Could not create the Pick pocket button.");
        return false;
    }
    if (!WireContextButton(g_button, &OnActivated))
    {
        LogError("Failed to configure the Pick pocket button.");
        return false;
    }
    ApplyCaption();
    return true;
}

bool IsEligible(Character* selected, Character* target)
{
    pickpocket::EligibilityInput input;
    if (!pickpocket::ReadEligibilityInput(selected, target, &input))
    {
        return false;
    }
    return pickpocket::Evaluate(input) == pickpocket::ELIGIBLE;
}

void Destroy()
{
    TL_TRY
    {
        MyGUI::Gui* guiManager = MyGUI::Gui::getInstancePtr();
        if (guiManager != NULL)
        {
            guiManager->destroyWidget(g_button);
        }
    }
    TL_CATCH { }
}
} // namespace

MyGUI::Button* PreparePickPocketButton(Character* selected, RootObject* target)
{
    g_target = NULL;
    Character* targetCharacter = CharacterFromRootObject(target);
    if (selected == NULL || targetCharacter == NULL || selected == targetCharacter ||
        !IsEligible(selected, targetCharacter))
    {
        SetVisible(false);
        return NULL;
    }
    if (!EnsureButton())
    {
        return NULL;
    }

    g_target = targetCharacter;
    return g_button;
}

MyGUI::Button* ActivePickPocketButton()
{
    return (g_target != NULL) ? g_button : NULL;
}

void HidePickPocketButton()
{
    g_target = NULL;
    SetVisible(false);
}

void DestroyPickPocketButton()
{
    g_target = NULL;
    if (g_button == NULL)
    {
        return;
    }
    UnwireContextButton(g_button);
    Destroy();
    g_button = NULL;
}
} // namespace totalloot
