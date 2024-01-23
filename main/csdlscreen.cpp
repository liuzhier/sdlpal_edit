#include "csdlscreen.h"
#include "cpalapp.h"
#include "cgl_render.h"
#include <iostream>
 
static CGL_Texture* number_texture{};

CSdlScreen::CSdlScreen() 
{
} 

CSdlScreen::~CSdlScreen()
{
    VideoShutDown();
}


void CSdlScreen::VIDEO_ShakeScreen(WORD wShakeTime, WORD wShakeLevel)
{
    g_wShakeLevel = wShakeLevel;
    g_wShakeTime = wShakeTime;
}

VOID CSdlScreen::VIDEO_FadeScreen(WORD  wSpeed)
/*++
  Purpose:

    Fade from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.
  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
    wSpeed++;
    DWORD time = SDL_GetTicks_New();
    RenderBlendCopy(gpGL->gpRenderTexture, gpTextureRealBak);
    RenderPresent(gpGL->gpRenderTexture);

    for (int i = 0; i < 64;i++)
    {
        if (g_wShakeTime)
        {
            if (g_wShakeTime & 1)
            {
                gpGL->setZoom(1.0 + g_wShakeLevel * 0.0075, 1.0 + g_wShakeLevel * 0.012, 0, 1);
            }
            g_wShakeTime--;
        }
        RenderBlendCopy(gpGL->gpRenderTexture, gpTextureRealBak);
        RenderBlendCopy(gpGL->gpRenderTexture, gpTextureReal, i * 3 + 63, 0);

        gpGL->setZoom(0, 0, 0, 0);

        RenderPresent(gpGL->gpRenderTexture);
        PAL_ProcessEvent();
        while (SDL_GetTicks_New() <= time)
        {
            PAL_ProcessEvent();
            PAL_Delay(2);
        }
        time += wSpeed * 6;
    }
    RenderBlendCopy(gpGL->gpRenderTexture, gpTextureReal);
    //RenderBlendCopy(gpGL->gpRenderTexture, gpGL->gpTextTexture);
    RenderPresent(gpGL->gpRenderTexture);
    time = SDL_GetTicks_New() - time;
    SDL_Delay(1);
}

SDL_Surface* CSdlScreen::VIDEO_CreateCompatibleSurface(
    SDL_Surface* pSource
)
{
    return VIDEO_CreateCompatibleSizedSurface(pSource, NULL);
}

VOID CSdlScreen::VIDEO_BackupScreen(SDL_Surface * s)
{
    RenderBlendCopy(gpTextureRealBak, s);
}

VOID CSdlScreen::VIDEO_BackupScreen(CGL_Texture* s)
{
    if(!s) s = gpTextureReal;
    RenderBlendCopy(gpTextureRealBak, s);
}

VOID CSdlScreen::VIDEO_RestoreScreen()
{
    RenderBlendCopy(gpTextureReal, gpTextureRealBak);
}

SDL_Surface* CSdlScreen::VIDEO_CreateCompatibleSizedSurface(
    SDL_Surface* pSource,
    const SDL_Rect* pSize
)
/*++
  Purpose:

    Create a surface that compatible with the source surface.

  Parameters:

    [IN]  pSource   - the source surface from which attributes are taken.
    [IN]  pSize     - the size (width & height) of the created surface.

  Return value:

    None.

--*/
{
    //
    // Create the surface
    //
    SDL_Surface* dest = SDL_CreateRGBSurface(pSource->flags,
        pSize ? pSize->w : pSource->w,
        pSize ? pSize->h : pSource->h,
        pSource->format->BitsPerPixel,
        pSource->format->Rmask, pSource->format->Gmask,
        pSource->format->Bmask, pSource->format->Amask);

    if (dest)
    {
        VIDEO_UpdateSurfacePalette(dest);
    }

    return dest;
}

VOID CSdlScreen::VIDEO_UpdateSurfacePalette(SDL_Surface* pSurface)
/*++
  Purpose:

    Use the global palette to update the palette of pSurface.

  Parameters:

    [IN]  pSurface - the surface whose palette should be updated.

  Return value:

    None.

--*/
{
    SDL_SetSurfacePalette(pSurface, gpPalette);
}

#define fabs(a) ((a) > 0 ? (a):(-(a)) )

