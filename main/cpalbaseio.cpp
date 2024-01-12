#include "cpalbaseio.h"
#include "cgl_render.h"
#include "caviplay.h"

CPalBaseIO::CPalBaseIO() :rgMagicItem()
{
	memset(&g_Battle, 0, sizeof(g_Battle));
	memset(rgMagicItem, 0, sizeof(rgMagicItem));
	//CAviPlay::setPAL(this);
	PAL_InitResources();
}

CPalBaseIO::~CPalBaseIO()
{
	//PAL_FreePlayerSprites();
	//PAL_FreeEventObjectSprites();
	PAL_FreeResources();
}


#define PAL_DelayUntilPC(t) \
   PAL_ProcessEvent(); \
   while (SDL_GetPerformanceCounter() < (t)) \
   { \
      PAL_ProcessEvent(); \
      SDL_Delay(1); \
   }

VOID CPalBaseIO::PAL_RNGPlay(
	INT           iNumRNG,
	INT           iStartFrame,
	INT           iEndFrame,
	INT           iSpeed
)
/*++
  Purpose:

	Play a RNG movie.

  Parameters:

	[IN]  iNumRNG - number of the RNG movie.

	[IN]  iStartFrame - start frame number.

	[IN]  iEndFrame - end frame number.

	[IN]  iSpeed - speed of playing.

  Return value:

	None.

--*/
{
	setAlpha(255);
	setColor(255, 255, 255);
	double         iDelay = (double)SDL_GetPerformanceFrequency() / (iSpeed == 0 ? 16 : iSpeed);
	uint8_t* rng = (uint8_t*)malloc(65000);
	uint8_t* buf = (uint8_t*)malloc(65000);
	FILE* fp = UTIL_OpenRequiredFile("rng.mkf");
#if 0
	SDL_Surface* rsurf = SDL_CreateRGBSurface(0, 320, 200, 32, MastR, MastG, MastB, MastA);
#else //
	SDL_Surface* mSurf1 = gpGL->creatSurfaceFromTexture(gpTextureReal);
	SDL_Surface* mSurf = SDL_CreateRGBSurface(0, 320, 200, 8, 0, 0, 0, 0);
	SDL_SetSurfacePalette(mSurf, gpPalette);
	SDL_BlitSurface(mSurf1, 0, mSurf, 0);
	SDL_FreeSurface(mSurf1);
#endif //

	for (double iTime = SDL_GetPerformanceCounter(); rng && buf && iStartFrame != iEndFrame; iStartFrame++)
	{
		if (PalQuit) break;

		iTime += iDelay;
		//
		// Read, decompress and render the frame
		//
		if (PAL_RNGReadFrame(buf, 65000, iNumRNG, iStartFrame, fp) < 0 ||
			PAL_RNGBlitToSurface(rng, (PAL_DeCompress)(buf, rng, 65000), mSurf) == -1)
		{
			//
			// Failed to get the frame, don't go further
			//
			break;
		}
		//
		// Update the screen
		//
		RenderBlendCopy(gpTextureReal, mSurf);
		RenderPresent(gpTextureReal);

		//
		// Fade in the screen if needed
		//
		if (gpGlobals->fNeedToFadeIn)
		{
			PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 2);
			gpGlobals->fNeedToFadeIn = FALSE;
		}

		//
		// Delay for a while
		//
		PAL_DelayUntilPC(iTime);
	}

	::fclose(fp);
	::free(rng);
	::free(buf);
	SDL_FreeSurface(mSurf);
}

INT CPalBaseIO::PAL_RLEBlitMonoColor(
	LPCBITMAPRLE      lpBitmapRLE,
	CGL_Texture *     lpDstText,
	PAL_POS           pos,
	BYTE              bColor,
	INT               iColorShift
)
{
	UINT          uiWidth = 0;
	UINT          uiHeight = 0;

	//
	// Check for NULL pointer.
	//
	if (lpBitmapRLE == NULL || lpDstText == NULL)
	{
		return -1;
	}

	//
	// Skip the 0x00000002 in the file header.
	//
	if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
		lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
	{
		lpBitmapRLE += 4;
	}

	//
	// Get the width and height of the bitmap.
	//
	uiWidth = lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
	uiHeight = lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);

	SDL_Surface* mSurf = SDL_CreateRGBSurface(0, uiWidth, uiHeight, 8, 0, 0, 0, 0);
	SDL_SetSurfacePalette(mSurf, gpPalette);
	if (!mSurf) return -1;
	//SDL_FillRect(mSurf, 0, *((UINT*)&mSurf->format->palette->colors[0]));
	if (PAL_RLEBlitMonoColor(lpBitmapRLE, mSurf,0,bColor,iColorShift) == -1)
	{
		SDL_FreeSurface(mSurf);
		return -1;
	}
	SDL_Rect dstRect = { PAL_X(pos), PAL_Y(pos), uiWidth,uiHeight };
	///
	if (lpDstText->w == RealWidth)
		setPictureRatio(&dstRect);
	//--------
	RenderBlendCopy(lpDstText, mSurf, 255, 3, &mSurf->format->palette->colors[0], &dstRect);
	SDL_FreeSurface(mSurf);
	return 0;


}

