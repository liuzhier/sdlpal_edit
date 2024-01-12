//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "cuibattle.h"

CUIBattle::CUIBattle()
{

}




BOOL CUIBattle::PAL_BattleUIIsActionValid(
	BATTLEUIACTION         ActionType
)
/*++
  Purpose:    Check if the specified action is valid.
  功能：  检查选定的动作是否有效。
  Parameters:    [IN]  ActionType - the type of the action.
  动作类型
  Return value:    TRUE if the action is valid, FALSE if not.
  如果动作有效则为TRUE，否则为FALSE。
  --*/
{
	WORD     wPlayerRole, w;
	int      i;

	wPlayerRole = gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole;

	switch (ActionType)
	{
	case kBattleUIActionAttack:		// 直接攻击
	case kBattleUIActionMisc:		// 杂项
		break;

	case kBattleUIActionMagic:		// 仙术
		if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] != 0)
		{ // 如果角色状态为咒封，则不能使用仙术
			return FALSE;
		}
		break;

	case kBattleUIActionCoopMagic:// 合体魔法
	{
		if (gpGlobals->wMaxPartyMemberIndex == 0)
		{		// 只有一个角色时，不能使用合体魔法
			return FALSE;
		}
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;

#ifndef PAL_CLASSIC
			if (gpGlobals->g.PlayerRoles.rgwHP[w] < gpGlobals->g.PlayerRoles.rgwMaxHP[w] / 5 ||
				gpGlobals->rgPlayerStatus[w][kStatusSleep] != 0 ||
				gpGlobals->rgPlayerStatus[w][kStatusConfused] != 0 ||
				gpGlobals->rgPlayerStatus[w][kStatusSilence] != 0 ||
				g_Battle.rgPlayer[i].flTimeMeter < 100 ||
				g_Battle.rgPlayer[i].state == kFighterAct)
#else
			if (PAL_IsPlayerDying(w) || !PAL_New_IfPlayerCanMove(w))
#endif
			{
				return FALSE;
			}
		}
		break;
	}
	}

	return TRUE;
}

VOID CUIBattle::PAL_BattleUIDrawMiscMenu(
	WORD       wCurrentItem,
	BOOL       fConfirmed
)
/*++
  Purpose:    Draw the misc menu.
  画出杂项菜单。
  Parameters:    [IN]  wCurrentItem - the current selected menu item.
  现在选中的菜单项。
  [IN]  fConfirmed - TRUE if confirmed, FALSE if not.
  如果确定值为TRUE，否则为FALSE。
  Return value:    None.
  --*/
{
	INT			i, x, y;
	BYTE		bColor;
	BYTE		bItemNum;


	MENUITEM rgMenuItem[10]{ 0 };
#ifdef PAL_CLASSIC
	{
		i = 0;
		x = 16;
		y = 14;
		rgMenuItem[i-1] = { i++, BATTLEUI_LABEL_AUTO, TRUE, PAL_XY(x, y + (i * 18)) };	//围攻
		rgMenuItem[i-1] = { i++, BATTLEUI_LABEL_INVENTORY, TRUE, PAL_XY(x, y + (i * 18)) };//道具
		rgMenuItem[i-1] = { i++, BATTLEUI_LABEL_DEFEND, TRUE, PAL_XY(x, y + (i * 18)) };	//防御
		rgMenuItem[i-1] = { i++, BATTLEUI_LABEL_FLEE, TRUE, PAL_XY(x, y + (i * 18)) };	//逃跑
		rgMenuItem[i-1] = { i++, BATTLEUI_LABEL_STATUS, TRUE, PAL_XY(x, y + (i * 18)) };	//状态
if(gConfig->m_Function_Set[2])//#ifdef SHOW_DATA_IN_BATTLE
		rgMenuItem[i-1] = { i++, BATTLEUI_LABEL_DATA , TRUE, PAL_XY(x, y + (i * 18)) };	//数据
//#endif	
#ifdef SHOW_ENEMY_STATUS
		rgMenuItem[i-1] = { i++, BATTLEUI_LABEL_ENEMY_STATUS, TRUE, PAL_XY(x, y + (i * 18)) };//敌方状态
#endif
	};
#else
	MENUITEM rgMenuItem[] = {
		// value   label                   enabled   position
			{  0,      BATTLEUI_LABEL_ITEM,    TRUE,     PAL_XY(16, 32)  },
			{  1,      BATTLEUI_LABEL_DEFEND,  TRUE,     PAL_XY(16, 50)  },
			{  2,      BATTLEUI_LABEL_AUTO,    TRUE,     PAL_XY(16, 68)  },
			{  3,      BATTLEUI_LABEL_FLEE,    TRUE,     PAL_XY(16, 86)  },
			{  4,      BATTLEUI_LABEL_STATUS,  TRUE,     PAL_XY(16, 104) }
	};
#endif

	bItemNum = i;// sizeof(rgMenuItem) / sizeof(MENUITEM);
	//
	// Draw the box
	//
	PAL_CreateBox(PAL_XY(2, 20), bItemNum - 1, 1, 0, FALSE);
		 //
		 // Draw the menu items
		 //画出菜单项
	for (i = 0; i < bItemNum; i++)
	{
		bColor = MENUITEM_COLOR;

		if (i == wCurrentItem)
		{
			if (fConfirmed)
			{
				bColor = MENUITEM_COLOR_CONFIRMED;
			}
			else
			{
				bColor = MENUITEM_COLOR_SELECTED;
			}
		}

		PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor, TRUE, FALSE);
	}
}

