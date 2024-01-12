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


#include "cbattle.h"
#include "cpalbaseio.h"
#include <assert.h>
#include "cscript.h"

#define PAL_RunTriggerScript  pCScript->PAL_RunTriggerScript
#define PAL_RunAutoScript     pCScript->PAL_RunAutoScript

CBattle::CBattle()
{

}


BATTLERESULT CBattle::PAL_BattleMain(
	VOID
)
/*++
  功能：   The main battle routine.
  战斗主程序
  参数：    无。
  返回值：    The result of the battle.
  战斗结果。
  --*/
{
	int         i;
	DWORD       dwTime;

	VIDEO_BackupScreen();

	ClearScreen();
	//
	// Generate the scene and draw the scene to the screen buffer
	//生成场景并且将场景存到屏幕缓存中。
	PAL_BattleMakeScene();
	RenderBlendCopy(gpTextureReal, g_Battle.lpSceneBuf);;
	//
	// Fade out the music and delay for a while
	//停止音乐并且等待一会
	PAL_PlayMUS(0, FALSE, 1);
	PAL_Delay(200);

	//
	// Switch the screen
	//
	VIDEO_SwitchScreen(5);

	//
	// Play the battle music
	//播放战斗音乐。
	PAL_PlayMUS(gpGlobals->wNumBattleMusic, TRUE, 0);

	//
	// Fade in the screen when needed
	//当需要时淡入画面
	if (gpGlobals->fNeedToFadeIn)
	{
		VIDEO_SetPalette(PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette));
		PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
		gpGlobals->fNeedToFadeIn = FALSE;
	}

	//
	// Run the pre-battle scripts for each enemies
	//每个敌人都要运行战前脚本
	PAL_BattleBackupStat();
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		g_Battle.rgEnemy[i].wScriptOnTurnStart =
			PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnTurnStart, i);

		if (g_Battle.BattleResult != kBattleResultPreBattle)
		{
			break;
		}
	}
	PAL_BattleDisplayStatChange();

	if (g_Battle.BattleResult == kBattleResultPreBattle)
	{
		g_Battle.BattleResult = kBattleResultOnGoing;
	}


	dwTime = SDL_GetTicks_New();

	PAL_ClearKeyState();

	for (int n = 0; n < MAX_PLAYERS_IN_PARTY; n++)
	{
		/////
		g_Battle.rgPlayer[n].fDefenAction = 0;
	}

	//
	// Run the main battle loop.
	//循环运行战斗回合
	g_Loop = 0;
//#ifdef NEW_ScriptOnBattleEnd
	if (gConfig->m_Function_Set[12])//修改战后覆盖的脚本
		g_ScriptOnBattleEnd.clear();
//#endif
	while(!PalQuit)
	{
		ClearScreen();
		//
		// Break out if the battle ended.
		//如果战斗结束，则同时结束战斗回合的循环。
		if (g_Battle.BattleResult != kBattleResultOnGoing || PalQuit)
		{
			break;
		}

		//
		// Wait for the time of one frame. Accept input here.
		//
		PAL_ProcessEvent();
		while (SDL_GetTicks_New() <= dwTime)
		{
			PAL_ProcessEvent();
			SDL_Delay(1);
		}

		//
		// Set the time of the next frame.
		//
		dwTime = SDL_GetTicks_New() + gConfig->m_Function_Set[23];//BATTLE_FRAME_TIME;

		//
		// Run the main frame routine.
		//
		PAL_BattleStartFrame();

		//
		// Update the screen.
		//更新屏幕
		VIDEO_UpdateScreen(gpTextureReal);
	}

	//
	// Return the battle result
	//返回战斗结果
	return g_Battle.BattleResult;
}

