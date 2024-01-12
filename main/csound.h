#ifndef CSOUND_H
#define CSOUND_H
#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0

#include "cpalui.h"

#define     PAL_MAX_SAMPLERATE          49716
#define     PAL_MAX_VOLUME              100
//#define PAL_SAMPLE_RATE     48000 /*49716*/
//#define PAL_CHANNELS        1


class CSound : public CPalUI
{
public:
    BOOL g_fNoMusic{ 0 };
    BOOL g_fNoSound{ 0 };
    //BOOL g_SoundThread{ 0 };
public:
    CSound();
    ~CSound();
     //播放声音
    VOID SOUND_Play(INT i);
    //播放音乐
    VOID PAL_PlayMUS(INT iNumRIX, BOOL fLoop, FLOAT flFadeTime);;
    //返回设备驱动地址
    SDL_AudioSpec* AUDIO_GetDeviceSpec(VOID);

    VOID SoundRunThread();

private:
    VOID SDLCALL AUDIO_FillBuffer(LPVOID udata, LPBYTE stream, INT len);
    void AUDIO_AdjustVolume(short* srcdst, int iVolume, int samples);
    void AUDIO_MixNative(short* dst, short* src, int samples);
    VOID AUDIO_PlayMusic(INT iNumRIX, BOOL fLoop, FLOAT flFadeTime);
    VOID AUDIO_PlaySound(INT iSoundNum);
    VOID AUDIO_PlayMusicInThread(INT iNumRIX, BOOL fLoop, FLOAT flFadeTime);
    const void* SOUND_LoadVOCData(LPCBYTE lpData, DWORD dwLen, struct tagWAVESPEC* lpSpec);
    const void* SOUND_LoadWAVEData(LPCBYTE lpData, DWORD dwLen, struct tagWAVESPEC* lpSpec);
    BOOL SOUND_Play(VOID* object, INT iSoundNum, BOOL fLoop, FLOAT flFadeTime);
    VOID SOUND_FillBuffer(VOID* object, LPBYTE stream, INT len);
    VOID SOUND_Shutdown(VOID *object);


    VOID RIX_FillBuffer(VOID*, LPBYTE stream, INT len);
    struct tagAUDIOPLAYER* RIX_Init(LPCSTR szFileName);
    INT RIX_Play(VOID* object, INT iNumRIX, BOOL fLoop, FLOAT flFadeTime);
    VOID RIX_Shutdown(VOID* object);

    struct tagAUDIOPLAYER* MP3_Init(VOID);
    VOID MP3_Close(struct tagMP3PLAYER *player);
    VOID MP3_FillBuffer(VOID* object, LPBYTE stream, INT len);
    VOID MP3_Shutdown(VOID* object);
    BOOL MP3_Play(VOID* object, INT iNum, BOOL fLoop, FLOAT flFadeTime);

    VOID MIDI_Play(int iNumRIX, BOOL fLoop);
    VOID MIDI_SetVolume(int iVolume);


    VOID soundRun();
    INT AUDIO_OpenDevice(VOID);
    struct tagAUDIOPLAYER* SOUND_Init(VOID);

    //SDL_AudioDeviceID gAudioDeviceID{ 0 };
 
    struct  tagAUDIODEVICE* gAudioDevice{ nullptr };
};

#endif // CSOUND_H


