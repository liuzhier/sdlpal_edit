

#include "cpalmain.h"
#include <iostream>
#include "caviplay.h"
#include "cscript.h"

using namespace std;

#define BITMAPNUM_SPLASH_UP         (gConfig->fIsWIN95 ? 0x03 : 0x26)
#define BITMAPNUM_SPLASH_DOWN       (gConfig->fIsWIN95 ? 0x04 : 0x27)
#define SPRITENUM_SPLASH_TITLE      0x47
#define SPRITENUM_SPLASH_CRANE      0x49
#define NUM_RIX_TITLE               0x05

#define PAL_RunTriggerScript  pCScript->PAL_RunTriggerScript
#define PAL_RunAutoScript     pCScript->PAL_RunAutoScript


CPalMain::CPalMain()
{
    CAviPlay::setPAL(this);
}

CPalMain::~CPalMain()
{
    //if(gpGlobals)
        //PAL_FreeGlobals();
    //gpGlobals = nullptr;
    if (gpPalette)
        SDL_FreePalette(gpPalette);
    gpPalette = nullptr;
}

int CPalMain::PAL_TrademarkScreen()
//运行启动动画1
{
    if (gConfig->fEnableAviPlay)
    {
        CAviPlay m;
        if (m.PAL_PlayAVI("avi/1.avi")) return 0;
    }
    PAL_SetPalette(3, FALSE);
    PAL_RNGPlay(6, 0, 1000, 25);
    PAL_Delay(1);
    PAL_FadeOut(1);
    return 0;
}

int CPalMain::PAL_SplashScreen()
//运行启动动画2

