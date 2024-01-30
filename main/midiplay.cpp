/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


#include <SDL2\SDL.h>
#include "csound.h"
#if 0 //因有内存泄露 ，启用旧的版本

#include "../sound/native_midi/native_midi.h"
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#ifdef _DEBUG
//#define new new(_NORMAL_BLOCK , __FILE__ , __LINE__)
#define new DEBUG_NEW
#endif

static int  g_iMidiCurrent = -1;
static NativeMidiSong* g_pMidi = NULL;
static int  g_iMidiVolume = PAL_MAX_VOLUME;


VOID CSound::MIDI_Play(int iNumRIX, BOOL fLoop)
{
#if PAL_HAS_NATIVEMIDI
	if (!native_midi_detect())
		return;

	if (native_midi_active(g_pMidi) && iNumRIX == g_iMidiCurrent)
	{
		return;
	}

	native_midi_stop(g_pMidi);
	native_midi_freesong(g_pMidi);
	g_pMidi = NULL;
	g_iMidiCurrent = -1;

	if ( iNumRIX <= 0)
	{
		return;
	}

	if (gConfig && gConfig->fIsWIN95)
	{
		string s = PalDir + va("musics\\%3.3d.mid", iNumRIX);
		g_pMidi = native_midi_loadsong(s.c_str());
	}

	if (!g_pMidi)
	{
		FILE* fp = NULL;
		uint8_t* buf = NULL;
		int      size;

		if ((fp = UTIL_OpenFile("midi.mkf")) != NULL)
		{
			if ((size = PAL_MKFGetChunkSize(iNumRIX, fp)) > 0 &&
				(buf = (uint8_t*)malloc(size)))
			{
				PAL_MKFReadChunk(buf, size, iNumRIX, fp);
			}
			fclose(fp);
		}

		if (buf)
		{
			SDL_RWops* rw = SDL_RWFromConstMem(buf, size);
			g_pMidi = native_midi_loadsong_RW(rw);
			SDL_RWclose(rw);
			free(buf);
		}
	}

	if (g_pMidi)
	{
		MIDI_SetVolume(g_iMidiVolume * 0.8);
		native_midi_start(g_pMidi, fLoop);
		g_iMidiCurrent = iNumRIX;
	}
#endif
}

VOID CSound::MIDI_SetVolume(int iVolume)
{
#if PAL_HAS_NATIVEMIDI
	g_iMidiVolume = iVolume;
	if (g_pMidi)
	{
		native_midi_setvolume(g_pMidi, iVolume * 127 / PAL_MAX_VOLUME);
	}
#endif
}
#else
//旧的版本

#include <SDL2\SDL_rwops.h>
#include<mmeapi.h>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define BE_SHORT(x) (x)
#define BE_LONG(x) (x)
#else
#define BE_SHORT(x)	((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#define BE_LONG(x)	((((x)&0x0000FF)<<24) | \
			 (((x)&0x00FF00)<<8) | \
			 (((x)&0xFF0000)>>8) | \
			 (((x)>>24)&0xFF))
#endif

struct _NativeMidiSong
{
	int MusicLoaded{};
	int MusicPlaying{};
	MIDIHDR MidiStreamHdr{};
	MIDIEVENT* NewEvents{};
	Uint16 ppqn{};
	int Size{};
	int NewPos{};
};

typedef struct
{
	Uint8* data;					/* MIDI message stream */
	int len;						/* length of the track data */
} MIDITrack;

typedef struct
{
	int division;					/* number of pulses per quarter note (ppqn) */
	int nTracks;                    /* number of tracks */
	MIDITrack* track;               /* tracks */
} MIDIFile;

typedef struct _NativeMidiSong NativeMidiSong;

#include <SDL2\SDL.h>

/* Midi Status Bytes */
#define MIDI_STATUS_NOTE_OFF	0x8
#define MIDI_STATUS_NOTE_ON	0x9
#define MIDI_STATUS_AFTERTOUCH	0xA
#define MIDI_STATUS_CONTROLLER	0xB
#define MIDI_STATUS_PROG_CHANGE	0xC
#define MIDI_STATUS_PRESSURE	0xD
#define MIDI_STATUS_PITCH_WHEEL	0xE
#define MIDI_STATUS_SYSEX	0xF

/* We store the midi events in a linked list; this way it is
   easy to shuffle the tracks together later on; and we are
   flexible in the size of each elemnt.
 */