INT CPalBaseIO::PAL_RLEBlitMonoColor(
	LPCBITMAPRLE      lpBitmapRLE,
	SDL_Surface* lpDstSurface,
	PAL_POS           pos,
	BYTE              bColor,
	INT               iColorShift
)
/*++
  Purpose:

  Blit an RLE-compressed bitmap to an SDL surface in mono-color form.
  NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

  [IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

  [OUT] lpDstSurface - pointer to the destination SDL surface.

  [IN]  pos - position of the destination area.

  [IN]  bColor - the color to be used while drawing.

  [IN]  iColorShift - shift the color by this value.

  Return value:

  0 = success, -1 = error.

  --*/
{
	UINT          i, j;
	INT           x, y;
	UINT          uiLen = 0;
	UINT          uiWidth = 0;
	UINT          uiHeight = 0;
	BYTE          T, b;
	INT           dx = PAL_X(pos);
	INT           dy = PAL_Y(pos);

	//
	// Check for NULL pointer.
	//
	if (lpBitmapRLE == NULL || lpDstSurface == NULL)
	{
		return -1;
	}

	//
	// Skip the 0x00000002 in the file header.
	//
	if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
		lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
	{
		lpBitmapRLE += 4;
	}

	//
	// Get the width and height of the bitmap.
	//
	uiWidth = lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
	uiHeight = lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);

	//
	// Calculate the total length of the bitmap.
	// The bitmap is 8-bpp, each pixel will use 1 byte.
	//
	uiLen = uiWidth * uiHeight;

	//
	// Start decoding and blitting the bitmap.
	//
	lpBitmapRLE += 4;
	bColor &= 0xF0;
	for (i = 0; i < uiLen;)
	{
		T = *lpBitmapRLE++;
		if ((T & 0x80) && T <= 0x80 + uiWidth)
		{
			i += T - 0x80;
		}
		else
		{
			for (j = 0; j < T; j++)
			{
				//
				// Calculate the destination coordination.
				// FIXME: This could be optimized
				//
				y = (i + j) / uiWidth + dy;
				x = (i + j) % uiWidth + dx;

				//
				// Skip the points which are out of the surface.
				//
				if (x < 0)
				{
					j += -x - 1;
					continue;
				}
				else if (x >= lpDstSurface->w)
				{
					j += x - lpDstSurface->w;
					continue;
				}

				if (y < 0)
				{
					j += -y * uiWidth - 1;
					continue;
				}
				else if (y >= lpDstSurface->h)
				{
					goto end; // No more pixels needed, break out
				}

				//
				// Put the pixel onto the surface (FIXME: inefficient).
				//
				b = lpBitmapRLE[j] & 0x0F;
				if ((INT)b + iColorShift > 0x0F)
				{
					b = 0x0F;
				}
				else if ((INT)b + iColorShift < 0)
				{
					b = 0;
				}
				else
				{
					b += iColorShift;
				}
				((LPBYTE)lpDstSurface->pixels)[y * lpDstSurface->pitch + x] = (b | bColor);
				//PAL_WhitePixels(lpDstSurface, x, y, b | bColor);
			}
			lpBitmapRLE += T;
			i += T;
		}
	}

end:
	//
	// Success
	//
	return 0;
}


