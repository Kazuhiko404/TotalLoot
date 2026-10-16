#include "menu_obstacles.h"

#include <algorithm>

namespace totalloot
{
namespace
{
const char* const kOwnWidgetPrefix = "TL_";

// Overlay buttons known to sit beside the context menu. The root enumeration
// should already find them; the by-name lookup is a second path in case a
// widget is parented oddly.
const char* const kKnownWidgetNames[] =
{
    "TS_SparringButton",
    "TS_ProfileButton",
    "TR_RecruitButton",
    "TP_EatButton",
    "TP_LootButton",
    "TP_AutoLootButton",
};

bool SafeScreenSize(int* outWidth, int* outHeight)
{
    TL_TRY
    {
        MyGUI::RenderManager* render = MyGUI::RenderManager::getInstancePtr();
        if (render == NULL)
        {
            return false;
        }
        const MyGUI::IntSize& size = render->getViewSize();
        *outWidth = size.width;
        *outHeight = size.height;
        return true;
    }
    TL_CATCH { return false; }
}

bool SafeSnapshotRoots(std::vector<MyGUI::Widget*>* out)
{
    TL_TRY
    {
        MyGUI::Gui* guiManager = MyGUI::Gui::getInstancePtr();
        if (guiManager == NULL)
        {
            return false;
        }
        MyGUI::EnumeratorWidgetPtr roots = guiManager->getEnumerator();
        while (roots.next())
        {
            out->push_back(roots.current());
        }
        return true;
    }
    TL_CATCH { return false; }
}

MyGUI::Widget* SafeFindByName(const std::string& name)
{
    TL_TRY
    {
        MyGUI::Gui* guiManager = MyGUI::Gui::getInstancePtr();
        if (guiManager == NULL)
        {
            return NULL;
        }
        return guiManager->findWidgetT(name, false);
    }
    TL_CATCH { return NULL; }
}

// Reads a visible, non-TL_ widget's absolute rect.
bool SafeReadForeignRect(MyGUI::Widget* widget, menuslot::Rect* out)
{
    TL_TRY
    {
        if (widget == NULL || !widget->getVisible())
        {
            return false;
        }
        const std::string& name = widget->getName();
        if (name.compare(0, 3, kOwnWidgetPrefix) == 0)
        {
            return false;
        }
        const MyGUI::IntCoord coord = widget->getAbsoluteCoord();
        *out = menuslot::Rect(coord.left, coord.top, coord.width, coord.height);
        return true;
    }
    TL_CATCH { return false; }
}

void AddCandidates(const std::vector<MyGUI::Widget*>& widgets,
                   const menuslot::Rect& menu,
                   std::vector<MyGUI::Widget*>* seen,
                   std::vector<menuslot::Rect>* out)
{
    for (size_t i = 0; i < widgets.size(); ++i)
    {
        MyGUI::Widget* widget = widgets[i];
        if (widget == NULL ||
            std::find(seen->begin(), seen->end(), widget) != seen->end())
        {
            continue;
        }
        seen->push_back(widget);

        menuslot::Rect rect;
        if (SafeReadForeignRect(widget, &rect) &&
            menuslot::IsPlausibleObstacle(rect, menu))
        {
            out->push_back(rect);
        }
    }
}
} // namespace

bool ReadScreenSize(int* outWidth, int* outHeight)
{
    return SafeScreenSize(outWidth, outHeight);
}

void CollectObstacles(const menuslot::Rect& menu, std::vector<menuslot::Rect>* out)
{
    out->clear();

    std::vector<MyGUI::Widget*> candidates;
    SafeSnapshotRoots(&candidates);

    const size_t knownCount = sizeof(kKnownWidgetNames) / sizeof(kKnownWidgetNames[0]);
    for (size_t i = 0; i < knownCount; ++i)
    {
        MyGUI::Widget* widget = SafeFindByName(std::string(kKnownWidgetNames[i]));
        if (widget != NULL)
        {
            candidates.push_back(widget);
        }
    }

    std::vector<MyGUI::Widget*> seen;
    AddCandidates(candidates, menu, &seen, out);
}
} // namespace totalloot
