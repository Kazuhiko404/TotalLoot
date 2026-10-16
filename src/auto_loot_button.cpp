#include "auto_loot_button.h"
#include "auto_loot_rules.h"
#include "loot_dispatch.h"
#include "loot_targets.h"
#include "loot_threats.h"
#include "loot_walk_session.h"
#include "button_block.h"
#include "context_button_input.h"
#include "target_character.h"
#include "log.h"

namespace totalloot
{
namespace
{
const char* const kWidgetName = "TL_AutoLootButton";

MyGUI::Button* g_button = NULL;
Character* g_target = NULL;     // the animal the menu was opened on

// setCaption builds a temporary UString, which cannot share a frame with __try.
void SetCaption(const char* text)
{
    g_button->setCaption(text);
}

void ApplyCaption(const char* text)
{
    TL_TRY { SetCaption(text); }
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
    Character* animal = g_target;
    HideAutoLootButton();

    if (animal == NULL)
    {
        LogInfo("Auto loot clicked but missing animal.");
        return;
    }
    ToggleAutoLoot(animal);
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
        LogError("Could not create the Auto loot button.");
        return false;
    }
    if (!WireContextButton(g_button, &OnActivated))
    {
        LogError("Failed to configure the Auto loot button.");
        return false;
    }
    return true;
}

autoloot::ButtonDecision Decide(Character* animal)
{
    autoloot::ButtonInput input;
    input.isAnimal = SafeIsAnimal(animal);
    input.isPlayerOwned = SafeIsPlayerOwned(animal);
    input.isDownOrDead = !IsActorOperational(animal);
    input.autoLooting = IsAutoLooting(animal);
    return autoloot::DecideButton(input);
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

MyGUI::Button* PrepareAutoLootButton(RootObject* target)
{
    g_target = NULL;
    Character* animal = CharacterFromRootObject(target);
    if (animal == NULL)
    {
        SetVisible(false);
        return NULL;
    }
    const autoloot::ButtonDecision decision = Decide(animal);
    if (!decision.visible)
    {
        SetVisible(false);
        return NULL;
    }
    if (!EnsureButton())
    {
        return NULL;
    }

    ApplyCaption(autoloot::Caption(decision.stop));
    g_target = animal;
    return g_button;
}

MyGUI::Button* ActiveAutoLootButton()
{
    return (g_target != NULL) ? g_button : NULL;
}

void HideAutoLootButton()
{
    g_target = NULL;
    SetVisible(false);
}

void DestroyAutoLootButton()
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