/*++
  Purpose:

    Show the splash screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
//#ifdef PAL_HAS_AVI
    if(gConfig->m_Function_Set[22])
        if (gConfig->fEnableAviPlay)
        {
            CAviPlay m;
            m.PAL_PlayAVI("avi/3.avi");
            if (m.PAL_PlayAVI("avi/2.avi")) return 1;
        }
//#endif
    SDL_Color* palette = PAL_GetPalette(1, FALSE);
    SDL_Surface* lpBitmap;
    CGL_Texture* lpBitmapAll;
    SDL_Rect       srcrect, dstrect;
    LPSPRITE       lpSpriteCrane;
    LPBITMAPRLE    lpBitmapTitle;
    LPBYTE         buf, buf2;
    int            cranepos[9][3]{}, i, iImgPos = 200, iCraneFrame = 0, iTitleHeight;
    DWORD          dwTime, dwBeginTime;

    if (palette == NULL)
    {
        fprintf(stderr, "ERROR: PAL_SplashScreen(): palette == NULL\n");
        return 0;
    }

    //
    // Allocate all the needed memory at once for simplification
    //
    buf = (LPBYTE)UTIL_calloc(1, 320 * 200 * 2);
    buf2 = (LPBYTE)(buf + 320 * 200);
    lpSpriteCrane = (LPSPRITE)buf2 + 32000;

    //
    // Create the surfaces
    //
    lpBitmap = SDL_CreateRGBSurface(0, 320, 200, 8, 0, 0, 0, 0);
    //lpBitmap = SDL_CreateRGBSurface(0, 320, 200, 32, MastR, MastG, MastB, MastA);
    VIDEO_SetPalette(palette);
    VIDEO_UpdateSurfacePalette(lpBitmap);
    //lpBitmapAll = new CGL_Texture(320, 400, 8, gpPalette->colors);
    lpBitmapAll = new CGL_Texture(320, 400, 32);
    //
    // Read the bitmaps
    //
    //copy  Down
    PAL_MKFReadChunk(buf, 320 * 200, BITMAPNUM_SPLASH_DOWN, gpGlobals->f.fpFBP);
    (PAL_DeCompress)(buf, buf2, 320 * 200);
    PAL_FBPBlitToSurface(buf2, lpBitmap);
    dstrect = { 0,200,320,200 };
    RenderBlendCopy(lpBitmapAll, lpBitmap, 255, 1, 0, &dstrect);

    //copy Up
    PAL_MKFReadChunk(buf, 320 * 200, BITMAPNUM_SPLASH_UP, gpGlobals->f.fpFBP);
    (PAL_DeCompress)(buf, buf2, 320 * 200);
    PAL_FBPBlitToSurface(buf2, lpBitmap);
    dstrect = { 0,0,320,200 };
    RenderBlendCopy(lpBitmapAll, lpBitmap, 255, 1, 0, &dstrect);

    //read Title
    PAL_MKFReadChunk(buf, 32000, SPRITENUM_SPLASH_TITLE, gpGlobals->f.fpMGO);
    (PAL_DeCompress)(buf, buf2, 32000);
    //取第一个精灵
    lpBitmapTitle = (LPBITMAPRLE)PAL_SpriteGetFrame(buf2, 0);
    PAL_MKFReadChunk(buf, 32000, SPRITENUM_SPLASH_CRANE, gpGlobals->f.fpMGO);
    (PAL_DeCompress)(buf, lpSpriteCrane, 32000);
    iTitleHeight = PAL_RLEGetHeight(lpBitmapTitle);
    lpBitmapTitle[2] = 0;
    lpBitmapTitle[3] = 0; // HACKHACK

    //
    // Generate the positions of the cranes
    //
    for (i = 0; i < 9; i++)
    {
        cranepos[i][0] = RandomLong(300, 600);
        cranepos[i][1] = RandomLong(0, 80);
        cranepos[i][2] = RandomLong(0, 8);
    }

    //
    // Play the title music
    //

    PAL_PlayMUS(NUM_RIX_TITLE, TRUE, 2);

    //
    // Clear all of the events and key states
    //
    PAL_ProcessEvent();
    PAL_ClearKeyState();

    dwBeginTime = SDL_GetTicks_New();

    setAlpha(255);
    while(!PalQuit)
    {
        PAL_ProcessEvent();
        dwTime = SDL_GetTicks_New() - dwBeginTime;

        //
        // Set the palette
        //
        if (dwTime < 15000)
        {
            //屏幕逐渐变亮
            setAlpha((float)dwTime / 15000 * 255);
        }
        //
        // Draw the screen
        //
        if (iImgPos > 1)
        {
            iImgPos--;
        }

        //
        // The All part...
        //
        srcrect = { 0,iImgPos,320,200 };
        RenderBlendCopy(gpTextureReal, lpBitmapAll, 255, 1, 0, 0, &srcrect);
        //
        // Draw the cranes...
        //
        for (i = 0; i < 9; i++)
        {
            LPCBITMAPRLE lpFrame = PAL_SpriteGetFrame(lpSpriteCrane,
                cranepos[i][2] = (cranepos[i][2] + (iCraneFrame & 1)) % 8);
            cranepos[i][1] += ((iImgPos > 1) && (iImgPos & 1)) ? 1 : 0;
            PAL_RLEBlitToTexture(lpFrame, gpTextureReal,
                PAL_XY(cranepos[i][0], cranepos[i][1]));
            cranepos[i][0]--;
        }
        iCraneFrame++;

        //
        // Draw the title...
        //
        if (PAL_RLEGetHeight(lpBitmapTitle) < iTitleHeight)
        {
            //
            // HACKHACK
            //
            WORD* const pw = (WORD*)&lpBitmapTitle[2];
            (*pw)++;
        }

        PAL_RLEBlitToTexture(lpBitmapTitle, gpTextureReal, PAL_XY(255, 10));
        RenderPresent(gpTextureReal, getAlpha());

        //
        // Check for keypress...
        //
        PAL_ProcessEvent();
        if (getKeyPress())
        {
            if (getKeyPress() & (kKeyMenu | kKeySearch))
            {
                //
                // User has pressed a key...
                //
                lpBitmapTitle[2] = iTitleHeight & 0xFF;
                lpBitmapTitle[3] = iTitleHeight >> 8; // HACKHACK

                PAL_RLEBlitToTexture(lpBitmapTitle, gpTextureReal, PAL_XY(255, 10));
                //RenderPresent(gpTextureReal);

                if (dwTime < 15000)
                {
                    //
                    // If the picture has not completed fading in, complete the rest
                    //
                    while (dwTime < 15000)
                    {
                        RenderPresent(gpTextureReal, (float)dwTime / 15000 * 255);
                        PAL_Delay(10);
                        dwTime += 250;
                    }
                }

                //
                // Quit the splash screen
                //
                PAL_PlayMUS(1, FALSE, 1);

                for (int n = 0; n < 40; n++)
                {
                    RenderPresent(gpTextureReal);
                    PAL_Delay(50);
                }
                break;
            }
            PAL_ClearKeyState();
        }
        //
        // Delay a while...
        //
        PAL_ProcessEvent();
        while (SDL_GetTicks_New() - dwBeginTime < dwTime + 75)
        {
            PAL_Delay(1);
            PAL_ProcessEvent();
        }
    }

    SDL_FreeSurface(lpBitmap);
    ::free(buf);
    delete lpBitmapAll;
    PAL_FadeOut(gpTextureReal, 3);
    return 0;
}


void CPalMain::run()
{
    PAL_TrademarkScreen();
    PAL_SetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
    if (gpWindow)
        SDL_SetWindowTitle(gpWindow, "仙剑1  ");
    //显示动画
    if (PalQuit) return;
    //开场动画
    PAL_SplashScreen();
    //PalQuit = 1;
    if (PalQuit) return;
    PAL_ProcessEvent();
    g_InputState.prevdir = kDirUnknown;
    g_InputState.dir = kDirUnknown;

    g_fNoMusic = 0;
    g_fNoSound = 0;

    //主程序
    if (PalQuit)return;
    PAL_GameMain();

    PalQuit = 1;
}

VOID CPalMain::PAL_GameStart(
    VOID
)
/*++
  Purpose:

    Do some initialization work when game starts (new game or load game).

  Parameters:

    None.

  Return value:

    None.

--*/
{
    PAL_SetLoadFlags(kLoadScene | kLoadPlayerSprite);

    if (!gpGlobals->fEnteringScene)
    {
        //
        // Fade in music if the player has loaded an old game.
        //
        PAL_PlayMUS(gpGlobals->wNumMusic, TRUE, 1);
    }

    gpGlobals->fNeedToFadeIn = TRUE;
    gpGlobals->dwFrameNum = 0;
}