VOID CSdlScreen::VIDEO_UpdateScreen(CGL_Texture* lpDstText, const SDL_Rect* lpSrcRect,const SDL_Rect* lpDstRect)
{
    if (lpDstText && lpDstText != gpGL->gpRenderTexture ) 
        RenderBlendCopy(gpGL->gpRenderTexture,lpDstText,getAlpha(),1, getpColor(),lpSrcRect,lpDstRect);
    if (g_wShakeTime)
    {
        if (g_wShakeTime & 1)
        {
            gpGL->setZoom(1.0 + g_wShakeLevel * 0.0075, 1.0 + g_wShakeLevel * 0.012, 0, 1);
        }
        g_wShakeTime--;
    }
    RenderPresent(gpGL->gpRenderTexture);
    gpGL->setZoom(0, 0, 0, 0);
}

/*++
  Purpose:

    Update the screen area specified by lpRect.

  Parameters:

    [IN]  lpRect - Screen area to update.

  Return value:

    None.

--*/



VOID CSdlScreen::PAL_FadeIn(
    INT         iPaletteNum,
    BOOL        fNight,
    INT         iDelay
)
/*++
  Purpose:

    Fade in the screen to the specified palette.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

    [IN]  iDelay - delay time for each step.

  Return value:

    None.

--*/
{
    VIDEO_SetPalette(PAL_GetPalette(iPaletteNum, fNight));

    if (iDelay <= 0) iDelay = 1;
    setAlpha(0);
    VIDEO_UpdateScreen(gpTextureReal);
    double v_time = (double)SDL_GetTicks_New() + (INT)iDelay * 500;
    while (v_time > SDL_GetTicks_New())
    {
        setAlpha(255 - (v_time - SDL_GetTicks_New()) * 255.0 / 500 / iDelay);
        VIDEO_UpdateScreen(gpTextureReal);
        PAL_Delay(1);
    }
    setAlpha(255);
    VIDEO_UpdateScreen(gpTextureReal);
}

VOID CSdlScreen::PAL_FadeOut(
    INT         iDelay
)
/*++
  Purpose:

    Fadeout screen to black from the specified palette.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

    [IN]  iDelay - delay time for each step.

  Return value:

    None.

--*/
{
    if (iDelay <= 0) iDelay = 1;

    setAlpha(255);
    VIDEO_UpdateScreen(gpTextureReal);
    for(int n=0;n<16;n++)
    {
        setAlpha(255 - 12 * n);
        VIDEO_UpdateScreen(gpTextureReal);
        PAL_Delay(iDelay * 15);
    }
    //
    RenderBlendCopy(gpTextureReal,(CGL_Texture*) nullptr, 0, 2);
    VIDEO_UpdateScreen(gpTextureReal);

    PAL_LARGE SDL_Color      newpalette[256];
    memset(newpalette, 0, sizeof(newpalette));
    VIDEO_SetPalette(newpalette);
    setAlpha(255); 
}

VOID CSdlScreen::PAL_FadeOut(CGL_Texture* dstText, INT iDelay)
{
    CGL_Texture * rpText = gpGL->gpRenderTexture;
    if (dstText) rpText = dstText;
    for (int n = 0; n < 16; n++)
    {
        gpGL->RenderBlendCopy(NULL, rpText, 255 - 12 * n, 10);
        SDL_GL_SwapWindow(gpGL->gpWindow);
        PAL_Delay(iDelay * 15);
    }
}

VOID CSdlScreen::PAL_SetPalette(
    INT         iPaletteNum,
    BOOL        fNight
)
/*++
  Purpose:

    Set the screen palette to the specified one.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

  Return value:

    None.

--*/
{
    SDL_Color* p = PAL_GetPalette(iPaletteNum, fNight);

    if (p != NULL)
    {
        VIDEO_SetPalette(p);
    }
}
     
VOID CSdlScreen::PAL_DrawText(LPCSTR lpszText, PAL_POS pos, BYTE bColor, BOOL fShadow, BOOL fUpdate, int size)
{
    //已经将输入改为UTF8格式
    gpGL->PAL_DrawTextUTF8(lpszText, pos, VIDEO_GetPalette()[bColor], fShadow, fUpdate, size);
    if (fUpdate)
    {
        //RenderBlendCopy(gpGL->gpRenderTexture, gpGL->gpTextTexture, 255, 3);
        RenderPresent(gpTextureReal);
    }
}