VOID CBattle::PAL_BattleWon(
	VOID
)
/*++
  功能：    Show the "you win" message and add the experience points for players.
  显示“胜利”信息并且增加玩家的经验值。
  参数：    无。
  默认值：    无。
  --*/
{
	const SDL_Rect   rect = { 65, 60, 200, 100 };
	const SDL_Rect   rect1 = { 80, 0, 180, 200 };

	int              i, j, iTotalCount;
	DWORD            dwExp;
	WORD             w, wLevel;
	BOOL             fLevelUp;
	PLAYERROLES      OrigPlayerRoles;

	//
	// Backup the initial player stats
	//
	OrigPlayerRoles = gpGlobals->g.PlayerRoles;

	if (g_Battle.iExpGained > 0)
	{
		//
		// Play the "battle win" music
		//播放“战斗胜利”的音乐
		PAL_PlayMUS(g_Battle.fIsBoss ? 2 : 3, FALSE, 0);

		//
		ClearScreen();
		//
		// Show the message about the total number of exp. and cash gained
		//显示全部获得的经验，以及获得的金钱
		PAL_CreateSingleLineBox(PAL_XY(83, 60), 8, FALSE);
		PAL_CreateSingleLineBox(PAL_XY(65, 105), 10, FALSE);

		PAL_DrawText(PAL_GetWord(BATTLEWIN_GETEXP_LABEL), PAL_XY(95, 70), 0, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(BATTLEWIN_BEATENEMY_LABEL), PAL_XY(77, 115), 0, FALSE, FALSE);
		PAL_DrawText(PAL_GetWord(BATTLEWIN_DOLLAR_LABEL), PAL_XY(197, 115), 0, FALSE, FALSE);
		PAL_DrawNumber(g_Battle.iExpGained, 5, PAL_XY(182, 74), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(g_Battle.iCashGained, 5, PAL_XY(162, 119), kNumColorYellow, kNumAlignMid); 

		{
			//VIDEO_UpdateScreen((SDL_Rect*)&rect);
			SDL_Rect bRect = rect;
			setPictureRatio(&bRect);
			VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
		}	
		PAL_WaitForKey(g_Battle.fIsBoss ? 5500 : 3000);
	}
	 
	// Add the cash value
	gpGlobals->dwCash += g_Battle.iCashGained;

	//
	// Add the experience points for each players
	//增加每个玩家的经验值
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (PalQuit)return;
		fLevelUp = FALSE;

		w = gpGlobals->rgParty[i].wPlayerRole;
		if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
		{
			continue; // don't care about dead players 已死亡的玩家不会获得经验值
		}

		wLevel = gpGlobals->g.PlayerRoles.rgwLevel[w];
//#ifdef STRENGTHEN_PLAYER //有特色的加强主角，灵儿初始五灵抗性20%，阿奴毒抗巫抗各30%，林月如额外恢复
		if(gConfig->m_Function_Set[19])
			switch (w)
			{	//赵灵儿属性增强
			case RoleID_ZhaoLingEr:
			{
				int x;
				for (x = 0; x < NUM_MAGIC_ELEMENTAL; x++)
				{
					gpGlobals->g.PlayerRoles.rgwElementalResistance[x][w] =
						min(20, max(0, wLevel - 20));
				}
				break;
			}

			case RoleID_LinYueRu:
			{
				gpGlobals->g.PlayerRoles.rgwPhysicalResistance[w] = min(20, max(0, wLevel));
				break;
			}

			case RoleID_ANu:
			{
				gpGlobals->g.PlayerRoles.rgwPoisonResistance[w] = min(30, max(0, wLevel - 40));
				gpGlobals->g.PlayerRoles.rgwSorceryResistance[w] = min(30, max(0, wLevel - 40));
				gpGlobals->g.PlayerRoles.rgwSorceryStrength[w] = min(50, max(0, wLevel - 40));
				break;
			}

			default:
				break;
			}
//#endif
		dwExp = gpGlobals->Exp.rgPrimaryExp[w].wExp;

#ifdef EDIT_EXP_CALCULATION
		dwExp += g_Battle.rgPlayer[i].dwExpGained;
#else
		dwExp += g_Battle.iExpGained;
#endif

		if (gpGlobals->g.PlayerRoles.rgwLevel[w] > MAX_LEVELS
#if 0
#else
#if FINISH_GAME_MORE_ONE_TIME
			&& !gpGlobals->bFinishGameTime
#endif
#endif
			)
		{
			gpGlobals->g.PlayerRoles.rgwLevel[w] = MAX_LEVELS;
		}

		DWORD LevelUpExp = 0;
		while(!PalQuit)
		{
#ifdef EDIT_EXP_CALCULATION
			LevelUpExp = PAL_New_GetLevelUpExp(gpGlobals->g.PlayerRoles.rgwLevel[w]);
#else
			LevelUpExp = gpGlobals->g.rgLevelUpExp[min(MAX_LEVELS, gpGlobals->g.PlayerRoles.rgwLevel[w])];
#endif
			if (dwExp < LevelUpExp)
			{
				break;
			}

			dwExp -= LevelUpExp;

			if (gpGlobals->g.PlayerRoles.rgwLevel[w] < MAX_LEVELS
#if 0

#else
#if FINISH_GAME_MORE_ONE_TIME 
				|| gpGlobals->bFinishGameTime
#endif
#endif
				)
			{
				fLevelUp = TRUE;
				PAL_PlayerLevelUp(w, 1);

				gpGlobals->g.PlayerRoles.rgwHP[w] = gpGlobals->g.PlayerRoles.rgwMaxHP[w];
				gpGlobals->g.PlayerRoles.rgwMP[w] = gpGlobals->g.PlayerRoles.rgwMaxMP[w];
			}
		}

#ifdef DWORD_EXP
		gpGlobals->Exp.rgPrimaryExp[w].wExp = dwExp;
#else
		gpGlobals->Exp.rgPrimaryExp[w].wExp = (WORD)min(0xFFFF, dwExp);
#endif 
		//gpGlobals->Exp.rgPrimaryExp[w].wExp = (WORD)min(0xFFFF, dwExp);

		if (fLevelUp)
		{
			////
			ClearScreen();
			// Player has gained a level. Show the message
			PAL_CreateSingleLineBox(PAL_XY(80, 0), 10, FALSE);
			PAL_CreateBox(PAL_XY(82, 32), 7, 8, 1, FALSE);

			PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_XY(110, 10), 0,
				FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(110 + 16 * 3, 10), 0, FALSE, FALSE);
			PAL_DrawText(PAL_GetWord(BATTLEWIN_LEVELUP_LABEL), PAL_XY(110 + 16 * 5, 10), 0, FALSE, FALSE);

			for (j = 0; j < 8; j++)
			{
				PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ARROW),
					gpTextureReal, PAL_XY(183, 48 + 18 * j));
			}

			PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(100, 44), BATTLEWIN_LEVELUP_LABEL_COLOR,
				TRUE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(100, 62), BATTLEWIN_LEVELUP_LABEL_COLOR,
				TRUE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(100, 80), BATTLEWIN_LEVELUP_LABEL_COLOR,
				TRUE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(100, 98), BATTLEWIN_LEVELUP_LABEL_COLOR,
				TRUE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(100, 116), BATTLEWIN_LEVELUP_LABEL_COLOR,
				TRUE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(100, 134), BATTLEWIN_LEVELUP_LABEL_COLOR,
				TRUE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(100, 152), BATTLEWIN_LEVELUP_LABEL_COLOR,
				TRUE, FALSE);
			PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(100, 170), BATTLEWIN_LEVELUP_LABEL_COLOR,
				TRUE, FALSE);

			//
			// Draw the original stats and stats after level up
			//显示角色升级之前和升级之后的状态
			PAL_DrawNumber(OrigPlayerRoles.rgwLevel[w], 4, PAL_XY(133, 47),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[w], 4, PAL_XY(195, 47),
				kNumColorYellow, kNumAlignRight);

			PAL_DrawNumber(OrigPlayerRoles.rgwHP[w], 4, PAL_XY(133, 64),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(OrigPlayerRoles.rgwMaxHP[w], 4, PAL_XY(158, 72),
				kNumColorBlue, kNumAlignRight);
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
				PAL_XY(156, 66));
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[w], 4, PAL_XY(195, 64),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[w], 4, PAL_XY(218, 72),
				kNumColorBlue, kNumAlignRight);
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
				PAL_XY(218, 66));

			PAL_DrawNumber(OrigPlayerRoles.rgwMP[w], 4, PAL_XY(133, 82),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(OrigPlayerRoles.rgwMaxMP[w], 4, PAL_XY(158, 90),
				kNumColorBlue, kNumAlignRight);
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
				PAL_XY(156, 84));
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[w], 4, PAL_XY(195, 82),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[w], 4, PAL_XY(218, 90),
				kNumColorBlue, kNumAlignRight);
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
				PAL_XY(218, 84));

			PAL_DrawNumber(OrigPlayerRoles.rgwAttackStrength[w] + PAL_GetPlayerAttackStrength(w) -
				gpGlobals->g.PlayerRoles.rgwAttackStrength[w],
				4, PAL_XY(133, 101), kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(PAL_GetPlayerAttackStrength(w), 4, PAL_XY(195, 101),
				kNumColorYellow, kNumAlignRight);

			PAL_DrawNumber(OrigPlayerRoles.rgwMagicStrength[w] + PAL_GetPlayerMagicStrength(w) -
				gpGlobals->g.PlayerRoles.rgwMagicStrength[w],
				4, PAL_XY(133, 119), kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(PAL_GetPlayerMagicStrength(w), 4, PAL_XY(195, 119),
				kNumColorYellow, kNumAlignRight);

			PAL_DrawNumber(OrigPlayerRoles.rgwDefense[w] + PAL_GetPlayerDefense(w) -
				gpGlobals->g.PlayerRoles.rgwDefense[w],
				4, PAL_XY(133, 137), kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(PAL_GetPlayerDefense(w), 4, PAL_XY(195, 137),
				kNumColorYellow, kNumAlignRight);

			PAL_DrawNumber(OrigPlayerRoles.rgwDexterity[w] + PAL_GetPlayerDexterity(w) -
				gpGlobals->g.PlayerRoles.rgwDexterity[w],
				4, PAL_XY(133, 155), kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(PAL_GetPlayerDexterity(w), 4, PAL_XY(195, 155),
				kNumColorYellow, kNumAlignRight);

			PAL_DrawNumber(OrigPlayerRoles.rgwFleeRate[w] + PAL_GetPlayerFleeRate(w) -
				gpGlobals->g.PlayerRoles.rgwFleeRate[w],
				4, PAL_XY(133, 173), kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(PAL_GetPlayerFleeRate(w), 4, PAL_XY(195, 173),
				kNumColorYellow, kNumAlignRight);

			//
			// Update the screen and wait for key
			//更新换面，等待按键
			{
				//VIDEO_UpdateScreen((SDL_Rect*)&rect);
				SDL_Rect bRect = rect1;
				setPictureRatio(&bRect);
				VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
			}			
			PAL_WaitForKey(3000);

			OrigPlayerRoles = gpGlobals->g.PlayerRoles;
			//ClearScreen();
		}

		//
		// Increasing of other hidden levels
		//
		iTotalCount = 0;

		iTotalCount += gpGlobals->Exp.rgAttackExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgDefenseExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgDexterityExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgFleeExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgHealthExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgMagicExp[w].wCount;
		iTotalCount += gpGlobals->Exp.rgMagicPowerExp[w].wCount;

		if (iTotalCount > 0)
		{
			//#ifdef EDIT_EXP_CALCULATION	
			switch (gConfig->m_Function_Set[13])//修改附加经验计算方式
			{
			case 1:
			{
				EXPERIENCE* pExp = (EXPERIENCE*)(&gpGlobals->Exp);
				WORD* pPL = (WORD*)(&gpGlobals->g.PlayerRoles);
				WORD* pOPL = (WORD*)(&OrigPlayerRoles);

				EXPERIENCE rgTempExp;
				WORD wTempPlayerPara = 0;
				WORD wTempOPlayerPara = 0;

				for (j = HealthExp; j <= FleeExp; j++)
				{
					rgTempExp = pExp[j * MAX_PLAYER_ROLES + w];
					dwExp = g_Battle.rgPlayer[i].dwExpGained;
					dwExp *= rgTempExp.wCount * 3.5;
					dwExp /= iTotalCount;
//#ifdef GAIN_MORE_HIDDEN_EXP
					if (gConfig->m_Function_Set[14])//额外附加经验
						dwExp *= 1.0 + min(1.0, iTotalCount / 50.0);
//#endif
					if (gConfig->m_Function_Set[21])//使用梦蛇后，各项属性增加，实现
						if (g_Battle.g_ifUseMenshe[w])
						{
							dwExp *= 1.5;
						}
					dwExp += rgTempExp.wExp;

					while (dwExp >= PAL_New_GetLevelUpBaseExp(rgTempExp.wLevel))
					{
						dwExp -= PAL_New_GetLevelUpBaseExp(rgTempExp.wLevel);
						//对应属性增加
						pPL[ExpTypeToPlayerPara[j] * MAX_PLAYER_ROLES + w] += 2;

						pExp[j * MAX_PLAYER_ROLES + w].wLevel++;
						//取消附加属性增加限制
						if (rgTempExp.wLevel > gpGlobals->Exp.rgPrimaryExp[w].wLevel)
						{
							//pExp[j * MAX_PLAYER_ROLES + w].wLevel = gpGlobals->Exp.rgPrimaryExp[w].wLevel;
						}

						rgTempExp = pExp[j * MAX_PLAYER_ROLES + w];
					}

					pExp[j * MAX_PLAYER_ROLES + w].wExp = dwExp;

					wTempPlayerPara = pPL[ExpTypeToPlayerPara[j] * MAX_PLAYER_ROLES + w];
					wTempOPlayerPara = pOPL[ExpTypeToPlayerPara[j] * MAX_PLAYER_ROLES + w];
					if (wTempPlayerPara > wTempOPlayerPara)
					{
						PAL_CreateSingleLineBox(PAL_XY(83, 60), 8, FALSE);
						PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_XY(95, 70), 0, FALSE, FALSE, 14);
						PAL_DrawText(PAL_GetWord(ExpTypeToLabel[j]), PAL_XY(143, 70), 0, FALSE, FALSE, 14);
						PAL_DrawText(PAL_GetWord(BATTLEWIN_LEVELUP_LABEL), PAL_XY(175, 70), 0, FALSE, FALSE, 14);
						PAL_DrawNumber(wTempPlayerPara - wTempOPlayerPara, 5, PAL_XY(188, 74), kNumColorYellow, kNumAlignRight);

						{
							SDL_Rect bRect = rect;
							setPictureRatio(&bRect);
							VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
						}					
						PAL_WaitForKey(3000);
					}
				}

			}
			break;

			case 0:
			{
#define CHECK_HIDDEN_EXP(expname, statname, label)          \
				{\
	dwExp = g_Battle.iExpGained;                             \
	dwExp *= gpGlobals->Exp.expname[w].wCount;               \
	dwExp /= iTotalCount;                                    \
	dwExp *= 2;                                              \
\
	dwExp += gpGlobals->Exp.expname[w].wExp;                 \
\
\
while (dwExp >= gpGlobals->g.rgLevelUpExp[min(MAX_LEVELS, gpGlobals->Exp.expname[w].wLevel)]) \
    {\
		dwExp -= gpGlobals->g.rgLevelUpExp[gpGlobals->Exp.expname[w].wLevel]; \
		gpGlobals->g.PlayerRoles.statname[w] += RandomLong(1, 2); \
		if (gpGlobals->Exp.expname[w].wLevel < MAX_LEVELS)    \
						{\
			gpGlobals->Exp.expname[w].wLevel++;\
						}\
	}\
\
   gpGlobals->Exp.expname[w].wExp = (WORD)dwExp;\
\
   if (gpGlobals->g.PlayerRoles.statname[w] != \
      OrigPlayerRoles.statname[w])\
		{\
      PAL_CreateSingleLineBox(PAL_XY(83, 60), 8, FALSE);\
      PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]), PAL_XY(95, 70), \
         0, FALSE, FALSE); \
      PAL_DrawText(PAL_GetWord(label), PAL_XY(143, 70),\
         0, FALSE, FALSE);\
      PAL_DrawText(PAL_GetWord(BATTLEWIN_LEVELUP_LABEL), PAL_XY(175, 70),  \
         0, FALSE, FALSE);\
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.statname[w] - \
         OrigPlayerRoles.statname[w],\
         5, PAL_XY(188, 74), kNumColorYellow, kNumAlignRight); \
      VIDEO_UpdateScreen(gpTextureReal);\
      PAL_WaitForKey(3000);\
		}\
	}
				CHECK_HIDDEN_EXP(rgHealthExp, rgwMaxHP, STATUS_LABEL_HP);
				CHECK_HIDDEN_EXP(rgMagicExp, rgwMaxMP, STATUS_LABEL_MP);
				CHECK_HIDDEN_EXP(rgAttackExp, rgwAttackStrength, STATUS_LABEL_ATTACKPOWER);
				CHECK_HIDDEN_EXP(rgMagicPowerExp, rgwMagicStrength, STATUS_LABEL_MAGICPOWER);
				CHECK_HIDDEN_EXP(rgDefenseExp, rgwDefense, STATUS_LABEL_RESISTANCE);
				CHECK_HIDDEN_EXP(rgDexterityExp, rgwDexterity, STATUS_LABEL_DEXTERITY);
				CHECK_HIDDEN_EXP(rgFleeExp, rgwFleeRate, STATUS_LABEL_FLEERATE);

#undef CHECK_HIDDEN_EXP
			}
			break;
			case 2:
			default:
				break;
			}
			//#else


		//#endif
		}

		// 学习当前等级的所有法术
		j = 0;
		LEVELUPMAGIC tempMagic;
		while (j < gpGlobals->g.nLevelUpMagic)
		{
			if (PalQuit)return;
			
			WORD w1 = min(w,5);
            tempMagic = gpGlobals->g.lprgLevelUpMagic[j].m[w1];

			if (tempMagic.wMagic == 0 || tempMagic.wLevel > gpGlobals->g.PlayerRoles.rgwLevel[w])
			{
				j++;
				continue;
			}

			if (PAL_AddMagic(w, tempMagic.wMagic))
			{

				PAL_CreateSingleLineBox(PAL_XY(65, 105), 10, FALSE);

				PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
					PAL_XY(75, 115), 0, FALSE, FALSE);
				PAL_DrawText(PAL_GetWord(BATTLEWIN_ADDMAGIC_LABEL), PAL_XY(75 + 16 * 3, 115),
					0, FALSE, FALSE);
				PAL_DrawText(PAL_GetWord(tempMagic.wMagic),
					PAL_XY(75 + 16 * 5, 115), 0x1B, FALSE, FALSE);

				{
					//VIDEO_UpdateScreen((SDL_Rect*)&rect);
					SDL_Rect bRect = rect;
					setPictureRatio(&bRect);
					VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
				}
				PAL_WaitForKey(3000);
			}
			j++;
		}
	}

	// 运行战后脚本
