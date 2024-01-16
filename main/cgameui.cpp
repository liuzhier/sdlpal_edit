/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
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

#include "cgameui.h"
#include <assert.h>

#include "cscript.h"

static int     g_iNumInventory = 0;
static WORD    g_wItemFlags = 0;
static BOOL    g_fNoDesc = FALSE;

#define e_NewWord_Linghuzhi  20011  
#define  PAL_ITEM_DESC_BOTTOM	(1 << 15)
#define PAL_RunTriggerScript  pCScript->PAL_RunTriggerScript
#define PAL_RunAutoScript     pCScript->PAL_RunAutoScript

CGameUI::CGameUI()
{

}

BOOL CGameUI::PAL_SwitchMenu(
	BOOL      fEnabled
)
/*++
  Purpose:

  Show a "Enable/Disable" selection box.

  Parameters:

  [IN]  fEnabled - whether the option is originally enabled or not.

  Return value:

  TRUE if user selected "Enable", FALSE if selected "Disable".

  --*/
{
	LPBOX           rgpBox[2]{};
	MENUITEM        rgMenuItem[2]{};
	int             i;
	WORD            wReturnValue;
	const SDL_Rect  rect = { 130, 100, 125, 50 };

	//
	// Create menu items
	//
	rgMenuItem[0].fEnabled = TRUE;
	rgMenuItem[0].pos = PAL_XY(145, 110);
	rgMenuItem[0].wValue = 0;
	rgMenuItem[0].wNumWord = SWITCHMENU_LABEL_DISABLE;

	rgMenuItem[1].fEnabled = TRUE;
	rgMenuItem[1].pos = PAL_XY(220, 110);
	rgMenuItem[1].wValue = 1;
	rgMenuItem[1].wNumWord = SWITCHMENU_LABEL_ENABLE;

	//
	// Create the boxes
	//
	for (i = 0; i < 2; i++)
	{
		rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(130 + 75 * i, 100), 2, TRUE);
	}

	
	{
		//VIDEO_UpdateScreen((SDL_Rect*)&rect);
		SDL_Rect bRect = rect;
		setPictureRatio(&bRect);
		VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
	}
	//
	// Activate the menu
	//
	wReturnValue = PAL_ReadMenu(rgMenuItem, 2, fEnabled ? 1 : 0, MENUITEM_COLOR, NULL);

	//
	// Delete the boxes
	//
	for (i = 0; i < 2; i++)
	{
		PAL_DeleteBox(rgpBox[i]);
	}

	{
		//VIDEO_UpdateScreen((SDL_Rect*)&rect);
		SDL_Rect bRect = rect;
		setPictureRatio(&bRect);
		VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
	}
	if (wReturnValue == MENUITEM_VALUE_CANCELLED)
	{
		return fEnabled;
	}

	return (wReturnValue == 0) ? FALSE : TRUE;
}


LPBOX CGameUI::PAL_ShowCash(
	DWORD      dwCash
)
/*++
  Purpose:

  Show the cash amount at the top left corner of the screen.

  Parameters:

  [IN]  dwCash - amount of cash.

  Return value:

  pointer to the saved screen part.

  --*/
{
	LPBOX     lpBox;

	//
	// Create the box.
	//
	lpBox = PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, TRUE);
	if (lpBox == NULL)
	{
		return NULL;
	}

	//
	// Draw the text label.
	//
	PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, FALSE, FALSE);

	//
	// Draw the cash amount.
	//
	PAL_DrawNumber(dwCash, 6, PAL_XY(49, 14), kNumColorYellow, kNumAlignRight);

	return lpBox;
}

VOID CGameUI::PAL_SystemMenu_OnItemChange(
	WORD        wCurrentItem
)
/*++
  Purpose:

  Callback function when user selected another item in the system menu.

  Parameters:

  [IN]  wCurrentItem - current selected item.

  Return value:

  None.

  --*/
{
	gpGlobals->iCurSystemMenuItem = wCurrentItem - 1;
}

BOOL CGameUI::PAL_SystemMenu(
	VOID
)
/*++
  Purpose:

  Show the system menu.

  Parameters:

  None.

  Return value:

  TRUE if user made some operations in the menu, FALSE if user cancelled.

  --*/
{
	LPBOX               lpMenuBox;
	WORD                wReturnValue;
	int                 iSlot, i, iSavedTimes;
	FILE* fp;
	const SDL_Rect      rect = { 40, 60, 100, 135 };

	//
	// Create menu items
	//
#ifdef PAL_CLASSIC
	MENUITEM        rgSystemMenuItem[5] =
	{
		// value  label                      enabled   pos
		{1, SYSMENU_LABEL_SAVE, TRUE, PAL_XY(53, 72)},
		{2, SYSMENU_LABEL_LOAD, TRUE, PAL_XY(53, 72 + 18)},
		{3, SYSMENU_LABEL_MUSIC, TRUE, PAL_XY(53, 72 + 36)},
		{4, SYSMENU_LABEL_SOUND, TRUE, PAL_XY(53, 72 + 54)},
		{5, SYSMENU_LABEL_QUIT, TRUE, PAL_XY(53, 72 + 72)},
	};
#else
	MENUITEM        rgSystemMenuItem[6] =
	{
		// value  label                      enabled   pos
		{ 1,      SYSMENU_LABEL_SAVE,        TRUE,     PAL_XY(53, 72) },
		{ 2,      SYSMENU_LABEL_LOAD,        TRUE,     PAL_XY(53, 72 + 18) },
		{ 3,      SYSMENU_LABEL_MUSIC,       TRUE,     PAL_XY(53, 72 + 36) },
		{ 4,      SYSMENU_LABEL_SOUND,       TRUE,     PAL_XY(53, 72 + 54) },
		{ 5,      SYSMENU_LABEL_BATTLEMODE,  TRUE,     PAL_XY(53, 72 + 72) },
		{ 6,      SYSMENU_LABEL_QUIT,        TRUE,     PAL_XY(53, 72 + 90) },
	};
#endif

	//
	// Create the menu box.
	//
	lpMenuBox = PAL_CreateBox(PAL_XY(40, 60), 4, 3, 0, TRUE);
	
	{
		//VIDEO_UpdateScreen((SDL_Rect*)&rect);
		SDL_Rect bRect = rect;
		setPictureRatio(&bRect);
		VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
	}
	//
	// Perform the menu.
	//
	{
		std::function<void(WORD )> obj = 
			std::bind(&CGameUI::PAL_SystemMenu_OnItemChange,this, std::placeholders::_1);
		wReturnValue = PAL_ReadMenu(rgSystemMenuItem, 5,
			gpGlobals->iCurSystemMenuItem, MENUITEM_COLOR, obj);
	}
	if (wReturnValue == MENUITEM_VALUE_CANCELLED)
	{
		//
		// User cancelled the menu
		//
		PAL_DeleteBox(lpMenuBox);
		{
			//VIDEO_UpdateScreen((SDL_Rect*)&rect);
			SDL_Rect bRect = rect;
			setPictureRatio(&bRect);
			VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
		}
		return FALSE;
	}

	switch (wReturnValue)
	{
	case 1:
		//
		// Save game
		//
		iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);

		if (iSlot != MENUITEM_VALUE_CANCELLED)
		{
			gpGlobals->bCurrentSaveSlot = (BYTE)iSlot;

			iSavedTimes = 0;
			int MaxSaveFile = gConfig->m_Function_Set[47];
			for (i = 1; i <= MaxSaveFile; i++)
			{
				fp = fopen(va("%s%d%s", PalDir.c_str(), i, ".rpg"), "rb");
				if (fp != NULL)
				{
					WORD wSavedTimes;
					fread(&wSavedTimes, sizeof(WORD), 1, fp);
					fclose(fp);
					wSavedTimes = SWAP16(wSavedTimes);
					if ((int)wSavedTimes > iSavedTimes)
					{
						iSavedTimes = wSavedTimes;
					}
				}
			}
			PAL_SaveGame(va("%s%d%s", PalDir.c_str(), iSlot, ".rpg"), iSavedTimes + 1);
		}
		break;

	case 2:
		//
		// Load game
		//
		iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);
		if (iSlot != MENUITEM_VALUE_CANCELLED)
		{
			PAL_PlayMUS(0, FALSE, 1);
			PAL_FadeOut(1);
			PAL_InitGameData(iSlot);
		}
		break;

	case 3:
		//
		// Music
		//
		g_fNoMusic = !PAL_SwitchMenu(!g_fNoMusic);

		break;

	case 4:
		//
		// Sound
		//
		g_fNoSound = !PAL_SwitchMenu(!g_fNoSound);
		break;

#ifndef PAL_CLASSIC
	case 5:
		//
		// Battle Mode
		//
		PAL_BattleSpeedMenu();
		break;

	case 6:
#else
	case 5:
#endif
		//
		// Quit
		//
		if (PAL_ConfirmMenu("退出吗？"))
		{
			PAL_PlayMUS(0, FALSE, 2);
			PAL_FadeOut(2);
			//PAL_Shutdown();
			PalQuit = TRUE;
		}
		break;
	}

	PAL_DeleteBox(lpMenuBox);
	return TRUE;
}