typedef struct MIDIEvent
{
	Uint32	time;		/* Time at which this midi events occurs */
	Uint8	status;		/* Status byte */
	Uint8	data[2];	/* 1 or 2 bytes additional data for most events */

	Uint32	extraLen;	/* For some SysEx events, we need additional storage */
	Uint8* extraData;

	struct MIDIEvent* next;
} MIDIEvent;


/* Load a midifile to memory, converting it to a list of MIDIEvents.
   This function returns a linked lists of MIDIEvents, 0 if an error occured.
 */
MIDIEvent* CreateMIDIEventList(SDL_RWops* rw, Uint16* division);

/* Release a MIDIEvent list after usage. */
void FreeMIDIEventList(MIDIEvent* head);

static UINT MidiDevice = MIDI_MAPPER;
static HMIDISTRM hMidiStream;
static NativeMidiSong* currentsong;


int native_midi_detect();
NativeMidiSong* native_midi_loadsong_1(const char* midifile);
NativeMidiSong* native_midi_loadsong_RW(SDL_RWops* rw);
void native_midi_freesong(NativeMidiSong* song);
void native_midi_start(NativeMidiSong* song);
void native_midi_stop();
int  native_midi_active();
void native_midi_setvolume(int volume);
const char* native_midi_error(void);

static int BlockOut(NativeMidiSong* song)
{
	MMRESULT err;
	int BlockSize;

	if ((song->MusicLoaded) && (song->NewEvents))
	{
		// proff 12/8/98: Added for savety
		midiOutUnprepareHeader((HMIDIOUT)hMidiStream, &song->MidiStreamHdr, sizeof(MIDIHDR));
		if (song->NewPos >= song->Size)
			return 0;
		BlockSize = (song->Size - song->NewPos);
		if (BlockSize <= 0)
			return 0;
		if (BlockSize > 36000)
			BlockSize = 36000;
		song->MidiStreamHdr.lpData = (LPSTR)((unsigned char*)song->NewEvents + song->NewPos);
		song->NewPos += BlockSize;
		song->MidiStreamHdr.dwBufferLength = BlockSize;
		song->MidiStreamHdr.dwBytesRecorded = BlockSize;
		song->MidiStreamHdr.dwFlags = 0;
		err = midiOutPrepareHeader((HMIDIOUT)hMidiStream, &song->MidiStreamHdr, sizeof(MIDIHDR));
		if (err != MMSYSERR_NOERROR)
			return 0;
		err = midiStreamOut(hMidiStream, &song->MidiStreamHdr, sizeof(MIDIHDR));
		return 0;
	}
	return 1;
}

static int ReadMIDIFile(MIDIFile* mididata, SDL_RWops* rw)
{
	int i = 0;
	Uint32	ID;
	Uint32	size;
	Uint16	format;
	Uint16	tracks;
	Uint16	division;

	if (!mididata)
		return 0;
	if (!rw)
		return 0;

	/* Make sure this is really a MIDI file */
	SDL_RWread(rw, &ID, 1, 4);
	if (BE_LONG(ID) != 'MThd')
		return 0;

	/* Header size must be 6 */
	SDL_RWread(rw, &size, 1, 4);
	size = BE_LONG(size);
	if (size != 6)
		return 0;

	/* We only support format 0 and 1, but not 2 */
	SDL_RWread(rw, &format, 1, 2);
	format = BE_SHORT(format);
	if (format != 0 && format != 1)
		return 0;

	SDL_RWread(rw, &tracks, 1, 2);
	tracks = BE_SHORT(tracks);
	mididata->nTracks = tracks;

	/* Allocate tracks */
	mididata->track = (MIDITrack*)calloc(1, sizeof(MIDITrack) * mididata->nTracks);
	if (NULL == mididata->track)
	{
		printf("Out of memory");
		goto bail;
	}

	/* Retrieve the PPQN value, needed for playback */
	SDL_RWread(rw, &division, 1, 2);
	mididata->division = BE_SHORT(division);


	for (i = 0; i < tracks; i++)
	{
		SDL_RWread(rw, &ID, 1, 4);	/* We might want to verify this is MTrk... */
		SDL_RWread(rw, &size, 1, 4);
		size = BE_LONG(size);
		mididata->track[i].len = size;
		mididata->track[i].data = (UINT8*)malloc(size);
		if (NULL == mididata->track[i].data)
		{
			printf("Out of memory");
			goto bail;
		}
		SDL_RWread(rw, mididata->track[i].data, 1, size);
	}
	return 1;

bail:
	for (; i >= 0; i--)
	{
		if (mididata->track[i].data)
			free(mididata->track[i].data);
	}

	return 0;
}