//#ifdef NEW_ScriptOnBattleEnd
	if (gConfig->m_Function_Set[12])
		for (int i = 0; i < g_ScriptOnBattleEnd.size(); i++)
		{
			if (PalQuit)return;
			PAL_RunTriggerScript(g_ScriptOnBattleEnd.at(i), i % 5);
		}
//#else
	else
		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (PalQuit)return;
			PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnBattleEnd, i);
		}
//#endif

	// 每一场战斗后自动恢复
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (PalQuit)return;
		w = gpGlobals->rgParty[i].wPlayerRole;
		Pal_New_RecoverAfterBattle(w, 50);
	}
}

VOID CBattle::PAL_BattleEnemyEscape(
	VOID
)
/*++
  目的：    Enemy flee the battle.
  敌人逃跑。
  参数：    无。
  返回值：    无。
  --*/
{
	int j, x, y, w;
	BOOL f = TRUE;

	SOUND_Play(45);

	//
	// Show the animation
	//显示逃跑动画
	while (f)
	{
		f = FALSE;

		for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
		{
			if (g_Battle.rgEnemy[j].wObjectID == 0)
			{
				continue;
			}

			x = PAL_X(g_Battle.rgEnemy[j].pos) - 5;
			y = PAL_Y(g_Battle.rgEnemy[j].pos);

			g_Battle.rgEnemy[j].pos = PAL_XY(x, y);

			w = PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[j].lpSprite, 0));

			if (x + w > 0)
			{
				f = TRUE;
			}
		}

		PAL_BattleMakeScene();
		RenderBlendCopy(gpTextureReal, g_Battle.lpSceneBuf);;

		VIDEO_UpdateScreen(gpTextureReal);
		PAL_Delay(10);
	}

	PAL_Delay(500);
	g_Battle.BattleResult = kBattleResultTerminated;
}