VOID CGameUI::PAL_InGameMagicMenu(
	VOID
)
/*++
  Purpose:

  Show the magic menu.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	MENUITEM         rgMenuItem[MAX_PLAYERS_IN_PARTY]{};
	int              i, y;
	static WORD      w;
	WORD             wMagic;
	const SDL_Rect   rect = { 35, 62, 95, 90 };

	//
	// Draw the player info boxes
	//
	y = 45;

	if (gpGlobals->wMaxPartyMemberIndex >= 3)
	{
		y = 10;
	}
	//
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
			TIMEMETER_COLOR_DEFAULT, TRUE);
		y += 78;
	}

	y = 75;

	//
	// Generate one menu items for each player in the party
	//
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		assert(i <= MAX_PLAYERS_IN_PARTY);

		rgMenuItem[i].wValue = i;
		rgMenuItem[i].wNumWord =
			gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole];
		rgMenuItem[i].fEnabled =
			(gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0);
		rgMenuItem[i].pos = PAL_XY(48, y);

		y += 18;
	}

	//
	// Draw the box
	//
	PAL_CreateBox(PAL_XY(35, 62), gpGlobals->wMaxPartyMemberIndex, 2, 0, FALSE);
	//VIDEO_UpdateScreen((SDL_Rect*)&rect);
	SDL_Rect bRect = rect;
	setPictureRatio(&bRect);
	VIDEO_UpdateScreen(gpTextureReal,&bRect,&bRect);

	w = PAL_ReadMenu(rgMenuItem, gpGlobals->wMaxPartyMemberIndex + 1, w, MENUITEM_COLOR, NULL);

	if (w == MENUITEM_VALUE_CANCELLED)
	{
		return;
	}

	wMagic = 0;

	while(!PalQuit)
	{
		wMagic = PAL_MagicSelectionMenu(gpGlobals->rgParty[w].wPlayerRole, FALSE, wMagic);
		if (wMagic == 0)
		{
			break;
		}

		if (((LPOBJECT)((LPOBJECT)gpGlobals->g.rgObject))[wMagic].magic.wFlags & kMagicFlagApplyToAll)
		{
			((LPOBJECT)((LPOBJECT)gpGlobals->g.rgObject))[wMagic].magic.wScriptOnUse =
				PAL_RunTriggerScript(((LPOBJECT)((LPOBJECT)gpGlobals->g.rgObject))[wMagic].magic.wScriptOnUse, 0);

			if (g_fScriptSuccess)
			{
				((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wScriptOnSuccess =
					PAL_RunTriggerScript(((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wScriptOnSuccess, 0);

				gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
					gpGlobals->g.lprgMagic[((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wMagicNumber].wCostMP;
			}

			if (gpGlobals->fNeedToFadeIn)
			{
				PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
				gpGlobals->fNeedToFadeIn = FALSE;
			}
		}
		else
		{
			//
			// Need to select which player to use the magic on.
			//
			WORD       wPlayer = 0;
			SDL_Rect   rect{};

			while (wPlayer != MENUITEM_VALUE_CANCELLED)
			{
				//
				// Redraw the player info boxes first
				//
				y = 45;

				if (gpGlobals->wMaxPartyMemberIndex >= 3)
				{
					y = 10;
				}

				for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
				{
					PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
						TIMEMETER_COLOR_DEFAULT, TRUE);
					y += 78;
				}

				//
				// Draw the cursor on the selected item
				//
				y = 70;
				if (gpGlobals->wMaxPartyMemberIndex >= 3)
				{
					y -= 35;
				}
				rect.x = y + 78 * wPlayer;
				rect.y = 193;
				rect.w = 9;
				rect.h = 6;

				PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
					gpTextureReal, PAL_XY(rect.x, rect.y));

				SDL_Rect bRect = rect;
				setPictureRatio(&bRect);
				VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);

				while(!PalQuit)
				{
					PAL_ClearKeyState();
					PAL_ProcessEvent();

					if (g_InputState.dwKeyPress & kKeyMenu)
					{
						wPlayer = MENUITEM_VALUE_CANCELLED;
						break;
					}
					else if (g_InputState.dwKeyPress & kKeySearch)
					{
						((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wScriptOnUse =
							PAL_RunTriggerScript(((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wScriptOnUse,
								gpGlobals->rgParty[wPlayer].wPlayerRole);

						if (g_fScriptSuccess)
						{
							((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wScriptOnSuccess =
								PAL_RunTriggerScript(((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wScriptOnSuccess,
									gpGlobals->rgParty[wPlayer].wPlayerRole);

							if (g_fScriptSuccess)
							{
								gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
									gpGlobals->g.lprgMagic[((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wMagicNumber].wCostMP;

								//
								// Check if we have run out of MP
								//
								if (gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] <
									gpGlobals->g.lprgMagic[((LPOBJECT)gpGlobals->g.rgObject)[wMagic].magic.wMagicNumber].wCostMP)
								{
									//
									// Don't go further if run out of MP
									//
									wPlayer = MENUITEM_VALUE_CANCELLED;
								}
							}
						}

						break;
					}
					else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
					{
						if (wPlayer > 0)
						{
							wPlayer--;
							break;
						}
						else
							wPlayer = gpGlobals->wMaxPartyMemberIndex;
						break;
					}
					else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown))
					{
						if (wPlayer < gpGlobals->wMaxPartyMemberIndex)
						{
							wPlayer++;
							break;
						}
						else
							wPlayer = 0;
						break;
					}

					VIDEO_UpdateScreen(gpTextureReal);
					SDL_Delay(1);
				}
			}
		}

		//
		// Redraw the player info boxes
		//
		y = 45;

		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
				TIMEMETER_COLOR_DEFAULT, TRUE);
			y += 78;
		}
	}
}

VOID CGameUI::PAL_InventoryMenu(
	VOID
)
/*++
  Purpose:

  Show the inventory menu.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	static WORD      w = 0;
	const SDL_Rect   rect = { 30, 60, 75, 60 };

	MENUITEM        rgMenuItem[2] =
	{
		// value  label                     enabled   pos
		{1, INVMENU_LABEL_USE, TRUE, PAL_XY(43, 73)},
		{2, INVMENU_LABEL_EQUIP, TRUE, PAL_XY(43, 73 + 18)},
	};

	PAL_CreateBox(PAL_XY(30, 60), 1, 1, 0, FALSE);
	{
		//VIDEO_UpdateScreen((SDL_Rect*)&rect);
		SDL_Rect bRect = rect;
		setPictureRatio(&bRect);
		VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
    }
	w = PAL_ReadMenu(rgMenuItem, 2, w - 1, MENUITEM_COLOR, NULL);

#ifdef SORT_INVENTORY
	PAL_New_SortInventory();
#endif

	switch (w)
	{
	case 1:
		PAL_GameUseItem();
		break;

	case 2:
		PAL_GameEquipItem();
		break;
	}
}

VOID CGameUI::PAL_InGameMenu_OnItemChange(
	WORD        wCurrentItem
)
/*++
  Purpose:

  Callback function when user selected another item in the in-game menu.

  Parameters:

  [IN]  wCurrentItem - current selected item.

  Return value:

  None.

  --*/
{
	gpGlobals->iCurMainMenuItem = wCurrentItem - 1;
}