/* Get Variable Length Quantity */
static int GetVLQ(MIDITrack* track, int* currentPos)
{
	int l = 0;
	Uint8 c;
	while (1)
	{
		c = track->data[*currentPos];
		(*currentPos)++;
		l += (c & 0x7f);
		if (!(c & 0x80))
			return l;
		l <<= 7;
	}
}


static MIDIEvent* createEvent(Uint32 time, Uint8 event, Uint8 a, Uint8 b)
{
	MIDIEvent* newEvent;

	newEvent = (MIDIEvent*)calloc(1, sizeof(MIDIEvent));
	if (newEvent)
	{
		newEvent->time = time;
		newEvent->status = event;
		newEvent->data[0] = a;
		newEvent->data[1] = b;
	}
	else
		printf("Out of memory");

	return newEvent;
}

/* Convert a single midi track to a list of MIDIEvents */
static MIDIEvent* MIDITracktoStream(MIDITrack* track)
{
	Uint32 atime = 0;
	Uint32 len = 0;
	Uint8 event, type, a, b;
	Uint8 laststatus = 0;
	Uint8 lastchan = 0;
	int currentPos = 0;
	int end = 0;
	MIDIEvent* head = (MIDIEvent*)createEvent(0, 0, 0, 0);	/* dummy event to make handling the list easier */
	MIDIEvent* currentEvent = head;

	while (!end)
	{
		if (currentPos >= track->len)
			break; /* End of data stream reached */

		atime += GetVLQ(track, &currentPos);
		event = track->data[currentPos++];

		/* Handle SysEx seperatly */
		if (((event >> 4) & 0x0F) == MIDI_STATUS_SYSEX)
		{
			if (event == 0xFF)
			{
				type = track->data[currentPos];
				currentPos++;
				switch (type)
				{
				case 0x2f: /* End of data marker */
					end = 1;
				case 0x51: /* Tempo change */
					/*
					a=track->data[currentPos];
					b=track->data[currentPos+1];
					c=track->data[currentPos+2];
					AddEvent(song, atime, MEVT_TEMPO, c, b, a);
					*/
					break;
				}
			}
			else
				type = 0;

			len = GetVLQ(track, &currentPos);

			/* Create an event and attach the extra data, if any */
			currentEvent->next = (MIDIEvent*)createEvent(atime, event, type, 0);
			currentEvent = currentEvent->next;
			if (NULL == currentEvent)
			{
				FreeMIDIEventList(head);
				return NULL;
			}
			if (len)
			{
				currentEvent->extraLen = len;
				currentEvent->extraData = (Uint8*)malloc(len);
				memcpy(currentEvent->extraData, &(track->data[currentPos]), len);
				currentPos += len;
			}
		}
		else
		{
			a = event;
			if (a & 0x80) /* It's a status byte */
			{
				/* Extract channel and status information */
				lastchan = a & 0x0F;
				laststatus = (a >> 4) & 0x0F;

				/* Read the next byte which should always be a data byte */
				a = track->data[currentPos++] & 0x7F;
			}
			switch (laststatus)
			{
			case MIDI_STATUS_NOTE_OFF:
			case MIDI_STATUS_NOTE_ON: /* Note on */
			case MIDI_STATUS_AFTERTOUCH: /* Key Pressure */
			case MIDI_STATUS_CONTROLLER: /* Control change */
			case MIDI_STATUS_PITCH_WHEEL: /* Pitch wheel */
				b = track->data[currentPos++] & 0x7F;
				currentEvent->next = createEvent(atime, (Uint8)((laststatus << 4) + lastchan), a, b);
				currentEvent = currentEvent->next;
				if (NULL == currentEvent)
				{
					FreeMIDIEventList(head);
					return NULL;
				}
				break;

			case MIDI_STATUS_PROG_CHANGE: /* Program change */
			case MIDI_STATUS_PRESSURE: /* Channel pressure */
				a &= 0x7f;
				currentEvent->next = (MIDIEvent*)createEvent(atime, (Uint8)((laststatus << 4) + lastchan), a, 0);
				currentEvent = currentEvent->next;
				if (NULL == currentEvent)
				{
					FreeMIDIEventList(head);
					return NULL;
				}
				break;

			default: /* Sysex already handled above */
				break;
			}
		}
	}

	currentEvent = head->next;
	free(head);	/* release the dummy head event */
	return currentEvent;
}