VOID CBattle::PAL_BattlePlayerEscape(
	VOID
)
/*++
  目的：    Player flee the battle.
  玩家在战斗中逃跑。
  参数：    无。
  返回值：    无。
  --*/
{
	int         i, j;
	WORD        wPlayerRole;

	SOUND_Play(45);

	PAL_BattleUpdateFighters();

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
		{
			g_Battle.rgPlayer[i].wCurrentFrame = 0;
		}
	}

	for (i = 0; i < 16; i++)
	{
		for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
		{
			wPlayerRole = gpGlobals->rgParty[j].wPlayerRole;

			if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
			{
				//
				// TODO: This is still not the same as the original game
				//
				switch (j)
				{
				case 0:
					if (gpGlobals->wMaxPartyMemberIndex > 0)
					{
						g_Battle.rgPlayer[j].pos =
							PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 4,
								PAL_Y(g_Battle.rgPlayer[j].pos) + 6);
						break;
					}

				case 1:
					g_Battle.rgPlayer[j].pos =
						PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 4,
							PAL_Y(g_Battle.rgPlayer[j].pos) + 4);
					break;

				case 2:
					g_Battle.rgPlayer[j].pos =
						PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 6,
							PAL_Y(g_Battle.rgPlayer[j].pos) + 3);
					break;

				case 3:
					g_Battle.rgPlayer[j].pos =
						PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 7,
							PAL_Y(g_Battle.rgPlayer[j].pos) + 2);
					break;

				case 4:
					g_Battle.rgPlayer[j].pos =
						PAL_XY(PAL_X(g_Battle.rgPlayer[j].pos) + 8,
							PAL_Y(g_Battle.rgPlayer[j].pos) + 1);
					break;

				default:
					assert(FALSE); // Not possible
					break;
				}
			}
		}

		PAL_BattleDelay(1, 0, FALSE);
	}

	//
	// Remove all players from the screen
	// 从屏幕中移走所有角色
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		g_Battle.rgPlayer[i].pos = PAL_XY(9999, 9999);
	}

	PAL_BattleDelay(1, 0, FALSE);

	g_Battle.BattleResult = kBattleResultFleed;
}

