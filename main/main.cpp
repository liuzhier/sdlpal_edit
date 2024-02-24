#include <iostream>
#include "CPalMain.h"
#include "cscript.h"
#include <wtypes.h>


//int WINAPI WinMain(int argc, char * argv[])
int WINAPI     wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    
    SDL_Init((Uint32)(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE));

    class CScript* a = new CScript;
    delete a;
 
    SDL_Quit();
    //exit(0);
    return 0;
}

