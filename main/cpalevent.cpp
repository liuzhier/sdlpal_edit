#include "cpalevent.h"
#include "CConfig.h"
#include <SDL2\SDL.h>
#include "palgpgl.h"
#include "cscript.h"

std::string		 CPalEvent::BasePath;
std::string		 CPalEvent::PalDir;
INT				 CPalEvent::PalQuit;

CPalEvent::CPalEvent()
{
}

CPalEvent::~CPalEvent()
{
	if (gConfig)
	{
		delete gConfig;
		gConfig = nullptr;
	}
}

int CPalEvent::PAL_PollEvent(
	SDL_Event* event
)
/*++
  Purpose:

	Poll and process one event.

  Parameters:

	[OUT] event - Events polled from SDL.

  Return value:

	Return value of PAL_PollEvent.

--*/
{
	SDL_Event evt;

	int ret = SDL_PollEvent(&evt);
	if (ret != 0)
	{
		PAL_EventFilter(&evt);
	}

	if (event != NULL)
	{
		*event = evt;
	}

	return ret;
}

int SDLCALL CPalEvent::PAL_EventFilter(const SDL_Event* lpEvent)
{

	/*++
  Purpose:

	SDL event filter function. A filter to process all events.

  Parameters:

	[IN]  lpEvent - pointer to the event.

  Return value:

	1 = the event will be added to the internal queue.
	0 = the event will be dropped from the queue.

--*/
	{
		switch (lpEvent->type)
		{
		case SDL_WINDOWEVENT:
			if (lpEvent->window.event == SDL_WINDOWEVENT_RESIZED)
			{
				//
				// resized the window
				//
				//VIDE_Resize(lpEvent->window.data1, lpEvent->window.data2);
			}
			break;
		case SDL_QUIT:
			//
			// clicked on the close button of the window. Quit immediately.
			//
			if (pCScript->PAL_ConfirmMenu("退出吗？"))
			{
				pCScript->PAL_PlayMUS(0, FALSE, 2);
				pCScript->PAL_FadeOut(2);
				PalQuit = TRUE;
			}
		}

		PAL_KeyboardEventFilter(lpEvent);
		//PAL_MouseEventFilter(lpEvent);
		//PAL_JoystickEventFilter(lpEvent);
		//PAL_TouchEventFilter(lpEvent);

		//
		// All events are handled here; don't put anything to the internal queue
		//
		return 0;
	}
	return 0;
}