SIZE CSdlScreen::PAL_DrawWideText(LPCWSTR lpszTextR, PAL_POS pos, BYTE bColor, BOOL fShadow, BOOL fUpdate, int size)
{
    return gpGL->PAL_DrawWideText(lpszTextR, pos, VIDEO_GetPalette()[bColor], fShadow, fUpdate, size);
        //VIDEO_UpdateScreen(gpTextureReal);
}


VOID CSdlScreen::PAL_DrawTextUTF8(LPCSTR s, PAL_POS pos, BYTE bColor, BOOL fShadow, BOOL fUpdate, int size)
{
    
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    LPWSTR lpszTextR = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, s, -1, lpszTextR, len);

    PAL_DrawWideText(lpszTextR,pos,bColor, fShadow, fUpdate, size);

    delete[] lpszTextR;
}


VOID CSdlScreen::VIDEO_SetPalette(SDL_Color rgPalette[256])
/*++
  Purpose:

    Set the palette of the screen.

  Parameters:

    [IN]  rgPalette - array of 256 colors.

  Return value:

    None.

--*/
{

    SDL_SetPaletteColors(gpPalette, rgPalette, 0, 256);

    //SDL_SetSurfacePalette(gpScreen, gpPalette);
 
    //
    // HACKHACK: need to invalidate gpglScreen->map otherwise the palette
    // would not be effective during blit
    //
    //SDL_SetSurfaceColorMod(gpScreen, 0, 0, 0);
    //SDL_SetSurfaceColorMod(gpScreen, 0xFF, 0xFF, 0xFF);

    gpGL->setPalette(rgPalette);
    if(number_texture)
        for (int n = 0; n < 256; n++)
        {
            number_texture->ColorTofColor(rgPalette[n], number_texture->gfPalette[n]);
        }
}

SDL_Color* CSdlScreen::PAL_GetPalette(
    INT         iPaletteNum,
    BOOL        fNight
)
/*++
  Purpose:

    Get the specified palette in pat.mkf file.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

  Return value:

    Pointer to the palette. NULL if failed.

--*/
{
    static SDL_Color      palette[256]{0};
    PAL_LARGE BYTE        buf[1536]{0};
    INT                   i;
    FILE* fp;

    string path = PalDir + "pat.mkf";
    fp = fopen(path.c_str(),"rb");
    SDL_zero(buf);
    //
    // Read the palette data from the pat.mkf file
    //
    UINT     uiOffset = 0;
    UINT     uiNextOffset = 0;

    //i = PAL_MKFReadChunk(buf, 1536, iPaletteNum, fp);
    fseek(fp, 4 * iPaletteNum, SEEK_SET);
    fread(&uiOffset, 4, 1, fp);
    fread(&uiNextOffset, 4, 1, fp);
    uiOffset = SWAP32(uiOffset);
    uiNextOffset = SWAP32(uiNextOffset);
    i = uiNextOffset - uiOffset;
    
    if (i > 0)
    {
        fseek(fp, uiOffset, SEEK_SET);
        fread(buf, i, 1, fp);
    }
    fclose(fp);

    if (i < 0)
    {
        //
        // Read failed
        //
        return NULL;
    }
    else if (i <= 256 * 3)
    {
        //
        // There is no night colors in the palette
        //
        fNight = FALSE;
    }

    for (i = 0; i < 256; i++)
    {
        palette[i].r = buf[(fNight ? 256 * 3 : 0) + i * 3] << 2;
        palette[i].g = buf[(fNight ? 256 * 3 : 0) + i * 3 + 1] << 2;
        palette[i].b = buf[(fNight ? 256 * 3 : 0) + i * 3 + 2] << 2;
        if (0 && i)
        {
            palette[i].r += (255 - palette[i].r) / 12;
            palette[i].g += (255 - palette[i].g) / 12;
            palette[i].b += (255 - palette[i].b) / 12;
        }
    }

    return palette;
}