BATTLERESULT CBattle::PAL_StartBattle(
	WORD			wEnemyTeam,
	BOOL			fIsBoss
)
/*++
  目的：    Start a battle.
  开始战斗。
  参数：    [IN]  wEnemyTeam - 敌人队伍人数。	the number of the enemy team.
  [IN]  fIsBoss - boss的fIsBoss值为TRUE（不允许逃跑）。
  返回值：    战斗结果。
  --*/
{
	BATTLERESULT   ret;
	int            j;
	WORD           w, wPrevWaveLevel;
	SHORT          sPrevWaveProgression;

	//
	// Set the screen waving effects
	//设置画面波浪效果
	wPrevWaveLevel = gpGlobals->wScreenWave;
	sPrevWaveProgression = gpGlobals->sWaveProgression;

	gpGlobals->sWaveProgression = 0;
	gpGlobals->wScreenWave = gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].wScreenWave;


	WORD* p = (WORD*)(&gpGlobals->g.PlayerRoles);
	for (j = Para_MaxHP; j <= Para_FleeRate; j++)
	{
		for (int i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			p[j * MAX_PLAYER_ROLES + w] = min(p[j * MAX_PLAYER_ROLES + w], MAX_PARAMETER + MAX_PARAMETER_EXTRA);
		}
	}

	// Make sure everyone in the party is alive, also clear all hidden EXP count records
	for (int i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;

		gpGlobals->g.PlayerRoles.rgwHP[w] = min(gpGlobals->g.PlayerRoles.rgwHP[w], gpGlobals->g.PlayerRoles.rgwMaxHP[w]);
		gpGlobals->g.PlayerRoles.rgwMP[w] = min(gpGlobals->g.PlayerRoles.rgwMP[w], gpGlobals->g.PlayerRoles.rgwMaxMP[w]);
		gpGlobals->g.PlayerRoles.rgwHP[w] = max(gpGlobals->g.PlayerRoles.rgwHP[w], 1);

		gpGlobals->rgPlayerStatus[w][kStatusPuppet] = 0;
		gpGlobals->Exp.rgHealthExp[w].wCount = 0;
		gpGlobals->Exp.rgMagicExp[w].wCount = 0;
		gpGlobals->Exp.rgAttackExp[w].wCount = 0;
		gpGlobals->Exp.rgMagicPowerExp[w].wCount = 0;
		gpGlobals->Exp.rgDefenseExp[w].wCount = 0;
		gpGlobals->Exp.rgDexterityExp[w].wCount = 0;
		gpGlobals->Exp.rgFleeExp[w].wCount = 0;
	}

	// 初始化
	{
		//memset(&g_Battle, 0, sizeof(g_Battle));
	}

	// Clear all item-using records
	for (int i = 0; i < MAX_INVENTORY; i++)
	{
		gpGlobals->rgInventory[i].nAmountInUse = 0;
	}

	// Store all enemies
	{
		int i;
		for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
		{
			memset(&(g_Battle.rgEnemy[i]), 0, sizeof(BATTLEENEMY));
			w = gpGlobals->g.lprgEnemyTeam[wEnemyTeam].rgwEnemy[i];

			if (w == 0xFFFF)
			{
				//continue;
				break;
			}

			if (w != 0)
			{
				g_Battle.rgEnemy[i].e = gpGlobals->g.lprgEnemy[((LPOBJECT)((LPOBJECT)gpGlobals->g.rgObject))[w].enemy.wEnemyID];
				g_Battle.rgEnemy[i].dwActualHealth = g_Battle.rgEnemy[i].e.wHealth;
				g_Battle.rgEnemy[i].dwMaxHealth = g_Battle.rgEnemy[i].dwActualHealth;
				g_Battle.rgEnemy[i].dwExp = g_Battle.rgEnemy[i].e.wExps;

//#ifdef STRENGTHEN_ENEMY
				if (gConfig->m_Function_Set[3])
					g_Battle.rgEnemy[i] = PAL_New_StrengthenEnemy(g_Battle.rgEnemy[i]);
//#endif
				g_Battle.rgEnemy[i].wObjectID = w;
				g_Battle.rgEnemy[i].state = kFighterWait;
				g_Battle.rgEnemy[i].wScriptOnTurnStart = ((LPOBJECT)gpGlobals->g.rgObject)[w].enemy.wScriptOnTurnStart;
				g_Battle.rgEnemy[i].wScriptOnBattleEnd = ((LPOBJECT)gpGlobals->g.rgObject)[w].enemy.wScriptOnBattleEnd;
				g_Battle.rgEnemy[i].wScriptOnReady = ((LPOBJECT)gpGlobals->g.rgObject)[w].enemy.wScriptOnReady;
				g_Battle.rgEnemy[i].iColorShift = 0;
#ifndef PAL_CLASSIC
				g_Battle.rgEnemy[i].flTimeMeter = 50;
				//flTimeMeter：时间条（满条即可行动）
				//
				// HACK: Otherwise the black thief lady will be too hard to beat
				//削减女飞贼的身法值
				if (g_Battle.rgEnemy[i].e.wDexterity == 164)
				{
					g_Battle.rgEnemy[i].e.wDexterity /= ((gpGlobals->wMaxPartyMemberIndex == 0) ? 6 : 3);
				}

				//
				// HACK: Heal up automatically for final boss
				//最终boss战前自动回复体力和真气
				if (g_Battle.rgEnemy[i].dwActualHealth > 30000)
				{
					for (w = 0; w < MAX_PLAYER_ROLES; w++)
					{
						gpGlobals->g.PlayerRoles.rgwHP[w] = gpGlobals->g.PlayerRoles.rgwMaxHP[w];
						gpGlobals->g.PlayerRoles.rgwMP[w] = gpGlobals->g.PlayerRoles.rgwMaxMP[w];
					}
				}

				//
				// Yet another HACKs
				//
				if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -32)
				{
					g_Battle.rgEnemy[i].e.wDexterity = 0; // for Grandma Knife //菜刀婆婆
				}
				else if (g_Battle.rgEnemy[i].e.wDexterity == 20)
				{
					//
					// for Fox Demon
					//
					if (gpGlobals->g.PlayerRoles.rgwLevel[0] < 15)
					{
						g_Battle.rgEnemy[i].e.wDexterity = 8;
					}
					else if (gpGlobals->g.PlayerRoles.rgwLevel[4] > 28 ||
						gpGlobals->Exp.rgPrimaryExp[4].wExp > 0)
					{
						g_Battle.rgEnemy[i].e.wDexterity = 60;
					}
				}
				else if (g_Battle.rgEnemy[i].e.wExp == 250 &&
					g_Battle.rgEnemy[i].e.wCash == 1100)
				{
					g_Battle.rgEnemy[i].e.wDexterity += 12; // for Snake Demon
				}
				else if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -60)
				{
					g_Battle.rgEnemy[i].e.wDexterity = 15; // for Spider
				}
				else if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -30)
				{
					g_Battle.rgEnemy[i].e.wDexterity = (WORD)-10; // for Stone Head
				}
				else if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -16)
				{
					g_Battle.rgEnemy[i].e.wDexterity = 0; // for Zombie
				}
				else if ((SHORT)g_Battle.rgEnemy[i].e.wDexterity == -20)
				{
					g_Battle.rgEnemy[i].e.wDexterity = -8; // for Flower Demon
				}
				else if (g_Battle.rgEnemy[i].e.wLevel < 20 &&
					gpGlobals->wNumScene >= 0xD8 && gpGlobals->wNumScene <= 0xE2)
				{
					//
					// for low-level monsters in the Cave of Trial
					//
					g_Battle.rgEnemy[i].e.wLevel += 15;
					g_Battle.rgEnemy[i].e.wDexterity += 25;
				}
				else if (gpGlobals->wNumScene == 0x90)
				{
					g_Battle.rgEnemy[i].e.wDexterity += 25; // for Tower Dragons
				}
				else if (g_Battle.rgEnemy[i].e.wLevel == 2 &&
					g_Battle.rgEnemy[i].e.wCash == 48)
				{
					g_Battle.rgEnemy[i].e.wDexterity += 8; // for Miao Fists
				}
				else if (g_Battle.rgEnemy[i].e.wLevel == 4 &&
					g_Battle.rgEnemy[i].e.wCash == 240)
				{
					g_Battle.rgEnemy[i].e.wDexterity += 18; // for Fat Miao
				}
				else if (g_Battle.rgEnemy[i].e.wLevel == 16 &&
					g_Battle.rgEnemy[i].e.wMagicRate == 4 &&
					g_Battle.rgEnemy[i].e.wAttackEquivItemRate == 4)
				{
					g_Battle.rgEnemy[i].e.wDexterity += 50; // for Black Spider
				}