VOID CPalMain::PAL_GameMain(
    VOID
)
/*++
  Purpose:

    The game entry routine.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    DWORD       dwTime;
//#ifdef SHOW_DATA_IN_BATTLE
    if (gConfig->m_Function_Set[2])
        gpGlobals->fShowDataInBattle = FALSE;
//#endif
    //
    // Show the opening menu.
    //
    gpGlobals->bCurrentSaveSlot = (BYTE)PAL_OpeningMenu();

    //
    // Initialize game data and set the flags to load the game resources.
    //
    PAL_InitGameData(gpGlobals->bCurrentSaveSlot);

    //
    // Run the main game loop.
    //
    dwTime = SDL_GetTicks_New();

    while(!PalQuit)
    {

        //
        // Do some initialization at game start.
        //
        if (gpGlobals->fGameStart)
        {	//Do some initialization work when game starts (new game or load game).
            PAL_GameStart();
            gpGlobals->fGameStart = FALSE;
            setColor(255, 255, 255);
            /////
            SDL_Color color = { 10,10,10,255 };
            RenderBlendCopy(gpTextureReal, (CGL_Texture*)nullptr, 255, 2, &color);
            RenderPresent(gpTextureReal);
        }

        //
        // Load the game resources if needed.
        //
        PAL_LoadResources();

        //
        // Clear the input state of previous frame.
        //
        PAL_ClearKeyState();

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
        dwTime = SDL_GetTicks_New() + gConfig->m_Function_Set[24];

        //
        // Run the main frame routine.        
        //
        PAL_StartFrame();
    }

}

VOID CPalMain::PAL_StartFrame(
    VOID
)
/*++
  Purpose:

  Starts a video frame. Called once per video frame.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
    //
    // Run the game logic of one frame
    //
    PAL_GameUpdate(TRUE);
    if (gpGlobals->fEnteringScene)
    {
        return;
    }

    //
    // Update the positions and gestures of party members
    //
    PAL_UpdateParty();
    if (!gpGlobals->fNeedToFadeIn)
    {
        setColor(255, 255, 255);
        setAlpha(255);
    }
    //
    // Update the scene
    //
    ClearScreen();
    PAL_MakeScene();
//#ifdef Draw_Chase_Range
    if (gConfig->m_Function_Set[8])
    {
        if (gpGlobals->wChaseRange == 0)
        {
            PAL_DrawNumber(gpGlobals->wChasespeedChangeCycles, 4, PAL_XY(294, 0), kNumColorCyan, kNumAlignRight);
        }
        else if (gpGlobals->wChaseRange == 3)
            PAL_DrawNumber(gpGlobals->wChasespeedChangeCycles, 4, PAL_XY(294, 0), kNumColorBlue, kNumAlignRight);
    }
//#endif // Draw_Chase_Range
    VIDEO_UpdateScreen(gpTextureReal);

    if (g_InputState.dwKeyPress & kKeyMenu)
    {
        //
        // Show the in-game menu
        //
        PAL_InGameMenu();
    }
    else if (g_InputState.dwKeyPress & kKeyUseItem)
    {
        //
        // Show the use item menu
        //
        PAL_GameUseItem();
    }
    else if (g_InputState.dwKeyPress & kKeyThrowItem)
    {
        //
        // Show the equipment menu
        //
        PAL_GameEquipItem();
    }
    else if (g_InputState.dwKeyPress & kKeyForce)
    {
        //
        // Show the magic menu
        //
        PAL_InGameMagicMenu();
    }
    else if (g_InputState.dwKeyPress & kKeyStatus)
    {
        //
        // Show the player status
        //
        PAL_PlayerStatus();
    }
    else if (g_InputState.dwKeyPress & kKeySearch)
    {
        //
        // Process search events
        //
        PAL_Search();
    }
    else if (g_InputState.dwKeyPress & kKeyFlee)
    {
        //
        // Quit Game
        //
        if (PAL_ConfirmMenu("退出吗？"))
        {
            PAL_PlayMUS(0, FALSE, 2);
            PAL_FadeOut(2);
            //exit(0);
            PalQuit = TRUE;
        }
    }

    if (--gpGlobals->wChasespeedChangeCycles == 0)
    {
        gpGlobals->wChaseRange = 1;
    }
}

VOID CPalMain::PAL_GameUpdate(
    BOOL       fTrigger
)
/*++
  Purpose:

  The main game logic routine. Update the status of everything.

  Parameters:

  [IN]  fTrigger - whether to process trigger events or not.

  Return value:

  None.

  --*/
{
    WORD            wEventObjectID, wDir;
    int             i;
    LPEVENTOBJECT   p;

    //
    // Check for trigger events
    //
    if (fTrigger)
    {
        //
        // Check if we are entering a new scene
        //
        if (gpGlobals->fEnteringScene)
        {
            //
            // Run the script for entering the scene
            //
            gpGlobals->fEnteringScene = FALSE;

            i = gpGlobals->wNumScene - 1;
            gpGlobals->g.rgScene[i].wScriptOnEnter =
                PAL_RunTriggerScript(gpGlobals->g.rgScene[i].wScriptOnEnter, 0xFFFF);

            if (gpGlobals->fEnteringScene || gpGlobals->fGameStart)
            {
                //
                // Don't go further as we're switching to another scene
                //
                return;
            }

            PAL_ClearKeyState();
            PAL_MakeScene();
        }

        //
        // Update the vanish time for all event objects
        //
        for (wEventObjectID = 0; wEventObjectID < gpGlobals->g.nEventObject; wEventObjectID++)
        {
            p = &gpGlobals->g.lprgEventObject[wEventObjectID];

            if (p->sVanishTime != 0)
            {
                p->sVanishTime += ((p->sVanishTime < 0) ? 1 : -1);
            }
        }

        //
        // Loop through all event objects in the current scene
        //
        for (wEventObjectID = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex + 1;
            wEventObjectID <= gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex;
            wEventObjectID++)
        {
            p = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];

            if (p->sVanishTime != 0)
            {
                continue;
            }

            if (p->sState < 0)
            {
                if (p->x < PAL_X(gpGlobals->viewport) ||
                    p->x > PAL_X(gpGlobals->viewport) + 320 ||
                    p->y < PAL_Y(gpGlobals->viewport) ||
                    p->y > PAL_Y(gpGlobals->viewport) + 320)
                {
                    p->sState = abs(p->sState);
                    p->wCurrentFrameNum = 0;
                }
            }
            else if (p->sState > 0 && p->wTriggerMode >= kTriggerTouchNear)
            {
                //
                // This event object can be triggered without manually exploring
                //
                if (abs(PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - p->x) +
                    abs(PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - p->y) * 2 <
                    (p->wTriggerMode - kTriggerTouchNear) * 32 + 16)
                {
                    //
                    // Player is in the trigger zone.
                    //

                    if (p->nSpriteFrames)
                    {
                        //
                        // The sprite has multiple frames. Try to adjust the direction.
                        //
                        int                xOffset, yOffset;

                        p->wCurrentFrameNum = 0;

                        xOffset = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - p->x;
                        yOffset = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - p->y;

                        if (xOffset > 0)
                        {
                            p->wDirection = ((yOffset > 0) ? kDirEast : kDirNorth);
                        }
                        else
                        {
                            p->wDirection = ((yOffset > 0) ? kDirSouth : kDirWest);
                        }

                        //
                        // Redraw the scene
                        //
                        PAL_UpdatePartyGestures(FALSE);

                        PAL_MakeScene();
                        VIDEO_UpdateScreen(gpTextureReal);
                    }

                    //
                    // Execute the script.
                    //
                    p->wTriggerScript = PAL_RunTriggerScript(p->wTriggerScript, wEventObjectID);

                    PAL_ClearKeyState();

                    if (gpGlobals->fEnteringScene || gpGlobals->fGameStart)
                    {
                        //
                        // Don't go further on scene switching
                        //
                        return;
                    }
                }
            }
        }
    }

    //
    // Run autoscript for each event objects
    //
    for (wEventObjectID = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex + 1;
        wEventObjectID <= gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex;
        wEventObjectID++)
    {
        p = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];

        if (p->sState > 0 && p->sVanishTime == 0)
        {
            WORD wScriptEntry = p->wAutoScript;
            if (wScriptEntry != 0)
            {
                p->wAutoScript = PAL_RunAutoScript(wScriptEntry, wEventObjectID);
                if (gpGlobals->fEnteringScene || gpGlobals->fGameStart)
                {
                    //
                    // Don't go further on scene switching
                    //
                    return;
                }
            }
        }

        //
        // Check if the player is in the way
        //
        if (fTrigger && p->sState >= kObjStateBlocker && p->wSpriteNum != 0 &&
            abs(p->x - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset)) +
            abs(p->y - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset)) * 2 <= 12)
        {
            //
            // Player is in the way, try to move a step
            //
            wDir = (p->wDirection + 1) % 4;
            for (i = 0; i < 4; i++)
            {
                int              x, y;
                PAL_POS          pos;

                x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
                y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

                x += ((wDir == kDirWest || wDir == kDirSouth) ? -16 : 16);
                y += ((wDir == kDirWest || wDir == kDirNorth) ? -8 : 8);

                pos = PAL_XY(x, y);

                if (!PAL_CheckObstacle(pos, TRUE, 0))
                {
                    //
                    // move here
                    //
                    gpGlobals->viewport = PAL_XY(
                        PAL_X(pos) - PAL_X(gpGlobals->partyoffset),
                        PAL_Y(pos) - PAL_Y(gpGlobals->partyoffset));

                    break;
                }

                wDir = (wDir + 1) % 4;
            }
        }
    }

    gpGlobals->dwFrameNum++;
}