WORD CUIBattle::PAL_BattleUIMiscMenuUpdate(
	VOID
)
/*++
  Purpose:    Update the misc menu.
  更新杂项菜单。
  Parameters:    None.
  Return value:    The selected item number. 0 if cancelled, 0xFFFF if not confirmed.
  选中的菜单项序号。如果取消则为0，如果未确定则为0xFFF。
  --*/
{
	int iMenuItemMaxIndex = 4;

if(gConfig->m_Function_Set[2])////#ifdef SHOW_DATA_IN_BATTLE
	iMenuItemMaxIndex++;
//#endif

#ifdef SHOW_ENEMY_STATUS
	iMenuItemMaxIndex++;
#endif

	//
	// Draw the menu
	//
	PAL_BattleUIDrawMiscMenu(g_iCurMiscMenuItem, FALSE);

	//
	// Process inputs
	//
	if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
	{// 按上键和左键时菜单项序号减一
		g_iCurMiscMenuItem--;
		if (g_iCurMiscMenuItem < 0)
		{
			g_iCurMiscMenuItem = iMenuItemMaxIndex;
		}
	}
	else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
	{  // 按下键和右键时菜单项序号加一
		g_iCurMiscMenuItem++;
		if (g_iCurMiscMenuItem > iMenuItemMaxIndex) // 当菜单项序号最大时，再按下键或者右键其序号变为最最小
		{
			g_iCurMiscMenuItem = 0;
		}
	}
	else if (g_InputState.dwKeyPress & kKeySearch)
	{ // 按下左CTRL键时相当于回车键
		return g_iCurMiscMenuItem + 1;
	}
	else if (g_InputState.dwKeyPress & kKeyMenu)
	{ // 按下菜单键退出杂项菜单
		return 0;
	}

	return 0xFFFF;
}

