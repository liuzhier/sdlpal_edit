#include "csound.h"

#include "../sound/adplug/emuopls.h"
#include "../sound/adplug/rix.h"
#include "../sound/adplug/convertopl.h"
#include "../sound/adplug/surroundopl.h"
#include "../sound/libmad/resampler.h"

#include <thread>
#include "caviplay.h"

#define USE_AUDIODEVICE

#ifdef USE_AUDIODEVICE
# define SDL_CloseAudio() SDL_CloseAudioDevice(gAudioDevice->id)
# define SDL_PauseAudio(pause_on) SDL_PauseAudioDevice(gAudioDevice->id, (pause_on))
# define SDL_OpenAudio(desired, obtained) \
	((gAudioDevice->id = SDL_OpenAudioDevice((gConfig->iAudioDevice >= 0 ? \
    SDL_GetAudioDeviceName(gConfig->iAudioDevice, 0) : \
    NULL), 0, (desired), (obtained), 0)) > 0 ? gAudioDevice->id : -1)
#endif
#define AUDIO_IsIntegerConversion(a) (((a) % gConfig->iSampleRate) == 0 || (gConfig->iSampleRate % (a)) == 0)

#define RIFF_RIFF (((uint32_t)'R') | (((uint32_t)'I') << 8) | (((uint32_t)'F') << 16) | (((uint32_t)'F') << 24))
#define RIFF_WAVE (((uint32_t)'W') | (((uint32_t)'A') << 8) | (((uint32_t)'V') << 16) | (((uint32_t)'E') << 24))
#define WAVE_fmt  (((uint32_t)'f') | (((uint32_t)'m') << 8) | (((uint32_t)'t') << 16) | (((uint32_t)' ') << 24))
#define WAVE_data (((uint32_t)'d') | (((uint32_t)'a') << 8) | (((uint32_t)'t') << 16) | (((uint32_t)'a') << 24))

typedef struct tagAUDIOPLAYER
{
    INT                        iMusic{};
    BOOL                       fLoop{};
	VOID (CSound::* Shutdown)(VOID*); 
    BOOL(CSound::* Play)(VOID*, INT, BOOL, FLOAT);
    VOID(CSound::* FillBuffer)(VOID*, LPBYTE, INT);
} AUDIOPLAYER, * LPAUDIOPLAYER;

typedef struct tagAUDIODEVICE
{
    SDL_AudioSpec spec{};		/* Actual-used sound specification */
    AUDIOPLAYER* pMusPlayer{};
    AUDIOPLAYER* pMp3Player{};
    AUDIOPLAYER* pSoundPlayer{};

    void* pSoundBuffer = nullptr;	/* The output buffer for sound */
    SDL_AudioDeviceID         id{};
    INT                       bMusicVolume{};	/* The BGM volume ranged in [0, 128] for better performance */
    INT                       bSoundVolume{};	/* The sound effect volume ranged in [0, 128] for better performance */
    BOOL                      fMusicEnabled{}; /* Is BGM enabled? */
    BOOL                      fSoundEnabled{}; /* Is sound effect enabled? */
    BOOL                      fOpened{};       /* Is the audio device opened? */
    ~tagAUDIODEVICE()
    { 
        if (pSoundBuffer) 
            free(pSoundBuffer);
        pSoundBuffer = nullptr;
    };
} AUDIODEVICE;

//static AUDIODEVICE gAudioDevice;

typedef struct tagRIXPLAYER :
    public AUDIOPLAYER
{
    Copl* opl;
    CrixPlayer* rix;
    void* resampler[2];
    BYTE                       buf[(PAL_MAX_SAMPLERATE + 69) / 70 * sizeof(short) * 2];
    LPBYTE                     pos;
    INT                        iNextMusic; // the next music number to switch to
    DWORD                      dwStartFadeTime;
    INT                        iTotalFadeOutSamples;
    INT                        iTotalFadeInSamples;
    INT                        iRemainingFadeSamples;
    enum { NONE, FADE_IN, FADE_OUT } FadeType; // fade in or fade out ?
    BOOL                       fNextLoop;
    BOOL                       fReady;
} RIXPLAYER, * LPRIXPLAYER;

//static SNDPLAYER gSndPlayer;
#ifdef PAL_HAS_NATIVEMIDI
//static BOOL         g_fUseMidi;
#endif

typedef struct tagWAVESPEC
{
    int                 size;
    int                 freq;
    SDL_AudioFormat     format;
    uint8_t             channels;
    uint8_t             align;
} WAVESPEC;

typedef const void* (CSound::* SoundLoader)(LPCBYTE, DWORD, WAVESPEC*);
typedef int(*ResampleMixer)(void* [2], const void*, const WAVESPEC*, void*, int, const void**);

typedef struct tagWAVEDATA
{
    struct tagWAVEDATA* next;

    void* resampler[2];	/* The resampler used for sound data */
    ResampleMixer       ResampleMix;
    const void* base;
    const void* current;
    const void* end;
    WAVESPEC            spec;
} WAVEDATA;

typedef struct tagSOUNDPLAYER
{
    INT                        iMusic;
    BOOL                       fLoop;
    VOID(CSound::* Shutdown)(VOID*);
    BOOL(CSound::* Play)(VOID*, INT, BOOL, FLOAT);
    VOID(CSound::* FillBuffer)(VOID*, LPBYTE, INT);

    FILE* mkf;		                /* File pointer to the MKF file */
    SoundLoader         LoadSound;	/* The function pointer for load WAVE/VOC data */
    WAVEDATA            soundlist;
    int                 cursounds;
} SOUNDPLAYER, * LPSOUNDPLAYER;

typedef struct RIFFHeader
{
    uint32_t signature;         /* 'RIFF' */
    uint32_t length;            /* Total length minus eight, little-endian */
    uint32_t type;              /* 'WAVE', 'AVI ', ... */
} RIFFHeader;

typedef struct RIFFChunkHeader
{
    uint32_t type;             /* 'fmt ', 'hdrl', 'movi' and so on */
    uint32_t length;           /* Total chunk length minus eight, little-endian */
} RIFFChunkHeader;

typedef struct RIFFChunk
{
    RIFFChunkHeader header;
    uint8_t         data[1];
} RIFFChunk;

typedef struct RIFFListHeader
{
    uint32_t signature;        /* 'LIST' */
    uint32_t length;           /* Total list length minus eight, little-endian */
    uint32_t type;             /* 'fmt ', 'hdrl', 'movi' and so on */
} RIFFListHeader;

typedef union RIFFBlockHeader
{
    struct {
        uint32_t  type;
        uint32_t  length;
    };
    RIFFChunkHeader chunk;
    RIFFListHeader  list;
} RIFFBlockHeader;

typedef struct WAVEFormatPCM
{
    uint16_t wFormatTag;      /* format type */
    uint16_t nChannels;       /* number of channels (i.e. mono, stereo, etc.) */
    uint32_t nSamplesPerSec;  /* sample rate */
    uint32_t nAvgBytesPerSec; /* for buffer estimation */
    uint16_t nBlockAlign;     /* block size of data */
    uint16_t wBitsPerSample;
} WAVEFormatPCM;

