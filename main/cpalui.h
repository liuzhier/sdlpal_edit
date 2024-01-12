#ifndef CPALUI_H
#define CPALUI_H
#include "cpalapp.h"
#include "cpalevent.h"
#include "cgl_render.h"
//#include <SDL.h>

//本类继承基本OI 操作的 CPalApp 实现与SDL相关操作
class CPalUI : public CPalApp
{
public:
    CPalUI();
    INT PAL_RLEBlitToTexture(LPCBITMAPRLE lpBitmapRLE, CGL_Texture* lpDstText, 
        PAL_POS pos,BOOL bShadow = FALSE, BOOL d = TRUE, const SDL_Color* pcolors = nullptr);
    INT PAL_RLEBlitToSurface(LPCBITMAPRLE lpBitmapRLE, SDL_Surface* lpDstSurface, PAL_POS pos, BOOL bShadow = FALSE, BOOL d = TRUE);
    INT PAL_RLEBlitWithColorShift(LPCBITMAPRLE lpBitmapRLE, CGL_Texture* lpDstSurface, PAL_POS pos, INT iColorShift,BOOL d = TRUE);
    INT PAL_RLEBlitWithColorShift(LPCBITMAPRLE lpBitmapRLE, SDL_Surface* lpDstSurface, PAL_POS pos, INT iColorShift, BOOL d = TRUE);
	INT PAL_FBPBlitToSurface(LPBYTE lpBitmapFBP, SDL_Surface* lpDstSurface);
    INT PAL_FBPBlitToSurface(LPBYTE lpBitmapFBP, CGL_Texture* lpDstText);
    LPCBITMAPRLE PAL_SpriteGetFrame(LPCSPRITE lpSprite, INT iFrameNum);

public:
	UINT PAL_RLEGetWidth(LPCBITMAPRLE    lpBitmapRLE);
    UINT PAL_RLEGetHeight(LPCBITMAPRLE  lpBitmapRLE);
};

#endif // CPALUI_H