VOID CSdlScreen::PAL_DrawNumber(
    UINT            iNum,
    UINT            nLength,
    PAL_POS         pos,
    NUMCOLOR        color,
    NUMALIGN        align,
    int iSize
)
/*++
  Purpose:

    Draw the specified number with the bitmaps in the UI sprite.

  Parameters:

    [IN]  iNum - the number to be drawn.

    [IN]  nLength - max. length of the number.

    [IN]  pos - position on the screen.

    [IN]  color - color of the number (yellow or blue).

    [IN]  align - align mode of the number.

  Return value:

    None.

--*/
{
#if 0

    UINT          nActualLength, i;
    int           x, y;
    LPCBITMAPRLE  rglpBitmap[10];

    //
    // Get the bitmaps. Blue starts from 29, Cyan from 56, Yellow from 19.
    //
    x = (color == kNumColorBlue) ? 29 : ((color == kNumColorCyan) ? 56 : 19);

    for (i = 0; i < 10; i++)
    {
        rglpBitmap[i] = PAL_SpriteGetFrame(gpSpriteUI, (UINT)x + i);
    }

    i = iNum;
    nActualLength = 0;

    //
    // Calculate the actual length of the number.
    //
    while (i > 0)
    {
        i /= 10;
        nActualLength++;
    }

    if (nActualLength > nLength)
    {
        nActualLength = nLength;
    }
    else if (nActualLength == 0)
    {
        nActualLength = 1;
    }

    x = PAL_X(pos) - 6;
    y = PAL_Y(pos);

    switch (align)
    {
    case kNumAlignLeft:
        x += 6 * nActualLength;
        break;

    case kNumAlignMid:
        x += 3 * (nLength + nActualLength);
        break;

    case kNumAlignRight:
        x += 6 * nLength;
        break;
    }

    //
    // Draw the number.
    //
    while (nActualLength-- > 0)
    {
        PAL_RLEBlitToSurface(rglpBitmap[iNum % 10], gpScreen, PAL_XY(x, y));
        x -= 6;
        iNum /= 10;
    }

# else
    {
        //LPCBITMAPRLE  rglpBitmap[10];
        if (!number_texture)
        {
            SDL_Surface * numSurf = SDL_CreateRGBSurface(0,60,3 * 8, 8, 0, 0, 0, 0);
            SDL_SetSurfacePalette(numSurf, gpPalette);
            LPCBITMAPRLE  rglpBitmap;
            WORD colors[3] = { 19,29,56 };
            for (int n = 0; n < 3; n++)
            {
                for (int  i = 0; i < 10; i++)
                {
                    rglpBitmap = PAL_SpriteGetFrame(gpSpriteUI, colors[n] + i);
                    PAL_RLEBlitToSurface(rglpBitmap, numSurf, PAL_XY(i * 6, n * 8));
                }
            }
            //SDL_SaveBMP(numSurf, "numSurf.bmp");
            number_texture = gpGL->creatglTextureFromSurface(numSurf);
            SDL_FreeSurface(numSurf);
        }
        
        string s = std::to_string(iNum);
        int leftlen = 0;
        if (s.length() < nLength)
        {
            switch (align)
            {
            case kNumAlignLeft:
                break;
            case kNumAlignMid:
                leftlen = (nLength - s.length()) / 2;
                break;
            case kNumAlignRight:
                leftlen = (nLength - s.length());
                break;
            default:
                break;
            }
            //在左边加空格
        }
        int x = PAL_X(pos) + leftlen * 6;
        int y = PAL_Y(pos);
        SDL_Rect srcRect = { 0,color * 8,6,8 };

        for (int n = 0; n < s.length(); n++, x += 6)
        {
            SDL_Rect srcRect = { ((s.c_str()[n] & 255) - '0') * 6,color * 8,6,8 };
            SDL_Rect dstRect = { x,y,6,iSize };
            setPictureRatio(&dstRect);
            
            RenderBlendCopy(gpTextureReal, number_texture, 255, 3, &gpPalette->colors[0], &dstRect, &srcRect);
        }
        //gpGL->PAL_DrawTextUTF8(s.c_str(), pos, dColor, 0, 0, 10);
    }
#endif

}

VOID CSdlScreen::RenderBlendCopy(CGL_Texture* rpRender, CGL_Texture* rpText1, const WORD rAlpha, const WORD mode, const SDL_Color* rColor, const SDL_Rect* dstRect, const SDL_Rect* srcRect)
{
    return gpGL->RenderBlendCopy(rpRender, rpText1, rAlpha, mode, rColor, dstRect, srcRect);
}

