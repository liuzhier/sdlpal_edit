#include "csound.h"

//mp3play
#include "../sound/libmad/music_mad.h"
#include "../sound/libmad/resampler.h"

typedef struct tagAUDIOPLAYER
{
	INT                        iMusic;
	BOOL                       fLoop;
	VOID(CSound::* Shutdown)(VOID*);
	BOOL(CSound::* Play)(VOID*, INT, BOOL, FLOAT);
	VOID(CSound::* FillBuffer)(VOID*, LPBYTE, INT);
} AUDIOPLAYER, * LPAUDIOPLAYER;

typedef struct tagMP3PLAYER
{
	INT                        iMusic;
	BOOL                       fLoop;
	VOID(CSound::* Shutdown)(VOID*);
	BOOL(CSound::* Play)(VOID*, INT, BOOL, FLOAT);
	VOID(CSound::* FillBuffer)(VOID*, LPBYTE, INT);
	mad_data* pMP3;
} MP3PLAYER, * LPMP3PLAYER;

VOID CSound::MP3_Close(struct tagMP3PLAYER* player)
{
	if (player->pMP3)
	{
		mad_stop(player->pMP3);
		mad_closeFile(player->pMP3);

		player->pMP3 = NULL;
	}
}

VOID CSound::MP3_FillBuffer(VOID* object,LPBYTE stream,INT len)
{
	LPMP3PLAYER player = (LPMP3PLAYER)object;
	if (player->pMP3) {
		if (!mad_isPlaying(player->pMP3) && player->fLoop)
		{
			mad_seek(player->pMP3, 0);
			mad_start(player->pMP3);
		}

		if (mad_isPlaying(player->pMP3))
			mad_getSamples(player->pMP3, stream, len);
	}
}

VOID CSound::MP3_Shutdown(VOID* object)
{
	if (object)
	{
		MP3_Close((LPMP3PLAYER)object);
		free(object);
	}
}

BOOL CSound::MP3_Play(VOID* object, INT iNum, BOOL fLoop, FLOAT flFadeTime)
{
	LPMP3PLAYER player = (LPMP3PLAYER)object;

	//
	// Check for NULL pointer.
	//
	if (player == NULL)
	{
		return FALSE;
	}

	player->fLoop = fLoop;

	if (iNum == player->iMusic)
	{
		return TRUE;
	}

	MP3_Close(player);

	player->iMusic = iNum;

	if (iNum > 0)
	{
		std::string fname = PalDir + va("/mp3/%2.2d.mp3", iNum);
		player->pMP3 = mad_openFile(fname.c_str(), AUDIO_GetDeviceSpec(), gConfig->iResampleQuality);

		if (player->pMP3)
		{
			mad_start(player->pMP3);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}
}

struct tagAUDIOPLAYER *CSound::MP3_Init(VOID)
{
	LPMP3PLAYER player;
	if ((player = (LPMP3PLAYER)malloc(sizeof(MP3PLAYER))) != NULL)
	{
		SDL_zero(*player);
		player->FillBuffer = &CSound::MP3_FillBuffer;
		player->Play = &CSound::MP3_Play;
		player->Shutdown = &CSound::MP3_Shutdown;

		player->pMP3 = NULL;
		player->iMusic = -1;
		player->fLoop = FALSE;
	}
	return (LPAUDIOPLAYER)player;
}