VOID CGameUI::PAL_InGameMenu(
	VOID
)
/*++
  Purpose:

  Show the in-game main menu.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	LPBOX                lpCashBox, lpMenuBox;
	WORD                 wReturnValue;
	const SDL_Rect       rect = { 0, 0, 150, 185 };

	//
	// Create menu items
	//
	MENUITEM        rgMainMenuItem[4] =
	{
		// value  label                      enabled   pos
		{1, GAMEMENU_LABEL_STATUS, TRUE, PAL_XY(16, 50)},
		{2, GAMEMENU_LABEL_MAGIC, TRUE, PAL_XY(16, 50 + 18)},
		{3, GAMEMENU_LABEL_INVENTORY, TRUE, PAL_XY(16, 50 + 36)},
		{4, GAMEMENU_LABEL_SYSTEM, TRUE, PAL_XY(16, 50 + 54)},
	};

	//
	// Display the cash amount.
	//
	lpCashBox = PAL_ShowCash(gpGlobals->dwCash);

	//
	// Create the menu box.
	//
	lpMenuBox = PAL_CreateBox(PAL_XY(3, 37), 3, 1, 0, TRUE);
	SDL_Rect bRect = rect;
	setPictureRatio(&bRect);
	VIDEO_UpdateScreen(gpTextureReal,&bRect,&bRect);

	//
	// Process the menu
	//
	while(!PalQuit)
	{
		wReturnValue = PAL_ReadMenu(rgMainMenuItem, 4,gpGlobals->iCurMainMenuItem, MENUITEM_COLOR, 
			std::bind(&CGameUI::PAL_InGameMenu_OnItemChange, this, std::placeholders::_1));

		if (wReturnValue == MENUITEM_VALUE_CANCELLED)
		{
			break;
		}

		switch (wReturnValue)
		{
		case 1:
			//
			// Status
			//
			PAL_PlayerStatus();
			goto out;

		case 2:
			//
			// Magic
			//
			PAL_InGameMagicMenu();
			goto out;

		case 3:
			//
			// Inventory
			//
			PAL_InventoryMenu();
			goto out;

		case 4:
			//
			// System
			//
			if (PAL_SystemMenu())
			{
				goto out;
			}
			break;
		}
	}

out:
	//
	// Remove the boxes.
	//
	PAL_DeleteBox(lpCashBox);
	PAL_DeleteBox(lpMenuBox);

	{
		//VIDEO_UpdateScreen((SDL_Rect*)&rect);
		SDL_Rect bRect = rect;
		setPictureRatio(&bRect);
		VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
	}
}

VOID CGameUI::PAL_PlayerStatus(
	VOID
)
/*++
  Purpose:

  Show the player status.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	PAL_LARGE BYTE   bufBackground[320 * 200];
	PAL_LARGE BYTE   bufImage[16384];
	int              iCurrent;
	int              iPlayerRole;
	int              i, y;
	WORD             w;

	const int        rgEquipPos[MAX_PLAYER_EQUIPMENTS][2] = {
			{190, 0}, {248, 40}, {252, 102}, {202, 134}, {142, 142}, {82, 126}
	};

	PAL_MKFDecompressChunk(bufBackground, 320 * 200, STATUS_BACKGROUND_FBPNUM,
		gpGlobals->f.fpFBP);
	iCurrent = 0;

	while (iCurrent >= 0 && iCurrent <= gpGlobals->wMaxPartyMemberIndex && PalQuit == FALSE)
	{
		iPlayerRole = gpGlobals->rgParty[iCurrent].wPlayerRole;

		//
		// Draw the background image
		//
		PAL_FBPBlitToSurface(bufBackground, gpTextureReal);
		//
		ClearScreen();
		//
		// Draw the text labels
		//
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_EXP), PAL_XY(6, 6), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(6, 32), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(6, 54), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(6, 76), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(6, 98), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(6, 118), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(6, 138), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(6, 158), MENUITEM_COLOR, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(6, 178), MENUITEM_COLOR, TRUE, FALSE);

		PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[iPlayerRole]),
			PAL_XY(110, 8), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);

		//
		// Draw the stats
		//
		PAL_DrawNumber(gpGlobals->Exp.rgPrimaryExp[iPlayerRole].wExp, 6,
			PAL_XY(58, 6), kNumColorYellow, kNumAlignRight);

		PAL_DrawNumber(PAL_New_GetLevelUpExp(gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole]),
			6, PAL_XY(58, 15), kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole], 3,
			PAL_XY(48, 35), kNumColorYellow, kNumAlignRight);

		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
			PAL_XY(65, 58));
		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
			PAL_XY(65, 80));
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[iPlayerRole], 4, PAL_XY(42, 56),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[iPlayerRole], 4, PAL_XY(69, 65),
			kNumColorBlue, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[iPlayerRole], 4, PAL_XY(42, 78),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[iPlayerRole], 4, PAL_XY(69, 87),
			kNumColorBlue, kNumAlignRight);

		PAL_DrawNumber(PAL_GetPlayerAttackStrength(iPlayerRole), 4,
			PAL_XY(42, 102), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerMagicStrength(iPlayerRole), 4,
			PAL_XY(42, 122), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerDefense(iPlayerRole), 4,
			PAL_XY(42, 142), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerDexterity(iPlayerRole), 4,
			PAL_XY(42, 162), kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerFleeRate(iPlayerRole), 4,
			PAL_XY(42, 182), kNumColorYellow, kNumAlignRight);

		//
		// Draw the equipments
		//
		for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
		{
			w = gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole];

			if (w == 0)
			{
				continue;
			}

			//
			// Draw the image
			//
			if (PAL_MKFReadChunk(bufImage, 16384,
				((LPOBJECT)gpGlobals->g.rgObject)[w].item.wBitmap, gpGlobals->f.fpBALL) > 0)
			{
				PAL_RLEBlitToTexture(bufImage, gpTextureReal,
					PAL_XY(rgEquipPos[i][0], rgEquipPos[i][1]));
			}

			//
			// Draw the text label
			//
			PAL_DrawText(PAL_GetWord(w),
				PAL_XY(rgEquipPos[i][0] + 5, rgEquipPos[i][1] + 38), STATUS_COLOR_EQUIPMENT, TRUE, FALSE);
		}

		//
		// Draw the image of player role
		//
		if (PAL_MKFReadChunk(bufImage, 16384,
			gpGlobals->g.PlayerRoles.rgwAvatar[iPlayerRole], gpGlobals->f.fpRGM) > 0)
		{
			PAL_RLEBlitToTexture(bufImage, gpTextureReal, PAL_XY(110, 30));
		}

		//
		// Draw all poisons
		//
		y = 58;

//#ifdef POISON_STATUS_EXPAND
		int wPoisonIntensity = 0;
//#endif

		for (i = 0; i < MAX_POISONS; i++)
		{
			w = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonID;

			if (w != 0 && ((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wPoisonLevel <= MAX_POISON_LEVEL)
			{
				if (i > 7)
				{
					if (i == 8)
					{
						y = 58;
					}
					PAL_DrawText(PAL_GetWord(w), PAL_XY(260, y),
						(BYTE)(((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wColor + 10), TRUE, FALSE);

//#ifdef POISON_STATUS_EXPAND
					if (gConfig->m_Function_Set[0]) {
						wPoisonIntensity = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonIntensity;
						if (wPoisonIntensity != 0)
						{
							PAL_DrawNumber(wPoisonIntensity, 2,
								PAL_XY(310, y + 4), kNumColorYellow, kNumAlignRight);
						}
					}
//#endif

					y += 18;
				}
				else
				{
					PAL_DrawText(PAL_GetWord(w), PAL_XY(185, y),
						(BYTE)(((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wColor + 10), TRUE, FALSE);

//#ifdef POISON_STATUS_EXPAND
					if (gConfig->m_Function_Set[0])
					{
						wPoisonIntensity = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonIntensity;
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
			PAL_Delay(5);
			//
			VIDEO_UpdateScreen(gpTextureReal);

			if (g_InputState.dwKeyPress & kKeyMenu)
			{
				iCurrent = -1;
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
			{
				iCurrent--;
				break;
			}
			else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown | kKeySearch))
			{
				iCurrent++;
				break;
			}
		}
	}
}

WORD CGameUI::PAL_ItemUseMenu(
	WORD           wItemToUse
)
/*++
  Purpose:

  Show the use item menu.

  Parameters:

  [IN]  wItemToUse - the object ID of the item to use.

  Return value:

  The selected player to use the item onto.
  MENUITEM_VALUE_CANCELLED if user cancelled.

  --*/
{
	BYTE           bColor, bSelectedColor;
	PAL_LARGE BYTE bufImage[2048];
	DWORD          dwColorChangeTime;
	static WORD    wSelectedPlayer = 0;
	SDL_Rect       rect = { 110, 2, 200, 180 };
	int            i;

	bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
	dwColorChangeTime = 0;

	while(!PalQuit)
	{
		if (wSelectedPlayer > gpGlobals->wMaxPartyMemberIndex)
		{
			wSelectedPlayer = 0;
		}

		//
		// Draw the box
		//
		PAL_CreateBox(PAL_XY(110, 2), 7, 9, 0, FALSE);

		//
		// Draw the stats of the selected player
		//
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(200, 16),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(200, 34),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(200, 52),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(200, 70),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(200, 88),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(200, 106),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(200, 124),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
		PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(200, 142),
			ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);

		i = gpGlobals->rgParty[wSelectedPlayer].wPlayerRole;

		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[i], 4, PAL_XY(240, 20),
			kNumColorYellow, kNumAlignRight);

		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
			PAL_XY(263, 38));
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[i], 4,
			PAL_XY(261, 44), kNumColorBlue, kNumAlignRight,7);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[i], 4,
			PAL_XY(240, 33), kNumColorYellow, kNumAlignRight,7);

		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
			PAL_XY(263, 56));
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[i], 4,
			PAL_XY(261, 64), kNumColorBlue, kNumAlignRight,7);
		PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[i], 4,
			PAL_XY(240, 53), kNumColorYellow, kNumAlignRight,7);

		PAL_DrawNumber(PAL_GetPlayerAttackStrength(i), 4, PAL_XY(240, 74),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerMagicStrength(i), 4, PAL_XY(240, 92),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerDefense(i), 4, PAL_XY(240, 110),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerDexterity(i), 4, PAL_XY(240, 128),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerFleeRate(i), 4, PAL_XY(240, 146),
			kNumColorYellow, kNumAlignRight);

		//
		// Draw the names of the players in the party
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			if (i == wSelectedPlayer)
			{
				bColor = bSelectedColor;
			}
			else
			{
				bColor = MENUITEM_COLOR;
			}

			PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole]),
				PAL_XY(125, 16 + 20 * i), bColor, TRUE, FALSE);
		}

		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpTextureReal,
			PAL_XY(120, 80));

		i = PAL_GetItemAmount(wItemToUse);

		if (i > 0)
		{
			//
			// Draw the picture of the item
			//
			if (PAL_MKFReadChunk(bufImage, 2048,
				((LPOBJECT)gpGlobals->g.rgObject)[wItemToUse].item.wBitmap, gpGlobals->f.fpBALL) > 0)
			{
				PAL_RLEBlitToTexture(bufImage, gpTextureReal, PAL_XY(127, 88));
			}

			//
			// Draw the amount and label of the item
			//
			PAL_DrawText(PAL_GetWord(wItemToUse), PAL_XY(116, 143), STATUS_COLOR_EQUIPMENT,
				TRUE, FALSE);
			PAL_DrawNumber(i, 2, PAL_XY(170, 133), kNumColorCyan, kNumAlignRight);
		}

		//
		// Update the screen area
		//
		
		//VIDEO_UpdateScreen(&rect);
		{
			SDL_Rect bRect = rect;
			setPictureRatio(&bRect);
			VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
		}
		//
		// Wait for key
		//
		PAL_ClearKeyState();

		while(!PalQuit)
		{
			//
			// See if we should change the highlight color
			//
			if (SDL_GetTicks_New() > dwColorChangeTime)
			{
				if ((WORD)bSelectedColor + 1 >=
					(WORD)MENUITEM_COLOR_SELECTED_FIRST + MENUITEM_COLOR_SELECTED_TOTALNUM)
				{
					bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
				}
				else
				{
					bSelectedColor++;
				}

				dwColorChangeTime = SDL_GetTicks_New() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

				//
				// Redraw the selected item.
				//
				PAL_DrawText(
					PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[wSelectedPlayer].wPlayerRole]),
					PAL_XY(125, 16 + 20 * wSelectedPlayer), bSelectedColor, FALSE, TRUE);
			}

			PAL_ProcessEvent();

			if (g_InputState.dwKeyPress != 0)
			{
				break;
			}

			VIDEO_UpdateScreen(gpTextureReal);
			SDL_Delay(1);
		}

		if (i <= 0)
		{
			return MENUITEM_VALUE_CANCELLED;
		}

		if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
		{
			if (wSelectedPlayer)
				wSelectedPlayer--;
			else
				wSelectedPlayer = gpGlobals->wMaxPartyMemberIndex;
		}
		else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
		{
			if (wSelectedPlayer < gpGlobals->wMaxPartyMemberIndex)
			{
				wSelectedPlayer++;
			}
			else
				wSelectedPlayer = 0;
		}
		else if (g_InputState.dwKeyPress & kKeyMenu)
		{
			break;
		}
		else if (g_InputState.dwKeyPress & kKeySearch)
		{
			return gpGlobals->rgParty[wSelectedPlayer].wPlayerRole;
		}
	}

	return MENUITEM_VALUE_CANCELLED;
}

VOID CGameUI::PAL_BuyMenu_OnItemChange(
	WORD           wCurrentItem
)
/*++
  Purpose:

  Callback function which is called when player selected another item
  in the buy menu.

  Parameters:

  [IN]  wCurrentItem - current item on the menu, indicates the object ID of
  the currently selected item.

  Return value:

  None.

  --*/
{
	const SDL_Rect      rect = { 20, 8, 128, 175 };
	int                 i, n;
	PAL_LARGE BYTE      bufImage[2048];

	//
	// Draw the picture of current selected item
	//
	PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpTextureReal,
		PAL_XY(35, 8));

	if (PAL_MKFReadChunk(bufImage, 2048,
		((LPOBJECT)gpGlobals->g.rgObject)[wCurrentItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
	{
		PAL_RLEBlitToTexture(bufImage, gpTextureReal, PAL_XY(42, 16));
	}

	//
	// See how many of this item we have in the inventory
	//
	n = 0;

	for (i = 0; i < MAX_INVENTORY; i++)
	{
		if (gpGlobals->rgInventory[i].wItem == 0)
		{
			break;
		}
		else if (gpGlobals->rgInventory[i].wItem == wCurrentItem)
		{
			n = gpGlobals->rgInventory[i].nAmount;
			break;
		}
	}
	//
	//
	// Draw the amount of this item in the inventory
	//
	//char m_buf[50];
	PAL_CreateSingleLineBox(PAL_XY(20, 105), 5, FALSE);
	PAL_DrawText(PAL_GetWord(BUYMENU_LABEL_CURRENT), PAL_XY(30, 115), 0, FALSE, FALSE);
	PAL_DrawNumber(n, 6, PAL_XY(69, 119), kNumColorYellow, kNumAlignRight);
	//PAL_DrawText(itoa(n, m_buf, 10), PAL_XY(69, 119), 0, FALSE, FALSE, 10);

	//
	// Draw the cash amount
	//
	PAL_CreateSingleLineBox(PAL_XY(20, 145), 5, FALSE);
	PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(30, 155), 0, FALSE, FALSE);
	//PAL_DrawText(itoa(gpGlobals->dwCash, m_buf, 10), PAL_XY(69, 159), 0, FALSE, FALSE, 10);
	PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(69, 159), kNumColorYellow, kNumAlignRight);

	//VIDEO_UpdateScreen(&rect);
	setPictureRatio((SDL_Rect *)&rect);
	VIDEO_UpdateScreen(gpTextureReal, &rect, &rect);
}

