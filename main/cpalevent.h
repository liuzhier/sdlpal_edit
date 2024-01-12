#pragma once
#ifndef CEVENT_H
#define CEVENT_H
#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0

#include "../s/stdafx.h"

#include "command.h"
#include <string>
#include "cconfig.h"

using namespace std;

typedef struct tagTEXTLIB
{
	LPBYTE          lpWordBuf;
	LPBYTE          lpMsgBuf;
	LPDWORD         lpMsgOffset;

	int             nWords;
	int             nMsgs;

	int             nCurrentDialogLine;
	WORD            bCurrentFontColor;
	PAL_POS         posIcon;
	PAL_POS         posDialogTitle;
	PAL_POS         posDialogText;
	BYTE            bDialogPosition;
	BYTE            bIcon;
	int             iDelayTime;
	BOOL            fUserSkip;
	BOOL            fPlayingRNG;

	BYTE            bufDialogIcons[282];
} TEXTLIB, * LPTEXTLIB;

enum PALKEY
{
	kKeyMenu = (1 << 0),
	kKeySearch = (1 << 1),
	kKeyDown = (1 << 2),
	kKeyLeft = (1 << 3),
	kKeyUp = (1 << 4),
	kKeyRight = (1 << 5),
	kKeyPgUp = (1 << 6),
	kKeyPgDn = (1 << 7),
	kKeyRepeat = (1 << 8),
	kKeyAuto = (1 << 9),
	kKeyDefend = (1 << 10),
	kKeyUseItem = (1 << 11),
	kKeyThrowItem = (1 << 12),
	kKeyFlee = (1 << 13),
	kKeyStatus = (1 << 14),
	kKeyForce = (1 << 15),

//#ifdef  SHOW_DATA_IN_BATTLE
	kKeyGetInfo = (1 << 16),
//#endif

#ifdef  SHOW_ENEMY_STATUS
	kKeyEnemyInfo = (1 << 17),
#endif // SHOW_ENEMY_STATUS

#ifdef  New_Level_Info
	kKeyLevelInfo = (1 << 18),
#endif // New_Level_Info

#ifdef  Show_New_Hiden_Exp
	kKeyExpInfo = (1 << 19),
#endif
#ifdef Show_Timer
	kKeyShowTimer = (1 << 20),
#endif
};

typedef enum tagPALDIRECTION
{
	kDirSouth = 0,
	kDirWest,
	kDirNorth,
	kDirEast,
	kDirUnknown
} PALDIRECTION, * LPPALDIRECTION;

typedef struct tagPALINPUTSTATE
{
	PALDIRECTION           dir, prevdir;
	DWORD                  dwKeyPress;
} PALINPUTSTATE;

typedef unsigned char Uint8;

typedef class GL_Render GL_Render;



//进行全局事件信号设置、被其他类调用，可重入，任何类继承此类均可，
class CPalEvent
{
public:
	CPalEvent();

	~CPalEvent();

private:
	int  SDLCALL PAL_EventFilter(const union SDL_Event* lpEvent);
	VOID PAL_KeyboardEventFilter(const union SDL_Event* lpEvent);
	int PAL_PollEvent(union SDL_Event* event);

public:
	//全局保留唯一一份数据
	BOOL		fInBattle{};           // TRUE if in battle
	volatile PALINPUTSTATE   g_InputState{};
	BOOL	g_switch_WINDOW_FULLSCREEN_DESKTOP{};
	class CConfig* gConfig{};
	class GL_Render* gpGL{};
	class CScript* pCScript{};

	SDL_Palette* gpPalette{};
	struct tagGLOBALVARS* gpGlobals{};
	BOOL		 PalSoundIn{};
	CWnd*        PalWnd{ nullptr };
	TEXTLIB		 g_TextLib{};
	LPSPRITE     gpSpriteUI{};

	static std::string  BasePath;
	static std::string  PalDir;
	static BOOL         PalQuit;

public:
	VOID PAL_ClearKeyState(VOID);
	unsigned long getKeyPress(VOID);
	BOOL get_switch_WINDOW_FULLSCREEN(VOID)
	{
		return  g_switch_WINDOW_FULLSCREEN_DESKTOP;
	}
	VOID set_switch_WINDOW_FULLSCREEN_DESKTOP(int flag);
	VOID PAL_Delay(UINT32 t);
	VOID PAL_ProcessEvent(VOID);
	void* UTIL_calloc(size_t n, size_t size);
public:
	char* va(const char* format, ...);
	VOID PAL_WaitForKey(WORD wTimeOut);
};

#endif // CEVENT_H