VOID CPalBaseIO::PAL_StartDialog(
	BYTE         bDialogLocation,
	BYTE         bFontColor,
	INT          iNumCharFace,
	BOOL         fPlayingRNG
)
/*++
  Purpose:

  Start a new dialog.
	开始一个新的对话框。
  Parameters:

  [IN]  bDialogLocation - the location of the text on the screen.文本在屏幕上的位置。

  [IN]  bFontColor - the font color of the text.

  [IN]  iNumCharFace - number of the character face in RGM.MKF.

  [IN]  fPlayingRNG - whether we are playing a RNG video or not.

  Return value:

  None.

  --*/
{
	PAL_LARGE BYTE * buf = (BYTE *)malloc(16420);
	SDL_Rect       rect;
	ClearScreen();
	if (fInBattle && !g_fUpdatedInBattle)
	{
		//
		// Update the screen in battle, or the graphics may seem messed up
		//
		VIDEO_UpdateScreen(gpTextureReal);
		g_fUpdatedInBattle = TRUE;
	}

	g_TextLib.bIcon = 0;
	g_TextLib.posIcon = 0;
	g_TextLib.nCurrentDialogLine = 0;
	g_TextLib.posDialogTitle = PAL_XY(12, 8);
	g_TextLib.fUserSkip = FALSE;

	if (bFontColor != 0)
	{
		g_TextLib.bCurrentFontColor = bFontColor;
	}

	if (fPlayingRNG && iNumCharFace)
	{
		VIDEO_BackupScreen(gpTextureReal);
		g_TextLib.fPlayingRNG = TRUE;
	}

	switch (bDialogLocation)
	{
	case kDialogUpper:
		if (iNumCharFace > 0)
		{
			//
			// Display the character face at the upper part of the screen
			//
			if (PAL_MKFReadChunk(buf, 16384, iNumCharFace, gpGlobals->f.fpRGM) > 0)
			{
				rect.w = PAL_RLEGetWidth((LPCBITMAPRLE)buf);
				rect.h = PAL_RLEGetHeight((LPCBITMAPRLE)buf);
				rect.x = 48 - rect.w / 2;
				rect.y = 55 - rect.h / 2;

				if (rect.x < 0)
				{
					rect.x = 0;
				}

				if (rect.y < 0)
				{
					rect.y = 0;
				}

				PAL_RLEBlitToTexture((LPCBITMAPRLE)buf, gpTextureReal, PAL_XY(rect.x, rect.y));

				if (rect.x < 0)
				{
					rect.x = 0;
				}
				if (rect.y < 0)
				{
					rect.y = 0;
				}
				{
					//VIDEO_UpdateScreen(&rect);
					SDL_Rect bRect = rect;
					setPictureRatio(&bRect);
					VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
				}
			}
		}
		g_TextLib.posDialogTitle = PAL_XY(iNumCharFace > 0 ? 80 : 12, 8);
		g_TextLib.posDialogText = PAL_XY(iNumCharFace > 0 ? 96 : 44, 26);
		break;

	case kDialogCenter:
		g_TextLib.posDialogText = PAL_XY(80, 40);
		break;

	case kDialogLower:
		if (iNumCharFace > 0)
		{
			//
			// Display the character face at the lower part of the screen
			//
			if (PAL_MKFReadChunk(buf, 16384, iNumCharFace, gpGlobals->f.fpRGM) > 0)
			{
				rect.x = 270 - PAL_RLEGetWidth((LPCBITMAPRLE)buf) / 2;
				rect.y = 144 - PAL_RLEGetHeight((LPCBITMAPRLE)buf) / 2;

				PAL_RLEBlitToTexture((LPCBITMAPRLE)buf, gpTextureReal, PAL_XY(rect.x, rect.y));

				VIDEO_UpdateScreen(gpTextureReal);
			}
		}
		g_TextLib.posDialogTitle = PAL_XY(iNumCharFace > 0 ? 4 : 12, 108);
		g_TextLib.posDialogText = PAL_XY(iNumCharFace > 0 ? 20 : 44, 126);
		break;

	case kDialogCenterWindow:
		g_TextLib.posDialogText = PAL_XY(160, 40);
		break;
	}

	g_TextLib.bDialogPosition = bDialogLocation;
	free(buf);
}