static MIDIEvent* MIDItoStream(MIDIFile* mididata)
{
	MIDIEvent** track;
	MIDIEvent* head = (MIDIEvent*)createEvent(0, 0, 0, 0);	/* dummy event to make handling the list easier */
	MIDIEvent* currentEvent = head;
	int trackID;

	if (NULL == head)
		return NULL;

	track = (MIDIEvent**)calloc(1, sizeof(MIDIEvent*) * mididata->nTracks);
	if (NULL == head)
		return NULL;

	/* First, convert all tracks to MIDIEvent lists */
	for (trackID = 0; trackID < mididata->nTracks; trackID++)
		track[trackID] = MIDITracktoStream(&mididata->track[trackID]);

	/* Now, merge the lists. */
	/* TODO */
	while (1)
	{
		Uint32 lowestTime = INT_MAX;
		int currentTrackID = -1;

		/* Find the next event */
		for (trackID = 0; trackID < mididata->nTracks; trackID++)
		{
			if (track[trackID] && (track[trackID]->time < lowestTime))
			{
				currentTrackID = trackID;
				lowestTime = track[currentTrackID]->time;
			}
		}

		/* Check if we processes all events */
		if (currentTrackID == -1)
			break;

		currentEvent->next = track[currentTrackID];
		track[currentTrackID] = track[currentTrackID]->next;

		currentEvent = currentEvent->next;


		lowestTime = 0;
	}

	/* Make sure the list is properly terminated */
	currentEvent->next = 0;

	currentEvent = head->next;
	free(track);
	free(head);	/* release the dummy head event */
	return currentEvent;
}

static void MIDItoStream(NativeMidiSong* song, MIDIEvent* evntlist)
{
	int eventcount;
	MIDIEvent* event;
	MIDIEVENT* newevent;

	eventcount = 0;
	event = evntlist;
	while (event)
	{
		eventcount++;
		event = event->next;
	}
	song->NewEvents = (MIDIEVENT*)malloc(eventcount * 3 * sizeof(DWORD));
	if (!song->NewEvents)
		return;
	memset(song->NewEvents, 0, (eventcount * 3 * sizeof(DWORD)));

	eventcount = 0;
	event = evntlist;
	newevent = song->NewEvents;
	while (event)
	{
		int status = (event->status & 0xF0) >> 4;
		switch (status)
		{
		case MIDI_STATUS_NOTE_OFF:
		case MIDI_STATUS_NOTE_ON:
		case MIDI_STATUS_AFTERTOUCH:
		case MIDI_STATUS_CONTROLLER:
		case MIDI_STATUS_PROG_CHANGE:
		case MIDI_STATUS_PRESSURE:
		case MIDI_STATUS_PITCH_WHEEL:
			newevent->dwDeltaTime = event->time;
			newevent->dwEvent = (event->status | 0x80) | (event->data[0] << 8) | (event->data[1] << 16) | (MEVT_SHORTMSG << 24);
			newevent = (MIDIEVENT*)((char*)newevent + (3 * sizeof(DWORD)));
			eventcount++;
			break;

		case MIDI_STATUS_SYSEX:
			if (event->status == 0xFF && event->data[0] == 0x51) /* Tempo change */
			{
				int tempo = (event->extraData[0] << 16) |
					(event->extraData[1] << 8) |
					event->extraData[2];
				newevent->dwDeltaTime = event->time;
				newevent->dwEvent = (MEVT_TEMPO << 24) | tempo;
				newevent = (MIDIEVENT*)((char*)newevent + (3 * sizeof(DWORD)));
				eventcount++;
			}
			break;
		}

		event = event->next;
	}

	song->Size = eventcount * 3 * sizeof(DWORD);

	{
		int time;
		int temptime;

		song->NewPos = 0;
		time = 0;
		newevent = song->NewEvents;
		while (song->NewPos < song->Size)
		{
			temptime = newevent->dwDeltaTime;
			newevent->dwDeltaTime -= time;
			time = temptime;
			if ((song->NewPos + 12) >= song->Size)
				newevent->dwEvent |= MEVT_F_CALLBACK;
			newevent = (MIDIEVENT*)((char*)newevent + (3 * sizeof(DWORD)));
			song->NewPos += 12;
		}
	}
	song->NewPos = 0;
	song->MusicLoaded = 1;
}

