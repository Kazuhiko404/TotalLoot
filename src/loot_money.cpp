#include "loot_money.h"

#include <cctype>

namespace totalloot
{
bool IsMoneyItem(Item* item)
{
    if (item == NULL) return false;
    try
    {
        if (item->itemFunction == ITEM_MONEY) return true;
        std::string name = item->getName();
        if (_stricmp(name.c_str(), "cats") == 0) return true;
        GameData* gd = item->getGameData();
        if (gd != NULL)
        {
            if (!gd->name.empty() && _stricmp(gd->name.c_str(), "cats") == 0) return true;
            if (!gd->stringID.empty())
            {
                std::string sid = gd->stringID;
                for (size_t i = 0; i < sid.size(); ++i)
                {
                    sid[i] = static_cast<char>(tolower(static_cast<unsigned char>(sid[i])));
                }
                if (sid.find("cats") != std::string::npos) return true;
            }
        }
    }
    catch (...)
    {
    }
    return false;
}

int GetMoneyItemValue(Item* item)
{
    if (item == NULL) return 0;
    int qty = item->quantity > 0 ? item->quantity : 1;
    int singleVal = 1;
    try
    {
        singleVal = item->getValueSingle(true);
    }
    catch (...)
    {
        singleVal = 1;
    }
    if (singleVal <= 0) singleVal = 1;
    return qty * singleVal;
}

bool CreditPlayerCats(Character* looter, int amount)
{
    if (amount <= 0) return false;
    bool ok = false;
    try
    {
        if (looter != NULL)
        {
            Ownerships* own = looter->getOwnerships();
            if (own != NULL)
            {
                own->addMoney(amount);
                ok = true;
            }
        }
        if (!ok && ou != NULL && ou->player != NULL)
        {
            Platoon* p = ou->player->getCurrentPlatoon();
            if (p != NULL)
            {
                Ownerships* own = p->getOwnerships();
                if (own != NULL)
                {
                    own->addMoney(amount);
                    ok = true;
                }
            }
        }
    }
    catch (...)
    {
        ok = false;
    }
    return ok;
}
} // namespace totalloot
