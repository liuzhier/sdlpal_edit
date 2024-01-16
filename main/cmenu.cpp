#include "cpalbaseio.h"

#include "cgl_render.h"
#include "caviplay.h"

#define MAINMENU_BACKGROUND_FBPNUM (gConfig->fIsWIN95 ? 2 :60)
#define CONFIRMMENU_LABEL_NO               19
#define CONFIRMMENU_LABEL_YES              20


#define RIX_NUM_OPENINGMENU                4
#define MAINMENU_LABEL_NEWGAME             7
#define MENUITEM_COLOR                     0x4F
#define MENUITEM_COLOR_INACTIVE            0x1C
#define MENUITEM_COLOR_CONFIRMED           0x2C
#define MENUITEM_COLOR_SELECTED_INACTIVE   0x1F
#define MENUITEM_COLOR_SELECTED_FIRST      0xF9
#define MENUITEM_COLOR_SELECTED_TOTALNUM   6
#define LOADMENU_LABEL_SLOT_FIRST          43

#define MENUITEM_COLOR_SELECTED                                    \
   (MENUITEM_COLOR_SELECTED_FIRST +                                \
      SDL_GetTicks_New() / (600 / MENUITEM_COLOR_SELECTED_TOTALNUM)    \
      % MENUITEM_COLOR_SELECTED_TOTALNUM)

#define MENUITEM_COLOR_EQUIPPEDITEM        0xC8

#define MAINMENU_LABEL_LOADGAME            8




INT CPalBaseIO::PAL_OpeningMenu(
	VOID
)
/*++
  Purpose:

  Show the opening menu.

  Parameters:

  None.

  Return value:

  Which saved slot to load from (1-5). 0 to start a new game.

  --*/
{
	WORD          wItemSelected{ 0 };
	WORD          wDefaultItem{ 0 };

	MENUITEM      rgMainMenuItem[2] = {
		// value   label                     enabled   position
			{0, MAINMENU_LABEL_NEWGAME, TRUE, PAL_XY(125, 95)},
			{1, MAINMENU_LABEL_LOADGAME, TRUE, PAL_XY(125, 112)}
	};

	//
	// Play the background music
	//
    PAL_PlayMUS(RIX_NUM_OPENINGMENU, TRUE, 1);

	//
	// Draw the background
	//
	setAlpha(0);
	PAL_DrawOpeningMenuBackground();
	PAL_FadeIn(0, FALSE, 2);

	while(!PalQuit)
	{
		//
		// Activate the menu		
		//ClearScreen();
		//
		wItemSelected = PAL_ReadMenu(rgMainMenuItem, 2, wDefaultItem, MENUITEM_COLOR, NULL);

		if (wItemSelected == 0 || wItemSelected == MENUITEM_VALUE_CANCELLED)
		{
			//
			// Start a new game
			//
			wItemSelected = 0;
			break;
		}
        else
		{
			//
			// Load game
			//
            wItemSelected = PAL_SaveSlotMenu(1);
			if (wItemSelected != MENUITEM_VALUE_CANCELLED)
			{
				break;
			}
			wDefaultItem = 1;
		}
	}
	ClearScreen();
	//
	// Fade out the screen and the music
	//
    PAL_PlayMUS(0, FALSE, 1);
	PAL_FadeOut(1);

//#ifdef PAL_HAS_AVI
if(gConfig->m_Function_Set[22])
	{
		CAviPlay m;
		m.PAL_PlayAVI(va("%s/avi/3.AVI", PalDir.c_str()));
	}
//#endif

	return (INT)wItemSelected;
}

