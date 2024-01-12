#ifndef CENEMY_H
#define CENEMY_H

#include "CGameUI.h"

class CEnemy:public CGameUI
{
public:
    CEnemy();
    VOID PAL_New_DecreaseHPForEnemy(WORD wEnemyIndex, INT iDamage);
    VOID PAL_New_CurePoisonForEnemyByKind(WORD wEnemyIndex, WORD wPoisonID);
    VOID PAL_New_CurePoisonForEnemyByLevel(WORD wEnemyIndex, WORD wMaxLevel);
    INT PAL_New_GetPoisonIndexForEnemy(WORD wEnemyIndex, WORD wPoisonID);
    VOID PAL_New_AddPoisonForEnemy(WORD wEnemyIndex, WORD wPoisonID);
    VOID PAL_New_SortPoisonsForEnemyByLevel(WORD wEnemyIndex);
    WORD PAL_New_GetEnemySorceryResistance(WORD wEnemyIndex);
    WORD PAL_New_GetEnemyPoisonResistance(WORD wEnemyIndex);
    WORD PAL_New_GetEnemyPhysicalResistance(WORD wEnemyIndex);
    WORD PAL_New_GetEnemyElementalResistance(WORD wEnemyIndex, INT iAttrib);
    VOID PAL_New_SetEnemyStatus(WORD wEnemyIndex, WORD wStatusID, WORD wNumRound);
    WORD PAL_New_GetEnemyAttackStrength(WORD wEnemyIndex);
    WORD PAL_New_GetEnemyMagicStrength(WORD wEnemyIndex);
    WORD PAL_New_GetEnemyDefense(WORD wEnemyIndex);
    WORD PAL_New_GetEnemyDexterity(WORD wEnemyIndex);
    WORD PAL_New_GetEnemyFleeRate(WORD wEnemyIndex);
    DWORD PAL_New_EstimateEnemyHealthByLevel(WORD wEnemyLevel);
    WORD PAL_GetEnemyActualDexterity(WORD wEnemyIndex);
};

#endif // CENEMY_H