VOID CSdlScreen::RenderBlendCopy(CGL_Texture* rpRender, SDL_Surface* rpSurf, const WORD rAlpha, const WORD mode, const SDL_Color* rColor, const SDL_Rect* dstRect, const SDL_Rect* srcRect)
{
    if (mode == 3 && rColor == nullptr && rpSurf->format->palette)
        rColor = &rpSurf->format->palette->colors[0];
    return gpGL->RenderBlendCopy(rpRender, rpSurf, rAlpha, mode, rColor, dstRect, srcRect);
}

VOID CSdlScreen::RenderPresent(CGL_Texture* glRender, INT dAlpha)
{
    //全屏切换
    
    if (gConfig->fFullScreen != CPalEvent::get_switch_WINDOW_FULLSCREEN())
    {
        gConfig->fFullScreen = CPalEvent::get_switch_WINDOW_FULLSCREEN();
        SDL_SetWindowFullscreen(gpWindow, gConfig->fFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        if (!gConfig->fFullScreen)
            SDL_SetWindowSize(gpWindow, WindowWidth, WindowHeight);

    }

    if (glRender != gpRenderTexture)
        RenderBlendCopy(gpRenderTexture, glRender);
    gpGL->RenderPresent(gpRenderTexture, dAlpha);
    gpGL->setZoom(0, 0, 0, 0);
}

VOID CSdlScreen::ClearScreen(const SDL_Rect* sRect)
{
    gpGL->clearScreen(sRect);
}


SDL_Color* CSdlScreen::VIDEO_GetPalette(VOID)
/*++
  Purpose:

    Get the current palette of the screen.

  Parameters:

    None.

  Return value:

    Pointer to the current palette.

--*/
{
    return gpPalette->colors;
}

VOID CSdlScreen::VIDEO_SwitchScreen(
    WORD           wSpeed
)
/*++
  Purpose:

    Switch the screen from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
    wSpeed++;
    wSpeed *= 10;
    for (int i = 0; i < 6; i++)
    {
        RenderBlendCopy(gpRenderTexture, gpTextureRealBak);
        RenderBlendCopy(gpRenderTexture, gpTextureReal, i * 51, 0);
        RenderPresent(gpRenderTexture);
        SDL_Delay(wSpeed);
    }
    RenderBlendCopy(gpRenderTexture, gpTextureReal);
    RenderPresent(gpRenderTexture);
}

VOID CSdlScreen::PAL_ColorFade(
    INT        iDelay,
    BYTE       bColor,
    BOOL       fFrom
)
/*++
  Purpose:

    Fade the palette from/to the specified color.

  Parameters:

    [IN]  iDelay - the delay time of each step.

    [IN]  bColor - the color to fade from/to.

    [IN]  fFrom - if TRUE then fade from bColor, else fade to bColor.

  Return value:

    None.

--*/
{

    iDelay *= 10;

    if (iDelay == 0)
    {
        iDelay = 10;
    }

    for (int i = 0; i < 64; i++)
    {
        if (fFrom)
        {
            RenderBlendCopy(gpGL->gpRenderTexture, gpTextureReal, 256 - (i << 2), 4, gpPalette->colors + bColor);
        }
        else
        {
            RenderBlendCopy(gpGL->gpRenderTexture, gpTextureReal, (i << 2), 4, gpPalette->colors + bColor);
        }
        RenderPresent(gpGL->gpRenderTexture);
        SDL_Delay(iDelay);
    }
}

VOID CSdlScreen::PAL_FadeToRed(
    VOID
)
/*++
  Purpose:

    Fade the whole screen to red color.(game over)

  Parameters:

    None.

  Return value:

    None.

--*/
{
    //VIDEO_BackupScreen();
    for (int i = 63; i >= 30; i--)
    {
        setColor( 255,i << 2,i << 2 );
        VIDEO_UpdateScreen(gpTextureReal);
        PAL_Delay(20);
    }
 }


INT CSdlScreen::PAL_RNGBlitTo(
    const uint8_t* rng,
    int length,
    VOID* dstData,
    SDATA pCallback
)
/*++
Purpose:

  Blit one frame in an RNG animation to an SDL surface.
  The surface should contain the last frame of the RNG, or blank if it's the first
  frame.

  NOTE: Assume the surface is already locked, and the surface is a 320x200 8-bit one.
  已增加对32-BIT
Parameters:

  [IN]  rng - Pointer to the RNG data.// 指源数据的指针

  [IN]  len - Length of the RNG data.//源数据长度

  IN    data 目标数据

  [OUT]  data //指向目标数据回调函数 返回0 成功 其他失败 （颜色，目标数据宽度，目标数据长度，目标数据指针）.

Return value:

  0 = success, -1 = error.

--*/

{
    int                   ptr = 0;
    int                   dst_ptr = 0;
    uint16_t              wdata = 0;
    int                   x, y, i, n;

    //
    // Check for invalid parameters.
    //
    SDL_Surface* lpDstSurface = (SDL_Surface*)dstData;
    
    if (lpDstSurface == NULL || length < 0)
    {
        return -1;
    }
    BOOL is32bit = lpDstSurface->format->BitsPerPixel == 32 ? TRUE : FALSE;
    UINT32* mColor = (UINT32*)VIDEO_GetPalette();
    //
    // Draw the frame to the surface.
    // FIXME: Dirty and ineffective code, needs to be cleaned up
    //
    while (ptr < length)
    {
        uint8_t data = rng[ptr++];
        switch (data)
        {
        case 0x00:
        case 0x13:
            //
            // End
            //
            goto end;

        case 0x02:
            dst_ptr += 2;
            break;

        case 0x03:
            data = rng[ptr++];
            dst_ptr += (data + 1) * 2;
            break;

        case 0x04:
            wdata = rng[ptr] | (rng[ptr + 1] << 8);
            ptr += 2;
            dst_ptr += ((unsigned int)wdata + 1) * 2;
            break;

        case 0x0a:
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            pCallback(rng[ptr++], x, y, dstData);
            if (++x >= 320)
            {
                x = 0;
                ++y;
            }
            pCallback(rng[ptr++], x, y, dstData);
            dst_ptr += 2;

        case 0x09:
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            pCallback(rng[ptr++], x, y, dstData);
            if (++x >= 320)
            {
                x = 0;
                ++y;
            }
            pCallback(rng[ptr++], x, y, dstData);
            dst_ptr += 2;

        case 0x08:
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            pCallback(rng[ptr++], x, y, dstData);
            if (++x >= 320)
            {
                x = 0;
                ++y;
            }
            pCallback(rng[ptr++], x, y, dstData);
            dst_ptr += 2;

        case 0x07:
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            pCallback(rng[ptr++], x, y, dstData);
            if (++x >= 320)
            {
                x = 0;
                ++y;
            }
            pCallback(rng[ptr++], x, y, dstData);
            dst_ptr += 2;

        case 0x06:
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            pCallback(rng[ptr++], x, y, dstData);
            if (++x >= 320)
            {
                x = 0;
                ++y;
            }
            pCallback(rng[ptr++], x, y, dstData);
            dst_ptr += 2;
            break;

        case 0x0b:
            data = *(rng + ptr++);
            for (i = 0; i <= data; i++)
            {
                x = dst_ptr % 320;
                y = dst_ptr / 320;
                pCallback(rng[ptr++], x, y, dstData);
                if (++x >= 320)
                {
                    x = 0;
                    ++y;
                }
                pCallback(rng[ptr++], x, y, dstData);
                dst_ptr += 2;
            }
            break;

        case 0x0c:
            wdata = rng[ptr] | (rng[ptr + 1] << 8);
            ptr += 2;
            for (i = 0; i <= wdata; i++)
            {
                x = dst_ptr % 320;
                y = dst_ptr / 320;
                pCallback(rng[ptr++], x, y, dstData);
                if (++x >= 320)
                {
                    x = 0;
                    ++y;
                }
                pCallback(rng[ptr++], x, y, dstData);
                dst_ptr += 2;
            }
            break;

        case 0x0d:
        case 0x0e:
        case 0x0f:
        case 0x10:
            for (i = 0; i < data - (0x0d - 2); i++)
            {
                x = dst_ptr % 320;
                y = dst_ptr / 320;
                pCallback(rng[ptr], x, y, dstData);
                if (++x >= 320)
                {
                    x = 0;
                    ++y;
                }
                pCallback(rng[ptr + 1], x, y, dstData);
                if (is32bit)
                {
                    ((UINT32*)(lpDstSurface->pixels))[y * lpDstSurface->pitch / 4 + x] = mColor[rng[ptr + 1]];
                }
                else
                    ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr + 1];
                dst_ptr += 2;
            }
            ptr += 2;
            break;

        case 0x11:
            data = *(rng + ptr++);
            for (i = 0; i <= data; i++)
            {
                x = dst_ptr % 320;
                y = dst_ptr / 320;
                pCallback(rng[ptr], x, y, dstData);
                if (++x >= 320)
                {
                    x = 0;
                    ++y;
                }
                pCallback(rng[ptr + 1], x, y, dstData);
                dst_ptr += 2;
            }
            ptr += 2;
            break;

        case 0x12:
            n = (rng[ptr] | (rng[ptr + 1] << 8)) + 1;
            ptr += 2;
            for (i = 0; i < n; i++)
            {
                x = dst_ptr % 320;
                y = dst_ptr / 320;
                pCallback(rng[ptr], x, y, dstData);
                if (++x >= 320)
                {
                    x = 0;
                    ++y;
                }
                pCallback(rng[ptr + 1], x, y, dstData);
                dst_ptr += 2;
            }
            ptr += 2;
            break;
        }
    }

end:
    return 0;
}