WORD CUIBattle::PAL_BattleUIMiscItemSubMenuUpdate(
	VOID
)
/*++
  Purpose:    Update the item sub menu of the misc menu.
  更新杂项菜单的子菜单。
  Parameters:    None.
  Return value:    The selected item number. 0 if cancelled, 0xFFFF if not confirmed.
  选中的菜单项序号。如果取消则为0，如果未确定则为0xFFF。
  --*/
{
	int             i;
	BYTE            bColor;

	MENUITEM rgMenuItem[] = {
		// value   label                      enabled   position
			{0, BATTLEUI_LABEL_USEITEM, TRUE, PAL_XY(44, 62)},
			{1, BATTLEUI_LABEL_THROWITEM, TRUE, PAL_XY(44, 80)},
	};

	//
	// Draw the menu
	//
#ifdef PAL_CLASSIC
	PAL_BattleUIDrawMiscMenu(1, TRUE);
#else
	PAL_BattleUIDrawMiscMenu(0, TRUE);
#endif
	PAL_CreateBox(PAL_XY(30, 50), 1, 1, 0, FALSE);

	//
	// Draw the menu items
	//
	for (i = 0; i < 2; i++)
	{
		bColor = MENUITEM_COLOR;

		if (i == g_iCurSubMenuItem)
		{
			bColor = MENUITEM_COLOR_SELECTED;
		}

		PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor,
			TRUE, FALSE);
	}

	//
	// Process inputs
	//
	if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
	{
		g_iCurSubMenuItem = 0;
	}
	else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
	{
		g_iCurSubMenuItem = 1;
	}
	else if (g_InputState.dwKeyPress & kKeySearch)
	{
		return g_iCurSubMenuItem + 1;
	}
	else if (g_InputState.dwKeyPress & kKeyMenu)
	{
		return 0;
	}

	return 0xFFFF;
}

VOID CUIBattle::PAL_BattleUIShowText(
	LPCSTR        lpszText,
	WORD          wDuration
)
/*++
  Purpose:    Show a text message in the battle.
  显示战斗中的文本信息。
  Parameters:    [IN]  lpszText - the text message to be shown.
  要显示的文本信息。
  [IN]  wDuration - the duration of the message, in milliseconds.
  信息的持续时间，以毫秒为单位
  Return value:    None.
  --*/
{
	if (SDL_GetTicks_New() < g_Battle.UI.dwMsgShowTime)
	{
		strcpy(g_Battle.UI.szNextMsg, lpszText);
		g_Battle.UI.wNextMsgDuration = wDuration;
	}
	else
	{
		strcpy(g_Battle.UI.szMsg, lpszText);
		g_Battle.UI.dwMsgShowTime = SDL_GetTicks_New() + wDuration;
	}
}

VOID CUIBattle::PAL_BattleUIPlayerReady(
	WORD          wPlayerIndex
)
/*++
  Purpose:

  Start the action selection menu of the specified player.
  开始选定角色的动作选择菜单。
  Parameters:    [IN]  wPlayerIndex - the player index.
  角色的序号
  Return value:    None.
  --*/
{
#ifndef PAL_CLASSIC
	WORD w = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
#endif

	g_Battle.UI.wCurPlayerIndex = wPlayerIndex;
	g_Battle.UI.state = kBattleUISelectMove;
	g_Battle.UI.wSelectedAction = 0;
	g_Battle.UI.MenuState = kBattleMenuMain;

#ifndef PAL_CLASSIC
	//
	// Play a sound which indicates the player is ready
	//
	if (gpGlobals->rgPlayerStatus[w][kStatusPuppet] == 0 &&
		gpGlobals->rgPlayerStatus[w][kStatusSleep] == 0 &&
		gpGlobals->rgPlayerStatus[w][kStatusConfused] == 0 &&
		!g_Battle.UI.fAutoAttack && !gpGlobals->fAutoBattle)
	{
		SOUND_PlayChannel(78, 1);
	}
#endif
}

VOID CUIBattle::PAL_BattleUIUseItem(
	VOID
)
/*++
  Purpose:    Use an item in the battle UI.
  在战斗界面中使用物品。
  Parameters:    None.
  Return value:   None.
  --*/
{
	WORD       wSelectedItem;
	// 初始化物品选择菜单
	wSelectedItem = PAL_ItemSelectMenuUpdate();

	if (wSelectedItem != 0xFFFF)
	{
		if (wSelectedItem != 0)
		{
			g_Battle.UI.wActionType = kBattleActionUseItem;
			g_Battle.UI.wObjectID = wSelectedItem;
			// 如果选择的使用物品是应用于全部玩家角色
			if (((LPOBJECT)((LPOBJECT)gpGlobals->g.rgObject))[wSelectedItem].item.wFlags & kItemFlagApplyToAll)
			{
				g_Battle.UI.state = kBattleUISelectTargetPlayerAll;
			}
			else // 如果选择的使用物品不是应用于全部玩家角色
			{
#ifdef PAL_CLASSIC
				g_Battle.UI.wSelectedIndex = 0;
#else
				g_Battle.UI.wSelectedIndex = g_Battle.UI.wCurPlayerIndex;
#endif
				g_Battle.UI.state = kBattleUISelectTargetPlayer; // 界面的选择状态为选择目标玩家
			}
		}
		else
		{
			g_Battle.UI.MenuState = kBattleMenuMain;
		}
	}
}