typedef struct tagVOCHEADER
{
    char    signature[0x14];	/* "Creative Voice File\x1A" */
    WORD    data_offset;		/* little endian */
    WORD	version;
    WORD	version_checksum;
} VOCHEADER, * LPVOCHEADER;
typedef const VOCHEADER* LPCVOCHEADER;

typedef struct tagAUDIO_Play//将声音播放放入子进程使用的结构
{
    BOOL musicLoop = 0;
    FLOAT flFadeTime = 0;
    BOOL locked = 0;
    int Music = 0;
    int sound = 0;
}AUDIO_Play;

static AUDIO_Play AudioPlay;

CSound::CSound()
{
}

CSound::~CSound()
{
    //SDL_DetachThread(pSoundThread);
    //等待进程退出
    while (PalSoundIn) {
        PAL_Delay(1);
    };
    
    if (gAudioDevice)
        delete gAudioDevice;
    gAudioDevice = nullptr;
}

VOID CSound::SOUND_Play(INT i) { return AUDIO_PlaySound(i); }


VOID CSound::soundRun()
{

    PalSoundIn = TRUE;

    AUDIO_OpenDevice();
    //
    //return;
    BYTE soundBuf[2048]{ 0 };
    auto pgsoundbak = gAudioDevice;
    while (!PalQuit)
    {
        if (AudioPlay.locked)//等待解锁
        {
            SDL_Delay(1);
            continue;
        }

        if (AudioPlay.Music > 0)
        {
            AUDIO_PlayMusicInThread(AudioPlay.Music, AudioPlay.musicLoop, AudioPlay.flFadeTime);
            AudioPlay.Music = 0;
        }

        if (PalQuit)break;

        if (AudioPlay.sound > 0)
        {
            if (gAudioDevice->pSoundPlayer)
            {
                (this->*gAudioDevice->pSoundPlayer->Play)(gAudioDevice->pSoundPlayer, abs(AudioPlay.sound), FALSE, 0.0f);
            }
            AudioPlay.sound = 0;
        }

        if (PalQuit)break;

        ASSERT(gAudioDevice == pgsoundbak);
        //以下运行原回调函数
        if (gAudioDevice && SDL_GetQueuedAudioSize(gAudioDevice->id) < 8192 * gConfig->iAudioChannels)
        {
            AUDIO_FillBuffer(gAudioDevice, (LPBYTE)soundBuf, 1024);
            SDL_QueueAudio(gAudioDevice->id, soundBuf, 1024);
        }
        Sleep(1);
    }
    //shutdown
    if (gConfig->eMIDISynth == SYNTH_NATIVE && gConfig->eMusicType == MUSIC_MIDI)
    {
        //关闭正在播放的MID
        MIDI_Play(0, 0);
        AudioPlay.Music = 0;
    }
    //清理
    //关音乐缓存
    if (gAudioDevice->pMusPlayer)
        (this->*gAudioDevice->pMusPlayer->Shutdown)(gAudioDevice->pMusPlayer);
    //关Mp3
    if (gConfig->fUseMp3 && gAudioDevice->pMp3Player != gAudioDevice->pMusPlayer && gAudioDevice->pMp3Player)
        (this->*gAudioDevice->pMp3Player->Shutdown)(gAudioDevice->pMp3Player);
    //关声音缓存
    SOUND_Shutdown(gAudioDevice->pSoundPlayer);
    //关设备驱动
    SDL_CloseAudio();
    delete gAudioDevice; 
    gAudioDevice = nullptr;
    PalSoundIn = FALSE;
}


INT CSound::AUDIO_OpenDevice(VOID)
/*++
  Purpose:

    Initialize the audio subsystem.

  Parameters:

    None.

  Return value:

    0 if succeed, others if failed.

--*/
{
    SDL_AudioSpec spec;
    gAudioDevice = new AUDIODEVICE;
    //清零
    SDL_zero(*gAudioDevice);

    auto  pgsoundbak = gAudioDevice;
    
    gAudioDevice->fOpened = FALSE;
    gAudioDevice->fMusicEnabled = TRUE;
    gAudioDevice->fSoundEnabled = TRUE;
    gAudioDevice->bMusicVolume = gConfig->iMusicVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
    gAudioDevice->bSoundVolume = gConfig->iSoundVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
 
    //
    // Initialize the resampler module
    //
    ASSERT(gAudioDevice == pgsoundbak);
    resampler_init();
    ASSERT(gAudioDevice == pgsoundbak);


    //
    // Open the audio device.
    //
    gAudioDevice->spec.freq = gConfig->iSampleRate;
    gAudioDevice->spec.format = AUDIO_S16SYS;
    gAudioDevice->spec.channels = gConfig->iAudioChannels;
    gAudioDevice->spec.samples = gConfig->wAudioBufferSize;
    gAudioDevice->spec.callback = NULL;
    gAudioDevice->spec.userdata = this;

    //UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio: requesting audio spec:freq %d, format %d, channels %d, samples %d\n", gAudioDevice->spec.freq, gAudioDevice->spec.format, gAudioDevice->spec.channels, gAudioDevice->spec.samples);

    if (SDL_OpenAudio(&gAudioDevice->spec, &spec) < 0)
    {
        //UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio ERROR: %s, got spec:freq %d, format %d, channels %d, samples %d\n", SDL_GetError(), spec.freq, spec.format, spec.channels, spec.samples);
        //
        // Failed
        //
        return -3;
    }
    else
    {
        //UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio succeed, got spec:freq %d, format %d, channels %d, samples %d\n", spec.freq, spec.format, spec.channels, spec.samples);
        gAudioDevice->pSoundBuffer = malloc(sizeof(short)* gConfig->wAudioBufferSize * gConfig->iAudioChannels  );
    }

    gAudioDevice->fOpened = TRUE;

    //
    // Initialize the sound subsystem.
    //
    gAudioDevice->pSoundPlayer = SOUND_Init();

    //
    // Initialize the music subsystem.
    //
    switch (gConfig->eMusicType)
    {
    case MUSIC_MP3:
        gAudioDevice->pMusPlayer = MP3_Init();
        break;
    case MUSIC_RIX:
    {
        string fname = PalDir + "mus.mkf";
        gAudioDevice->pMusPlayer = RIX_Init(fname.c_str());
        break;
    }
    case MUSIC_OGG:
        //gAudioDevice->pMusPlayer = OGG_Init();
        break;
    case MUSIC_OPUS:
        //gAudioDevice->pMusPlayer = OPUS_Init();
        break;
    case MUSIC_MIDI:
/*
        if (gConfig->eMIDISynth == SYNTH_TIMIDITY)
            gAudioDevice->pMusPlayer = TIMIDITY_Init();
        else if (gConfig->eMIDISynth == SYNTH_TINYSOUNDFONT)
            gAudioDevice->pMusPlayer = TSF_Init();
        break;
        */
    default:
        break;
    }
    //如果优先使用MP3 进行初始化
    if (gConfig->fUseMp3 && gConfig->eMusicType != MUSIC_MP3)
    {
        gAudioDevice->pMp3Player = MP3_Init();
    }
    //
    // Initialize the CD audio.
    //

    //
    // Let the callback function run so that musics will be played.
    //
    SDL_PauseAudio(0);

    return 0;
}

