#ifndef CBATTLE_H
#define CBATTLE_H

#include "cfight.h"


class CBattle:public CFight
{
public:
    CBattle();
	~CBattle() {};
	BATTLERESULT PAL_BattleMain(VOID);
	VOID PAL_BattleWon(VOID);
	VOID PAL_BattleEnemyEscape(VOID);
	VOID PAL_BattlePlayerEscape(VOID);
	BATTLERESULT PAL_StartBattle(WORD wEnemyTeam, BOOL fIsBoss);
	VOID Pal_New_RecoverAfterBattle(WORD wPlayerRole, WORD wPercent);
//#ifdef STRENGTHEN_ENEMY
	BATTLEENEMY PAL_New_StrengthenEnemy(BATTLEENEMY be);
//#endif
};

#endif // CBATTLE_H