VOID CUIBattle::PAL_BattleUIThrowItem(
	VOID
)
/*++
  Purpose:    Throw an item in the battle UI.
  在战斗界面中投掷物品。
  Parameters:    None.
  Return value:    None.
  --*/
{
	WORD wSelectedItem = PAL_ItemSelectMenuUpdate();
	// 初始化物品选择菜单
	if (wSelectedItem != 0xFFFF)
	{
		if (wSelectedItem != 0)
		{
			g_Battle.UI.wActionType = kBattleActionThrowItem;// 动作状态为投掷物品
			g_Battle.UI.wObjectID = wSelectedItem;
			// 如果选择的投掷物品是应用于全部敌人
			if (((LPOBJECT)gpGlobals->g.rgObject)[wSelectedItem].item.wFlags & kItemFlagApplyToAll)
			{
				g_Battle.UI.state = kBattleUISelectTargetEnemyAll;
			}
			else
			{ // 如果选择的投掷物品是应用于单个敌人
				g_Battle.UI.wSelectedIndex = g_Battle.UI.wPrevEnemyTarget;
				g_Battle.UI.state = kBattleUISelectTargetEnemy;
			}
		}
		else
		{
			g_Battle.UI.MenuState = kBattleMenuMain;
		}
	}
}

WORD CUIBattle::PAL_BattleUIPickAutoMagic(
	WORD          wPlayerRole,
	WORD          wRandomRange
)
/*++
  Purpose:    Pick a magic for the specified player for automatic usage.
  为选定的玩家选择一个自动释放的魔法。
  Parameters:    [IN]  wPlayerRole - the player role ID.
  玩家角色的序号
  [IN]  wRandomRange - the range of the magic power.
  魔法威力的排序
  Return value:    The object ID of the selected magic. 0 for physical attack.
  选择的魔法的序号。0表示物理攻击。
  --*/
{
	WORD             wMagic = 0, w, wMagicNum;
	int              i, iMaxPower = 0, iPower;

	if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] != 0)
	{
		return 0;
	}

	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		w = gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole];
		if (w == 0)
		{
			continue;
		}

		wMagicNum = ((LPOBJECT)gpGlobals->g.rgObject)[w].magic.wMagicNumber;

		//
		// skip if the magic is an ultimate move or not enough MP
		//如果该仙术的基础伤害值小于0，或此仙术消耗真气为1，或者角色现有真气不足，则跳过
		if (gpGlobals->g.lprgMagic[wMagicNum].wCostMP == 1 ||
			gpGlobals->g.lprgMagic[wMagicNum].wCostMP > gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] ||
			(SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) <= 0)
		{
			continue;
		}

		//如果该仙术消耗金钱，跳过
		{
			WORD  wEntryObject = ((LPOBJECT)gpGlobals->g.rgObject)[w].magic.wScriptOnUse;
			if (wEntryObject)
			{
				if (gpGlobals->g.lprgScriptEntry[wEntryObject].wOperation == 0x001E)
					continue;
			}
		}

		iPower = (SHORT)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) +
			RandomLong(0, wRandomRange);

		if (iPower > iMaxPower)
		{
			iMaxPower = iPower;
			wMagic = w;
		}
	}

	return wMagic;
}



