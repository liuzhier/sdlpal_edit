#include "cpalui.h"

BYTE
PAL_CalcShadowColor(
   BYTE bSourceColor
)
{
    return ((bSourceColor & 0xF0) | ((bSourceColor & 0x0F) >> 1));
}

CPalUI::CPalUI()
{
    //loadAllPalette();

    gpPalette = SDL_AllocPalette(256);
}



INT CPalUI::PAL_RLEBlitToTexture(
    LPCBITMAPRLE      lpBitmapRLE,//RLE地址
    CGL_Texture*      lpDstText,//目標紋理
    PAL_POS           pos,//位置
    BOOL              bShadow,//是否顯示陰影
    BOOL              Ratio,//是否放大
    const SDL_Color *       pcolors
)
{
    UINT          uiLen{ 0 };
    UINT          uiWidth{ 0 };
    UINT          uiHeight{ 0 };
    INT           x{ 0 }, y{ 0 }, dx{ 0 }, dy{0};
    UINT          i{ 0 }, j{0}, k, sx;

    if (!lpBitmapRLE || !lpDstText)
        return -1;
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
    /*
    SDL_Surface* mSurf = SDL_CreateRGBSurface(0, uiWidth, uiHeight, 8, 0, 0, 0, 0);
    SDL_SetSurfacePalette(mSurf, gpPalette);
    //SDL_FillRect(mSurf, 0, *((UINT*)&mSurf->format->palette->colors[0]));
    if (!mSurf) return -1;
    
    if (PAL_RLEBlitToSurface(lpBitmapRLE, mSurf, 0) == -1)
    {
        SDL_FreeSurface(mSurf);
        return -1;
    }
    SDL_Rect dstRect = { PAL_X(pos), PAL_Y(pos), uiWidth,uiHeight };
    ///
    if (lpDstText->w == RealWidth)
        setPictureRatio(&dstRect);
    SDL_Color* color = &mSurf->format->palette->colors[0];
    gpGL->RenderBlendCopy(lpDstText, mSurf, 255, 3, color, &dstRect);
    SDL_FreeSurface(mSurf);
    */
    if (!pcolors)
        pcolors = gpPalette->colors;
    vector<UINT32> tBuf;
    uiLen = uiWidth * uiHeight;
    tBuf.resize(uiLen);

    //
    // Start decoding and blitting the bitmap.
    //
    lpBitmapRLE += 4;
    UINT uiSrcX{ 0 };
    for (int i = 0; i < uiLen;)
    {
        auto T = *lpBitmapRLE++;
        if ((T & 0x80) && T <= 0x80 + uiWidth)
        {
            i += T - 0x80;
            uiSrcX += T - 0x80;
            if (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
        else
        {
            //
            // Prepare coordinates.
            //
            j = 0;
            sx = uiSrcX;
            x = dx + uiSrcX;
            y = dy;

            //
            // Skip the points which are out of the surface.
            //
            if (y < 0)
            {
                j += -y * uiWidth;
                y = 0;
            }
            else if (y >= uiHeight)
            {
                goto end; // No more pixels needed, break out
            }

            while (j < T)
            {
                //
                // Skip the points which are out of the surface.
                //
                if (x < 0)
                {
                    j += -x;
                    if (j >= T) break;
                    sx += -x;
                    x = 0;
                }
                else if (x >= uiWidth)
                {
                    j += uiWidth - sx;
                    x -= sx;
                    sx = 0;
                    y++;
                    if (y >= uiHeight)
                    {
                        goto end; // No more pixels needed, break out
                    }
                    continue;
                }

                //
                // Put the pixels in row onto the surface
                //
                k = T - j;
                if (uiWidth - x < k) 
                    k = uiWidth - x;
                if (uiWidth - sx < k)
                    k = uiWidth - sx;
                sx += k;
                auto p =  &tBuf[ y * uiWidth];
                if (bShadow)
                {
                    j += k;
                    for (; k != 0; k--)
                    {
                        auto bColor = PAL_CalcShadowColor(p[x]);
                        p[x] = *(UINT32*)( & gpPalette->colors[bColor]);
                        /////
                        if (p[x] == 0)
                            p[x] = 1;
                        x++;
                    }
                }
                else
                {
                    for (; k != 0; k--)
                    {
                        p[x] = *( (UINT32*)&gpPalette->colors[lpBitmapRLE[j]]);
                        /////
                        if (p[x] == 0)
                            p[x] = 1;
                        j++;
                        x++;
                    }
                }

                if (sx >= uiWidth)
                {
                    sx -= uiWidth;
                    x -= uiWidth;
                    y++;
                    if (y >= uiHeight)
                    {
                        goto end; // No more pixels needed, break out
                    }
                }
            }
            lpBitmapRLE += T;
            i += T;
            uiSrcX += T;
            while (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
    }

end:

    CGL_Texture* bText = new CGL_Texture(uiWidth, uiHeight, 32);
    gpGL->UpdateTexture(bText, NULL, &tBuf[0], uiWidth * 4);

    SDL_Rect rect{ 0 };
    rect.x = PAL_X(pos);
    rect.y = PAL_Y(pos);
    rect.w = uiWidth;
    rect.h = uiHeight;

    if (Ratio)
        setPictureRatio(&rect);
 
    gpGL->RenderBlendCopy(lpDstText, bText, 255, 3, 0, &rect);

    delete bText;
    return 0;
}



INT CPalUI::PAL_RLEBlitToSurface(
    LPCBITMAPRLE      lpBitmapRLE,
    SDL_Surface*      lpDstSurface,
    PAL_POS           pos,
    BOOL              bShadow,
    BOOL              d  
)
/*++
  Purpose:

    Blit an RLE-compressed bitmap to an SDL surface.
    NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

    [IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

    [IN]  pos - position of the destination area.

    [IN]  bShadow - flag to mention whether blit source color or just shadow.

  Return value:

    0 = success, -1 = error.

--*/
{
    UINT          i, j, k, sx;
    INT           x, y;
    UINT          uiLen = 0;
    UINT          uiWidth = 0;
    UINT          uiHeight = 0;
    UINT          uiSrcX = 0;
    BYTE          T;
    INT           dx = PAL_X(pos);
    INT           dy = PAL_Y(pos);
    LPBYTE        p;

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
    // Check whether bitmap intersects the surface.
    //
    if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
        uiHeight + dy <= 0 || dy >= lpDstSurface->h)
    {
        goto end;
    }

    //
    // Calculate the total length of the bitmap.
    // The bitmap is 8-bpp, each pixel will use 1 byte.
    //
    uiLen = uiWidth * uiHeight;

    //
    // Start decoding and blitting the bitmap.
    //
    lpBitmapRLE += 4;
    for (i = 0; i < uiLen;)
    {
        T = *lpBitmapRLE++;
        if ((T & 0x80) && T <= 0x80 + uiWidth)
        {
            i += T - 0x80;
            uiSrcX += T - 0x80;
            if (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
        else
        {
            //
            // Prepare coordinates.
            //
            j = 0;
            sx = uiSrcX;
            x = dx + uiSrcX;
            y = dy;

            //
            // Skip the points which are out of the surface.
            //
            if (y < 0)
            {
                j += -y * uiWidth;
                y = 0;
            }
            else if (y >= lpDstSurface->h)
            {
                goto end; // No more pixels needed, break out
            }

            while (j < T)
            {
                //
                // Skip the points which are out of the surface.
                //
                if (x < 0)
                {
                    j += -x;
                    if (j >= T) break;
                    sx += -x;
                    x = 0;
                }
                else if (x >= lpDstSurface->w)
                {
                    j += uiWidth - sx;
                    x -= sx;
                    sx = 0;
                    y++;
                    if (y >= lpDstSurface->h)
                    {
                        goto end; // No more pixels needed, break out
                    }
                    continue;
                }

                //
                // Put the pixels in row onto the surface
                //
                k = T - j;
                if (lpDstSurface->w - x < k) 
                    k = lpDstSurface->w - x;
                if (uiWidth - sx < k) 
                    k = uiWidth - sx;
                sx += k;
                p = ((LPBYTE)lpDstSurface->pixels) + y * lpDstSurface->pitch;
                if (bShadow)
                {
                    j += k;
                    for (; k != 0; k--)
                    {
                        p[x] = PAL_CalcShadowColor(p[x]);
                        /////
                        if (p[x] == 0)p[x] = 1;
                        x++;
                    }
                }
                else
                {
                    for (; k != 0; k--)
                    {
                        p[x] = lpBitmapRLE[j];
                        /////
                        if (p[x] == 0)p[x] = 1;
                        j++;
                        x++;
                    }
                }

                if (sx >= uiWidth)
                {
                    sx -= uiWidth;
                    x -= uiWidth;
                    y++;
                    if (y >= lpDstSurface->h)
                    {
                        goto end; // No more pixels needed, break out
                    }
                }
            }
            lpBitmapRLE += T;
            i += T;
            uiSrcX += T;
            while (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
    }

end:
    //
    // Success
    //
    return 0;
}


INT
CPalUI::PAL_RLEBlitWithColorShift(
    LPCBITMAPRLE      lpBitmapRLE,
    CGL_Texture     * lpDstText,
    PAL_POS           pos,
    INT               iColorShift,
    BOOL              Ratio  
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
#if 0

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

    if (PAL_RLEBlitWithColorShift(lpBitmapRLE, mSurf, 0,iColorShift) == -1)
    {
        SDL_FreeSurface(mSurf);
        return -1;
    }

    SDL_Rect dstRect = { PAL_X(pos), PAL_Y(pos), uiWidth,uiHeight };
    ///
    if (Ratio)
        setPictureRatio(&dstRect);
    SDL_Color* color = &mSurf->format->palette->colors[0];
    gpGL->RenderBlendCopy(lpDstText, mSurf, 255, 3, color, &dstRect);
    SDL_FreeSurface(mSurf);
    return 0;
#else
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

    UINT dx{0}, dy{0};
    INT x{ 0 }, y{ 0 };
    INT uiSrcX{ 0 };

    //
    // Calculate the total length of the bitmap.
    // The bitmap is 8-bpp, each pixel will use 1 byte.
    //
    UINT uiLen = uiWidth * uiHeight;

    vector<UINT>  tBuf(uiLen);

    //
    // Start decoding and blitting the bitmap.
    //
    lpBitmapRLE += 4;
    for (int i = 0; i < uiLen;)
    {
        auto T = *lpBitmapRLE++;
        if ((T & 0x80) && T <= 0x80 + uiWidth)
        {
            i += T - 0x80;
            uiSrcX += T - 0x80;
            if (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
        else
        {
            //
            // Prepare coordinates.
            //
            int j = 0;
            auto sx = uiSrcX;
            x = dx + uiSrcX;
            y = dy;

            //
            // Skip the points which are out of the surface.
            //
            if (y < 0)
            {
                j += -y * uiWidth;
                y = 0;
            }
            else if (y >= uiHeight)
            {
                goto end; // No more pixels needed, break out
            }

            while (j < T)
            {
                //
                // Skip the points which are out of the surface.
                //
                if (x < 0)
                {
                    j += -x;
                    if (j >= T) break;
                    sx += -x;
                    x = 0;
                }
                else if (x >= uiWidth)
                {
                    j += uiWidth - sx;
                    x -= sx;
                    sx = 0;
                    y++;
                    if (y >= uiHeight)
                    {
                        goto end; // No more pixels needed, break out
                    }
                    continue;
                }

                //
                // Put the pixels in row onto the surface
                //
                int k = T - j;
                if (uiWidth - x < k) k = uiWidth - x;
                if (uiWidth - sx < k) k = uiWidth - sx;
                sx += k;

                UINT* p = &tBuf[y * uiWidth];
                for (; k != 0; k--)
                {
                    BYTE b = (lpBitmapRLE[j] & 0x0F);
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

                    p[x] = *((UINT*)&gpPalette->colors[(b | (lpBitmapRLE[j] & 0xF0))]);
                    ///////.......由于0 是透明色采用1替代
                    if (!p[x])
                        p[x] = 1;
                    j++;
                    x++;
                }

                if (sx >= uiWidth)
                {
                    sx -= uiWidth;
                    x -= uiWidth;
                    y++;
                    if (y >= uiHeight)
                    {
                        goto end; // No more pixels needed, break out
                    }
                }
            }
            lpBitmapRLE += T;
            i += T;
            uiSrcX += T;
            while (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
    }

end:
    //
    // Success
    //tBuf存儲畫面數據
    CGL_Texture* bText = new CGL_Texture(uiWidth, uiHeight, 32);
    gpGL->UpdateTexture(bText, NULL, &tBuf[0], uiWidth * 4);

    SDL_Rect rect{  };
    rect.x = PAL_X(pos);
    rect.y = PAL_Y(pos);
    rect.w = uiWidth;
    rect.h = uiHeight;

    if (Ratio)
        setPictureRatio(&rect);

    gpGL->RenderBlendCopy(lpDstText, bText, 255, 3, 0, &rect);

    delete bText;

    return 0;

#endif //0
}

INT
CPalUI::PAL_RLEBlitWithColorShift(
    LPCBITMAPRLE      lpBitmapRLE,
    SDL_Surface* lpDstSurface,
    PAL_POS           pos,
    INT               iColorShift,
    BOOL
)
/*++
  Purpose:

    Blit an RLE-compressed bitmap to an SDL surface.
    NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

    [IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

    [IN]  pos - position of the destination area.

    [IN]  iColorShift - shift the color by this value.

  Return value:

    0 = success, -1 = error.

--*/
{
    UINT          i, j, k, sx;
    INT           x, y;
    UINT          uiLen = 0;
    UINT          uiWidth = 0;
    UINT          uiHeight = 0;
    UINT          uiSrcX = 0;
    BYTE          T, b;
    INT           dx = PAL_X(pos);
    INT           dy = PAL_Y(pos);
    LPBYTE        p;

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
    // Check whether bitmap intersects the surface.
    //
    if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
        uiHeight + dy <= 0 || dy >= lpDstSurface->h)
    {
        goto end;
    }

    //
    // Calculate the total length of the bitmap.
    // The bitmap is 8-bpp, each pixel will use 1 byte.
    //
    uiLen = uiWidth * uiHeight;

    //
    // Start decoding and blitting the bitmap.
    //
    lpBitmapRLE += 4;
    for (i = 0; i < uiLen;)
    {
        T = *lpBitmapRLE++;
        if ((T & 0x80) && T <= 0x80 + uiWidth)
        {
            i += T - 0x80;
            uiSrcX += T - 0x80;
            if (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
        else
        {
            //
            // Prepare coordinates.
            //
            j = 0;
            sx = uiSrcX;
            x = dx + uiSrcX;
            y = dy;

            //
            // Skip the points which are out of the surface.
            //
            if (y < 0)
            {
                j += -y * uiWidth;
                y = 0;
            }
            else if (y >= lpDstSurface->h)
            {
                goto end; // No more pixels needed, break out
            }

            while (j < T)
            {
                //
                // Skip the points which are out of the surface.
                //
                if (x < 0)
                {
                    j += -x;
                    if (j >= T) break;
                    sx += -x;
                    x = 0;
                }
                else if (x >= lpDstSurface->w)
                {
                    j += uiWidth - sx;
                    x -= sx;
                    sx = 0;
                    y++;
                    if (y >= lpDstSurface->h)
                    {
                        goto end; // No more pixels needed, break out
                    }
                    continue;
                }

                //
                // Put the pixels in row onto the surface
                //
                k = T - j;
                if (lpDstSurface->w - x < k) k = lpDstSurface->w - x;
                if (uiWidth - sx < k) k = uiWidth - sx;
                sx += k;
                p = ((LPBYTE)lpDstSurface->pixels) + y * lpDstSurface->pitch;
                for (; k != 0; k--)
                {
                    b = (lpBitmapRLE[j] & 0x0F);
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

                    p[x] = (b | (lpBitmapRLE[j] & 0xF0));
                    ///////.......由于0 是透明色采用1替代
                    if (!p[x])p[x] = 1;
                    j++;
                    x++;
                }

                if (sx >= uiWidth)
                {
                    sx -= uiWidth;
                    x -= uiWidth;
                    y++;
                    if (y >= lpDstSurface->h)
                    {
                        goto end; // No more pixels needed, break out
                    }
                }
            }
            lpBitmapRLE += T;
            i += T;
            uiSrcX += T;
            while (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
    }

end:
    //
    // Success
    //
    return 0;
}






INT
CPalUI::PAL_FBPBlitToSurface(
    LPBYTE            lpBitmapFBP,
    SDL_Surface* lpDstSurface
)
/*++
  Purpose:

    Blit an uncompressed bitmap in FBP.MKF to an SDL surface.
    NOTE: Assume the surface is already locked, and the surface is a 8-bit 320x200 one.

  Parameters:

    [IN]  lpBitmapFBP - pointer to the RLE-compressed bitmap to be decoded.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    0 = success, -1 = error.

--*/
{
    int       x, y;
    //LPBYTE    p;

    if (lpBitmapFBP == NULL || lpDstSurface == NULL ||
        lpDstSurface->w != 320 || lpDstSurface->h != 200)
    {
        return -1;
    }

    //
    // simply copy everything to the surface
    //
    for (y = 0; y < 200; y++)
        if (lpDstSurface->format->BitsPerPixel == 8)
        {
            LPBYTE p = (LPBYTE)(lpDstSurface->pixels) + y * lpDstSurface->pitch;
            for (x = 0; x < 320; x++)
            {
                *(p++) = *(lpBitmapFBP++);
            }
        }
        else
        {
            DWORD32 *p = (DWORD32*)(lpDstSurface->pixels) + y * lpDstSurface->pitch / 4;
            for (x = 0; x < 320; x++)
            {
                *(p++) = *((DWORD32*)(&gpPalette->colors[*(lpBitmapFBP++)]));
            }
        }

    return 0;
}


INT
CPalUI::PAL_FBPBlitToSurface(
    LPBYTE            lpBitmapFBP,
    CGL_Texture* lpDstText
)
{
    if (!lpBitmapFBP || !lpDstText)
        return -1;
    SDL_Surface* mSurf = SDL_CreateRGBSurface(0, 320, 200, 8, 0, 0, 0, 0);
    if (!mSurf)
        return -1;
    SDL_SetSurfacePalette(mSurf, gpPalette);

    if (PAL_FBPBlitToSurface(lpBitmapFBP, mSurf) == -1)
    {
        SDL_FreeSurface(mSurf);
        return -1;
    }

    gpGL->RenderBlendCopy(lpDstText, mSurf);

    SDL_FreeSurface(mSurf);
    return 0;
}



LPCBITMAPRLE
CPalUI::PAL_SpriteGetFrame(
    LPCSPRITE       lpSprite,
    INT             iFrameNum
)
/*++
  Purpose:

    Get the pointer to the specified frame from a sprite.

  Parameters:

    [IN]  lpSprite - pointer to the sprite.

    [IN]  iFrameNum - number of the frame.

  Return value:

    Pointer to the specified frame. NULL if the frame does not exist.

--*/
{
    int imagecount, offset;

    if (lpSprite == NULL)
    {
        return NULL;
    }
    //PAL_LoadGame
    //
    // Hack for broken sprites like the Bloody-Mouth Bug
    //
 //   imagecount = (lpSprite[0] | (lpSprite[1] << 8)) - 1;
    imagecount = (lpSprite[0] | (lpSprite[1] << 8));

    if (iFrameNum < 0 || iFrameNum >= imagecount)
    {
        //
        // The frame does not exist
        //
        return NULL;
    }

    //
    // Get the offset of the frame
    //
    iFrameNum <<= 1;
    offset = ((lpSprite[iFrameNum] | (lpSprite[iFrameNum + 1] << 8)) << 1);
    if (offset == 0x18444) 
        offset = (WORD)offset;
    return &lpSprite[offset];
}

UINT CPalUI::PAL_RLEGetWidth(LPCBITMAPRLE lpBitmapRLE)
/*++
Purpose:

Get the width of an RLE-compressed bitmap.

Parameters:

[IN]  lpBitmapRLE - pointer to an RLE-compressed bitmap.

Return value:

Integer value which indicates the height of the bitmap.

--*/
{
    if (lpBitmapRLE == NULL)
    {
        return 0;
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
    // Return the width of the bitmap.
    //
    return lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
}

UINT CPalUI::PAL_RLEGetHeight(LPCBITMAPRLE lpBitmapRLE)
/*++
Purpose:

Get the height of an RLE-compressed bitmap.

Parameters:

[IN]  lpBitmapRLE - pointer of an RLE-compressed bitmap.

Return value:

Integer value which indicates the height of the bitmap.

--*/
{
    if (lpBitmapRLE == NULL)
    {
        return 0;
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
    // Return the height of the bitmap.
    //
    return lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);
}

