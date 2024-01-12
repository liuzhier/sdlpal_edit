#ifndef CGAMEUI_H
#define CGAMEUI_H

//#include "cscript.h"
#include "cscene.h"

class CGameUI: public CScene
{
public :

public:
    CGameUI();
    BOOL PAL_SwitchMenu(BOOL fEnabled);
    LPBOX PAL_ShowCash(DWORD dwCash);
    VOID PAL_SystemMenu_OnItemChange(WORD wCurrentItem);
    BOOL PAL_SystemMenu(VOID);
    VOID PAL_InGameMagicMenu(VOID);
    VOID PAL_InventoryMenu(VOID);
    VOID PAL_InGameMenu_OnItemChange(WORD wCurrentItem);
    VOID PAL_InGameMenu(VOID);
    VOID PAL_PlayerStatus(VOID);
    WORD PAL_ItemUseMenu(WORD wItemToUse);
    VOID PAL_BuyMenu_OnItemChange(WORD wCurrentItem);
#//ifdef  New_EDIT_SCRIPT_OPERATION_0X0034
    //
    VOID PAL_BuyMenu_OnCollectValueChange(WORD wCurrentItem);
//#endif
    VOID PAL_BuyMenu(WORD wStoreNum);
    VOID PAL_SellMenu_OnItemChange(WORD wCurrentItem);
    VOID PAL_SellMenu(VOID);
    VOID PAL_EquipItemMenu(WORD wItem);
    VOID PAL_GameUseItem(VOID);
    WORD PAL_ItemSelectMenu(LPITEMCHANGED_CALLBACK lpfnMenuItemChanged, WORD wItemFlags);
    VOID PAL_ItemSelectMenuInit(WORD wItemFlags);
    VOID PAL_PlayerInfoBox(PAL_POS pos, WORD wPlayerRole, INT iTimeMeter, BYTE bTimeMeterColor, BOOL fUpdate);
    WORD PAL_MagicSelectionMenu(WORD wPlayerRole, BOOL fInBattle, WORD wDefaultMagic);
    WORD PAL_MagicSelectionMenuUpdate(VOID);
    WORD PAL_ItemSelectMenuUpdate(VOID);
    VOID PAL_MagicSelectionMenuInit(WORD wPlayerRole, BOOL fInBattle, WORD wDefaultMagic);
    VOID PAL_GameEquipItem(VOID);
    VOID PAL_UpdateEquipments(VOID);
    VOID PAL_InitGameData(INT iSaveSlot);
    VOID PAL_Search(VOID);
#ifdef FINISH_GAME_MORE_ONE_TIME
    VOID PAL_New_GoBackAndLoadDefaultGame(VOID);
#endif
};

#endif // CGAMEUI_H