VOID CPalBaseIO::PAL_DialogWaitForKey(
	VOID
)
/*++
  Purpose:

  Wait for player to press a key after showing a dialog.

  Parameters:

  None.

  Return value:

  None.

  --*/
{

	//
	// get the current palette
	//

	if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
		g_TextLib.bDialogPosition != kDialogCenter)
	{
		//
		// show the icon
		//
		LPCBITMAPRLE p = PAL_SpriteGetFrame(g_TextLib.bufDialogIcons, g_TextLib.bIcon);
		if (p != NULL)
		{
			SDL_Rect rect;


			rect.x = PAL_X(g_TextLib.posIcon);
			rect.y = PAL_Y(g_TextLib.posIcon);
			rect.w = 16;
			rect.h = 16;

			PAL_RLEBlitToTexture(p, gpTextureReal, g_TextLib.posIcon);
			//ClearScreen(&rect);
			//VIDEO_UpdateScreen(&rect);
			setPictureRatio(&rect);
			VIDEO_UpdateScreen(gpTextureReal, &rect, &rect);
		}
	}

	PAL_ClearKeyState();

	while (!PalQuit)
	{
		PAL_Delay(1);

		if (g_InputState.dwKeyPress != 0)
		{
			break;
		}
		VIDEO_UpdateScreen(gpTextureReal);
	}

	if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
		g_TextLib.bDialogPosition != kDialogCenter)
	{
		///
		//ClearScreen(NULL);
		PAL_SetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
	}

	PAL_ClearKeyState();

	g_TextLib.fUserSkip = FALSE;
}

VOID CPalBaseIO::PAL_ShowDialogText(
	LPCSTR       s
)
/*++
  Purpose:

  Show one line of the dialog text.

  Parameters:

  [IN]  lpszText - the text to be shown.

  Return value:

  None.

  --*/
{
	if (s == NULL)
		return;
	int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	LPWSTR lpszTextR = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, s, -1, lpszTextR, len);
	
	PAL_ShowWideDialogText(lpszTextR);

	delete[] lpszTextR;
}