void CALLBACK MidiProc(HMIDIIN hMidi, UINT uMsg, unsigned long dwInstance,
	unsigned long dwParam1, unsigned long dwParam2)
{
	switch (uMsg)
	{
	case MOM_DONE:
		if ((currentsong->MusicLoaded) && (dwParam1 == (unsigned long)&currentsong->MidiStreamHdr))
			BlockOut(currentsong);
		break;
	case MOM_POSITIONCB:
		if ((currentsong->MusicLoaded) && (dwParam1 == (unsigned long)&currentsong->MidiStreamHdr))
			currentsong->MusicPlaying = 0;
		break;
	default:
		break;
	}
}

int native_midi_detect()
{
	MMRESULT merr;
	HMIDISTRM MidiStream;

	merr = midiStreamOpen(&MidiStream, &MidiDevice, (DWORD)1, 
		(unsigned long)MidiProc, (unsigned long)0, CALLBACK_FUNCTION);
	if (merr != MMSYSERR_NOERROR)
		return 0;
	midiStreamClose(MidiStream);
	return 1;
}

MIDIEvent* CreateMIDIEventList(SDL_RWops* rw, Uint16* division)
{
	MIDIFile* mididata = NULL;
	MIDIEvent* eventList;
	int trackID;

	mididata = (MIDIFile*)calloc(1, sizeof(MIDIFile));
	if (!mididata)
		return NULL;

	/* Open the file */
	if (rw != NULL)
	{
		/* Read in the data */
		if (!ReadMIDIFile(mididata, rw))
		{
			free(mididata);
			return NULL;
		}
	}
	else
	{
		free(mididata);
		return NULL;
	}

	if (division)
		*division = mididata->division;

	eventList = MIDItoStream(mididata);

	for (trackID = 0; trackID < mididata->nTracks; trackID++)
	{
		if (mididata->track[trackID].data)
			free(mididata->track[trackID].data);
	}
	free(mididata->track);
	free(mididata);

	return eventList;
}

void FreeMIDIEventList(MIDIEvent* head)
{
	MIDIEvent* cur, * next;

	cur = head;

	while (cur)
	{
		next = cur->next;
		if (cur->extraData)
			free(cur->extraData);
		free(cur);
		cur = next;
	}
}

NativeMidiSong* native_midi_loadsong_1(const char* midifile)
{
	NativeMidiSong* newsong;
	MIDIEvent* evntlist = NULL;
	SDL_RWops* rw;

	newsong = (NativeMidiSong*)malloc(sizeof(NativeMidiSong));
	if (!newsong)
		return NULL;
	memset(newsong, 0, sizeof(NativeMidiSong));

	/* Attempt to load the midi file */
	rw = SDL_RWFromFile(midifile, "rb");
	if (rw) {
		evntlist = CreateMIDIEventList(rw, &newsong->ppqn);
		SDL_RWclose(rw);
		if (!evntlist)
		{
			free(newsong);
			return NULL;
		}
	}

	MIDItoStream(newsong, evntlist);

	FreeMIDIEventList(evntlist);

	return newsong;
}

NativeMidiSong* native_midi_loadsong_RW(SDL_RWops* rw)
{
	NativeMidiSong* newsong;
	MIDIEvent* evntlist = NULL;

	newsong = (NativeMidiSong*)malloc(sizeof(NativeMidiSong));
	if (!newsong)
		return NULL;
	memset(newsong, 0, sizeof(NativeMidiSong));

	/* Attempt to load the midi file */
	evntlist = CreateMIDIEventList(rw, &newsong->ppqn);
	if (!evntlist)
	{
		free(newsong);
		return NULL;
	}

	MIDItoStream(newsong, evntlist);

	FreeMIDIEventList(evntlist);

	return newsong;
}

void  native_midi_freesong(NativeMidiSong* song)
{
	if (hMidiStream)
	{
		midiStreamStop(hMidiStream);
		midiStreamClose(hMidiStream);
	}
	if (song)
	{
		if (song->NewEvents)
			free(song->NewEvents);
		free(song);
	}
}