//#ifdef SHOW_DATA_IN_BATTLE
VOID CUIBattle::PAL_New_BattleUIShowData(
	VOID
)
/*++
Purpose: 在战斗中显示一些数据。
--*/
{
	int              i, j;
	WORD             wPlayerRole, w;

	//显示敌人血量
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}
		PAL_DrawNumber(g_Battle.rgEnemy[i].dwActualHealth, 6, PAL_XY(40 * i, 0), kNumColorYellow, kNumAlignRight);

		//灵壶值wCollectValue
		if (g_Battle.rgEnemy[i].e.wCollectValue)
		{
			PAL_DrawNumber(((SHORT)g_Battle.rgEnemy[i].e.wCollectValue)
				, 5, PAL_XY(i * 40, 12), kNumColorYellow, kNumAlignRight);
		}

		//不能移动轮次
		if (PAL_New_EnemyNotMoved(i))
			PAL_DrawNumber(((SHORT)PAL_New_EnemyNotMoved(i))
				, 5, PAL_XY(i * 40, 24), kNumColorYellow, kNumAlignRight);


	}

	//显示我方的对各属性仙术的抗性
	int startPos = 320 - 20 * (gpGlobals->wMaxPartyMemberIndex + 1);
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
		for (j = 0; j < NUM_MAGIC_ELEMENTAL; j++)
		{
			w = PAL_GetPlayerElementalResistance(wPlayerRole, j);
			PAL_DrawNumber(w, 3, PAL_XY(startPos + 20 * i, 10 * j), kNumColorYellow, kNumAlignRight);
		}

		w = PAL_GetPlayerPoisonResistance(wPlayerRole);
		PAL_DrawNumber(w, 3, PAL_XY(startPos + 20 * i, 10 * j), kNumColorYellow, kNumAlignRight);

		j++;
		w = PAL_New_GetPlayerSorceryResistance(wPlayerRole);
		PAL_DrawNumber(w, 3, PAL_XY(startPos + 20 * i, 10 * j), kNumColorYellow, kNumAlignRight);

		//j++;
		//w = PAL_New_GetPlayerSorceryStrength(wPlayerRole);
		//PAL_DrawNumber(w, 3, PAL_XY(startPos + 20 * i, 10 * j), kNumColorYellow, kNumAlignRight);
	}

	//显示我方有益状态的剩余轮次
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
		for (j = kStatusPuppet; j < kStatusAll; j++)
		{
			w = gpGlobals->rgPlayerStatus[wPlayerRole][j];
			PAL_DrawNumber(min(99, w), 3, PAL_XY(startPos + 20 * i,
				78 + 10 * (j - kStatusPuppet)), kNumColorYellow, kNumAlignRight);
		}
		//显示不能动轮次
		//PAL_New_IfPlayerCanMove(w);
		w = 0;
		w = max(gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep], w);
		w = max(gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed], w);
		w = max(gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused], w);
		PAL_DrawNumber(min(99, w), 3, PAL_XY(startPos + 20 * i,
			128), kNumColorBlue, kNumAlignRight);


#ifdef  Show_New_Hiden_exp
#ifdef  _DEBUG
		//count
		PAL_DrawNumber(((SHORT)PAL_GetExpCount(wPlayerRole))
			, 4, PAL_XY(i * 20 + startPos - 8, 167), kNumColorBlue, kNumAlignRight);
#endif //  _DEBUG
#endif

	}


	//显示场景加成
	for (i = 0; i < 5; i++)
	{
		int j;
		j = gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].rgsMagicEffect[i];
		PAL_DrawNumber(j >= 0 ? j : -j //显示场景加成
			, 1, PAL_XY(3, i * 14 + 40),
			j >= 0 ? kNumColorYellow : kNumColorBlue, kNumAlignRight);
	}

}
//#endif

VOID CUIBattle::PAL_BattleUIShowNum(
	WORD           wNum,
	PAL_POS        pos,
	NUMCOLOR       color
)
/*++
  Purpose:    Show a number on battle screen (indicates HP/MP change).
  在战斗屏幕上显示数字（表示生命值和魔法值的变化）。
  Parameters:    [IN]  wNum - number to be shown.
  要显示的数字。
  [IN]  pos - position of the number on the screen.
  数字在屏幕上的位置。
  [IN]  color - color of the number.
  数字的颜色。
  Return value:    None.
  --*/
{
	int     i;

	for (i = 0; i < BATTLEUI_MAX_SHOWNUM; i++)
	{
		if (g_Battle.UI.rgShowNum[i].wNum == 0)
		{
			g_Battle.UI.rgShowNum[i].wNum = wNum;
			g_Battle.UI.rgShowNum[i].pos = PAL_XY(PAL_X(pos) - 15, PAL_Y(pos));
			g_Battle.UI.rgShowNum[i].color = color;
			g_Battle.UI.rgShowNum[i].dwTime = SDL_GetTicks_New();

			break;
		}
	}
}