VOID CPalMain::PAL_PartyWalkTo(
    INT            x,
    INT            y,
    INT            h,
    INT            iSpeed
)
/*++
  Purpose:

  Make the party walk to the map position specified by (x, y, h)
  at the speed of iSpeed.

  Parameters:

  [IN]  x - Column number of the tile.

  [IN]  y - Line number in the map.

  [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
  (See map.h for details.)

  [IN]  iSpeed - the speed to move.

  Return value:

  None.

  --*/
{
    int           xOffset, yOffset, i, dx, dy;
    DWORD         t;

    xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
    yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);

    t = 0;

    while (xOffset != 0 || yOffset != 0)
    {
        PAL_ProcessEvent();
        while (SDL_GetTicks_New() <= t)
        {
            PAL_ProcessEvent();
            SDL_Delay(1);
        }

        t = SDL_GetTicks_New() + gConfig->m_Function_Set[24];

        //
        // Store trail
        //
        for (i = 3; i >= 0; i--)
        {
            gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
        }
        gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
        gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
        gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

        if (yOffset < 0)
        {
            gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
        }
        else
        {
            gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirSouth : kDirEast);
        }

        dx = PAL_X(gpGlobals->viewport);
        dy = PAL_Y(gpGlobals->viewport);

        if (abs(xOffset) <= iSpeed * 2)
        {
            dx += xOffset;
        }
        else
        {
            dx += iSpeed * (xOffset < 0 ? -2 : 2);
        }

        if (abs(yOffset) <= iSpeed)
        {
            dy += yOffset;
        }
        else
        {
            dy += iSpeed * (yOffset < 0 ? -1 : 1);
        }

        //
        // Move the viewport
        //
        gpGlobals->viewport = PAL_XY(dx, dy);

        PAL_UpdatePartyGestures(TRUE);
        PAL_GameUpdate(FALSE);
        PAL_MakeScene();
        VIDEO_UpdateScreen(gpTextureReal);

        xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
        yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);
    }

    PAL_UpdatePartyGestures(FALSE);
}