static int
SOUND_ResampleMix_S16_Mono_Mono(
    void* resampler[2],
    const void* lpData,
    const WAVESPEC* lpSpec,
    void* lpBuffer,
    int                    iBufLen,
    const void** llpData
)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) mono PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    int src_samples = lpSpec->size >> 1;
    const short* src = (const short*)lpData;
    short* dst = (short*)lpBuffer;
    int channel_len = iBufLen, total_bytes = 0;

    while (total_bytes < channel_len && src_samples > 0)
    {
        int j, to_write = resampler_get_free_count(resampler[0]);
        if (to_write > src_samples) to_write = src_samples;
        for (j = 0; j < to_write; j++)
            resampler_write_sample(resampler[0], SDL_SwapLE16(*src++));
        src_samples -= to_write;
        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
            *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            total_bytes += sizeof(short);
            resampler_remove_sample(resampler[0]);
        }
    }

    if (llpData) *llpData = src;
    return total_bytes;
}

static int
SOUND_ResampleMix_S16_Mono_Stereo(
    void* resampler[2],
    const void* lpData,
    const WAVESPEC* lpSpec,
    void* lpBuffer,
    int                    iBufLen,
    const void** llpData
)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) mono PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    int src_samples = lpSpec->size >> 1;
    const short* src = (const short*)lpData;
    short* dst = (short*)lpBuffer;
    int channel_len = iBufLen >> 1, total_bytes = 0;

    while (total_bytes < channel_len && src_samples > 0)
    {
        int j, to_write = resampler_get_free_count(resampler[0]);
        if (to_write > src_samples) to_write = src_samples;
        for (j = 0; j < to_write; j++)
            resampler_write_sample(resampler[0], SDL_SwapLE16(*src++));
        src_samples -= to_write;
        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
            dst[0] = dst[1] = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            total_bytes += sizeof(short); dst += 2;
            resampler_remove_sample(resampler[0]);
        }
    }

    if (llpData) *llpData = src;
    return total_bytes;
}

static int
SOUND_ResampleMix_S16_Stereo_Mono(
    void* resampler[2],
    const void* lpData,
    const WAVESPEC* lpSpec,
    void* lpBuffer,
    int                    iBufLen,
    const void** llpData
)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) stereo PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    int src_samples = lpSpec->size >> 2;
    const short* src = (const short*)lpData;
    short* dst = (short*)lpBuffer;
    int channel_len = iBufLen, total_bytes = 0;

    while (total_bytes < channel_len && src_samples > 0)
    {
        int j, to_write = resampler_get_free_count(resampler[0]);
        if (to_write > src_samples) to_write = src_samples;
        for (j = 0; j < to_write; j++)
        {
            resampler_write_sample(resampler[0], SDL_SwapLE16(*src++));
            resampler_write_sample(resampler[1], SDL_SwapLE16(*src++));
        }
        src_samples -= to_write;
        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            int sample = (((resampler_get_sample(resampler[0]) >> 8) + (resampler_get_sample(resampler[1]) >> 8)) >> 1) + *dst;
            *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            total_bytes += sizeof(short);
            resampler_remove_sample(resampler[0]);
            resampler_remove_sample(resampler[1]);
        }
    }

    if (llpData) *llpData = src;
    return total_bytes;
}

static int
SOUND_ResampleMix_S16_Stereo_Stereo(
    void* resampler[2],
    const void* lpData,
    const WAVESPEC* lpSpec,
    void* lpBuffer,
    int                    iBufLen,
    const void** llpData
)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) stereo PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    int src_samples = lpSpec->size >> 2;
    const short* src = (const short*)lpData;
    short* dst = (short*)lpBuffer;
    int channel_len = iBufLen >> 1, total_bytes = 0;

    while (total_bytes < channel_len && src_samples > 0)
    {
        int j, to_write = resampler_get_free_count(resampler[0]);
        if (to_write > src_samples) to_write = src_samples;
        for (j = 0; j < to_write; j++)
        {
            resampler_write_sample(resampler[0], SDL_SwapLE16(*src++));
            resampler_write_sample(resampler[1], SDL_SwapLE16(*src++));
        }
        src_samples -= to_write;
        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            int sample;
            sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
            *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            sample = (resampler_get_sample(resampler[1]) >> 8) + *dst;
            *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            total_bytes += sizeof(short);
            resampler_remove_sample(resampler[0]);
            resampler_remove_sample(resampler[1]);
        }
    }

    if (llpData) *llpData = src;
    return total_bytes;
}