VOID CPalBaseIO::PAL_ShowWideDialogText(
	LPCWSTR       lpszText
)
/*++
  Purpose:

  Show one line of the dialog text.

  Parameters:

  [IN]  lpszText - the text to be shown.

  Return value:

  None.

  --*/
{
	SDL_Rect        rect;
	if (lpszText == NULL)
		return;
	int             x, y, len = lstrlen(lpszText);

	PAL_ClearKeyState();
	g_TextLib.bIcon = 0;

	if (fInBattle && !g_fUpdatedInBattle)
	{
		//
		// Update the screen in battle, or the graphics may seem messed up
		//
		VIDEO_UpdateScreen(gpTextureReal);
		g_fUpdatedInBattle = TRUE;
	}

	if (g_TextLib.nCurrentDialogLine > 3)
	{
		//
		// The rest dialogs should be shown in the next page.
		//
		PAL_DialogWaitForKey();
		g_TextLib.nCurrentDialogLine = 0;
		VIDEO_RestoreScreen();
		VIDEO_UpdateScreen(gpTextureReal);
	}

	x = PAL_X(g_TextLib.posDialogText);
	y = PAL_Y(g_TextLib.posDialogText) + g_TextLib.nCurrentDialogLine * 18;

	if (g_TextLib.bDialogPosition == kDialogCenterWindow)
	{
		//
		// The text should be shown in a small window at the center of the screen
		//
		{
			PAL_POS    pos;
			LPBOX      lpBox;

			//
			// Create the window box
			//
			pos = PAL_XY(PAL_X(g_TextLib.posDialogText) - len * 8, PAL_Y(g_TextLib.posDialogText));
			lpBox = PAL_CreateSingleLineBox(pos, (len + 1) , TRUE);

			rect.x = PAL_X(pos);
			rect.y = PAL_Y(pos);
			rect.w = 320 - rect.x * 2 + 32;
			rect.h = 32;

			//
			// Show the text on the screen
			//
			pos = PAL_XY(PAL_X(pos) + 8 + ((len & 1) << 2), PAL_Y(pos) + 10);
			PAL_DrawWideText(lpszText, pos, 0, FALSE, FALSE);
			{
				SDL_Rect bRect = rect;
				setPictureRatio(&bRect);
				VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
			}

			PAL_DialogWaitForKey();

			//
			// Delete the box
			//
			PAL_DeleteBox(lpBox);
			{
				//VIDEO_UpdateScreen(&rect);
				SDL_Rect bRect = rect;
				setPictureRatio(&bRect);
				VIDEO_UpdateScreen(gpTextureReal, &bRect, &bRect);
			}

			PAL_EndDialog();
		}
	}
	else
	{
		if (g_TextLib.nCurrentDialogLine == 0 &&
			g_TextLib.bDialogPosition != kDialogCenter &&
			(((BYTE)lpszText[len - 1] == 0x47 && (BYTE)lpszText[len - 2] == 0xA1) ||
				((BYTE)lpszText[len - 1] == 0xBA && (BYTE)lpszText[len - 2] == 0xA3) ||
				((BYTE)lpszText[len - 1] == 0xC3 && (BYTE)lpszText[len - 2] == 0xA1) ||
				((BYTE)lpszText[len - 1] == ':')))
		{
			//
			// name of character
			//
			PAL_DrawWideText(lpszText, g_TextLib.posDialogTitle, FONT_COLOR_CYAN_ALT, TRUE, TRUE);
		}
		else
		{
			//
			// normal texts
			//
			WCHAR text[3];

			if (!g_TextLib.fPlayingRNG && g_TextLib.nCurrentDialogLine == 0)
			{
				//
				// Save the screen before we show the first line of dialog
				//
				ClearScreen();
				VIDEO_BackupScreen();
			}

			while (lpszText != NULL && *lpszText != '\0')
			{
				switch (*lpszText)
				{
				case '-':
					//
					// Set the font color to Cyan
					//
					if (g_TextLib.bCurrentFontColor == FONT_COLOR_CYAN)
					{
						g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
					}
					else
					{
						g_TextLib.bCurrentFontColor = FONT_COLOR_CYAN;
					}
					lpszText++;
					break;
					//#ifndef PAL_WIN95
				case '\'':
					//
					// Set the font color to Red
					//
					if (!gConfig->fIsWIN95)
					{
						if (g_TextLib.bCurrentFontColor == FONT_COLOR_RED)
						{
							g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
						}
						else
						{
							g_TextLib.bCurrentFontColor = FONT_COLOR_RED;
						}
					}
					lpszText++;
					break;
//#endif
				case '\"':
					//
					// Set the font color to Yellow
					//
					if (g_TextLib.bCurrentFontColor == FONT_COLOR_YELLOW)
					{
						g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
					}
					else
					{
						g_TextLib.bCurrentFontColor = FONT_COLOR_YELLOW;
					}
					lpszText++;
					break;

				case '$':
					//
					// Set the delay time of text-displaying
					//
					g_TextLib.iDelayTime = _wtoi(lpszText + 1) * 10 / 7;
					lpszText += 3;
					break;

				case '~':
					//
					// Delay for a period and quit
					//
					PAL_Delay(_wtoi(lpszText + 1) * 80 / 7);
					g_TextLib.nCurrentDialogLine = 0;
					g_TextLib.fUserSkip = FALSE;
					return; // don't go further

				case ')':
					//
					// Set the waiting icon
					//
					g_TextLib.bIcon = 1;
					lpszText++;
					break;

				case '(':
					//
					// Set the waiting icon
					//
					g_TextLib.bIcon = 2;
					lpszText++;
					break;

				case '\\':
					lpszText++;
					continue;
					//break;
				default:
				{
					text[0] = *lpszText;
					text[1] = '\0';
					lpszText++;
				}

					PAL_DrawWideText(text, PAL_XY(x, y), g_TextLib.bCurrentFontColor, TRUE, TRUE);
					x += (iswascii( text[0])  ? 8 : 16);

					if (!g_TextLib.fUserSkip)
					{
						VIDEO_UpdateScreen(gpTextureReal);
						PAL_ClearKeyState();
						PAL_Delay(g_TextLib.iDelayTime * 8);

						if (g_InputState.dwKeyPress & (kKeySearch | kKeyMenu))
						{
							//
							// User pressed a key to skip the dialog
							//
							g_TextLib.fUserSkip = TRUE;
						}
					}
				}
			}
			VIDEO_UpdateScreen(gpTextureReal);
			g_TextLib.posIcon = PAL_XY(x, y);
			g_TextLib.nCurrentDialogLine++;
		}
	}
}


VOID CPalBaseIO::PAL_ClearDialog(
	BOOL       fWaitForKey
)
/*++
  Purpose:

  Clear the state of the dialog.

  Parameters:

  [IN]  fWaitForKey - whether wait for any key or not.

  Return value:

  None.

  --*/
{
	if (g_TextLib.nCurrentDialogLine > 0 && fWaitForKey)
	{
		PAL_DialogWaitForKey();
	}

	g_TextLib.nCurrentDialogLine = 0;

	if (g_TextLib.bDialogPosition == kDialogCenter)
	{
		g_TextLib.posDialogTitle = PAL_XY(12, 8);
		g_TextLib.posDialogText = PAL_XY(44, 26);
		g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
		g_TextLib.bDialogPosition = kDialogUpper;
	}
}