VOID CPalBaseIO::PAL_DrawOpeningMenuBackground(VOID)
/*++
  Purpose:

  Draw the background of the main menu.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	LPBYTE        buf;
	//SDL_Surface   *lpBitmapDown;

	buf = (LPBYTE)malloc(320 * 200);
	if (buf == NULL)
	{
		return;
	}

	//
	// Read the picture from fbp.mkf.
	//
	PAL_MKFDecompressChunk(buf, 320 * 200, MAINMENU_BACKGROUND_FBPNUM, gpGlobals->f.fpFBP);

	PAL_SetPalette(0, FALSE);
	//
	// ...and blit it to the screen buffer.
	//
	PAL_FBPBlitToSurface(buf, gpTextureReal);
	free(buf);
}


WORD CPalBaseIO::PAL_ReadMenu(
	LPMENUITEM                rgMenuItem,
	INT                       nMenuItem,
	WORD                      wDefaultItem,
	BYTE                      bLabelColor,
	LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged
)
/*++
  Purpose:

	Execute a menu.

  Parameters:

	[IN]  lpfnMenuItemChanged - Callback function which is called when user
								changed the current menu item.

	[IN]  rgMenuItem - Array of the menu items.

	[IN]  nMenuItem - Number of menu items.

	[IN]  wDefaultItem - default item index.

	[IN]  bLabelColor - color of the labels.

  Return value:

	Return value of the selected menu item. MENUITEM_VALUE_CANCELLED if cancelled.

--*/
{
	int               i;
	WORD              wCurrentItem = (wDefaultItem < nMenuItem) ? wDefaultItem : 0;

	PAL_ClearKeyState();
	while(!PalQuit)
	{
		//
		// Draw all the menu texts.
		//
		for (i = 0; i < nMenuItem; i++)
		{
			BYTE bColor = bLabelColor;

			if (!rgMenuItem[i].fEnabled)
			{
				if (i == wCurrentItem)
				{
					bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
				}
				else
				{
					bColor = MENUITEM_COLOR_INACTIVE;
				}
			}

			PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos,
				bColor, TRUE, TRUE);
		}

		if (lpfnMenuItemChanged != NULL)
		{
			lpfnMenuItemChanged(rgMenuItem[wCurrentItem].wValue);
		}
		//PAL_ClearKeyState();
		//
		// Redraw the selected item if needed.
		//
		if (rgMenuItem[wCurrentItem].fEnabled)
		{
			PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
				rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE);
		}

		PAL_ProcessEvent();
		if (getKeyPress())
		{
			if (getKeyPress() & (kKeyDown | kKeyRight))
			{
				//
				// User pressed the down or right arrow key
				//
				if (rgMenuItem[wCurrentItem].fEnabled)
				{
					//
					// Dehighlight the unselected item.
					//
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE);
				}
				else
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE);
				}

				wCurrentItem++;

				if (wCurrentItem >= nMenuItem)
				{
					wCurrentItem = 0;
				}

				//
				// Highlight the selected item.
				//
				if (rgMenuItem[wCurrentItem].fEnabled)
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE);
				}
				else
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED_INACTIVE, FALSE, TRUE);
				}

				/*if (lpfnMenuItemChanged != NULL)
				{
					(lpfnMenuItemChanged)(rgMenuItem[wCurrentItem].wValue);
				}
				*/
			}
			else if (getKeyPress() & (kKeyUp | kKeyLeft))
			{
				//
				// User pressed the up or left arrow key
				//
				if (rgMenuItem[wCurrentItem].fEnabled)
				{
					//
					// Dehighlight the unselected item.
					//
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE);
				}
				else
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE);
				}

				if (wCurrentItem > 0)
				{
					wCurrentItem--;
				}
				else
				{
					wCurrentItem = nMenuItem - 1;
				}

				//
				// Highlight the selected item.
				//
				if (rgMenuItem[wCurrentItem].fEnabled)
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE);
				}
				else
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED_INACTIVE, FALSE, TRUE);
				}

				if (lpfnMenuItemChanged != NULL)
				{
					(lpfnMenuItemChanged)(rgMenuItem[wCurrentItem].wValue);
				}
			}
			else if (getKeyPress() & kKeyMenu)
			{
				//
				// User cancelled
				//
				if (rgMenuItem[wCurrentItem].fEnabled)
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE);
				}
				else
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE);
				}

				break;
			}
			else if (getKeyPress() & kKeySearch)
			{
				//
				// User pressed Enter
				//
				if (rgMenuItem[wCurrentItem].fEnabled)
				{
					PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
						rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_CONFIRMED, FALSE, TRUE);

					return rgMenuItem[wCurrentItem].wValue;
				}
			}

			PAL_ClearKeyState();
		}
		//
		// Use delay function to avoid high CPU usage.
		//
		//VIDEO_UpdateScreen(gpTextureReal);
		PAL_Delay(20);
	}
	//ClearScreen();
	return MENUITEM_VALUE_CANCELLED;
}

//#define maxSaveFile (gConfig-> m_Function_Set[47])