INT CSdlScreen::PAL_RNGBlitToSurface(
    const uint8_t* rng,
    int              length,
    SDL_Surface* lpDstSurface
)
/*++
  Purpose:

    Blit one frame in an RNG animation to an SDL surface.
    The surface should contain the last frame of the RNG, or blank if it's the first
    frame.

    NOTE: Assume the surface is already locked, and the surface is a 320x200 8-bit one.
    增加对32-BIT 表面支持
  Parameters:

    [IN]  rng - Pointer to the RNG data.

    [IN]  length - Length of the RNG data.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    0 = success, -1 = error.

--*/
{
    auto bCallback = [&](UINT8 srcVal, int x, int y, void* dstData)->int
    {

        SDL_Surface* lpDstSurface = (SDL_Surface*)dstData;
        BOOL is32bit = lpDstSurface->format->BitsPerPixel == 32 ? TRUE : FALSE;
        UINT32* mColor = (UINT32*)VIDEO_GetPalette();
        if (is32bit)
        {
            ((UINT32*)(lpDstSurface->pixels))[y * (lpDstSurface->pitch >> 2) + x] = mColor[srcVal] ? mColor[srcVal] : 1;
        }
        else
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = srcVal;
        return 0;
    };
    return PAL_RNGBlitTo(rng, length, lpDstSurface, bCallback);
}