VOID CPalEvent::PAL_KeyboardEventFilter(
	const SDL_Event* lpEvent
)
/*++
  Purpose:

	Handle keyboard events.

  Parameters:

	[IN]  lpEvent - pointer to the event.

  Return value:

	None.

--*/
{
	switch (lpEvent->type)
	{

	case SDL_KEYDOWN:
		//
		// Pressed a key
		//
		if (lpEvent->key.keysym.mod & KMOD_ALT)
		{
			if (lpEvent->key.keysym.sym == SDLK_RETURN || lpEvent->key.keysym.sym == SDLK_KP_ENTER)
			{
				//
				// Pressed Alt+Enter (toggle fullscreen)...
				//
				g_switch_WINDOW_FULLSCREEN_DESKTOP ^= 1;
				return;
			}
			else if (lpEvent->key.keysym.sym == SDLK_F4)
			{
				//
				// Pressed Alt+F4 (Exit program)...
				//
				if (pCScript->PAL_ConfirmMenu("退出吗？"))
				{
					pCScript->PAL_PlayMUS(0, FALSE, 2);
					pCScript->PAL_FadeOut(2);
					PalQuit = TRUE;
				}
			}
		}

		switch (lpEvent->key.keysym.sym)
		{


//#ifdef SHOW_DATA_IN_BATTLE
		case SDLK_t:
			//t 
			g_InputState.dwKeyPress |= kKeyGetInfo;
			break;
//#endif //
#ifdef SHOW_ENEMY_STATUS
			//kKeyEnemyInfo
		case SDLK_g:
			//g 
			g_InputState.dwKeyPress |= kKeyEnemyInfo;
			break;
#endif //

		case SDLK_UP:
		case SDLK_KP8:
		case SDLK_j:
		case SDLK_h:
			if (fInBattle || g_InputState.dir != kDirNorth)
			{
				g_InputState.prevdir = (fInBattle ? kDirUnknown : g_InputState.dir);
				g_InputState.dir = kDirNorth;
			}
			g_InputState.dwKeyPress |= kKeyUp;
			break;

		case SDLK_DOWN:
		case SDLK_KP2:
		case SDLK_n:
			if (fInBattle || g_InputState.dir != kDirSouth)
			{
				g_InputState.prevdir = (fInBattle ? kDirUnknown : g_InputState.dir);
				g_InputState.dir = kDirSouth;
			}
			g_InputState.dwKeyPress |= kKeyDown;
			break;

		case SDLK_LEFT:
		case SDLK_KP4:
		case SDLK_b:
			if (fInBattle || g_InputState.dir != kDirWest)
			{
				g_InputState.prevdir = (fInBattle ? kDirUnknown : g_InputState.dir);
				g_InputState.dir = kDirWest;
			}
			g_InputState.dwKeyPress |= kKeyLeft;
			break;

		case SDLK_RIGHT:
		case SDLK_KP6:
		case SDLK_m:
			if (fInBattle || g_InputState.dir != kDirEast)
			{
				g_InputState.prevdir = (fInBattle ? kDirUnknown : g_InputState.dir);
				g_InputState.dir = kDirEast;
			}
			g_InputState.dwKeyPress |= kKeyRight;
			break;

		case SDLK_ESCAPE:
		//case SDLK_INSERT:
		//case SDLK_LALT:
		//case SDLK_RALT:
		case SDLK_KP0:
			g_InputState.dwKeyPress |= kKeyMenu;
			break;

		case SDLK_RETURN:
		case SDLK_SPACE:
		case SDLK_KP_ENTER:
		case SDLK_LCTRL:
			g_InputState.dwKeyPress |= kKeySearch;
			break;

		case SDLK_PAGEUP:
		case SDLK_KP9:
			g_InputState.dwKeyPress |= kKeyPgUp;
			break;

		case SDLK_PAGEDOWN:
		case SDLK_KP3:
			g_InputState.dwKeyPress |= kKeyPgDn;
			break;

		case SDLK_7: //7 for mobile device
		case SDLK_r:
			g_InputState.dwKeyPress |= kKeyRepeat;
			break;

		case SDLK_2: //2 for mobile device
		case SDLK_a:
			g_InputState.dwKeyPress |= kKeyAuto;
			break;

		case SDLK_d:
			g_InputState.dwKeyPress |= kKeyDefend;
			break;

		case SDLK_e:
			g_InputState.dwKeyPress |= kKeyUseItem;
			break;

		case SDLK_w:
			g_InputState.dwKeyPress |= kKeyThrowItem;
			break;

		case SDLK_q:
			g_InputState.dwKeyPress |= kKeyFlee;
			break;

		case SDLK_s:
			g_InputState.dwKeyPress |= kKeyStatus;
			break;

		case SDLK_f:
		case SDLK_5: // 5 for mobile device
			g_InputState.dwKeyPress |= kKeyForce;
			break;

		case SDLK_HASH: //# for mobile device
		case SDLK_p:
			//VIDEO_SaveScreenshot();
			break;

		default:
			break;
		}
		break;

	case SDL_KEYUP:
		//
		// Released a key
		//
		switch (lpEvent->key.keysym.sym)
		{
		case SDLK_UP:
		case SDLK_KP8:
		case SDLK_j:
		case SDLK_h:
			if (g_InputState.dir == kDirNorth)
			{
				g_InputState.dir = g_InputState.prevdir;
			}
			g_InputState.prevdir = kDirUnknown;
			break;

		case SDLK_DOWN:
		case SDLK_KP2:
		case SDLK_n:
			if (g_InputState.dir == kDirSouth)
			{
				g_InputState.dir = g_InputState.prevdir;
			}
			g_InputState.prevdir = kDirUnknown;
			break;

		case SDLK_LEFT:
		case SDLK_KP4:
		case SDLK_b:
			if (g_InputState.dir == kDirWest)
			{
				g_InputState.dir = g_InputState.prevdir;
			}
			g_InputState.prevdir = kDirUnknown;
			break;

		case SDLK_RIGHT:
		case SDLK_KP6:
		case SDLK_m:
			if (g_InputState.dir == kDirEast)
			{
				g_InputState.dir = g_InputState.prevdir;
			}
			g_InputState.prevdir = kDirUnknown;
			break;

		default:
			break;
		}
		break;
	}
}

 VOID CPalEvent::PAL_ClearKeyState(VOID) { g_InputState.dwKeyPress = 0; }

 unsigned long CPalEvent::getKeyPress(VOID) { return g_InputState.dwKeyPress; }

 VOID CPalEvent::set_switch_WINDOW_FULLSCREEN_DESKTOP(int flag) { (g_switch_WINDOW_FULLSCREEN_DESKTOP = flag); }

 VOID CPalEvent::PAL_Delay(UINT32 t) {
	UINT32 timeout = SDL_GetTicks_New() + t;
	PAL_ProcessEvent();
	while (SDL_GetTicks_New() < timeout)
	{
		if (PalQuit)return;
		PAL_ProcessEvent(); SDL_Delay(1);
	}
}

VOID CPalEvent::PAL_ProcessEvent(VOID)
/*++
  Purpose:

	Process all events.

  Parameters:

	None.

  Return value:

	None.

--*/
{

	while (PAL_PollEvent(NULL));
}

//申请内存，参数为申请地址的单位元素长度,元素个数
void* CPalEvent::UTIL_calloc(size_t n, size_t size)
{
	// handy wrapper for operations we always forget, like checking calloc's returned pointer.

	void* buffer;

	// first off, check if buffer size is valid
	if (n == 0 || size == 0)
		TerminateOnError("UTIL_calloc() called Width invalid parameters\n");

	buffer = calloc(n, size); // allocate real memory space

							  // last check, check if malloc call succeeded
	if (buffer == NULL)
		TerminateOnError("UTIL_calloc() failure for %d bytes (out of memory?)\n", size * n);

	SDL_memset(buffer, 0, size * n);

	return buffer; // nothing went wrong, so return buffer pointer
}


char* CPalEvent::va(const char* format, ...)
/*++
  Purpose:

	Does a varargs printf into a temp buffer, so we don't need to have
	varargs versions of all text functions.

  Parameters:

	format - the format string.

  Return value:

	Pointer to the result string.

--*/
{
	static char string[256];
	va_list     argptr;

	va_start(argptr, format);
	vsnprintf(string, 256, format, argptr);
	va_end(argptr);

	return string;
}

VOID CPalEvent::PAL_WaitForKey(WORD wTimeOut)
/*++
  Purpose:

  Wait for any key.

  Parameters:

  [IN]  wTimeOut - the maximum time of the waiting. 0 = wait forever.

  Return value:

  None.

  --*/
{
	DWORD     dwTimeOut = SDL_GetTicks_New() + wTimeOut;

	PAL_ClearKeyState();

	while (wTimeOut == 0 || SDL_GetTicks_New() < dwTimeOut)
	{
		if (PalQuit)return;

		PAL_Delay(5);

		if (getKeyPress() & (kKeySearch | kKeyMenu))
		{
			break;
		}
	}
}
