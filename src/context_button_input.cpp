#include "context_button_input.h"
#include "click_gesture.h"
#include "log.h"

namespace totalloot
{
namespace
{
typedef MyGUI::delegates::IDelegate4<MyGUI::Widget*, int, int, MyGUI::MouseButton> MouseDelegate;

const int kMaxButtons = 4;

struct Binding
{
    MyGUI::Widget* widget;
    ContextButtonAction action;
};

Binding g_bindings[kMaxButtons];
clickgesture::State g_gesture;

ContextButtonAction ActionFor(MyGUI::Widget* sender)
{
    for (int i = 0; i < kMaxButtons; ++i)
    {
        if (g_bindings[i].widget == sender)
        {
            return g_bindings[i].action;
        }
    }
    return NULL;
}

// Left (click) and right (the menu's hold-and-release gesture) act; middle and
// wheel buttons never do.
bool IsActingButton(MyGUI::MouseButton button)
{
    return button == MyGUI::MouseButton::Left || button == MyGUI::MouseButton::Right;
}

void OnMouseEvent(MyGUI::Widget* sender, int /*left*/, int /*top*/,
                  MyGUI::MouseButton button)
{
    if (!IsActingButton(button))
    {
        return;
    }
    ContextButtonAction action = ActionFor(sender);
    if (action == NULL)
    {
        return;
    }
    const clickgesture::Verdict verdict = clickgesture::Classify(&g_gesture);
    if (verdict == clickgesture::ACT)
    {
        action();
    }
    else if (verdict == clickgesture::STRAY)
    {
        LogInfo("Context button event ignored: no armed gesture.");
    }
}

bool Remember(MyGUI::Widget* widget, ContextButtonAction action)
{
    for (int i = 0; i < kMaxButtons; ++i)
    {
        if (g_bindings[i].widget == NULL || g_bindings[i].widget == widget)
        {
            g_bindings[i].widget = widget;
            g_bindings[i].action = action;
            return true;
        }
    }
    return false;
}

// Separate delegate instances: MyGUI owns each one it is given, so the same
// pointer must not be registered on two events.
bool Subscribe(MyGUI::Button* button)
{
    TL_TRY
    {
        button->setNeedMouseFocus(true);
        button->setNeedKeyFocus(false);
        button->setVisible(false);
        button->eventMouseButtonPressed += MyGUI::newDelegate(&OnMouseEvent);
        button->eventMouseButtonReleased += MyGUI::newDelegate(&OnMouseEvent);
        return true;
    }
    TL_CATCH { return false; }
}
} // namespace

bool WireContextButton(MyGUI::Button* button, ContextButtonAction action)
{
    if (button == NULL || action == NULL || !Remember(button, action))
    {
        return false;
    }
    if (!Subscribe(button))
    {
        UnwireContextButton(button);
        return false;
    }
    return true;
}

void UnwireContextButton(MyGUI::Button* button)
{
    for (int i = 0; i < kMaxButtons; ++i)
    {
        if (g_bindings[i].widget == button)
        {
            g_bindings[i].widget = NULL;
            g_bindings[i].action = NULL;
        }
    }
}

void ArmContextButtons()
{
    clickgesture::Arm(&g_gesture);
}
} // namespace totalloot
