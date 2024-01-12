#ifndef CUIBATTLE_H
#define CUIBATTLE_H

#include "cbattlebase.h"
//#include "csdlscreen.h"

class CUIBattle: public CBattleBase
{
public:
    CUIBattle();
    BOOL PAL_BattleUIIsActionValid(BATTLEUIACTION ActionType);
    VOID PAL_BattleUIDrawMiscMenu(WORD wCurrentItem, BOOL fConfirmed);
    WORD PAL_BattleUIMiscMenuUpdate(VOID);
    WORD PAL_BattleUIMiscItemSubMenuUpdate(VOID);
    VOID PAL_BattleUIShowText(LPCSTR lpszText, WORD wDuration);
    VOID PAL_BattleUIPlayerReady(WORD wPlayerIndex);
    VOID PAL_BattleUIUseItem(VOID);
    VOID PAL_BattleUIThrowItem(VOID);
    WORD PAL_BattleUIPickAutoMagic(WORD wPlayerRole, WORD wRandomRange);
//#ifdef SHOW_DATA_IN_BATTLE
    VOID PAL_New_BattleUIShowData(VOID);
//#endif
#ifdef SHOW_ENEMY_STATUS
    VOID PAL_New_EnemyStatus(VOID);
#endif
    VOID PAL_BattleUIShowNum(WORD wNum, PAL_POS pos, NUMCOLOR color);
};

#endif // CUIBATTLE_H