static int
SOUND_ResampleMix_U8_Mono_Mono(
    void* resampler[2],
    const void* lpData,
    const WAVESPEC* lpSpec,
    void* lpBuffer,
    int                    iBufLen,
    const void** llpData
)
/*++
  Purpose:

    Resample 8-bit unsigned mono PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    int src_samples = lpSpec->size;
    const uint8_t* src = (const uint8_t*)lpData;
    short* dst = (short*)lpBuffer;
    int channel_len = iBufLen, total_bytes = 0;

    while (total_bytes < channel_len && src_samples > 0)
    {
        int j, to_write = resampler_get_free_count(resampler[0]);
        if (to_write > src_samples) to_write = src_samples;
        for (j = 0; j < to_write; j++)
            resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
        src_samples -= to_write;
        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
            *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            total_bytes += sizeof(short);
            resampler_remove_sample(resampler[0]);
        }
    }

    if (llpData) *llpData = src;
    return total_bytes;
}

static int
SOUND_ResampleMix_U8_Mono_Stereo(
    void* resampler[2],
    const void* lpData,
    const WAVESPEC* lpSpec,
    void* lpBuffer,
    int                    iBufLen,
    const void** llpData
)
/*++
  Purpose:

    Resample 8-bit unsigned mono PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    int src_samples = lpSpec->size;
    const uint8_t* src = (const uint8_t*)lpData;
    short* dst = (short*)lpBuffer;
    int channel_len = iBufLen >> 1, total_bytes = 0;

    while (total_bytes < channel_len && src_samples > 0)
    {
        int j, to_write = resampler_get_free_count(resampler[0]);
        if (to_write > src_samples) to_write = src_samples;
        for (j = 0; j < to_write; j++)
            resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
        src_samples -= to_write;
        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
            dst[0] = dst[1] = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            total_bytes += sizeof(short); dst += 2;
            resampler_remove_sample(resampler[0]);
        }
    }

    if (llpData) *llpData = src;
    return total_bytes;
}

static int
SOUND_ResampleMix_U8_Stereo_Mono(
    void* resampler[2],
    const void* lpData,
    const WAVESPEC* lpSpec,
    void* lpBuffer,
    int                    iBufLen,
    const void** llpData
)
/*++
  Purpose:

    Resample 8-bit unsigned stereo PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    int src_samples = lpSpec->size >> 1;
    const uint8_t* src = (const uint8_t*)lpData;
    short* dst = (short*)lpBuffer;
    int channel_len = iBufLen, total_bytes = 0;

    while (total_bytes < channel_len && src_samples > 0)
    {
        int j, to_write = resampler_get_free_count(resampler[0]);
        if (to_write > src_samples) to_write = src_samples;
        for (j = 0; j < to_write; j++)
        {
            resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
            resampler_write_sample(resampler[1], (*src++ ^ 0x80) << 8);
        }
        src_samples -= to_write;
        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            int sample = (((resampler_get_sample(resampler[0]) >> 8) + (resampler_get_sample(resampler[1]) >> 8)) >> 1) + *dst;
            *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            total_bytes += sizeof(short);
            resampler_remove_sample(resampler[0]);
            resampler_remove_sample(resampler[1]);
        }
    }

    if (llpData) *llpData = src;
    return total_bytes;
}

static int
SOUND_ResampleMix_U8_Stereo_Stereo(
    void* resampler[2],
    const void* lpData,
    const WAVESPEC* lpSpec,
    void* lpBuffer,
    int                    iBufLen,
    const void** llpData
)
/*++
  Purpose:

    Resample 8-bit unsigned stereo PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
    int src_samples = lpSpec->size >> 1;
    const uint8_t* src = (const uint8_t*)lpData;
    short* dst = (short*)lpBuffer;
    int channel_len = iBufLen >> 1, total_bytes = 0;

    while (total_bytes < channel_len && src_samples > 0)
    {
        int j, to_write = resampler_get_free_count(resampler[0]);
        if (to_write > src_samples) to_write = src_samples;
        for (j = 0; j < to_write; j++)
        {
            resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
            resampler_write_sample(resampler[1], (*src++ ^ 0x80) << 8);
        }
        src_samples -= to_write;
        while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
        {
            int sample;
            sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
            *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            sample = (resampler_get_sample(resampler[1]) >> 8) + *dst;
            *dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
            total_bytes += sizeof(short);
            resampler_remove_sample(resampler[0]);
            resampler_remove_sample(resampler[1]);
        }
    }

    if (llpData) *llpData = src;
    return total_bytes;
}

AUDIOPLAYER* CSound::SOUND_Init(VOID)
/*++
  Purpose:

    Initialize the sound subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    const char* mkfs[2]{0};
    SoundLoader func[2]{0};
    int i;

    if (gConfig->fIsWIN95)
    {
        mkfs[0] = "sounds.mkf"; func[0] = &CSound::SOUND_LoadWAVEData;
        mkfs[1] = "voc.mkf"; func[1] = &CSound::SOUND_LoadVOCData;
    }
    else
    {
        mkfs[0] = "voc.mkf"; func[0] = &CSound::SOUND_LoadVOCData;
        mkfs[1] = "sounds.mkf"; func[1] = &CSound::SOUND_LoadWAVEData;
    }

    for (i = 0; i < 2; i++)
    {
        FILE* mkf = UTIL_OpenFile(mkfs[i]);
        if (mkf)
        {
            LPSOUNDPLAYER player = (LPSOUNDPLAYER)malloc(sizeof(SOUNDPLAYER));
            memset(&player->soundlist, 0, sizeof(WAVEDATA));
            player->Play = &CSound::SOUND_Play;
            player->FillBuffer = &CSound::SOUND_FillBuffer;
            player->Shutdown = &CSound::SOUND_Shutdown;

            player->LoadSound = func[i];
            player->mkf = mkf;
            player->soundlist.resampler[0] = resampler_create();
            player->soundlist.resampler[1] = resampler_create();
            player->cursounds = 0;
            return (LPAUDIOPLAYER)player;
        }
    }

    return NULL;
}


VOID CSound::RIX_FillBuffer(
    VOID*  object,
    LPBYTE     stream,
    INT        len
)
/*++
    Purpose:

    Fill the background music into the sound buffer. Called by the SDL sound
    callback function only (audio.c: AUDIO_FillBuffer).

    Parameters:

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

    Return value:

    None.

--*/
{
    LPRIXPLAYER pRixPlayer = (LPRIXPLAYER)object;

    if (pRixPlayer == NULL || !pRixPlayer->fReady)
    {
        //
        // Not initialized
        //
        return;
    }

    while (len > 0)
    {
        INT       volume, delta_samples = 0, vol_delta = 0;

        //
        // fading in or fading out
        //
        switch (pRixPlayer->FadeType)
        {
        case RIXPLAYER::FADE_IN:
            if (pRixPlayer->iRemainingFadeSamples <= 0)
            {
                pRixPlayer->FadeType = RIXPLAYER::NONE;
                volume = SDL_MIX_MAXVOLUME;
            }
            else
            {
                volume = (INT)(SDL_MIX_MAXVOLUME * (1.0 - (double)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeInSamples));
                delta_samples = (pRixPlayer->iTotalFadeInSamples / SDL_MIX_MAXVOLUME) & ~(gConfig->iAudioChannels - 1); vol_delta = 1;
            }
            break;
        case RIXPLAYER::FADE_OUT:
            if (pRixPlayer->iTotalFadeOutSamples == pRixPlayer->iRemainingFadeSamples && pRixPlayer->iTotalFadeOutSamples > 0)
            {
                UINT  now = SDL_GetTicks_New();
                INT u = AUDIO_GetDeviceSpec()->freq;
                INT   passed_samples = ((INT)(now - pRixPlayer->dwStartFadeTime) > 0) ? (INT)((now - pRixPlayer->dwStartFadeTime) * u / 1000) : 0;
                pRixPlayer->iRemainingFadeSamples -= passed_samples;
            }
            if (pRixPlayer->iMusic == -1 || pRixPlayer->iRemainingFadeSamples <= 0)
            {
                //
                // There is no current playing music, or fading time has passed.
                // Start playing the next one or stop playing.
                //
                if (pRixPlayer->iNextMusic > 0)
                {
                    pRixPlayer->iMusic = pRixPlayer->iNextMusic;
                    pRixPlayer->iNextMusic = -1;
                    pRixPlayer->fLoop = pRixPlayer->fNextLoop;
                    pRixPlayer->FadeType = RIXPLAYER::FADE_IN;
                    if (pRixPlayer->iMusic > 0)
                        pRixPlayer->dwStartFadeTime += pRixPlayer->iTotalFadeOutSamples * 1000 / gConfig->iSampleRate;
                    else
                        pRixPlayer->dwStartFadeTime = SDL_GetTicks_New();
                    pRixPlayer->iTotalFadeOutSamples = 0;
                    pRixPlayer->iRemainingFadeSamples = pRixPlayer->iTotalFadeInSamples;
                    pRixPlayer->rix->rewind(pRixPlayer->iMusic);
                    if (pRixPlayer->resampler[0]) resampler_clear(pRixPlayer->resampler[0]);
                    if (pRixPlayer->resampler[1]) resampler_clear(pRixPlayer->resampler[1]);
                    continue;
                }
                else
                {
                    pRixPlayer->iMusic = -1;
                    pRixPlayer->FadeType = RIXPLAYER::NONE;
                    return;
                }
            }
            else
            {
                volume = (INT)(SDL_MIX_MAXVOLUME * ((double)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeOutSamples));
                delta_samples = (pRixPlayer->iTotalFadeOutSamples / SDL_MIX_MAXVOLUME) & ~(gConfig->iAudioChannels - 1); vol_delta = -1;
            }
            break;
        default:
            if (pRixPlayer->iMusic <= 0)
            {
                //
                // No current playing music
                //
                return;
            }
            else
            {
                volume = SDL_MIX_MAXVOLUME;
            }
        }

        //
        // Fill the buffer with sound data
        //
        int buf_max_len = gConfig->iSampleRate / 70 * gConfig->iAudioChannels * sizeof(short);
        bool fContinue = true;
        while (len > 0 && fContinue)
        {
            if (pRixPlayer->pos == NULL || pRixPlayer->pos - pRixPlayer->buf >= buf_max_len)
            {
                pRixPlayer->pos = pRixPlayer->buf;
                if (!pRixPlayer->rix->update())
                {
                    if (!pRixPlayer->fLoop)
                    {
                        //
                        // Not loop, simply terminate the music
                        //
                        pRixPlayer->iMusic = -1;
                        if (pRixPlayer->FadeType != RIXPLAYER::FADE_OUT && pRixPlayer->iNextMusic == -1)
                        {
                            pRixPlayer->FadeType = RIXPLAYER::NONE;
                        }
                        return;
                    }
                    pRixPlayer->rix->rewindReInit(pRixPlayer->iMusic, false);
                    if (!pRixPlayer->rix->update())
                    {
                        //
                        // Something must be wrong
                        //
                        pRixPlayer->iMusic = -1;
                        pRixPlayer->FadeType = RIXPLAYER::NONE;
                        return;
                    }
                }
                int sample_count = gConfig->iSampleRate / 70;
                if (pRixPlayer->resampler[0])
                {
                    unsigned int samples_written = 0;
                    short* finalBuf = (short*)pRixPlayer->buf;

                    while (sample_count)
                    {
                        int to_write = resampler_get_free_count(pRixPlayer->resampler[0]);
                        if (to_write)
                        {
                            short* tempBuf = (short*)alloca(to_write * gConfig->iAudioChannels * sizeof(short));
                            int temp_buf_read = 0;
                            pRixPlayer->opl->update(tempBuf, to_write);
                            for (int i = 0; i < to_write * gConfig->iAudioChannels; i++)
                                resampler_write_sample(pRixPlayer->resampler[i % gConfig->iAudioChannels], tempBuf[temp_buf_read++]);
                        }

                        int to_get = resampler_get_sample_count(pRixPlayer->resampler[0]);
                        if (to_get > sample_count) to_get = sample_count;
                        for (int i = 0; i < to_get * gConfig->iAudioChannels; i++)
                            finalBuf[samples_written++] = resampler_get_and_remove_sample(pRixPlayer->resampler[i % gConfig->iAudioChannels]);
                        sample_count -= to_get;
                    }
                }
                else
                {
                    pRixPlayer->opl->update((short*)(pRixPlayer->buf), sample_count);
                }
            }

            int l = buf_max_len - (pRixPlayer->pos - pRixPlayer->buf);
            l = (l > len) ? len / sizeof(short) : l / sizeof(short);

            //
            // Put audio data into buffer and adjust volume
            //
            if (pRixPlayer->FadeType != RIXPLAYER::NONE)
            {
                short* ptr = (short*)stream;
                for (int i = 0; i < l && pRixPlayer->iRemainingFadeSamples > 0; volume += vol_delta)
                {
                    int j = 0;
                    for (j = 0; i < l && j < delta_samples; i++, j++)
                    {
                        *ptr++ = *(short*)pRixPlayer->pos * volume / SDL_MIX_MAXVOLUME;
                        pRixPlayer->pos += sizeof(short);
                    }
                    pRixPlayer->iRemainingFadeSamples -= j;
                }
                fContinue = (pRixPlayer->iRemainingFadeSamples > 0);
                len -= (LPBYTE)ptr - stream; stream = (LPBYTE)ptr;
            }
            else
            {
                memcpy(stream, pRixPlayer->pos, l * sizeof(short));
                pRixPlayer->pos += l * sizeof(short);
                stream += l * sizeof(short);
                len -= l * sizeof(short);
            }
        }
    }
}