INT CPalBaseIO::PAL_SaveSlotMenu(
	WORD        wDefaultSlot
)
/*++
  Purpose:

  Show the load game menu.

  Parameters:

  [IN]  wDefaultSlot - default save slot number (1-5).
  [IN]  BIsSave - is save true save?
  Return value:

  Which saved slot to load from (1-5). MENUITEM_VALUE_CANCELLED if cancelled.

  --*/
{
	//LPBOX           rgpBox[5];
	int             i;
	FILE* fp;
	WORD            wItemSelected;
	WORD            wSavedTimes;
	int maxSaveFile = gConfig->m_Function_Set[47];

	std::vector<MENUITEM>rgMenuItem;
	rgMenuItem.resize(maxSaveFile);
	//进度1-10，需要重新组成词组

	//
	// Create the boxes and create the menu items
	//
	LPBOX cpBox = PAL_CreateBox(PAL_XY(195, 3), 8, 4, 1, TRUE);

	SDL_Rect  rect = { 195, 3, cpBox->wWidth, cpBox->wHeight };

	//RenderBlendCopy(gpRenderTexture, gpScreen);
	int vSpace[] = {35,30,25,20,17,17};

	for (i = 0; i < maxSaveFile; i++)
	{
		rgMenuItem[i].wValue = i + 1;
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].wNumWord = 20012 + i;// LOADMENU_LABEL_SLOT_FIRST + i;
		rgMenuItem[i].pos = PAL_XY(210, 12 + vSpace[maxSaveFile-5] * i);
	}
	
	// Draw the numbers of saved times
	//
	for (i = 1; i <= maxSaveFile; i++)
	{
		fp = fopen(va("%s%d%s", PalDir.c_str(), i, ".rpg"), "rb");
		if (fp == NULL)
		{
			wSavedTimes = 0;
		}
		else
		{
			fread(&wSavedTimes, sizeof(WORD), 1, fp);
			wSavedTimes = SWAP16(wSavedTimes);
			fclose(fp);
		}

		//
		// Draw the number
		//
		//PAL_DrawNumber((UINT)wSavedTimes, 4, PAL_XY(265, 35 * i - 13),
			//kNumColorYellow, kNumAlignRight, 10);
		PAL_DrawNumber((UINT)wSavedTimes, 4, rgMenuItem[i - 1].pos + PAL_XY(55, 5),
			kNumColorYellow, kNumAlignRight, 10);
	}

	RenderPresent(gpTextureReal);
	//
	// Activate the menu
	//
	wItemSelected = PAL_ReadMenu(rgMenuItem.data(), maxSaveFile, wDefaultSlot - 1, MENUITEM_COLOR, NULL);
	
	//
	// Delete the boxes
	//
	PAL_DeleteBox(cpBox);
	VIDEO_UpdateScreen(gpTextureReal);

	return wItemSelected;
}