#endif
			}
		}

		g_Battle.wMaxEnemyIndex = i - 1;
	}
	// Store all players
	for (int i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		g_Battle.rgPlayer[i].flTimeMeter = 15.0f;
#ifndef PAL_CLASSIC
		g_Battle.rgPlayer[i].flTimeSpeedModifier = 2.0f;
		g_Battle.rgPlayer[i].sTurnOrder = -1;
#endif
		g_Battle.rgPlayer[i].wHidingTime = 0;
		g_Battle.rgPlayer[i].state = kFighterWait;
		g_Battle.rgPlayer[i].action.sTarget = -1;
		g_Battle.rgPlayer[i].fDefending = FALSE;
		g_Battle.rgPlayer[i].wCurrentFrame = 0;
		g_Battle.rgPlayer[i].iColorShift = FALSE;

#ifdef EDIT_EXP_CALCULATION
		g_Battle.rgPlayer[i].dwExpGained = 0;
#endif
	}

	//
	// Load sprites and background
	//加载 和背景
	PAL_LoadBattleSprites();//将角色、敌人的形象读入各自的lpSprite
	PAL_LoadBattleBackground();//将地图读入lpBackground

	//
	// Create the surface for scene buffer
	//
	g_Battle.lpSceneBuf = new CGL_Texture(320, 200, 32);

	if (g_Battle.lpSceneBuf == NULL)
	{
		TerminateOnError("PAL_StartBattle(): creating surface for scene buffer failed!");
	}