BOOL CSound::SOUND_Play(VOID* object, INT iSoundNum, BOOL fLoop, FLOAT flFadeTime)
/*++
  Purpose:

    Play a sound in voc.mkf/sounds.mkf file.

  Parameters:

    [IN]  object - Pointer to the SOUNDPLAYER instance.
    [IN]  iSoundNum - number of the sound; the absolute value is used.
    [IN]  fLoop - Not used, should be zero.
    [IN]  flFadeTime - Not used, should be zero.

  Return value:

    None.

--*/
{
    LPSOUNDPLAYER  player = (LPSOUNDPLAYER)object;
    const SDL_AudioSpec* devspec = AUDIO_GetDeviceSpec();
    WAVESPEC         wavespec;
    ResampleMixer    mixer;
    WAVEDATA* cursnd;
    void* buf;
    const void* snddata;
    int              len, i;

    //
    // Check for NULL pointer.
    //
    if (player == NULL)
    {
        return FALSE;
    }

    //
    // Get the length of the sound file.
    //
    len = PAL_MKFGetChunkSize(iSoundNum, player->mkf);
    if (len <= 0)
    {
        return FALSE;
    }

    buf = malloc(len);
    if (buf == NULL)
    {
        return FALSE;
    }

    //
    // Read the sound file from the MKF archive.
    //
    PAL_MKFReadChunk((LPBYTE)buf, len, iSoundNum, player->mkf);

    snddata = (this->*player->LoadSound)((LPBYTE)buf, len, &wavespec);
    if (snddata == NULL)
    {
        free(buf);
        return FALSE;
    }

    if (wavespec.channels == 1 && devspec->channels == 1)
        mixer = (wavespec.format == AUDIO_S16) ? SOUND_ResampleMix_S16_Mono_Mono : SOUND_ResampleMix_U8_Mono_Mono;
    else if (wavespec.channels == 1 && devspec->channels == 2)
        mixer = (wavespec.format == AUDIO_S16) ? SOUND_ResampleMix_S16_Mono_Stereo : SOUND_ResampleMix_U8_Mono_Stereo;
    else if (wavespec.channels == 2 && devspec->channels == 1)
        mixer = (wavespec.format == AUDIO_S16) ? SOUND_ResampleMix_S16_Stereo_Mono : SOUND_ResampleMix_U8_Stereo_Mono;
    else if (wavespec.channels == 2 && devspec->channels == 2)
        mixer = (wavespec.format == AUDIO_S16) ? SOUND_ResampleMix_S16_Stereo_Stereo : SOUND_ResampleMix_U8_Stereo_Stereo;
    else
    {
        free(buf);
        return FALSE;
    }

    //AUDIO_Lock();
    SDL_LockAudioDevice(gAudioDevice->id);

    cursnd = &player->soundlist;
    while (cursnd->next && cursnd->base)
        cursnd = cursnd->next;
    if (cursnd->base)
    {
        WAVEDATA* obj = (WAVEDATA*)malloc(sizeof(WAVEDATA));
        memset(obj, 0, sizeof(WAVEDATA));
        cursnd->next = obj;
        cursnd = cursnd->next;
    }

    for (i = 0; i < wavespec.channels; i++)
    {
        if (!cursnd->resampler[i])
            cursnd->resampler[i] = resampler_create();
        else
            resampler_clear(cursnd->resampler[i]);
        resampler_set_quality(cursnd->resampler[i], AUDIO_IsIntegerConversion(wavespec.freq) ? RESAMPLER_QUALITY_MIN : gConfig->iResampleQuality);
        resampler_set_rate(cursnd->resampler[i], (double)wavespec.freq / (double)devspec->freq);
    }

    cursnd->base = buf;
    cursnd->current = snddata;
    cursnd->end = (const uint8_t*)snddata + wavespec.size;
    cursnd->spec = wavespec;
    cursnd->ResampleMix = mixer;
    player->cursounds++;

    //AUDIO_Unlock()
    SDL_UnlockAudioDevice(gAudioDevice->id);

    return TRUE;
}