VOID CPalMain::PAL_SceneFade(
    INT         iPaletteNum,
    BOOL        fNight,
    INT         iStep
)
/*++
  Purpose:

    Fade in or fade out the screen. Update the scene during the process.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

    [IN]  iStep - positive to fade in, nagative to fade out.

  Return value:

    None.

--*/
{
    PAL_SetPalette(iPaletteNum, fNight);

    gpGlobals->fNeedToFadeIn = FALSE;

    if (iStep == 0)
    {
        iStep = 1;
    }

    if (iStep > 0)
    {
        for (int i = 0; i < 64; i += iStep)
        {
            DWORD time = SDL_GetTicks_New() + 100;

            //
            // Generate the scene
            //
            PAL_ClearKeyState();
            g_InputState.dir = kDirUnknown;
            g_InputState.prevdir = kDirUnknown;
            PAL_GameUpdate(FALSE);
            PAL_MakeScene();
            VIDEO_BackupScreen();
            RenderBlendCopy(gpTextureReal, gpTextureRealBak, i << 2);
            RenderPresent(gpTextureReal);

            PAL_ProcessEvent();

            while (SDL_GetTicks_New() < time)
            {
                PAL_ProcessEvent();
                SDL_Delay(1);
            }
        }
    }
    else
    {
        for (int i = 63; i >= 0; i += iStep)
        {
            DWORD time = SDL_GetTicks_New() + 100;

            //
            // Generate the scene
            //
            PAL_ClearKeyState();
            g_InputState.dir = kDirUnknown;
            g_InputState.prevdir = kDirUnknown;
            PAL_GameUpdate(FALSE);
            PAL_MakeScene();
            VIDEO_BackupScreen();
            RenderBlendCopy(gpTextureReal, gpTextureRealBak, i << 2);
            RenderPresent(gpTextureReal);


            PAL_ProcessEvent();

            while (SDL_GetTicks_New() < time)
            {
                PAL_ProcessEvent();
                SDL_Delay(1);
            }
        }
    }
}