#ifdef SHOW_ENEMY_STATUS
VOID CUIBattle::PAL_New_EnemyStatus(
	VOID
)
{
	//在战斗中显示怪物信息
	PAL_LARGE BYTE* bufBackground = (PAL_LARGE  BYTE*)malloc(320 * 200);

	int              iCurrent;
	int              iEnemyRole;
	int              i, y;
	WORD             w;
	int i_n;

	PAL_MKFDecompressChunk(bufBackground, 320 * 200, STATUS_BACKGROUND_FBPNUM,
		gpGlobals->f.fpFBP);
	iCurrent = 0;

	while (iCurrent >= 0 && iCurrent <= g_Battle.wMaxEnemyIndex && PalQuit == FALSE)
	{
		iEnemyRole = g_Battle.rgEnemy[iCurrent].wObjectID;
		if (g_Battle.rgEnemy[iCurrent].dwActualHealth == 0)
		{
			iCurrent++;
			continue;
		}
		// Draw the background image
		//

		PAL_FBPBlitToSurface(bufBackground, gpTextureReal);

		//ClearScreen();
		//显示怪物图片
		int x__pos = PAL_RLEGetWidth(PAL_SpriteGetFrame(
			g_Battle.rgEnemy[iCurrent].lpSprite,
			g_Battle.rgEnemy[iCurrent].wCurrentFrame)) / 2;
		int y__pos = PAL_RLEGetHeight(PAL_SpriteGetFrame(
			g_Battle.rgEnemy[iCurrent].lpSprite,
			g_Battle.rgEnemy[iCurrent].wCurrentFrame)) / 2;
		PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(
			g_Battle.rgEnemy[iCurrent].lpSprite,
			g_Battle.rgEnemy[iCurrent].wCurrentFrame),
			gpTextureReal, PAL_XY(160 - x__pos, 80 - y__pos), 2);


		//
		// Draw the text labels
		//
		PAL_DrawText(PAL_GetWord(21), PAL_XY(6, 24), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].e.wCash, 5,
			PAL_XY(54, 28), kNumColorYellow, kNumAlignRight);//金钱

		int iLevel;
		iLevel = g_Battle.rgEnemy[iCurrent].e.wLevel;

		PAL_DrawText(PAL_GetWord(STATUS_LABEL_EXP), PAL_XY(6, 6), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].dwExp, 7,
			PAL_XY(54, 10), kNumColorYellow, kNumAlignRight);//经验

		PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(193, 0), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].e.wLevel, 3,
			PAL_XY(196, 24), kNumColorYellow, kNumAlignMid);//怪物等级

		PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(6, 43), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText("/", PAL_XY(72, 45), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].dwActualHealth, 6,
			PAL_XY(41, 45), kNumColorYellow, kNumAlignRight);//体力
		PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].dwMaxHealth,
			6, PAL_XY(78, 50), kNumColorYellow, kNumAlignLeft);//体力 /MAX

		PAL_DrawText(PAL_GetWord(e_NewWord_LinFang),
			PAL_XY(6, 82), MENUITEM_COLOR, TRUE, FALSE);

		for (i_n = 0; i_n < NUM_MAGIC_ELEMENTAL; i_n++)
		{
			PAL_DrawNumber((SHORT)g_Battle.rgEnemy[iCurrent].e.wElemResistance[i_n], 2,
				PAL_XY(40 + i_n * 13, 86), kNumColorYellow, kNumAlignMid);//
		}


		PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(252, 43), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(PAL_New_GetEnemyDefense(iCurrent), 5,
			PAL_XY(254, 66), kNumColorYellow, kNumAlignRight);//防御

		PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(256, 106), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(PAL_New_GetEnemyMagicStrength(iCurrent), 5,
			PAL_XY(262, 126), kNumColorYellow, kNumAlignRight);//魔力

		PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(208, 134), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(PAL_New_GetEnemyAttackStrength(iCurrent), 5,
			PAL_XY(212, 154), kNumColorYellow, kNumAlignRight);//武术

		PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(142, 142), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(PAL_New_GetEnemyDexterity(iCurrent), 5,
			PAL_XY(146, 162), kNumColorYellow, kNumAlignRight);//身法

		PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(82, 126), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(PAL_New_GetEnemyFleeRate(iCurrent), 5,
			PAL_XY(86, 144), kNumColorYellow, kNumAlignRight);//运气

		PAL_DrawText(PAL_GetWord(iEnemyRole),
			PAL_XY(110, 8), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);//名字


		PAL_DrawNumber(iCurrent, 2,
			PAL_XY(300, 184), kNumColorYellow, kNumAlignRight);//位置

		if (g_Battle.rgEnemy[iCurrent].e.wStealItem)
		{
			LPSTR buf = new CHAR[60];
			LPSTR buf1 = new char[40];
			strcpy(buf1, PAL_GetWord(e_NewWord_TouQie));
			strncpy(buf, va("%s/  %s", buf1, PAL_GetWord(g_Battle.rgEnemy[iCurrent].e.wStealItem)), 39);
			PAL_DrawText(buf, PAL_XY(6, 160), MENUITEM_COLOR, TRUE, FALSE);//偷窃物品
			delete[] buf;
			delete[] buf1;
			int m;
			m = g_Battle.rgEnemy[iCurrent].e.nStealItem;
			PAL_DrawNumber(m, 2,
				PAL_XY(42, 164), kNumColorYellow, kNumAlignRight);//物品数量
		}
		else
		{
			int m;
			m = g_Battle.rgEnemy[iCurrent].e.nStealItem;
			PAL_DrawText(PAL_GetWord(e_NewWord_TouQie),
				PAL_XY(6, 160), MENUITEM_COLOR, TRUE, FALSE);//偷窃金钱
			PAL_DrawNumber(m, 5,
				PAL_XY(48, 164), kNumColorYellow, kNumAlignRight);//金钱数量

		}


		PAL_DrawText(PAL_GetWord(e_NewWord_WuKang), PAL_XY(6, 100), MENUITEM_COLOR, TRUE, FALSE);//巫抗
		PAL_DrawNumber(((LPOBJECT)gpGlobals->g.rgObject)[iEnemyRole].enemy.wResistanceToSorcery, 3,
			PAL_XY(54, 104), kNumColorYellow, kNumAlignRight);

		PAL_DrawText(PAL_GetWord(e_NewWord_DuFang), PAL_XY(6, 120), MENUITEM_COLOR, TRUE, FALSE);//毒抗
		PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].e.wPoisonResistance, 3,
			PAL_XY(54, 124), kNumColorYellow, kNumAlignRight);

		PAL_DrawText(PAL_GetWord(e_NewWord_WuLiKang), PAL_XY(6, 140), MENUITEM_COLOR, TRUE, FALSE);//物抗
		PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].e.wPhysicalResistance, 3,
			PAL_XY(54, 144), kNumColorYellow, kNumAlignRight);

		PAL_DrawText(PAL_GetWord(e_NewWord_WuGongFuJia), PAL_XY(6, 180), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawNumber(g_Battle.rgEnemy[iCurrent].e.wAttackEquivItemRate, 3,
			PAL_XY(65, 184), kNumColorYellow, kNumAlignRight);
		PAL_DrawText(PAL_GetWord(g_Battle.rgEnemy[iCurrent].e.wAttackEquivItem),//攻击附加
			PAL_XY(80, 181), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);

		if (g_Battle.rgEnemy[iCurrent].e.wDualMove)
			PAL_DrawText(PAL_GetWord(e_NewWord_LiangCiGongJi), PAL_XY(248, 162), 
				MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);//两次攻击

		// 显示魔法
		if ((SHORT)g_Battle.rgEnemy[iCurrent].e.wMagic > 0)
		{
			PAL_DrawText(PAL_GetWord(e_NewWord_MuoFa),
				PAL_XY(156, 180), MENUITEM_COLOR, TRUE, FALSE);//攻击魔法
			PAL_DrawText(PAL_GetWord(g_Battle.rgEnemy[iCurrent].e.wMagic),
				PAL_XY(206, 180), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);//攻击魔法
		}

		//显示掉落物品及概率
		{
			WORD  wEntryObject = g_Battle.rgEnemy[iCurrent].wScriptOnBattleEnd;
			if (wEntryObject)
			{
				while (gpGlobals->g.lprgScriptEntry[wEntryObject].wOperation != 0x0006 &&
					gpGlobals->g.lprgScriptEntry[wEntryObject].wOperation != 0x001f &&
					gpGlobals->g.lprgScriptEntry[wEntryObject].wOperation != 0)
				{
					if (gpGlobals->g.lprgScriptEntry[wEntryObject].wOperation == 0x0003)
					{
						wEntryObject = gpGlobals->g.lprgScriptEntry[wEntryObject].rgwOperand[0];
					}
					else
					{
						wEntryObject++;
					}
				}
				if (gpGlobals->g.lprgScriptEntry[wEntryObject].wOperation == 0x0006 &&
					gpGlobals->g.lprgScriptEntry[wEntryObject + 1].wOperation == 0x001f)
				{
					PAL_DrawNumber(gpGlobals->g.lprgScriptEntry[wEntryObject].rgwOperand[0], 3,
						PAL_XY(10, 66), kNumColorYellow, kNumAlignRight);
					PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgScriptEntry[wEntryObject + 1].rgwOperand[0]),
						PAL_XY(34, 62), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);
					if (gpGlobals->g.lprgScriptEntry[wEntryObject + 1].rgwOperand[1] > 1)
						PAL_DrawNumber(gpGlobals->g.lprgScriptEntry[wEntryObject].rgwOperand[1], 2,
							PAL_XY(94, 66), kNumColorYellow, kNumAlignRight);

				}
				else if (gpGlobals->g.lprgScriptEntry[wEntryObject].wOperation == 0x001f)
				{
					PAL_DrawNumber(100, 3,
						PAL_XY(10, 66), kNumColorYellow, kNumAlignRight);
					PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgScriptEntry[wEntryObject].rgwOperand[0]),
						PAL_XY(34, 62), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);
					if (gpGlobals->g.lprgScriptEntry[wEntryObject + 1].rgwOperand[1] > 1)
						PAL_DrawNumber(gpGlobals->g.lprgScriptEntry[wEntryObject].rgwOperand[1], 2,
							PAL_XY(94, 66), kNumColorYellow, kNumAlignRight);
				}
			}
		}
		//
		// Draw all poisons
		//
		y = 56;
		//显示中毒状态
		for (i = 0; i < MAX_POISONS; i++)
		{
			w = g_Battle.rgEnemy[iCurrent].rgPoisons[i].wPoisonID;

			if (w != 0)
			{
				PAL_DrawText(PAL_GetWord(w), PAL_XY(195, y),
					(BYTE)(((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wColor + 10), TRUE, FALSE);
//#ifdef POISON_STATUS_EXPAND
				if (gConfig->m_Function_Set[0]) 
				{
					INT  wPoisonIntensity = g_Battle.rgEnemy[iCurrent].rgPoisons[i].wPoisonIntensity;

					if (wPoisonIntensity != 0)
					{
						PAL_DrawNumber(wPoisonIntensity, 2,
							PAL_XY(235, y + 4), kNumColorYellow, kNumAlignRight);
					}
				}
//#endif

				y += 18;
			}
		}

		//
		// Update the screen
		//

		VIDEO_UpdateScreen(gpTextureReal);

		//
		// Wait for input
		//
		PAL_ClearKeyState();

		while(!PalQuit)
		{
			PAL_Delay(10);

			if (g_InputState.dwKeyPress & kKeyMenu)
			{
				iCurrent = -1;
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
			{
				for (iCurrent--; iCurrent >= 0 && g_Battle.rgEnemy[iCurrent].dwActualHealth == 0; iCurrent--);
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown | kKeySearch))
			{
				iCurrent++;
				break;
			}
			//VIDEO_UpdateScreen(gpTextureReal);
		}
	}
	free(bufBackground);

}
#endif