const void* CSound::SOUND_LoadWAVEData(LPCBYTE lpData, DWORD dwLen, struct tagWAVESPEC* lpSpec)
/*++
  Purpose:

    Return the WAVE data pointer inside the input buffer.

  Parameters:

    [IN]  lpData - pointer to the buffer of the WAVE file.

    [IN]  dwLen - length of the buffer of the WAVE file.

    [OUT] lpSpec - pointer to the SDL_AudioSpec structure, which contains
                    some basic information about the WAVE file.

  Return value:

    Pointer to the WAVE data inside the input buffer, NULL if failed.
--*/
{
    const RIFFHeader* lpRiff = (const RIFFHeader*)lpData;
    const RIFFChunkHeader* lpChunk = NULL;
    const WAVEFormatPCM* lpFormat = NULL;
    const uint8_t* lpWaveData = NULL;
    uint32_t len, type;

    if (dwLen < sizeof(RIFFHeader) || SDL_SwapLE32(lpRiff->signature) != RIFF_RIFF ||
        SDL_SwapLE32(lpRiff->type) != RIFF_WAVE || dwLen < SDL_SwapLE32(lpRiff->length) + 8)
    {
        return NULL;
    }

    lpChunk = (const RIFFChunkHeader*)(lpRiff + 1); dwLen -= sizeof(RIFFHeader);
    while (dwLen >= sizeof(RIFFChunkHeader))
    {
        len = SDL_SwapLE32(lpChunk->length);
        type = SDL_SwapLE32(lpChunk->type);
        if (dwLen >= sizeof(RIFFChunkHeader) + len)
            dwLen -= sizeof(RIFFChunkHeader) + len;
        else
            return NULL;

        switch (type)
        {
        case WAVE_fmt:
            lpFormat = (const WAVEFormatPCM*)(lpChunk + 1);
            if (len != sizeof(WAVEFormatPCM) || lpFormat->wFormatTag != SDL_SwapLE16(0x0001))
            {
                return NULL;
            }
            break;
        case WAVE_data:
            lpWaveData = (const uint8_t*)(lpChunk + 1);
            dwLen = 0;
            break;
        }
        lpChunk = (const RIFFChunkHeader*)((const uint8_t*)(lpChunk + 1) + len);
    }

    if (lpFormat == NULL || lpWaveData == NULL)
    {
        return NULL;
    }

    lpSpec->channels = SDL_SwapLE16(lpFormat->nChannels);
    lpSpec->format = (SDL_SwapLE16(lpFormat->wBitsPerSample) == 16) ? AUDIO_S16 : AUDIO_U8;
    lpSpec->freq = SDL_SwapLE32(lpFormat->nSamplesPerSec);
    lpSpec->size = len;
    lpSpec->align = SDL_SwapLE16(lpFormat->nChannels) * SDL_SwapLE16(lpFormat->wBitsPerSample) >> 3;

    return lpWaveData;
}

const void* CSound::SOUND_LoadVOCData(LPCBYTE lpData, DWORD dwLen, struct tagWAVESPEC* lpSpec)
/*++
  Purpose:

    Return the VOC data pointer inside the input buffer. Currently supports type 01 block only.

  Parameters:

    [IN]  lpData - pointer to the buffer of the VOC file.

    [IN]  dwLen - length of the buffer of the VOC file.

    [OUT] lpSpec - pointer to the SDL_AudioSpec structure, which contains
                   some basic information about the VOC file.

  Return value:

    Pointer to the WAVE data inside the input buffer, NULL if failed.

    Reference: http://sox.sourceforge.net/AudioFormats-11.html
--*/
{
    LPCVOCHEADER lpVOC = (LPCVOCHEADER)lpData;

    if (dwLen < sizeof(VOCHEADER) || memcmp(lpVOC->signature, "Creative Voice File\x1A", 0x14) || SDL_SwapLE16(lpVOC->data_offset) >= dwLen)
    {
        return NULL;
    }

    lpData += SDL_SwapLE16(lpVOC->data_offset);
    dwLen -= SDL_SwapLE16(lpVOC->data_offset);

    while (dwLen && *lpData)
    {
        DWORD len;
        if (dwLen >= 4)
        {
            len = lpData[1] | (lpData[2] << 8) | (lpData[3] << 16);
            if (dwLen >= len + 4)
                dwLen -= len + 4;
            else
                return NULL;
        }
        else
        {
            return NULL;
        }
        if (*lpData == 0x01)
        {
            if (lpData[5] != 0) return NULL;	/* Only 8-bit is supported */

            lpSpec->format = AUDIO_U8;
            lpSpec->channels = 1;
            lpSpec->freq = ((1000000 / (256 - lpData[4]) + 99) / 100) * 100; /* Round to next 100Hz */
            lpSpec->size = len - 2;
            lpSpec->align = 1;

            return lpData + 6;
        }
        else
        {
            lpData += len + 4;
        }
    }

    return NULL;
}

void CSound::AUDIO_AdjustVolume(short* srcdst, int iVolume, int samples)
{
    if (iVolume == SDL_MIX_MAXVOLUME) return;
    if (iVolume == 0) { memset(srcdst, 0, samples << 1); return; }
    while (samples > 0)
    {
        *srcdst = *srcdst * iVolume / SDL_MIX_MAXVOLUME;
        samples--; srcdst++;
    }
}

