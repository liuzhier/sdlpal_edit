#ifndef CSDLSCREEN_H
#define CSDLSCREEN_H

#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0

#include "command.h"
//#include "cgl_texture.h"
#include "csound.h"
#include <vector>
#include<functional>

//using namespace std;

#define  MastA 0xFF000000
#define  MastB 0x00FF0000
#define  MastG 0x0000FF00
#define  MastR 0x000000FF
#define MENUITEM_VALUE_CANCELLED      0xFFFF


typedef enum tagNUMALIGN
{
    kNumAlignLeft,
    kNumAlignMid,
    kNumAlignRight
} NUMALIGN;

typedef enum tagNUMCOLOR
{
    kNumColorYellow,
    kNumColorBlue,
    kNumColorCyan
} NUMCOLOR;

typedef class GL_Render GL_Render;

typedef std::function< int(UINT8 srcVal, int w, int h, VOID* dstData)> SDATA;

//using SDATA = [](UINT8 srcVal, int x, int y, void* dstData)->int;
//本类用于实现屏幕操作各种相关函数
class CSdlScreen :public CSound
{
public:
    CSdlScreen();
    ~CSdlScreen();

public:
    //设置屏幕摇动参数，次数和水平
    void VIDEO_ShakeScreen(WORD wShakeTime, WORD wShakeLevel);
    //从备份缓存区过渡到屏幕缓存区，输入参数 速度，独占时间，之中屏幕可以抖动
    void VIDEO_FadeScreen(WORD speed);
    SDL_Surface* VIDEO_CreateCompatibleSizedSurface(SDL_Surface* pSource, const SDL_Rect* pSize);
    SDL_Surface* VIDEO_CreateCompatibleSurface(SDL_Surface* pSource); 
    INT PAL_RNGBlitTo(const uint8_t* rng, int len, VOID * data, SDATA ddata);
    INT PAL_RNGBlitToSurface(const uint8_t* rng, int length, SDL_Surface* lpDstSurface);
    VOID VideoInit();
    VOID VideoShutDown();
    VOID VIDEO_BackupScreen(SDL_Surface* s);
    VOID VIDEO_BackupScreen(CGL_Texture* s = NULL);
    VOID VIDEO_RestoreScreen();
    VOID VIDEO_UpdateScreen(CGL_Texture* dstText, const SDL_Rect* lpSrcRect = NULL, const SDL_Rect* lpDstRect = NULL);
    //VOID VIDEO_UpdateScreen(const SDL_Rect* lpRect , SDL_Surface* s );
    VOID VIDEO_UpdateSurfacePalette(SDL_Surface* pSurface);
    SDL_Color* VIDEO_GetPalette(VOID);
    VOID VIDEO_SwitchScreen(WORD wSpeed);
    VOID PAL_ColorFade(INT iDelay, BYTE bColor, BOOL fFrom);
    VOID PAL_FadeToRed(VOID);
    VOID PAL_FadeIn(INT iPaletteNum, BOOL fNight, INT iDelay);
    VOID PAL_FadeOut(INT iDelay);
    VOID PAL_FadeOut(CGL_Texture * dstText, INT iDelay);
    VOID PAL_SetPalette(INT iPaletteNum, BOOL fNight);
    VOID PAL_DrawText(LPCSTR lpszText, PAL_POS  pos, BYTE  bColor, BOOL fShadow, BOOL fUpdate, int size = 16);
    SIZE PAL_DrawWideText(LPCWSTR lpszTextR, PAL_POS pos,BYTE bColor, 
        BOOL fShadow, BOOL fUpdate, int size = 15);
    VOID PAL_DrawTextUTF8(LPCSTR lpszText, PAL_POS pos, BYTE bColor, BOOL fShadow, BOOL fUpdate, int size);
    VOID VIDEO_SetPalette(SDL_Color rgPalette[256]);
    SDL_Color* PAL_GetPalette(INT iPaletteNum, BOOL fNight);
    VOID PAL_DrawNumber(UINT iNum, UINT nLength, PAL_POS pos, NUMCOLOR color, NUMALIGN align, int size = 8);
    
    VOID RenderBlendCopy(CGL_Texture* rpRender, CGL_Texture* rpText1,
        const WORD rAlpha = 255, const WORD mode = 1, const SDL_Color* rColor = NULL,
        const SDL_Rect* dstRect = NULL, const SDL_Rect* srcRect = NULL);

    VOID RenderBlendCopy(CGL_Texture* rpRender, SDL_Surface* rpSurf,
        const WORD rAlpha = 255, const WORD mode = 1, const SDL_Color* rColor = NULL,
        const SDL_Rect* dstRect = NULL, const SDL_Rect* srcRect = NULL);
    VOID RenderPresent(CGL_Texture* glRender, INT dAlpha = 255);

    inline VOID setAlpha(const WORD alpha) { g_Alpha = alpha; }
    inline WORD getAlpha(VOID) { return g_Alpha; }
    inline VOID setColor(const SDL_Color color){g_Color = color;}
    inline VOID setColor(const int r,const int g,const int b) { g_Color.r = r; g_Color.g = g; g_Color.b = b; }
    inline const SDL_Color getColor(VOID) { return g_Color; };
    inline const SDL_Color* getpColor(VOID) { return &g_Color; };
    VOID ClearScreen(const SDL_Rect* sRect = NULL);

 
private:
    WORD g_wShakeTime{0};
    WORD g_wShakeLevel{0};
    SDL_Color g_Color = {255,255,255,255};
    WORD   g_Alpha{255};//
public:
    CGL_Texture* gpTextureReal{NULL};//显示纹理
    CGL_Texture* gpTextureRealBak{NULL};//显示纹理备份
    SDL_Window* gpWindow{NULL};
    CGL_Texture* gpRenderTexture{NULL};

};

#endif // CSDLSCREEN_H