VOID CPalBaseIO::PAL_EndDialog(VOID)
/*++
  Purpose:

  Ends a dialog.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	PAL_ClearDialog(TRUE);

	//
	// Set some default parameters, as there are some parts of script
	// which doesn't have a "start dialog" instruction before showing the dialog.
	//
	g_TextLib.posDialogTitle = PAL_XY(12, 8);
	g_TextLib.posDialogText = PAL_XY(44, 26);
	g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
	g_TextLib.bDialogPosition = kDialogUpper;
	g_TextLib.fUserSkip = FALSE;
	g_TextLib.fPlayingRNG = FALSE;
}

BOOL CPalBaseIO::PAL_IsInDialog(
	VOID
)
/*++
  Purpose:

  Check if there are dialog texts on the screen.

  Parameters:

  None.

  Return value:

  TRUE if there are dialog texts on the screen, FALSE if not.

  --*/
{
	return (g_TextLib.nCurrentDialogLine != 0);
}

BOOL CPalBaseIO::PAL_DialogIsPlayingRNG(
	VOID
)
/*++
  Purpose:

  Check if the script used the RNG playing parameter when displaying texts.

  Parameters:

  None.

  Return value:

  TRUE if the script used the RNG playing parameter, FALSE if not.

  --*/
{
	return g_TextLib.fPlayingRNG;
}


VOID CPalBaseIO::PAL_ApplyWave(CGL_Texture * lpText)
{
	static int time;
	CGL_Texture* mTexture = new CGL_Texture(lpText->w, lpText->h, lpText->BitsPerPixel);
	//
	if (lpText->gfPalette)
	{
		mTexture->gfPalette = new PAL_fColor[256];
		memcpy(mTexture->gfPalette, lpText->gfPalette, sizeof(PAL_fColor[256]));
	}
	RenderBlendCopy(mTexture, lpText);
	gpGlobals->wScreenWave += gpGlobals->sWaveProgression;
	if (gpGlobals->wScreenWave == 0 || gpGlobals->wScreenWave >= 256)
	{
		//
		// No need to wave the screen
		//
		gpGlobals->wScreenWave = 0;
		gpGlobals->sWaveProgression = 0;
		delete mTexture;
		return;
	}

	gpGL->setMave(10, 0, gpGlobals->wScreenWave * 0.003, 0.1 * time++);
	RenderBlendCopy(lpText, mTexture);
	gpGL->setMave(0, 0, 0, 0);
	delete mTexture;
}

VOID CPalBaseIO::PAL_ApplyWave(
	SDL_Surface* lpSurface
)
/*++
   Purpose:

   Apply screen waving effect when needed.

   Parameters:

   [OUT] lpSurface - the surface to be proceed.

   Return value:

   None.

   --*/
{
	int                  wave[32];
	int                  i, a, b;
	static int           index = 0;
	LPBYTE               p;
	BYTE                 buf[320];

	gpGlobals->wScreenWave += gpGlobals->sWaveProgression;

	if (gpGlobals->wScreenWave == 0 || gpGlobals->wScreenWave >= 256)
	{
		//
		// No need to wave the screen
		//
		gpGlobals->wScreenWave = 0;
		gpGlobals->sWaveProgression = 0;
		return;
	}

	//
	// Calculate the waving offsets.
	//
	a = 0;
	b = 60 + 8;

	for (i = 0; i < 16; i++)
	{
		b -= 8;
		a += b;

		//
		// WARNING: assuming the screen width is 320
		//
		wave[i] = a * gpGlobals->wScreenWave / 256;
		wave[i + 16] = 320 - wave[i];
	}

	//
	// Apply the effect.
	// WARNING: only works with 320x200 8-bit surface.
	//
	a = index;
	p = (LPBYTE)(lpSurface->pixels);

	//
	// Loop through all lines in the screen buffer.
	//
	for (i = 0; i < 200; i++)
	{
		b = wave[a];

		if (b > 0)
		{
			//
			// Do a shift on the current line with the calculated offset.
			//
			memcpy(buf, p, b);
			//memmove(p, p + b, 320 - b);
			memmove(p, &p[b], 320 - b);
			//memcpy(p + 320 - b, buf, b);
			memcpy(&p[320 - b], buf, b);
		}

		a = (a + 1) % 32;
		p += lpSurface->pitch;
	}

	index = (index + 1) % 32;
}

VOID CPalBaseIO::PAL_DialogSetDelayTime(
	INT          iDelayTime
)
/*++
  Purpose:

  Set the delay time for dialog.

  Parameters:

  [IN]  iDelayTime - the delay time to be set.

  Return value:

  None.

  --*/
{
	g_TextLib.iDelayTime = iDelayTime;
}