LPBOX CPalBaseIO::PAL_CreateBox(PAL_POS pos, INT nRows, INT nColumns, INT iStyle, BOOL fSaveScreen
)
/*++
  Purpose:

	Create a box on the screen.

  Parameters:

	[IN]  pos - position of the box.

	[IN]  nRows - number of rows of the box.

	[IN]  nColumns - number of columns of the box.

	[IN]  iStyle - style of the box (0 or 1).

	[IN]  fSaveScreen - whether save the used screen area or not.

  Return value:

	Pointer to a BOX structure. NULL if failed.
	If fSaveScreen is false, then always returns NULL.

--*/
{
	int              i, j, x, m, n;
	LPCBITMAPRLE     rglpBorderBitmap[3][3] = { 0 };
	LPBOX            lpBox = NULL;
	CGL_Texture   * save;
	SDL_Rect         rect{0};

	//
	// Get the bitmaps
	//
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			rglpBorderBitmap[i][j] = PAL_SpriteGetFrame(gpSpriteUI, i * 3 + j + iStyle * 9);
		}
	}

	rect.x = PAL_X(pos);
	rect.y = PAL_Y(pos);
	rect.w = 0;
	rect.h = 0;

	//
	// Get the total width and total height of the box
	//
	for (i = 0; i < 3; i++)
	{
		if (i == 1)
		{
			rect.w += PAL_RLEGetWidth(rglpBorderBitmap[0][i]) * nColumns;
			rect.h += PAL_RLEGetHeight(rglpBorderBitmap[i][0]) * nRows;
		}
		else
		{
			rect.w += PAL_RLEGetWidth(rglpBorderBitmap[0][i]);
			rect.h += PAL_RLEGetHeight(rglpBorderBitmap[i][0]);
		}
	}

	if (fSaveScreen)
	{
		//
		// Save the used part of the screen
		//
		SDL_Rect bRect;
		bRect = rect;
		setPictureRatio(&rect);
		save = new CGL_Texture(rect.w, rect.h, 32);

		if (save == NULL)
		{
			return NULL;
		}

		lpBox = (LPBOX)calloc(1, sizeof(BOX));
		if (lpBox == NULL)
		{
			delete  save;
			return NULL;
		}

		RenderBlendCopy(save, gpTextureReal, 255, 1, 0, NULL, &rect);
		lpBox->lpSavedArea = save;
		//
		rect = bRect;
		lpBox->pos = pos;
		lpBox->wWidth = rect.w;
		lpBox->wHeight = rect.h;
	}

	//
	// Border takes 2 additional rows and columns...
	//
	nRows += 2;
	nColumns += 2;

	//
	// Draw the box
	//
	for (i = 0; i < nRows; i++)
	{
		x = rect.x;
		m = (i == 0) ? 0 : ((i == nRows - 1) ? 2 : 1);

		for (j = 0; j < nColumns; j++)
		{
			n = (j == 0) ? 0 : ((j == nColumns - 1) ? 2 : 1);
			PAL_RLEBlitToTexture(rglpBorderBitmap[m][n], gpTextureReal, PAL_XY(x, rect.y));
			x += PAL_RLEGetWidth(rglpBorderBitmap[m][n]);
		}

		rect.y += PAL_RLEGetHeight(rglpBorderBitmap[m][0]);
	}

	return lpBox;
}

VOID CPalBaseIO::PAL_DeleteBox(LPBOX lpBox
)
/*++
  Purpose:

	Delete a box and restore the saved part of the screen.

  Parameters:

	[IN]  lpBox - pointer to the BOX struct.

  Return value:

	None.

--*/
{
	SDL_Rect        rect{0};

	//
	// Check for NULL pointer.
	//
	if (lpBox == NULL)
	{
		return;
	}

	//
	// Restore the saved screen part
	//
	rect.x = PAL_X(lpBox->pos);
	rect.y = PAL_Y(lpBox->pos);
	rect.w = lpBox->wWidth;
	rect.h = lpBox->wHeight;

	setPictureRatio(&rect);
	RenderBlendCopy(gpTextureReal, lpBox->lpSavedArea, 255, 1, 0, &rect, 0);
	//
	// Free the memory used by the box
	//
	delete lpBox->lpSavedArea;
	//SDL_FreeSurface(lpBox->lpSavedArea);
	free(lpBox);
}

BOOL CPalBaseIO::PAL_ConfirmMenu(LPCSTR text)
/*++
  Purpose:

  Show a "Yes or No?" confirm box.


  Parameters:

  text Drow words if no NULL

  Return value:

  TRUE if user selected Yes, FALSE if selected No.

  --*/
{
	LPBOX           rgpBox[2] { 0 };
	LPBOX			textBox = NULL;
	MENUITEM        rgMenuItem[2]{0};
	int             i;
	WORD            wReturnValue;

	const SDL_Rect  rect = { 130, 100, 125, 50 };
	
	//备份调色版
	SDL_Color  oldPalette[256];
	memcpy(oldPalette, gpPalette->colors, sizeof(oldPalette));

	PAL_SetPalette(0, 0);

	//PAL_ClearText(&rect);
	//
	// Create menu items
	//
	rgMenuItem[0].fEnabled = TRUE;
	rgMenuItem[0].pos = PAL_XY(145, 110);
	rgMenuItem[0].wValue = 0;
	rgMenuItem[0].wNumWord = CONFIRMMENU_LABEL_NO;

	rgMenuItem[1].fEnabled = TRUE;
	rgMenuItem[1].pos = PAL_XY(220, 110);
	rgMenuItem[1].wValue = 1;
	rgMenuItem[1].wNumWord = CONFIRMMENU_LABEL_YES;

	if (text)
	{
		textBox = PAL_CreateSingleLineBox(PAL_XY(118, 66), 8, TRUE);
		PAL_DrawText(text, PAL_XY(128, 78), 0, FALSE, FALSE);
	}
	//
	// Create the boxes
	//
	for (i = 0; i < 2; i++)
	{
		rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(130 + 75 * i, 100), 2, TRUE);
	}

	VIDEO_UpdateScreen(gpTextureReal);

	//
	// Activate the menu
	//
	wReturnValue = PAL_ReadMenu(rgMenuItem, 2, 1, MENUITEM_COLOR, NULL);

	//
	// Delete the boxes
	//
	for (i = 0; i < 2; i++)
	{
		PAL_DeleteBox(rgpBox[i]);
	}
	if (text)
		PAL_DeleteBox(textBox);
	VIDEO_UpdateScreen(gpTextureReal);
	//恢复调色版
	VIDEO_SetPalette(oldPalette);
	
	return (wReturnValue == MENUITEM_VALUE_CANCELLED || wReturnValue == 0) ? FALSE : TRUE;
}