//#ifdef  New_EDIT_SCRIPT_OPERATION_0X0034
//修改灵狐炼丹方式为商店
VOID CGameUI::PAL_BuyMenu_OnCollectValueChange(WORD wCurrentItem)
{
	const SDL_Rect      rect = { 10, 8, 128, 175 };
	int                 i, n;
	PAL_LARGE BYTE      bufImage[2048];

	ClearScreen(&rect); 
	//
	// Draw the picture of current selected item
	//
	PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpTextureReal,
		PAL_XY(35, 8));

	if (PAL_MKFReadChunk(bufImage, 2048,
		((LPOBJECT)gpGlobals->g.rgObject)[wCurrentItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
	{
		PAL_RLEBlitToTexture(bufImage, gpTextureReal, PAL_XY(42, 16));
	}

	//
	// See how many of this item we have in the inventory
	//
	n = 0;

	for (i = 0; i < MAX_INVENTORY; i++)
	{
		if (gpGlobals->rgInventory[i].wItem == 0)
		{
			break;
		}
		else if (gpGlobals->rgInventory[i].wItem == wCurrentItem)
		{
			n = gpGlobals->rgInventory[i].nAmount;
			break;
		}
	}

	//
	// Draw the amount of this item in the inventory
	//
	PAL_CreateSingleLineBox(PAL_XY(20, 105), 5, FALSE);
	PAL_DrawText(PAL_GetWord(BUYMENU_LABEL_CURRENT), PAL_XY(30, 115), 0, FALSE, FALSE);
	PAL_DrawNumber(n, 6, PAL_XY(69, 119), kNumColorYellow, kNumAlignRight);

	//
	// Draw the cash amount
	//
	PAL_CreateSingleLineBox(PAL_XY(20, 145), 5, FALSE);
	PAL_DrawText(PAL_GetWord(e_NewWord_Linghuzhi), PAL_XY(30, 155), 0, FALSE, FALSE);
	PAL_DrawNumber(gpGlobals->wCollectValue, 6, PAL_XY(69, 159), kNumColorYellow, kNumAlignRight);

	{
		//VIDEO_UpdateScreen(&rect);
		SDL_Rect bRect = rect;
		setPictureRatio(&bRect);
		VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
	}
}
//#endif


VOID CGameUI::PAL_BuyMenu(
	WORD           wStoreNum
)
/*++
  Purpose:

  Show the buy item menu.

  Parameters:

  [IN]  wStoreNum - number of the store to buy items from.

  Return value:

  None.
//
  --*/
{
	MENUITEM        rgMenuItem[MAX_STORE_ITEM]{};
	int             i, y;
	WORD            w;
	SDL_Rect        rect = { 125, 8, 190, 190 };

	//
	// create the menu items
	//
	y = 22;

	for (i = 0; i < MAX_STORE_ITEM; i++)
	{
		if (gpGlobals->g.lprgStore[wStoreNum].rgwItems[i] == 0)
		{
			break;
		}

		rgMenuItem[i].wValue = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
		rgMenuItem[i].wNumWord = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].pos = PAL_XY(150, y);

		y += 18;
	}

	//
	// Draw the box
	//
	PAL_CreateBox(PAL_XY(125, 8), 8, 8, 1, FALSE);


	w = 0;

	while(!PalQuit)
	{

		//
		// Draw the number of prices
		//
		for (int y1 = 0,w1; y1 < i; y1++)
		{
			if (wStoreNum == 0)
			{
				w1 = y1 + 1;
			}
			else
			{
				w1 = ((LPOBJECT)gpGlobals->g.rgObject)[rgMenuItem[y1].wValue].item.wPrice;
			}
			PAL_DrawNumber(w1, 6, PAL_XY(235, 25 + y1 * 18), kNumColorCyan, kNumAlignRight);
		}

		//VIDEO_UpdateScreen(&rect);
		{
			SDL_Rect bRect = rect;
			setPictureRatio(&bRect);
			VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
		}
		if (wStoreNum == 0)
		{
//#ifdef  New_EDIT_SCRIPT_OPERATION_0X0034
			if (gConfig->m_Function_Set[4])			w = PAL_ReadMenu(rgMenuItem, i, w, MENUITEM_COLOR,
				std::bind(&CGameUI::PAL_BuyMenu_OnCollectValueChange, this, std::placeholders::_1));
//#endif
			;
		}
		else
			w = PAL_ReadMenu(rgMenuItem, i, w, MENUITEM_COLOR,
				std::bind(&CGameUI::PAL_BuyMenu_OnItemChange, this, std::placeholders::_1));

		if (w == MENUITEM_VALUE_CANCELLED)
		{
			break;
		}

		if (wStoreNum == 0)
		{
//#ifdef  New_EDIT_SCRIPT_OPERATION_0X0034
			if (gConfig->m_Function_Set[4])
			{
				//
				for (y = 0; y < i && w != rgMenuItem[y].wValue; y++);

				if (y < gpGlobals->wCollectValue)
				{
					if (PAL_ConfirmMenu())
					{
						gpGlobals->wCollectValue -= y + 1;
						PAL_AddItemToInventory(w, 1);
					}
				}
			}
//#endif
		}
		else if (((LPOBJECT)gpGlobals->g.rgObject)[w].item.wPrice <= gpGlobals->dwCash)
		{
			if (PAL_ConfirmMenu())
			{
				//
				// Player bought an item
				//
				gpGlobals->dwCash -= ((LPOBJECT)gpGlobals->g.rgObject)[w].item.wPrice;
				PAL_AddItemToInventory(w, 1);
			}
		}

		//
		// Place the cursor to the current item on next loop
		//
		for (y = 0; y < i; y++)
		{
			if (w == rgMenuItem[y].wValue)
			{
				w = y;
				break;
			}
		}
	}
}

VOID CGameUI::PAL_SellMenu_OnItemChange(
	WORD         wCurrentItem
)
/*++
  Purpose:

  Callback function which is called when player selected another item
  in the sell item menu.

  Parameters:

  [IN]  wCurrentItem - current item on the menu, indicates the object ID of
  the currently selected item.

  Return value:

  None.

  --*/
{
	//
	// Draw the cash amount
	//
	PAL_CreateSingleLineBox(PAL_XY(100, 150), 5, FALSE);
	PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(110, 160), 0, FALSE, FALSE);
	PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(149, 164), kNumColorYellow, kNumAlignRight);

	//
	// Draw the price
	//
	PAL_CreateSingleLineBox(PAL_XY(220, 150), 5, FALSE);

	if (((LPOBJECT)gpGlobals->g.rgObject)[wCurrentItem].item.wFlags & kItemFlagSellable)
	{
		PAL_DrawText(PAL_GetWord(SELLMENU_LABEL_PRICE), PAL_XY(230, 160), 0, FALSE, FALSE);
		PAL_DrawNumber(((LPOBJECT)gpGlobals->g.rgObject)[wCurrentItem].item.wPrice / 2, 6,
			PAL_XY(269, 164), kNumColorYellow, kNumAlignRight);
	}
}