#if SDL_VERSION_ATLEAST(2, 0, 0)
	//SDL_SetSurfacePalette(g_Battle.lpSceneBuf, gpPalette);
#else
	SDL_SetPalette(g_Battle.lpSceneBuf, SDL_PHYSPAL | SDL_LOGPAL, VIDEO_GetPalette(), 0, 256);
#endif

	PAL_UpdateEquipments();

	g_Battle.iExpGained = 0;
	g_Battle.iCashGained = 0;

	g_Battle.fIsBoss = fIsBoss;
	g_Battle.fEnemyCleared = FALSE;
	g_Battle.fEnemyMoving = FALSE;
	g_Battle.iHidingTime = 0;
	g_Battle.wMovingPlayerIndex = 0;

	g_Battle.UI.szMsg[0] = '\0';
	g_Battle.UI.szNextMsg[0] = '\0';
	g_Battle.UI.dwMsgShowTime = 0;
	g_Battle.UI.state = kBattleUIWait;
	g_Battle.UI.fAutoAttack = FALSE;
	g_Battle.UI.wSelectedIndex = 0;
	g_Battle.UI.wPrevEnemyTarget = 0;
	g_Battle.UI.wSelectedAction = 0;

	memset(g_Battle.UI.rgShowNum, 0, sizeof(g_Battle.UI.rgShowNum));

	g_Battle.lpSummonSprite = NULL;
	g_Battle.sBackgroundColorShift = 0;

	fInBattle = TRUE;
	g_Battle.BattleResult = kBattleResultPreBattle;

	PAL_BattleUpdateFighters();

	//
	// Load the battle effect sprite.
	
	{
		int i = PAL_MKFGetChunkSize(10, gpGlobals->f.fpDATA);
		g_Battle.lpEffectSprite = (LPBYTE)malloc(i);

		PAL_MKFReadChunk(g_Battle.lpEffectSprite, i, 10, gpGlobals->f.fpDATA);
	}
#ifdef PAL_CLASSIC
	g_Battle.Phase = kBattlePhaseSelectAction;
	g_Battle.fRepeat = FALSE;
	g_Battle.fForce = FALSE;
	g_Battle.fFlee = FALSE;
#endif
	if (gConfig->m_Function_Set[21])//使用梦蛇后，各项属性增加,初始化
		memset(g_Battle.g_ifUseMenshe, 0, sizeof(g_Battle.g_ifUseMenshe));
#ifdef PAL_ALLOW_KEYREPEAT
	SDL_EnableKeyRepeat(120, 75);
#endif

	//
	// Run the main battle routine.
	//
	ret = PAL_BattleMain();

#ifdef PAL_ALLOW_KEYREPEAT
	SDL_EnableKeyRepeat(0, 0);
	PAL_ClearKeyState();
	g_InputState.prevdir = kDirUnknown;
#endif

	//
	// Clear all item-using records
	//清除所有正在使用的物品数量
	for (w = 0; w < MAX_INVENTORY; w++)
	{
		gpGlobals->rgInventory[w].nAmountInUse = 0;
	}
	//
	// Clear all player status, poisons and temporary effects
	//清除所有角色状态，毒，和暂时性效果
	PAL_ClearAllPlayerStatus();
	for (w = 0; w < MAX_PLAYER_ROLES; w++)
	{
		PAL_CurePoisonByLevel(w, MAX_POISON_LEVEL);
		PAL_RemoveEquipmentEffect(w, kBodyPartExtra);
	}

	if (ret == kBattleResultWon || ret == kBattleResultEnemyFleed)
	{
		//
		// Player won the battle. Add the Experience points.
		//玩家获得胜利。增加玩家的经验值（调用PAL_BattleWon()函数）。
		PAL_BattleWon();
	}

	//
	// Free all the battle sprites
	//
	PAL_FreeBattleSprites();
 	free(g_Battle.lpEffectSprite);

	//
	// Free the surfaces for the background picture and scene buffer
	//
	SDL_FreeSurface(g_Battle.lpBackground);
	//SDL_FreeSurface(g_Battle.lpSceneBuf);
	delete g_Battle.lpSceneBuf;
	g_Battle.lpBackground = NULL;
	g_Battle.lpSceneBuf = NULL;

	fInBattle = FALSE;

	PAL_PlayMUS(gpGlobals->wNumMusic, TRUE, 1);

	//
	// Restore the screen waving effects
	//
	gpGlobals->sWaveProgression = sPrevWaveProgression;
	gpGlobals->wScreenWave = wPrevWaveLevel;

	return ret;
}