VOID CSdlScreen::VideoInit()
{
    gpGL = new GL_Render(pCScript , PalWnd);
    gpGL->KeepAspectRatio = gConfig->fKeepAspectRatio;//保持窗口比率
    //初始化文字纹理
    int bIsBig5 = (gpGlobals->bIsBig5 ? 1 : 0) + (gConfig->fIsUseBig5 ? 2 : 0);

    if (gpGL->FontInit(PalDir, bIsBig5)) 
    {
        //初始字体建立失败，退出，退出码10
        exit(10);
    }

    gpRenderTexture = gpGL->gpRenderTexture;

    gpTextureReal = new CGL_Texture(RealWidth, RealHeight, 32);
    gpTextureRealBak = new CGL_Texture(RealWidth, RealHeight, 32);

    gpWindow = gpGL->gpWindow;
    gpGL->gpTextTexture = gpTextureReal;

}

VOID CSdlScreen::VideoShutDown()
{
    if (gpTextureReal) delete gpTextureReal;
    if (gpTextureRealBak) delete gpTextureRealBak;
    gpTextureReal = nullptr;
    gpTextureRealBak = nullptr;
    if (gpGL)
        delete gpGL;
    gpGL = nullptr;
    gpWindow = nullptr;//非new
    if (number_texture)
        delete number_texture;
    number_texture = nullptr;
    gpRenderTexture = nullptr;//非new
    return VOID();
}