VOID CGameUI::PAL_SellMenu(
	VOID
)
/*++
  Purpose:

  Show the sell item menu.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	WORD      w;

	while(!PalQuit)
	{
		auto obj = std::bind(&CGameUI::PAL_SellMenu_OnItemChange, this, std::placeholders::_1);
		w = PAL_ItemSelectMenu(obj, kItemFlagSellable);
		if (w == 0)
		{
			break;
		}

		if (PAL_ConfirmMenu())
		{
			if (PAL_AddItemToInventory(w, -1))
			{
				gpGlobals->dwCash += ((LPOBJECT)gpGlobals->g.rgObject)[w].item.wPrice / 2;
			}
		}
	}
}

VOID CGameUI::PAL_EquipItemMenu(
	WORD        wItem
)
/*++
  Purpose:

  Show the menu which allow players to equip the specified item.

  Parameters:

  [IN]  wItem - the object ID of the item.

  Return value:

  None.

  --*/
{
	PAL_LARGE BYTE   bufBackground[320 * 200];
	PAL_LARGE BYTE   bufImage[2048];
	WORD             w;
	int              iCurrentPlayer, i;
	BYTE             bColor, bSelectedColor;
	DWORD            dwColorChangeTime;

	gpGlobals->wLastUnequippedItem = wItem;

	PAL_MKFDecompressChunk(bufBackground, 320 * 200, EQUIPMENU_BACKGROUND_FBPNUM,
		gpGlobals->f.fpFBP);

	iCurrentPlayer = 0;
	bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
	dwColorChangeTime = SDL_GetTicks_New() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

	while(!PalQuit)
	{
		wItem = gpGlobals->wLastUnequippedItem;

		//
		// Draw the background
		//
		PAL_FBPBlitToSurface(bufBackground, gpTextureReal);
		ClearScreen();
		//
		// Draw the item picture
		//
		if (PAL_MKFReadChunk(bufImage, 2048,
			((LPOBJECT)gpGlobals->g.rgObject)[wItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
		{
			PAL_RLEBlitToTexture(bufImage, gpTextureReal, PAL_XY(16, 16));
		}

		//
		// Draw the current equipment of the selected player
		//
		w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;
		for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
		{
			if (gpGlobals->g.PlayerRoles.rgwEquipment[i][w] != 0)
			{
				PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwEquipment[i][w]),
					PAL_XY(130, 11 + i * 22), MENUITEM_COLOR, TRUE, FALSE);
			}
		}

		//
		// Draw the stats of the currently selected player
		//
		PAL_DrawNumber(PAL_GetPlayerAttackStrength(w), 4, PAL_XY(260, 14),
			kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerMagicStrength(w), 4, PAL_XY(260, 36),
			kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerDefense(w), 4, PAL_XY(260, 58),
			kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerDexterity(w), 4, PAL_XY(260, 80),
			kNumColorCyan, kNumAlignRight);
		PAL_DrawNumber(PAL_GetPlayerFleeRate(w), 4, PAL_XY(260, 102),
			kNumColorCyan, kNumAlignRight);

		//
		// Draw a box for player selection
		//
		PAL_CreateBox(PAL_XY(2, 95), gpGlobals->wMaxPartyMemberIndex, 2, 0, FALSE);

		//
		// Draw the label of players
		//
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;

			if (iCurrentPlayer == i)
			{
				if (((LPOBJECT)gpGlobals->g.rgObject)[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
				{
					bColor = bSelectedColor;
				}
				else
				{
					bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
				}
			}
			else
			{
				if (((LPOBJECT)gpGlobals->g.rgObject)[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
				{
					bColor = MENUITEM_COLOR;
				}
				else
				{
					bColor = MENUITEM_COLOR_INACTIVE;
				}
			}

			PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
				PAL_XY(15, 108 + 18 * i), bColor, TRUE, FALSE);
		}

		//
		// Draw the text label and amount of the item
		//
		if (wItem != 0)
		{
			PAL_DrawText(PAL_GetWord(wItem), PAL_XY(5, 70), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, 13);
			PAL_DrawNumber(PAL_GetItemAmount(wItem), 2, PAL_XY(75, 73), kNumColorCyan, kNumAlignRight);
		}

		//
		// Update the screen
		//
		VIDEO_UpdateScreen(gpTextureReal);

		//
		// Accept input
		//
		PAL_ClearKeyState();

		while(!PalQuit)
		{
			PAL_ProcessEvent();

			//
			// See if we should change the highlight color
			//
			if (SDL_GetTicks_New() > dwColorChangeTime)
			{
				if ((WORD)bSelectedColor + 1 >=
					(WORD)MENUITEM_COLOR_SELECTED_FIRST + MENUITEM_COLOR_SELECTED_TOTALNUM)
				{
					bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
				}
				else
				{
					bSelectedColor++;
				}

				dwColorChangeTime = SDL_GetTicks_New() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

				//
				// Redraw the selected item if needed.
				//
				w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

				if (((LPOBJECT)gpGlobals->g.rgObject)[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
				{
					PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
						PAL_XY(15, 108 + 18 * iCurrentPlayer), bSelectedColor, TRUE, TRUE);
				}
			}

			if (g_InputState.dwKeyPress != 0)
			{
				break;
			}

			VIDEO_UpdateScreen(gpTextureReal);
			SDL_Delay(1);
		}

		if (wItem == 0)
		{
			return;
		}

		if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
		{
			iCurrentPlayer--;
			if (iCurrentPlayer < 0)
			{
				//iCurrentPlayer = 0;
				iCurrentPlayer = gpGlobals->wMaxPartyMemberIndex;
			}
		}
		else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
		{
			iCurrentPlayer++;
			if (iCurrentPlayer > gpGlobals->wMaxPartyMemberIndex)
			{
				//iCurrentPlayer = gpGlobals->wMaxPartyMemberIndex;
				iCurrentPlayer = 0;
			}
		}
		else if (g_InputState.dwKeyPress & kKeyMenu)
		{
			return;
		}
		else if (g_InputState.dwKeyPress & kKeySearch)
		{
			w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

			if (((LPOBJECT)gpGlobals->g.rgObject)[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
			{
				//
				// Run the equip script
				//
				((LPOBJECT)gpGlobals->g.rgObject)[wItem].item.wScriptOnEquip =
					PAL_RunTriggerScript(((LPOBJECT)gpGlobals->g.rgObject)[wItem].item.wScriptOnEquip,
						gpGlobals->rgParty[iCurrentPlayer].wPlayerRole);
			}
		}
	}
}

VOID CGameUI::PAL_GameUseItem(
	VOID
)
/*++
  Purpose:

  Allow player use an item in the game.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	WORD         wObject;

	while(!PalQuit)
	{
		wObject = PAL_ItemSelectMenu(NULL, kItemFlagUsable);

		if (wObject == 0)
		{
			return;
		}

		if (!(((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wFlags & kItemFlagApplyToAll))
		{
			//
			// Select the player to use the item on
			//
			WORD     wPlayer = 0;

			while(!PalQuit)
			{
				wPlayer = PAL_ItemUseMenu(wObject);

				if (wPlayer == MENUITEM_VALUE_CANCELLED)
				{
					break;
				}

				//
				// Run the script
				//
				((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wScriptOnUse =
					PAL_RunTriggerScript(((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wScriptOnUse, wPlayer);

				//
				// Remove the item if the item is consuming and the script succeeded
				//
				if ((((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wFlags & kItemFlagConsuming) &&
					g_fScriptSuccess)
				{
					PAL_AddItemToInventory(wObject, -1);
				}
			}
		}
		else
		{
			//
			// Run the script
			//
			((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wScriptOnUse =
				PAL_RunTriggerScript(((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wScriptOnUse, 0xFFFF);

			//
			// Remove the item if the item is consuming and the script succeeded
			//
			if ((((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wFlags & kItemFlagConsuming) &&
				g_fScriptSuccess)
			{
				PAL_AddItemToInventory(wObject, -1);
			}

			return;
		}
	}
}

WORD CGameUI::PAL_ItemSelectMenu(
	LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged,
	WORD                      wItemFlags
)
/*++
  Purpose:

	Show the item selection menu.

  Parameters:

	[IN]  lpfnMenuItemChanged - Callback function which is called when user
								changed the current menu item.

	[IN]  wItemFlags - flags for usable item.

  Return value:

	The object ID of the selected item. 0 if cancelled.

--*/
{
	int              iPrevIndex;
	WORD             w;
	DWORD            dwTime;

	PAL_ItemSelectMenuInit(wItemFlags);
	iPrevIndex = gpGlobals->iCurInvMenuItem;

	PAL_ClearKeyState();

	if (lpfnMenuItemChanged != NULL)
	{
		g_fNoDesc = TRUE;
		(lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
	}

	dwTime = SDL_GetTicks_New();

	while(!PalQuit)
	{
		if (lpfnMenuItemChanged == NULL)
		{
			PAL_MakeScene();
		}

		w = PAL_ItemSelectMenuUpdate();
		VIDEO_UpdateScreen(gpTextureReal);

		PAL_ClearKeyState();

		PAL_ProcessEvent();
		while (SDL_GetTicks_New() < dwTime)
		{
			PAL_ProcessEvent();
			if (g_InputState.dwKeyPress != 0)
			{
				break;
			}
			SDL_Delay(5);
		}

		dwTime = SDL_GetTicks_New() + gConfig->m_Function_Set[24];

		if (w != 0xFFFF)
		{
			g_fNoDesc = FALSE;
			return w;
		}

		if (iPrevIndex != gpGlobals->iCurInvMenuItem)
		{
			if (gpGlobals->iCurInvMenuItem >= 0 && gpGlobals->iCurInvMenuItem < MAX_INVENTORY)
			{
				if (lpfnMenuItemChanged != NULL)
				{
					(lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
				}
			}

			iPrevIndex = gpGlobals->iCurInvMenuItem;
		}
	}
	if (PalQuit)return 0;
	assert(FALSE);
	return 0; // should not really reach here
}

VOID CGameUI::PAL_ItemSelectMenuInit(
	WORD                      wItemFlags
)
/*++
  Purpose:

	Initialize the item selection menu.

  Parameters:

	[IN]  wItemFlags - flags for usable item.

  Return value:

	None.

--*/
{
	int           i, j;
	WORD          w;

	g_wItemFlags = wItemFlags;

	//
	// Compress the inventory
	//
	PAL_CompressInventory();

	//
	// Count the total number of items in inventory
	//
	g_iNumInventory = 0;
	while (g_iNumInventory < MAX_INVENTORY &&
		gpGlobals->rgInventory[g_iNumInventory].wItem != 0)
	{
		g_iNumInventory++;
	}

	//
	// Also add usable equipped items to the list
	//
	if ((wItemFlags & kItemFlagUsable) && !fInBattle)
	{
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;

			for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
			{
				if (((LPOBJECT)gpGlobals->g.rgObject)[gpGlobals->g.PlayerRoles.rgwEquipment[j][w]].item.wFlags & kItemFlagUsable)
				{
					if (g_iNumInventory < MAX_INVENTORY)
					{
						gpGlobals->rgInventory[g_iNumInventory].wItem = gpGlobals->g.PlayerRoles.rgwEquipment[j][w];
						gpGlobals->rgInventory[g_iNumInventory].nAmount = 0;
						gpGlobals->rgInventory[g_iNumInventory].nAmountInUse = (WORD)-1;

						g_iNumInventory++;
					}
				}
			}
		}
	}
}

VOID CGameUI::PAL_PlayerInfoBox(
	PAL_POS         pos,
	WORD            wPlayerRole,
	INT             iTimeMeter,
	BYTE            bTimeMeterColor,
	BOOL            fUpdate
)
/*++
  Purpose:    Show the player info box.
  显示玩家信息栏。
  Parameters:    [IN]  pos - the top-left corner position of the box.
  信息栏左上角的位置。
  [IN]  wPlayerRole - the player role ID to be shown.
  显示玩家角色的名称。
  [IN]  iTimeMeter - the value of time meter. 0 = empty, 100 = full.
  计时器的值。0位空，100为满。
  [IN]  bTimeMeterColor - the color of time meter.
  计时器的颜色
  [IN]  fUpdate - whether to update the screen area or not.
  是否更新屏幕区域
  Return value:    None.
  返回值：  无。
  --*/
{
	SDL_Rect        rect{};
	BYTE            bPoisonColor;
	int             i, iPartyIndex;
	WORD            wMaxLevel, w;

	const BYTE      rgStatusPos[kStatusAll][2] =
	{
		{35, 19},  // confused	// 混乱——随机攻击友方单位
		{34, 0},   // slow		// 减速——速度变慢
		{54, 1},   // sleep		// 睡眠——无法行动
		{55, 20},  // silence		// 咒封——无法施放魔法
		{0, 0},    // puppet		// 傀儡——能持续攻击（仅对已死亡的角色有效）
		{0, 0},    // bravery		// 勇气——增加物理攻击的攻击力
		{0, 0},    // protect		// 防护——防御值上升
		{0, 0},    // haste		// 急速——速度提升
		{0, 0},    // dualattack	// 双重攻击
	};
	//各种状态对应显示的文字
	const WORD      rgwStatusWord[kStatusAll] =
	{
		0x1D,  // confused
		0x1B,  // slow
		0x1C,  // sleep
		0x1A,  // silence
		0x00,  // puppet
		0x00,  // bravery
		0x00,  // protect
		0x00,  // haste
		0x00,  // dualattack
	};
	//各种状态对应显示的文字的颜色
	const BYTE      rgbStatusColor[kStatusAll] =
	{
		0x5F,  // confused
		0x0E,  // slow
		0x0E,  // sleep
		0x3C,  // silence
		0x00,  // puppet
		0x00,  // bravery
		0x00,  // protect
		0x00,  // haste
		0x00,  // dualattack
	};
	//清空区域内文字
	rect.x = PAL_X(pos) - 2;
	rect.y = PAL_Y(pos) - 4;
	rect.w = 77;
	rect.h = 39;
	//ClearScreen( &rect);
	//
	// Draw the box
	//
	PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERINFOBOX),
		gpTextureReal, pos);

	//
	// Draw the player face
	//
	wMaxLevel = 0;
	bPoisonColor = 0xFF;

	for (iPartyIndex = 0; iPartyIndex <= gpGlobals->wMaxPartyMemberIndex; iPartyIndex++)
	{
		if (gpGlobals->rgParty[iPartyIndex].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}
	// 根据每个角色所中的毒来更改角色脸部颜色
	if (iPartyIndex <= gpGlobals->wMaxPartyMemberIndex)
	{
		for (i = 0; i < MAX_POISONS; i++)//MAX_POISONS在每个角色身上同时有效的毒的最大数量，游戏规定为16。
		{
			w = gpGlobals->rgPoisonStatus[i][iPartyIndex].wPoisonID;
			if (w != 0 && ((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wColor != 0)
			{
				if (((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wPoisonLevel >= wMaxLevel)
				{//找出等级最大的毒
					wMaxLevel = ((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wPoisonLevel;
					bPoisonColor = (BYTE)(((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wColor);
				}
			}
		}
	}

	if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
	{
		//
		// Always use the black/white color for dead players
		// and do not use the time meter
		//
		bPoisonColor = 0;
		iTimeMeter = 0;
	}

	if (bPoisonColor == 0xFF)// 若角色未中毒
	{
		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERFACE_FIRST + wPlayerRole),
			gpTextureReal, PAL_XY(PAL_X(pos) - 2, PAL_Y(pos) - 4));
	}
	else// 若角色中毒
	{
		PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERFACE_FIRST + wPlayerRole),
			gpTextureReal, PAL_XY(PAL_X(pos) - 2, PAL_Y(pos) - 4), bPoisonColor, 0);
	}

	//
	// Draw the HP and MP value
	//

	PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
		PAL_XY(PAL_X(pos) + 46, PAL_Y(pos) + 6));
	PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole], 5,
		PAL_XY(PAL_X(pos) + 48, PAL_Y(pos) + 8), kNumColorYellow, kNumAlignLeft);
	PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole], 5,
		PAL_XY(PAL_X(pos) + 18, PAL_Y(pos) + 5), kNumColorYellow, kNumAlignRight);

	PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpTextureReal,
		PAL_XY(PAL_X(pos) + 46, PAL_Y(pos) + 22));
	PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole], 5,
		PAL_XY(PAL_X(pos) + 48, PAL_Y(pos) + 24), kNumColorCyan, kNumAlignLeft);
	PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole], 5,
		PAL_XY(PAL_X(pos) + 18, PAL_Y(pos) + 21), kNumColorCyan, kNumAlignRight);

	//
	// Draw Statuses
	//
	if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
	{
		for (i = 0; i < kStatusAll; i++)
		{
			if (gpGlobals->rgPlayerStatus[wPlayerRole][i] > 0 &&
				rgwStatusWord[i] != 0)
			{
				PAL_DrawText(PAL_GetWord(rgwStatusWord[i]),
					PAL_XY(PAL_X(pos) + rgStatusPos[i][0], PAL_Y(pos) + rgStatusPos[i][1]),
					rgbStatusColor[i], TRUE, FALSE);
			}
		}
	}

	//
	// Update the screen area if needed
	//
	if (fUpdate)
	{
		rect.x = PAL_X(pos) - 2;
		rect.y = PAL_Y(pos) - 4;
		rect.w = 77;
		rect.h = 39;

		setPictureRatio(&rect);
		VIDEO_UpdateScreen(gpTextureReal, &rect, &rect);
		//VIDEO_UpdateScreen(&rect);
	}
}