LPAUDIOPLAYER CSound:: RIX_Init(LPCSTR szFileName)
/*++
  Purpose:

    Initialize the RIX player subsystem.

  Parameters:

    [IN]  szFileName - Filename of the mus.mkf file.

  Return value:

    0 if success, -1 if cannot allocate memory, -2 if file not found.
--*/
{
    if (!szFileName) return NULL;

    LPRIXPLAYER pRixPlayer = new RIXPLAYER;
    if (pRixPlayer == NULL)
    {
        return NULL;
    }
    else
    {
        memset(pRixPlayer, 0, sizeof(RIXPLAYER));
        pRixPlayer->FillBuffer = &CSound::RIX_FillBuffer;
        pRixPlayer->Shutdown = &CSound::RIX_Shutdown;
        pRixPlayer->Play = &CSound::RIX_Play;
    }

    auto chip = (Copl::ChipType)gConfig->eOPLChip;
    if (chip == Copl::TYPE_OPL2 && gConfig->fUseSurroundOPL)
    {
        chip = Copl::TYPE_DUAL_OPL2;
    }

    Copl* opl = CEmuopl::CreateEmuopl((OPLCORE::TYPE)gConfig->eOPLCore, chip, gConfig->iOPLSampleRate);
    if (NULL == opl)
    {
        delete pRixPlayer;
        return NULL;
    }

    if (gConfig->fUseSurroundOPL)
    {
        Copl* tmpopl = new CSurroundopl(gConfig->iOPLSampleRate, gConfig->iSurroundOPLOffset, opl);
        if (NULL == tmpopl)
        {
            delete opl;
            delete pRixPlayer;
            return NULL;
        }
        opl = tmpopl;
    }

    pRixPlayer->opl = new CConvertopl(opl, true, gConfig->iAudioChannels == 2);
    if (pRixPlayer->opl == NULL)
    {
        delete opl;
        delete pRixPlayer;
        return NULL;
    }

    pRixPlayer->rix = new CrixPlayer(pRixPlayer->opl);
    if (pRixPlayer->rix == NULL)
    {
        delete pRixPlayer->opl;
        delete pRixPlayer;
        return NULL;
    }

    //
    // Load the MKF file.
    //
    if (!pRixPlayer->rix->load(szFileName, CProvider_Filesystem()))
    {
        delete pRixPlayer->rix;
        delete pRixPlayer->opl;
        delete pRixPlayer;
        pRixPlayer = NULL;
        return NULL;
    }

    if (gConfig->iOPLSampleRate != gConfig->iSampleRate)
    {
        for (int i = 0; i < gConfig->iAudioChannels; i++)
        {
            pRixPlayer->resampler[i] = resampler_create();
            resampler_set_quality(pRixPlayer->resampler[i], AUDIO_IsIntegerConversion(gConfig->iOPLSampleRate) ? RESAMPLER_QUALITY_MIN : gConfig->iResampleQuality);
            resampler_set_rate(pRixPlayer->resampler[i], (double)gConfig->iOPLSampleRate / (double)gConfig->iSampleRate);
        }
    }

#if USE_RIX_EXTRA_INIT
    if (gConfig->pExtraFMRegs && gConfig->pExtraFMVals)
    {
        pRixPlayer->rix->set_extra_init(gConfig->pExtraFMRegs, gConfig->pExtraFMVals, gConfig->dwExtraLength);
    }
#endif

    //
    // Success.
    //
    pRixPlayer->FadeType = RIXPLAYER::NONE;
    pRixPlayer->iMusic = pRixPlayer->iNextMusic = -1;
    pRixPlayer->pos = NULL;
    pRixPlayer->fLoop = FALSE;
    pRixPlayer->fNextLoop = FALSE;
    pRixPlayer->fReady = FALSE;

    return pRixPlayer;
}

VOID CSound::SOUND_FillBuffer(VOID* object, LPBYTE stream, INT len)
/*++
  Purpose:

    Fill the background music into the sound buffer. Called by the SDL sound
    callback function only (audio.c: AUDIO_FillBuffer).

  Parameters:

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

  Return value:

    None.

--*/
{
     LPSOUNDPLAYER player = (LPSOUNDPLAYER)object;
    if (player)
    {
        WAVEDATA* cursnd = &player->soundlist;
        int sounds = 0;
        do
        {
            if (cursnd->base)
            {
                cursnd->ResampleMix(cursnd->resampler, cursnd->current, &cursnd->spec, stream, len, &cursnd->current);
                cursnd->spec.size = (const uint8_t*)cursnd->end - (const uint8_t*)cursnd->current;
                if (cursnd->spec.size < cursnd->spec.align)
                {
                    free((void*)cursnd->base);
                    cursnd->base = cursnd->current = cursnd->end = NULL;
                    player->cursounds--;
                }
                else
                    sounds++;
            }
        } while ((cursnd = cursnd->next) && sounds < player->cursounds);
    }
}

void CSound::AUDIO_MixNative(short* dst, short* src, int samples)
{
    while (samples > 0)
    {
        int val = *src++ + *dst;
        if (val > SHRT_MAX)
            *dst++ = SHRT_MAX;
        else if (val < SHRT_MIN)
            *dst++ = SHRT_MIN;
        else
            *dst++ = (short)val;
        samples--;
    }
}

VOID CSound::RIX_Shutdown( VOID* object )
/*++
    Purpose:

    Shutdown the RIX player subsystem.

    Parameters:

    None.

    Return value:

    None.

--*/
{
    if (object != NULL)
    {
        LPRIXPLAYER pRixPlayer = (LPRIXPLAYER)object;
        pRixPlayer->fReady = FALSE;
        for (int i = 0; i < gConfig->iAudioChannels; i++)
            if (pRixPlayer->resampler[i])
                resampler_delete(pRixPlayer->resampler[i]);
        delete pRixPlayer->rix;
        delete pRixPlayer->opl;
        delete pRixPlayer;
    }
}