LPBOX  CPalBaseIO::PAL_CreateSingleLineBox(
	PAL_POS        pos,
	INT            nLen,
	BOOL           fSaveScreen
)
/*++
  Purpose:

	Create a single-line box on the screen.

  Parameters:

	[IN]  pos - position of the box.

	[IN]  nLen - length of the box.

	[IN]  fSaveScreen - whether save the used screen area or not.

  Return value:

	Pointer to a BOX structure. NULL if failed.
	If fSaveScreen is false, then always returns NULL.

--*/
{
	static const int      iNumLeftSprite = 44;
	static const int      iNumMidSprite = 45;
	static const int      iNumRightSprite = 46;

	LPCBITMAPRLE          lpBitmapLeft;
	LPCBITMAPRLE          lpBitmapMid;
	LPCBITMAPRLE          lpBitmapRight;
	CGL_Texture * save;
	SDL_Rect              rect{0};
	LPBOX                 lpBox = NULL;
	int                   i;

	//
	// Get the bitmaps
	//
	lpBitmapLeft = PAL_SpriteGetFrame(gpSpriteUI, iNumLeftSprite);
	lpBitmapMid = PAL_SpriteGetFrame(gpSpriteUI, iNumMidSprite);
	lpBitmapRight = PAL_SpriteGetFrame(gpSpriteUI, iNumRightSprite);

	rect.x = PAL_X(pos);
	rect.y = PAL_Y(pos);

	//
	// Get the total width and total height of the box
	//
	rect.w = PAL_RLEGetWidth(lpBitmapLeft) + PAL_RLEGetWidth(lpBitmapRight);
	rect.w += PAL_RLEGetWidth(lpBitmapMid) * nLen;
	rect.h = PAL_RLEGetHeight(lpBitmapLeft);

	//清空区域内的文字
	//ClearScreen(&rect);

	if (fSaveScreen)
	{
		//
		// Save the used part of the screen
		//
		SDL_Rect bRect = rect;
		setPictureRatio(&rect);
		save = new CGL_Texture(rect.w, rect.h, 32);
		if (save == NULL)
		{
			return NULL;
		}

		lpBox = (LPBOX)calloc(1, sizeof(BOX));
		if (lpBox == NULL)
		{
			//SDL_FreeSurface(gpScreen);
			delete save;
			return NULL;
		}

		RenderBlendCopy(save, gpTextureReal, 255, 1, 0, NULL, &rect);
		rect = bRect;
		lpBox->pos = pos;
		lpBox->lpSavedArea = save;
		lpBox->wHeight = (WORD)rect.h;
		lpBox->wWidth = (WORD)rect.w;
	}

	//
	// Draw the box
	//
	
	SDL_Color* pcolor = PAL_GetPalette(0, 0);

	PAL_RLEBlitToTexture(lpBitmapLeft, gpTextureReal, pos, 0, 1, pcolor);

	rect.x += PAL_RLEGetWidth(lpBitmapLeft);

	for (i = 0; i < nLen; i++)
	{
		PAL_RLEBlitToTexture(lpBitmapMid, gpTextureReal, PAL_XY(rect.x, rect.y), 0, 1, pcolor);
		rect.x += PAL_RLEGetWidth(lpBitmapMid);
	}

	PAL_RLEBlitToTexture(lpBitmapRight, gpTextureReal, PAL_XY(rect.x, rect.y), 0, 1, pcolor);

	return lpBox;
}