WORD CGameUI::PAL_MagicSelectionMenu(
	WORD         wPlayerRole,
	BOOL         fInBattle,
	WORD         wDefaultMagic
)
/*++
  Purpose:

	Show the magic selection menu.

  Parameters:

	[IN]  wPlayerRole - the player ID.

	[IN]  fInBattle - TRUE if in battle, FALSE if not.

	[IN]  wDefaultMagic - the default magic item.

  Return value:

	The selected magic. 0 if cancelled.

--*/
{
	WORD            w;
	int             i;
	DWORD           dwTime;

	PAL_MagicSelectionMenuInit(wPlayerRole, fInBattle, wDefaultMagic);
	PAL_ClearKeyState();

	dwTime = SDL_GetTicks_New();

	while(!PalQuit)
	{
		PAL_MakeScene();

		w = 45;

		if (gpGlobals->wMaxPartyMemberIndex >= 3)
		{
			w = 10;
		}

		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			PAL_PlayerInfoBox(PAL_XY(w, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
				TIMEMETER_COLOR_DEFAULT, FALSE);
			w += 78;
		}

		w = PAL_MagicSelectionMenuUpdate();
		VIDEO_UpdateScreen(gpTextureReal);

		PAL_ClearKeyState();

		if (w != 0xFFFF)
		{
			return w;
		}

		PAL_ProcessEvent();
		while (SDL_GetTicks_New() < dwTime)
		{
			PAL_ProcessEvent();
			if (g_InputState.dwKeyPress != 0 )
			{
				break;
			}
			SDL_Delay(1);
		}

		dwTime = SDL_GetTicks_New() + FRAME_TIME;
	}

	return 0; // should not really reach here
}

WORD CGameUI::PAL_MagicSelectionMenuUpdate(
	VOID
)
/*++
  Purpose:

	Update the magic selection menu.

  Parameters:

	None.

  Return value:

	The selected magic. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
/*/#ifndef PAL_WIN95
	int         i, j, k;
	BYTE        bColor;
#else
	int         i, j, k, line;
	BYTE        bColor;
	WORD        wScript;
#endif
*/
	int         i, j, k, line;
	BYTE        bColor;
	WORD        wScript;

	//
	// Check for inputs
	//
	if (g_InputState.dwKeyPress & kKeyUp)
	{
		g_iCurrentItem -= 3;
	}
	else if (g_InputState.dwKeyPress & kKeyDown)
	{
		g_iCurrentItem += 3;
	}
	else if (g_InputState.dwKeyPress & kKeyLeft)
	{
		g_iCurrentItem--;
	}
	else if (g_InputState.dwKeyPress & kKeyRight)
	{
		g_iCurrentItem++;
	}
	else if (g_InputState.dwKeyPress & kKeyPgUp)
	{
		g_iCurrentItem -= 3 * 5;
	}
	else if (g_InputState.dwKeyPress & kKeyPgDn)
	{
		g_iCurrentItem += 3 * 5;
	}
	else if (g_InputState.dwKeyPress & kKeyMenu)
	{
		return 0;
	}

	//
	// Make sure the current menu item index is in bound
	//
	if (g_iCurrentItem < 0)
	{
		g_iCurrentItem = 0;
	}
	else if (g_iCurrentItem >= g_iNumMagic)
	{
		g_iCurrentItem = g_iNumMagic - 1;
	}

	//
	// Create the box.
	//
	PAL_CreateBox(PAL_XY(10, 42), 4, 16, 1, FALSE);

//#ifndef PAL_WIN95
	if (!gConfig->fIsWIN95)
	{
		if (gpGlobals->lpObjectDesc == NULL)
		{
			//
			// Draw the cash amount.
			//
			PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, FALSE);
			PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, FALSE, FALSE);
			PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(49, 14), kNumColorYellow, kNumAlignRight);

			//
			// Draw the MP of the selected magic.
			//
			PAL_CreateSingleLineBox(PAL_XY(215, 0), 5, FALSE);
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH),
				gpTextureReal, PAL_XY(260, 14));
			PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wMP, 4, PAL_XY(230, 14),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(g_wPlayerMP, 4, PAL_XY(265, 14), kNumColorCyan, kNumAlignRight);
		}
		else
		{
			{
				SDL_Rect dRect = { 100,0,320,40 };
				ClearScreen(&dRect);
			}
			char szDesc[512], * next;
			const char* d = PAL_GetObjectDesc(gpGlobals->lpObjectDesc, rgMagicItem[g_iCurrentItem].wMagic);

			//
			// Draw the magic description.
			//
			if (d != NULL)
			{
				k = 3;
				strcpy(szDesc, d);
				d = szDesc;

				while(!PalQuit)
				{
					next = (LPSTR)strchr(d, '*');
					if (next != NULL)
					{
						*next = '\0';
						next++;
					}

					PAL_DrawText(d, PAL_XY(100, k), DESCTEXT_COLOR, TRUE, FALSE);
					k += 16;

					if (next == NULL)
					{
						break;
					}

					d = next;
				}
			}

			//
			// Draw the MP of the selected magic.
			//
			PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, FALSE);
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH),
				gpTextureReal, PAL_XY(45, 14));
			PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wMP, 5, PAL_XY(15, 14),
				kNumColorYellow, kNumAlignRight);
			PAL_DrawNumber(g_wPlayerMP, 5, PAL_XY(50, 14), kNumColorCyan, kNumAlignLeft);
		}
	}
//#endif
	else

	{
		wScript = ((LPOBJECT)gpGlobals->g.rgObject)[rgMagicItem[g_iCurrentItem].wMagic].item.wScriptDesc;
		line = 0;
		while (wScript && gpGlobals->g.lprgScriptEntry[wScript].wOperation != 0)
		{
			if (gpGlobals->g.lprgScriptEntry[wScript].wOperation == 0xFFFF)
			{
				int line_incr = (gpGlobals->g.lprgScriptEntry[wScript].rgwOperand[1] != 1) ? 1 : 0;
				wScript = PAL_RunAutoScript(wScript, line);
				//wScript++;
				line += line_incr;
			}
			else
			{
				wScript = PAL_RunAutoScript(wScript, 0);
			}
		}

		//
		// Draw the MP of the selected magic.
		//
		PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, FALSE);
		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH),
			gpTextureReal, PAL_XY(45, 14));
		PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wMP, 4, PAL_XY(15, 14),
			kNumColorYellow, kNumAlignRight);
		PAL_DrawNumber(g_wPlayerMP, 4, PAL_XY(50, 14), kNumColorCyan, kNumAlignRight);
	}


	//
	// Draw the texts of the current page
	//
	i = g_iCurrentItem / 3 * 3 - 3 * 2;
	if (i < 0)
	{
		i = 0;
	}

	for (j = 0; j < 5; j++)
	{
		for (k = 0; k < 3; k++)
		{
			bColor = MENUITEM_COLOR;

			if (i >= g_iNumMagic)
			{
				//
				// End of the list reached
				//
				j = 5;
				break;
			}

			if (i == g_iCurrentItem)
			{
				if (rgMagicItem[i].fEnabled)
				{
					bColor = MENUITEM_COLOR_SELECTED;
				}
				else
				{
					bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
				}
			}
			else if (!rgMagicItem[i].fEnabled)
			{
				bColor = MENUITEM_COLOR_INACTIVE;
			}

			//
			// Draw the text
			//
			PAL_DrawText(PAL_GetWord(rgMagicItem[i].wMagic),
				PAL_XY(35 + k * 87, 54 + j * 18), bColor, TRUE, FALSE);

			//
			// Draw the cursor on the current selected item
			//
			if (i == g_iCurrentItem)
			{
				PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
					gpTextureReal, PAL_XY(60 + k * 87, 64 + j * 18));
			}

			i++;
		}
	}

	if (g_InputState.dwKeyPress & kKeySearch)
	{
		if (rgMagicItem[g_iCurrentItem].fEnabled)
		{
			j = g_iCurrentItem % 3;
			k = (g_iCurrentItem < 3 * 2) ? (g_iCurrentItem / 3) : 2;

			j = 35 + j * 87;
			k = 54 + k * 18;

			PAL_DrawText(PAL_GetWord(rgMagicItem[g_iCurrentItem].wMagic), PAL_XY(j, k),
				MENUITEM_COLOR_CONFIRMED, FALSE, TRUE);

			return rgMagicItem[g_iCurrentItem].wMagic;
		}
	}

	return 0xFFFF;
}