VOID CSound::SOUND_Shutdown(VOID * object)//tagAUDIOPLAYER *
/*++
  Purpose:

    Shutdown the sound subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    LPSOUNDPLAYER player = (LPSOUNDPLAYER)object;
    if (player)
    {
        WAVEDATA* cursnd = &player->soundlist;
        do
        {
            if (cursnd->resampler[0]) resampler_delete(cursnd->resampler[0]);
            if (cursnd->resampler[1]) resampler_delete(cursnd->resampler[1]);
            if (cursnd->base) free((void*)cursnd->base);
        } while ((cursnd = cursnd->next) != NULL);
        cursnd = player->soundlist.next;
        while (cursnd)
        {
            WAVEDATA* old = cursnd;
            cursnd = cursnd->next;
            free(old);
        }
        if (player->mkf) fclose(player->mkf);
    }
    free(player);
}

VOID CSound::AUDIO_PlayMusicInThread(INT iNumRIX, BOOL fLoop, FLOAT flFadeTime)
{
    if (gConfig->fUseMp3 && gAudioDevice->pMp3Player)
    {
        int ret = 0;
        SDL_LockAudioDevice(gAudioDevice->id);
        ret = (this->*gAudioDevice->pMp3Player->Play)(gAudioDevice->pMp3Player, iNumRIX, fLoop, flFadeTime);
        SDL_UnlockAudioDevice(gAudioDevice->id);
        if (ret) iNumRIX = 0;
        else gAudioDevice->pMp3Player->iMusic = 0;
    }

    if (gConfig->eMIDISynth == SYNTH_NATIVE && gConfig->eMusicType == MUSIC_MIDI)//38 35 =0
    {
        MIDI_Play(iNumRIX, fLoop);
        return;
    }



    //AUDIO_Lock();
    SDL_LockAudioDevice(gAudioDevice->id);
    if (gAudioDevice->pMusPlayer)
    {
        (this->*gAudioDevice->pMusPlayer->Play)(gAudioDevice->pMusPlayer, iNumRIX, fLoop, flFadeTime);
    }
    //AUDIO_Unlock()
    SDL_UnlockAudioDevice(gAudioDevice->id);

}

VOID CSound::AUDIO_PlayMusic(INT iNumRIX, BOOL fLoop, FLOAT flFadeTime)
{
    AudioPlay.locked = 1;
    AudioPlay.Music = iNumRIX;
    AudioPlay.musicLoop = fLoop;
    AudioPlay.flFadeTime = flFadeTime;
    AudioPlay.locked = 0;
}

VOID CSound::AUDIO_PlaySound(INT iSoundNum)
/*++
  Purpose:

    Play a sound in voc.mkf/sounds.mkf file.

  Parameters:

    [IN]  iSoundNum - number of the sound; the absolute value is used.

  Return value:

    None.

--*/
{
    // Unlike musics that use the 'load as required' strategy, sound player
    // load the entire sound file at once, which may cause about 0.5s or longer
    // latency for large sound files. To prevent this latency affects audio playing,
    // the mutex lock is obtained inside the SOUND_Play function rather than here.
    AudioPlay.locked = 1;
    AudioPlay.sound = iSoundNum;
    AudioPlay.locked = 0;

}

BOOL CSound::RIX_Play(
    VOID* object,
    INT       iNumRIX,
    BOOL      fLoop,
    FLOAT     flFadeTime
)
/*++
    Purpose:

    Start playing the specified music.

    Parameters:

    [IN]  iNumRIX - number of the music. 0 to stop playing current music.

    [IN]  fLoop - Whether the music should be looped or not.

    [IN]  flFadeTime - the fade in/out time when switching music.

    Return value:

    None.

--*/
{
    LPRIXPLAYER pRixPlayer = (LPRIXPLAYER)object;

    //
    // Check for NULL pointer.
    //
    if (pRixPlayer == NULL)
    {
        return FALSE;
    }

    if (iNumRIX == pRixPlayer->iMusic && pRixPlayer->iNextMusic == -1)
    {
        /* Will play the same music without any pending play changes,
           just change the loop attribute */
        pRixPlayer->fLoop = fLoop;
        return TRUE;
    }

    if (pRixPlayer->FadeType != RIXPLAYER::FADE_OUT)
    {
        if (pRixPlayer->FadeType == RIXPLAYER::FADE_IN && pRixPlayer->iTotalFadeInSamples > 0 && pRixPlayer->iRemainingFadeSamples > 0)
        {
            pRixPlayer->dwStartFadeTime = SDL_GetTicks_New() - (int)((float)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeInSamples * flFadeTime * (1000 / 2));
        }
        else
        {
            pRixPlayer->dwStartFadeTime = SDL_GetTicks_New();
        }
        pRixPlayer->iTotalFadeOutSamples = (int)round(flFadeTime / 2.0f * gConfig->iSampleRate) * gConfig->iAudioChannels;
        pRixPlayer->iRemainingFadeSamples = pRixPlayer->iTotalFadeOutSamples;
        pRixPlayer->iTotalFadeInSamples = pRixPlayer->iTotalFadeOutSamples;
    }
    else
    {
        pRixPlayer->iTotalFadeInSamples = (int)round(flFadeTime / 2.0f * gConfig->iSampleRate) * gConfig->iAudioChannels;
    }

    pRixPlayer->iNextMusic = iNumRIX;
    pRixPlayer->FadeType = RIXPLAYER::FADE_OUT;
    pRixPlayer->fNextLoop = fLoop;
    pRixPlayer->fReady = TRUE;

    return TRUE;
}


VOID SDLCALL CSound::AUDIO_FillBuffer(LPVOID udata, LPBYTE stream, INT len)
/*++
  Purpose:

    SDL sound callback function.

  Parameters:

    [IN]  udata - pointer to user-defined parameters (Not used).

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

  Return value:

    None.

--*/
{
    //if (!this) return;//
    memset(stream, 0, len);

    //
    // Play music
    //
    //if (gAudioDevice->fMusicEnabled && gAudioDevice->iMusicVolume > 0)
    if (!g_fNoMusic && gAudioDevice->bMusicVolume > 0)
    {
        if (gAudioDevice->pMp3Player && gAudioDevice->pMp3Player->iMusic > 0)
        {
            (this->*gAudioDevice->pMp3Player->FillBuffer)(gAudioDevice->pMp3Player, stream, len);
        }
        
        else
        if (gAudioDevice->pMusPlayer)
        {
            (this->*gAudioDevice->pMusPlayer->FillBuffer)(gAudioDevice->pMusPlayer, stream, len);
        }

        //
        // Adjust volume for music
        //
        AUDIO_AdjustVolume((short*)stream, gAudioDevice->bMusicVolume, len >> 1);
    }

    //
    // Play sound
    //
    //if (gAudioDevice->fSoundEnabled && gAudioDevice->pSoundPlayer && gAudioDevice->iSoundVolume > 0)
    if (!g_fNoSound && gAudioDevice->pSoundPlayer && gAudioDevice->bSoundVolume > 0)
    {
        memset(gAudioDevice->pSoundBuffer, 0, len);

        (this->*gAudioDevice->pSoundPlayer->FillBuffer)(gAudioDevice->pSoundPlayer, (LPBYTE)gAudioDevice->pSoundBuffer, len);

        //
        // Adjust volume for sound
        //
        AUDIO_AdjustVolume((short*)gAudioDevice->pSoundBuffer, gAudioDevice->bSoundVolume, len >> 1);

        //
        // Mix sound & music
        //
        AUDIO_MixNative((short*)stream, (short*)gAudioDevice->pSoundBuffer, len >> 1);
    }

    //
    // Play sound for AVI
    //
    CAviPlay::AVI_FillAudioBuffer(CAviPlay::AVI_GetPlayState(), (LPBYTE)stream, len);
}

VOID CSound::PAL_PlayMUS(INT iNumRIX, BOOL fLoop, FLOAT flFadeTime)
{
    return AUDIO_PlayMusic(iNumRIX, fLoop, flFadeTime);
}

//返回设备驱动地址
struct  SDL_AudioSpec*
    CSound::AUDIO_GetDeviceSpec(
        VOID
    )
{
    return &gAudioDevice->spec;
}

VOID CSound::SoundRunThread()
{
    thread _soundThread([&]()->void {
        soundRun();
        });
    _soundThread.detach();

}