VOID CBattle::Pal_New_RecoverAfterBattle(
	WORD			wPlayerRole,
	WORD			wPercent
)
{
	WORD	wDiff = 0;
	WORD	wRecoverValue = 0;
	//WORD	wMaxValue = 0;
	//
	// Recover automatically after each battle
	// 每一场战斗后自动恢复
	wPercent = min(100, wPercent);

	wDiff = max(0, gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] - gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole]);
	if (wDiff < 20.0 * pow(1.0 * gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] + 5, 0.66))
		gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] += wDiff;
	else
	{
		wRecoverValue = min(wDiff * wPercent / 100, max(MAX_PARAMETER, gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole]) / 2);
		gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] += wRecoverValue;
	}
	if (gConfig->m_Function_Set[15])//额外恢复
	{
		wDiff = max(0, gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] - gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole]);
		if (wDiff < 20.0 * pow(1.0 * gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] + 5, 0.66))
			gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] += wDiff;
		else
		{
			wDiff = max(0, gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] - gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole]);
			wRecoverValue = min(wDiff * wPercent / 100, max(MAX_PARAMETER, gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole]) / 2);
			gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] += wRecoverValue;
		}
	}
}


//#ifdef STRENGTHEN_ENEMY
BATTLEENEMY CBattle::PAL_New_StrengthenEnemy(
	BATTLEENEMY			be
)
{
	FLOAT	fHPPerLevel = 0;
	FLOAT	fTimes = 0;
	DWORD	dwTempHp = 0;
	DWORD	dwTempExp = 0;
	DWORD	dwTempValue = 0;
	DWORD	dwTempLevel = 0;
	const WORD wOriginLevel = be.e.wLevel;

	INT index = PAL_New_GetPlayerIndexByPara(Para_Level, FALSE);
	WORD wPlayerRole = gpGlobals->rgParty[index].wPlayerRole;
	WORD wLevel = gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole];

	be.dwActualHealth = max(be.dwActualHealth, be.e.wHealth);

	//根据敌人的等级调整敌人血量
	if (be.e.wLevel > 99 && be.e.wHealth >= 65000)
	{
		be.dwActualHealth += 10000;
	}

	//根据我方角色的最低等级调整敌人血量
	if (wLevel >= be.e.wLevel + 10)
	{
		dwTempLevel = wLevel - 10;
		if (gpGlobals->wMaxPartyMemberIndex != 0)
		{
			dwTempHp = PAL_New_EstimateEnemyHealthByLevel(dwTempLevel) * 0.2;
			fHPPerLevel = min(150, be.e.wHealth / (FLOAT)max(1, be.e.wLevel));
			fHPPerLevel = max(fHPPerLevel, min(150, be.dwExp / (FLOAT)max(1, be.e.wLevel) * 10));
			dwTempHp += fHPPerLevel * dwTempLevel * 0.6;
		}
		else
		{
			dwTempHp = PAL_New_EstimateEnemyHealthByLevel(dwTempLevel) * 0.2;
			fHPPerLevel = min(100, be.e.wHealth / (FLOAT)max(1, be.e.wLevel));
			fHPPerLevel = max(fHPPerLevel, min(100, be.dwExp / (FLOAT)max(1, be.e.wLevel) * 10));
			dwTempHp += fHPPerLevel * dwTempLevel * 0.4;
		}
		dwTempHp = max(be.dwActualHealth, dwTempHp);
		fTimes = dwTempHp / (FLOAT)(be.e.wHealth + 1);
		dwTempExp = be.dwExp * fTimes * 0.75;
		dwTempExp = max(dwTempExp, be.dwExp);
		dwTempExp = min(dwTempExp, 0xFFFF);
		dwTempValue = (be.e.wCollectValue == 0) ? 0 : be.e.wCollectValue * (1 + fTimes / 20) + 1;
	}
	be.dwActualHealth = max(be.dwActualHealth, dwTempHp);
	be.dwExp = max(be.dwExp, dwTempExp);
	be.e.wCollectValue = (be.e.wCollectValue == 0) ? 0 : max(be.e.wCollectValue, dwTempValue);
	be.e.wLevel = max(dwTempLevel, be.e.wLevel);
	if (be.e.wLevel > 53)
		be.dwExp *= 1.0 * PAL_New_GetLevelUpBaseExp(be.e.wLevel) / PAL_New_GetLevelUpBaseExp(53);

	//根据我方角色的数量调整敌人血量
	if (gpGlobals->wMaxPartyMemberIndex >= 3)
	{
		be.dwActualHealth += (gpGlobals->wMaxPartyMemberIndex - 2) * be.dwActualHealth / 3;
	}

	if (gpGlobals->wMaxPartyMemberIndex == 0)
		be.dwActualHealth *= 0.8;
#ifdef FINISH_GAME_MORE_ONE_TIME	
	be.dwActualHealth += gpGlobals->bFinishGameTime * 2000 * (1 + gpGlobals->wMaxPartyMemberIndex / 5.0);
	be.dwExp += gpGlobals->bFinishGameTime * 1000;
	be.e.wCollectValue = (be.e.wCollectValue == 0) ? 0 : (be.e.wCollectValue + gpGlobals->bFinishGameTime * 2);
	be.e.wLevel = max(gpGlobals->bFinishGameTime * 90 + wOriginLevel, dwTempLevel);
#endif 


//#ifdef KO_ENEMY_FAST
	if (gConfig->is_KO_ENEMY_FAST)
		be.dwActualHealth = 1;
//#endif 

	if (be.dwActualHealth > 100)
	{
		be.dwActualHealth -= be.dwActualHealth % 10;
	}
	be.dwMaxHealth = be.dwActualHealth;

	return be;
}
//#endif


