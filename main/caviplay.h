#ifndef CAVIPLAY_H
#define CAVIPLAY_H
#include "command.h"
#include <SDL2\SDL.h>
class CAviPlay
{
    
public:
    CAviPlay();
    
    struct AVIPlayState* PAL_ReadAVIInfo(FILE* fp,struct AVIPlayState* avi);
    void PAL_AVIFeedAudio(struct AVIPlayState* avi, uint8_t* buffer, uint32_t size);
    void PAL_AVIInit(void);
    void PAL_AVIShutdown(void);
    void PAL_RenderAVIFrameToSurface(SDL_Surface* lpSurface, const struct RIFFChunk* lpChunk);
    BOOL PAL_PlayAVI(LPCSTR lpszPath);
    RIFFChunk* PAL_ReadDataChunk(FILE* fp, long endPos, void* userbuf, uint32_t buflen, int mult);
    
    static VOID SDLCALL AVI_FillAudioBuffer(void* udata, uint8_t* stream, int len);
    static void* AVI_GetPlayState(void);
    static VOID setPAL(class CPalBaseIO* p);
};

#endif // CAVIPLAY_H