void native_midi_start(NativeMidiSong* song)
{
	MMRESULT merr;
	MIDIPROPTIMEDIV mptd;

	native_midi_stop();
	if (!hMidiStream)
	{
		merr = midiStreamOpen(&hMidiStream, &MidiDevice, 1,(DWORD_PTR) & MidiProc, 0, CALLBACK_FUNCTION);
		if (merr != MMSYSERR_NOERROR)
		{
			hMidiStream = 0;
			return;
		}
		//midiStreamStop(hMidiStream);
		currentsong = song;
		currentsong->NewPos = 0;
		currentsong->MusicPlaying = 1;
		mptd.cbStruct = sizeof(MIDIPROPTIMEDIV);
		mptd.dwTimeDiv = currentsong->ppqn;
		merr = midiStreamProperty(hMidiStream, (LPBYTE)&mptd, MIDIPROP_SET | MIDIPROP_TIMEDIV);
		BlockOut(song);
		merr = midiStreamRestart(hMidiStream);
	}
}

void native_midi_stop()
{
	if (!hMidiStream)
		return;
	midiStreamStop(hMidiStream);
	midiStreamClose(hMidiStream);
	currentsong = NULL;
	hMidiStream = 0;
}

int native_midi_active()
{
	return currentsong->MusicPlaying;
}

void native_midi_setvolume(int volume)
{
	int calcVolume;
	if (volume > 128)
		volume = 128;
	if (volume < 0)
		volume = 0;
	calcVolume = (65535 * volume / 128);

	midiOutSetVolume((HMIDIOUT)hMidiStream, MAKELONG(calcVolume, calcVolume));
}

const char* native_midi_error(void)
{
	return "";
}


static INT iMidCurrent = -1;
static BOOL fMidLoop = FALSE;
static NativeMidiSong* g_pMidi = NULL;

NativeMidiSong* native_midi_loadsong(const char* midifile)
{
	NativeMidiSong* newsong;
	MIDIEvent* evntlist = NULL;
	SDL_RWops* rw;

	newsong = (NativeMidiSong*)malloc(sizeof(NativeMidiSong));
	if (!newsong)
		return NULL;
	memset(newsong, 0, sizeof(NativeMidiSong));

	/* Attempt to load the midi file */
	rw = SDL_RWFromFile(midifile, "rb");
	if (rw) {
		evntlist = CreateMIDIEventList(rw, &newsong->ppqn);
		SDL_RWclose(rw);
		if (!evntlist)
		{
			free(newsong);
			return NULL;
		}
	}

	MIDItoStream(newsong, evntlist);

	FreeMIDIEventList(evntlist);

	return newsong;
}

VOID CSound::MIDI_Play(int iNumRIX, BOOL fLoop)


/*++
  Purpose:

  Start playing the specified music in MIDI format.

  Parameters:

  [IN]  iNumRIX - number of the music. 0 to stop playing current music.

  [IN]  fLoop - Whether the music should be looped or not.

  Return value:

  None.

  --*/
{
	FILE* fp{nullptr};
	unsigned char* buf{ nullptr };
	int            size{};
	SDL_RWops* rw{};
	char             filename[1024]{ 0 };

	if (g_pMidi != NULL && iNumRIX == iMidCurrent && native_midi_active())
	{
		return;
	}
	//SOUND_PlayCDA(-1);
	native_midi_freesong(g_pMidi);
	g_pMidi = NULL;
	iMidCurrent = -1;

	if (g_fNoMusic || iNumRIX <= 0)
	{
		return;
	}
	if (gConfig && gConfig->fIsWIN95)
	{
		string s = PalDir + va("musics\\%3.3d.mid", iNumRIX);
		g_pMidi = native_midi_loadsong(s.c_str());
	}

	if (!g_pMidi)
	{
		//string s = PalDir + "midi.mkf";
		fp = UTIL_OpenFile("midi.mkf");
		if (fp == NULL)
		{
			return;
		}

		if (iNumRIX > PAL_MKFGetChunkCount(fp))
		{
			fclose(fp);
			return;
		}

		size = PAL_MKFGetChunkSize(iNumRIX, fp);
		if (size <= 0)
		{
			fclose(fp);
			return;
		}

		buf = (unsigned char*)malloc(size);

		PAL_MKFReadChunk((LPBYTE)buf, size, iNumRIX, fp);
		fclose(fp);

		rw = SDL_RWFromConstMem((const void*)buf, size);

		g_pMidi = native_midi_loadsong_RW(rw);
	}

	if (g_pMidi != NULL)
	{
		native_midi_start(g_pMidi);

		iMidCurrent = iNumRIX;
		fMidLoop = fLoop;
	}
	if (rw)
		SDL_RWclose(rw);
	if (buf)
		free(buf);
}



#endif