WORD CGameUI::PAL_ItemSelectMenuUpdate(VOID)
/*++
  Purpose:

	Initialize the item selection menu.

  Parameters:

	None.

  Return value:

	The object ID of the selected item. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
/*#ifndef PAL_WIN95
	int                i, j, k;
	WORD               wObject;
#else
	int                i, j, k, line;
	WORD               wObject, wScript;
#endif
*/
	int                i, j, k, line;
	WORD               wObject, wScript;

	BYTE               bColor;
	static BYTE        bufImage[2048];
	static WORD        wPrevImageIndex = 0xFFFF;
	//
	// Process input
	//
	if (g_InputState.dwKeyPress & kKeyUp)
	{
		gpGlobals->iCurInvMenuItem -= 3;
	}
	else if (g_InputState.dwKeyPress & kKeyDown)
	{
		gpGlobals->iCurInvMenuItem += 3;
	}
	else if (g_InputState.dwKeyPress & kKeyLeft)
	{
		gpGlobals->iCurInvMenuItem--;
	}
	else if (g_InputState.dwKeyPress & kKeyRight)
	{
		gpGlobals->iCurInvMenuItem++;
	}
	else if (g_InputState.dwKeyPress & kKeyPgUp)
	{
		gpGlobals->iCurInvMenuItem -= 3 * 7;
	}
	else if (g_InputState.dwKeyPress & kKeyPgDn)
	{
		gpGlobals->iCurInvMenuItem += 3 * 7;
	}
	else if (g_InputState.dwKeyPress & kKeyMenu)
	{
		return 0;
	}

	//
	// Make sure the current menu item index is in bound
	//
	if (gpGlobals->iCurInvMenuItem < 0)
	{
		gpGlobals->iCurInvMenuItem = 0;
	}
	else if (gpGlobals->iCurInvMenuItem >= g_iNumInventory)
	{
		gpGlobals->iCurInvMenuItem = g_iNumInventory - 1;
	}

	//
	// Redraw the box
	//
	PAL_CreateBox(PAL_XY(2, 0), 6, 17, 1, FALSE);

	//
	// Draw the texts in the current page
	//
	i = gpGlobals->iCurInvMenuItem / 3 * 3 - 3 * 4;
	if (i < 0)
	{
		i = 0;
	}

	for (j = 0; j < 7; j++)
	{
		for (k = 0; k < 3; k++)
		{
			wObject = gpGlobals->rgInventory[i].wItem;
			bColor = MENUITEM_COLOR;

			if (i >= MAX_INVENTORY || wObject == 0)
			{
				//
				// End of the list reached
				//
				j = 7;
				break;
			}

			if (i == gpGlobals->iCurInvMenuItem)
			{
				if (!(((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wFlags & g_wItemFlags) ||
					(SHORT)gpGlobals->rgInventory[i].nAmount <= (SHORT)gpGlobals->rgInventory[i].nAmountInUse)
				{
					//
					// This item is not selectable
					//
					bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
				}
				else
				{
					//
					// This item is selectable
					//
					if (gpGlobals->rgInventory[i].nAmount == 0)
					{
						bColor = MENUITEM_COLOR_EQUIPPEDITEM;
					}
					else
					{
						bColor = MENUITEM_COLOR_SELECTED;
					}
				}
			}
			else if (!(((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wFlags & g_wItemFlags) ||
				(SHORT)gpGlobals->rgInventory[i].nAmount <= (SHORT)gpGlobals->rgInventory[i].nAmountInUse)
			{
				//
				// This item is not selectable
				//
				bColor = MENUITEM_COLOR_INACTIVE;
			}
			else if (gpGlobals->rgInventory[i].nAmount == 0)
			{
				bColor = MENUITEM_COLOR_EQUIPPEDITEM;
			}

			//
			// Draw the text
			//
			PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * 100, 12 + j * 18),
				bColor, TRUE, FALSE);

			//
			// Draw the cursor on the current selected item
			//
			if (i == gpGlobals->iCurInvMenuItem)
			{
				PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
					gpTextureReal, PAL_XY(40 + k * 100, 22 + j * 18));
			}

			//
			// Draw the amount of this item
			//
			if ((SHORT)gpGlobals->rgInventory[i].nAmount - (SHORT)gpGlobals->rgInventory[i].nAmountInUse > 1)
			{
				PAL_DrawNumber(gpGlobals->rgInventory[i].nAmount - gpGlobals->rgInventory[i].nAmountInUse,
					2, PAL_XY(96 + k * 100, 17 + j * 18), kNumColorCyan, kNumAlignRight, 10);
			}

			i++;
		}
	}

	//
	// Draw the picture of current selected item
	//
	PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpTextureReal,
		PAL_XY(5, 140));

	wObject = gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem;

	if (((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wBitmap != wPrevImageIndex)
	{
		if (PAL_MKFReadChunk(bufImage, 2048,
			((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wBitmap, gpGlobals->f.fpBALL) > 0)
		{
			wPrevImageIndex = ((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wBitmap;
		}
		else
		{
			wPrevImageIndex = 0xFFFF;
		}
	}

	if (wPrevImageIndex != 0xFFFF)
	{
		PAL_RLEBlitToTexture(bufImage, gpTextureReal, PAL_XY(12, 148));
	}

	//
	// Draw the description of the selected item
	//
//#ifndef PAL_WIN95
	if (!gConfig->fIsWIN95)
	{
		if (!g_fNoDesc && gpGlobals->lpObjectDesc != NULL)
		{
			char szDesc[512], * next;
			const char* d = PAL_GetObjectDesc(gpGlobals->lpObjectDesc, wObject);
			{
				SDL_Rect dRect = { 75,150,320,200 };
				ClearScreen(&dRect);
			}
			if (d != NULL)
			{
				k = 150;
				strcpy(szDesc, d);
				d = szDesc;

				while(!PalQuit)
				{
					next = (LPSTR)strchr(d, '*');
					if (next != NULL)
					{
						*next = '\0';
						next++;
					}

					PAL_DrawText(d, PAL_XY(75, k), DESCTEXT_COLOR, TRUE, FALSE);
					k += 16;

					if (next == NULL)
					{
						break;
					}

					d = next;
				}
			}
		}
	}
//#endif
	else
	{
		if (!g_fNoDesc)
		{
			wScript = ((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wScriptDesc;
			line = 0;
			while (wScript && gpGlobals->g.lprgScriptEntry[wScript].wOperation != 0)
			{
				if (gpGlobals->g.lprgScriptEntry[wScript].wOperation == 0xFFFF)
				{
					int line_incr = (gpGlobals->g.lprgScriptEntry[wScript].rgwOperand[1] != 1) ? 1 : 0;
					wScript = PAL_RunAutoScript(wScript, PAL_ITEM_DESC_BOTTOM | line);
					line += line_incr;
				}
				else
				{
					wScript = PAL_RunAutoScript(wScript, 0);
				}
			}
		}
	}

	if (g_InputState.dwKeyPress & kKeySearch)
	{
		if ((((LPOBJECT)gpGlobals->g.rgObject)[wObject].item.wFlags & g_wItemFlags) &&
			(SHORT)gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount >
			(SHORT)gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmountInUse)
		{
			if (gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount > 0)
			{
				j = (gpGlobals->iCurInvMenuItem < 3 * 4) ? (gpGlobals->iCurInvMenuItem / 3) : 4;
				k = gpGlobals->iCurInvMenuItem % 3;

				PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * 100, 12 + j * 18),
					MENUITEM_COLOR_CONFIRMED, FALSE, FALSE);
			}

			return wObject;
		}
	}

	return 0xFFFF;
}

VOID CGameUI::PAL_MagicSelectionMenuInit(
	WORD         wPlayerRole,
	BOOL         fInBattle,
	WORD         wDefaultMagic
)
/*++
  Purpose:

	Initialize the magic selection menu.

  Parameters:

	[IN]  wPlayerRole - the player ID.

	[IN]  fInBattle - TRUE if in battle, FALSE if not.

	[IN]  wDefaultMagic - the default magic item.

  Return value:

	None.

--*/
{
	WORD       w;
	int        i, j;

	g_iCurrentItem = 0;
	g_iNumMagic = 0;

	g_wPlayerMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];

	//
	// Put all magics of this player to the array
	//
	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		w = gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole];
		if (w > MAX_OBJECTS)
		{
			gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] = 0;
			w = 0;
		}
		if (w != 0)
		{
			rgMagicItem[g_iNumMagic].wMagic = w;

			w = ((LPOBJECT)gpGlobals->g.rgObject)[w].magic.wMagicNumber;
			rgMagicItem[g_iNumMagic].wMP = gpGlobals->g.lprgMagic[w].wCostMP;

			rgMagicItem[g_iNumMagic].fEnabled = TRUE;

			if (rgMagicItem[g_iNumMagic].wMP > g_wPlayerMP)
			{
				rgMagicItem[g_iNumMagic].fEnabled = FALSE;
			}

			w = ((LPOBJECT)gpGlobals->g.rgObject)[rgMagicItem[g_iNumMagic].wMagic].magic.wFlags;
			if (fInBattle)
			{
				if (!(w & kMagicFlagUsableInBattle))
				{
					rgMagicItem[g_iNumMagic].fEnabled = FALSE;
				}
			}
			else
			{
				if (!(w & kMagicFlagUsableOutsideBattle))
				{
					rgMagicItem[g_iNumMagic].fEnabled = FALSE;
				}
			}

			g_iNumMagic++;
		}
	}

	//
	// Sort the array
	//
	for (i = 0; i < g_iNumMagic - 1; i++)
	{
		BOOL fCompleted = TRUE;

		for (j = 0; j < g_iNumMagic - 1 - i; j++)
		{
			if (rgMagicItem[j].wMagic > rgMagicItem[j + 1].wMagic)
			{
				struct MAGICITEM t = rgMagicItem[j];
				rgMagicItem[j] = rgMagicItem[j + 1];
				rgMagicItem[j + 1] = t;

				fCompleted = FALSE;
			}
		}

		if (fCompleted)
		{
			break;
		}
	}

	//
	// Place the cursor to the default item
	//
	for (i = 0; i < g_iNumMagic; i++)
	{
		if (rgMagicItem[i].wMagic == wDefaultMagic)
		{
			g_iCurrentItem = i;
			break;
		}
	}
}

VOID CGameUI::PAL_Search(
	VOID
)
/*++
  Purpose:

  Process searching trigger events.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	int                x, y, xOffset, yOffset, dx, dy, dh, ex, ey, eh, i, k, l;
	LPEVENTOBJECT      p;
	PAL_POS            rgPos[13]{0};

	//
	// Get the party location
	//
	x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
	y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

	if (gpGlobals->wPartyDirection == kDirNorth || gpGlobals->wPartyDirection == kDirEast)
	{
		xOffset = 16;
	}
	else
	{
		xOffset = -16;
	}

	if (gpGlobals->wPartyDirection == kDirEast || gpGlobals->wPartyDirection == kDirSouth)
	{
		yOffset = 8;
	}
	else
	{
		yOffset = -8;
	}

	rgPos[0] = PAL_XY(x, y);

	for (i = 0; i < 4; i++)
	{
		rgPos[i * 3 + 1] = PAL_XY(x + xOffset, y + yOffset);
		rgPos[i * 3 + 2] = PAL_XY(x, y + yOffset * 2);
		rgPos[i * 3 + 3] = PAL_XY(x + xOffset, y);
		x += xOffset;
		y += yOffset;
	}

	for (i = 0; i < 13; i++)
	{
		//
		// Convert to map location
		//
		dh = ((PAL_X(rgPos[i]) % 32) ? 1 : 0);
		dx = PAL_X(rgPos[i]) / 32;
		dy = PAL_Y(rgPos[i]) / 16;

		//
		// Loop through all event objects
		//
		for (k = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
			k < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; k++)
		{
			p = &(gpGlobals->g.lprgEventObject[k]);
			ex = p->x / 32;
			ey = p->y / 16;
			eh = ((p->x % 32) ? 1 : 0);

			if (p->sState <= 0 || p->wTriggerMode >= kTriggerTouchNear ||
				p->wTriggerMode * 6 - 4 < i || dx != ex || dy != ey || dh != eh)
			{
				continue;
			}

			//
			// Adjust direction/gesture for party members and the event object
			//
			if (p->nSpriteFrames * 4 > p->wCurrentFrameNum)
			{
				p->wCurrentFrameNum = 0; // use standing gesture
				p->wDirection = (gpGlobals->wPartyDirection + 2) % 4; // face the party

				for (l = 0; l <= gpGlobals->wMaxPartyMemberIndex; l++)
				{
					//
					// All party members should face the event object
					//
					gpGlobals->rgParty[l].wFrame = gpGlobals->wPartyDirection * 3;
				}

				//
				// Redraw everything
				//
				PAL_MakeScene();
				VIDEO_UpdateScreen(gpTextureReal);
			}

			//
			// Execute the script
			//
			p->wTriggerScript = PAL_RunTriggerScript(p->wTriggerScript, k + 1);

			//
			// Clear inputs and delay for a short time
			//
			PAL_Delay(50);
			PAL_ClearKeyState();

			return; // don't go further
		}
	}
}

VOID CGameUI::PAL_GameEquipItem(
	VOID
)
/*++
  Purpose:

  Allow player equip an item in the game.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	WORD      wObject;

	while(!PalQuit)
	{
		wObject = PAL_ItemSelectMenu(NULL, kItemFlagEquipable);

		if (wObject == 0)
		{
			return;
		}

		PAL_EquipItemMenu(wObject);
	}
}

VOID CGameUI::PAL_UpdateEquipments(
	VOID
)
/*++
  Purpose:

  Update the effects of all equipped items for all players.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	int      i, j;
	WORD     w;

	memset(&(gpGlobals->rgEquipmentEffect), 0, sizeof(gpGlobals->rgEquipmentEffect));

	for (i = 0; i < MAX_PLAYER_ROLES; i++)
	{
		for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
		{
			w = gpGlobals->g.PlayerRoles.rgwEquipment[j][i];

			if (w != 0)
			{
				SHORT showscript = gConfig->is_TEST;
				//如果显示自动脚本，就不显示普通脚本
				gConfig->is_TEST &= gConfig->is_ShowScript[0];
				((LPOBJECT)gpGlobals->g.rgObject)[w].item.wScriptOnEquip =
					PAL_RunTriggerScript(((LPOBJECT)gpGlobals->g.rgObject)[w].item.wScriptOnEquip, (WORD)i);
				gConfig->is_TEST = showscript;
			}
		}
	}
}

VOID CGameUI::PAL_InitGameData(
	INT         iSaveSlot
)
/*++
  Purpose:

  Initialize the game data (used when starting a new game or loading a saved game).

  Parameters:

  [IN]  iSaveSlot - Slot of saved game.

  Return value:

  None.

  --*/
{
	PAL_InitGlobalGameData();

	if (iSaveSlot > 0)
	{
		gpGlobals->bCurrentSaveSlot = (BYTE)iSaveSlot;
	}


	if (iSaveSlot < 0)
	{
#ifdef FINISH_GAME_MORE_ONE_TIME
		PAL_New_GoBackAndLoadDefaultGame();
#endif
	}
	else if (iSaveSlot == 0 || PAL_LoadGame(va("%s%d%s", PalDir.c_str(), iSaveSlot, ".rpg")) != 0)
	{
		//
		// Cannot load the saved game file. Load the defaults.
		//
		PAL_LoadDefaultGame();
	}


	gpGlobals->fGameStart = TRUE;
	gpGlobals->fNeedToFadeIn = FALSE;
	gpGlobals->iCurInvMenuItem = 0;
	gpGlobals->fInBattle = FALSE;

	memset(gpGlobals->rgPlayerStatus, 0, sizeof(gpGlobals->rgPlayerStatus));
	PAL_UpdateEquipments();
	g_InputState.prevdir = kDirUnknown;
	g_InputState.dir = kDirUnknown;
#ifdef	Controlled100
	{
		//我方控制100%成功
		for (int n = 0; n < gpGlobals->g.nScriptEntry; n++)
		{
			LPSCRIPTENTRY p = &gpGlobals->g.lprgScriptEntry[n];
			if (p->wOperation == 0x002E)
			{
				if ((gpGlobals->g.lprgScriptEntry[n - 1]).wOperation == 0x0006 &&
					gpGlobals->g.lprgScriptEntry[n - 1].rgwOperand[0] > 40)
				{
					gpGlobals->g.lprgScriptEntry[n - 1].rgwOperand[0] = 100;
				}
			}
			if (p->wOperation == 0x002D && p->rgwOperand[0] < 4)
			{
				if ((gpGlobals->g.lprgScriptEntry[n - 1]).wOperation == 0x0006 &&
					gpGlobals->g.lprgScriptEntry[n - 1].rgwOperand[0] > 40)
				{
					gpGlobals->g.lprgScriptEntry[n - 1].rgwOperand[0] = 100;
				}
			}
		}
	}
#endif

}

#define LOAD_DATA(buf, size, chunknum, fp)                       \
{                                                             \
    PAL_MKFReadChunk((LPBYTE)(buf), (size), (chunknum), (fp)); \
    DO_BYTESWAP(buf, size);                                    \
}

#ifdef FINISH_GAME_MORE_ONE_TIME
VOID
CGameUI:: PAL_New_GoBackAndLoadDefaultGame(
	VOID
)
/*++
Purpose:穿越回去，除了主角的各项属性，其余的都恢复到最初的数据
--*/
{
	GAMEDATA* p = &gpGlobals->g;
	UINT32             i;
	PLAYERROLES playerRoles;

	memset(&playerRoles, 0, sizeof(playerRoles));
	//
	// Load the default data from the game data files.
	//
	LOAD_DATA(p->lprgEventObject, p->nEventObject * sizeof(EVENTOBJECT),
		0, gpGlobals->f.fpSSS);
	PAL_MKFReadChunk((LPBYTE)(p->rgScene), sizeof(p->rgScene), 1, gpGlobals->f.fpSSS);
	DO_BYTESWAP(p->rgScene, sizeof(p->rgScene));

	if (gConfig->fIsWIN95)
	{
		PAL_MKFReadChunk((LPBYTE)(p->rgObject), sizeof(OBJECT), 2, gpGlobals->f.fpSSS);
		DO_BYTESWAP(p->rgObject, sizeof(OBJECT));
	}
	else
	{
		OBJECT_DOS objects[MAX_OBJECTS]{};
		PAL_MKFReadChunk((LPBYTE)(objects), sizeof(objects), 2, gpGlobals->f.fpSSS);
		DO_BYTESWAP(objects, sizeof(objects));
		//
		// Convert the DOS-style data structure to WIN-style data structure
		//转化格式
		for (i = 0; i < MAX_OBJECTS; i++)
		{
			memcpy(&((LPOBJECT)p->rgObject)[i], &objects[i], sizeof(OBJECT_DOS));
			((LPOBJECT)p->rgObject)[i].rgwData[6] = objects[i].rgwData[5];     // wFlags
			((LPOBJECT)p->rgObject)[i].rgwData[5] = 0;                         // wScriptDesc or wReserved2
		}
	}

	PAL_MKFReadChunk((LPBYTE)(&playerRoles), sizeof(PLAYERROLES), 3, gpGlobals->f.fpDATA);
	DO_BYTESWAP(&(playerRoles), sizeof(PLAYERROLES));

	// Set some other default data.
	gpGlobals->wNumMusic = 0;
	gpGlobals->wNumPalette = 0;
	gpGlobals->wNumScene = 1;
	gpGlobals->fNightPalette = FALSE;
	gpGlobals->wMaxPartyMemberIndex = 0;
	gpGlobals->viewport = PAL_XY(0, 0);
	gpGlobals->wLayer = 0;
	gpGlobals->wChaseRange = 1;

#ifndef PAL_CLASSIC
	gpGlobals->bBattleSpeed = 2;
#endif

	gpGlobals->bFinishGameTime++;

	memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
	memset(gpGlobals->rgParty, 0, sizeof(gpGlobals->rgParty));
	memset(gpGlobals->rgTrail, 0, sizeof(gpGlobals->rgTrail));
	//memset(&(gpGlobals->Exp), 0, sizeof(gpGlobals->Exp));

	for (i = 0; i < MAX_PLAYER_ROLES; i++)
	{
		gpGlobals->Exp.rgPrimaryExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgHealthExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgMagicExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgAttackExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgMagicPowerExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgDefenseExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgDexterityExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
		gpGlobals->Exp.rgFleeExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
	}

#ifdef FINISH_GAME_MORE_ONE_TIME


	switch (gpGlobals->bFinishGameTime)
	{
	case 0:
	case 1:
	{
		gpGlobals->dwCash = 0;
		gpGlobals->wCollectValue = 0;
		memset(gpGlobals->rgInventory, 0, sizeof(gpGlobals->rgInventory));
		// 此处还应去除每个人的绝技，例如酒神、剑灭、梦蛇、满天花雨、无心咒等
		// 还原佩戴的物品
		for (i = 0; i < MAX_PLAYER_ROLES; i++)
		{
			gpGlobals->g.PlayerRoles.rgwEquipment[kBodyPartWear][i] = playerRoles.rgwEquipment[kBodyPartWear][i];
		}
		break;
	}

	default:
	{
		break;
	}
	}
#endif

	PAL_PlayerLevelUp(RoleID_ZhaoLingEr, 4);
	PAL_PlayerLevelUp(RoleID_LinYueRu, 6);
	PAL_PlayerLevelUp(RoleID_ANu, 45);
	PAL_PlayerLevelUp(RoleID_WuHou, 80);
	PAL_PlayerLevelUp(RoleID_GaiLuoJiao, 80);

	for (i = 0; i < MAX_PLAYER_ROLES; i++)
	{
		gpGlobals->g.PlayerRoles.rgwAvatar[i] = playerRoles.rgwAvatar[i];
		gpGlobals->g.PlayerRoles.rgwSpriteNumInBattle[i] = playerRoles.rgwSpriteNumInBattle[i];
		gpGlobals->g.PlayerRoles.rgwSpriteNum[i] = playerRoles.rgwSpriteNum[i];
		gpGlobals->g.PlayerRoles.rgwWalkFrames[i] = playerRoles.rgwWalkFrames[i];
		gpGlobals->g.PlayerRoles.rgwHP[i] = gpGlobals->g.PlayerRoles.rgwMaxHP[i];
		gpGlobals->g.PlayerRoles.rgwMP[i] = gpGlobals->g.PlayerRoles.rgwMaxMP[i];
	}

	gpGlobals->fEnteringScene = TRUE;
}
#endif

