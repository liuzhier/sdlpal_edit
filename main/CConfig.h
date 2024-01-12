#ifndef CCONFIG_H
#define CCONFIG_H
#pragma once

#include "../s/stdafx.h"

#include <SDL.h>
#include "command.h"
#include "../sound/adplug/opltypes.h"
#include <string>

typedef enum tagLOGLEVEL
{
	LOGLEVEL_MIN,
	LOGLEVEL_VERBOSE = LOGLEVEL_MIN,
	LOGLEVEL_DEBUG,
	LOGLEVEL_INFO,
	LOGLEVEL_WARNING,
	LOGLEVEL_ERROR,
	LOGLEVEL_FATAL,
	LOGLEVEL_MAX = LOGLEVEL_FATAL,
} LOGLEVEL;

typedef enum tagMUSICTYPE
{
	MUSIC_MIDI,
	MUSIC_RIX,
	MUSIC_MP3,
	MUSIC_OGG,
	MUSIC_OPUS
} MUSICTYPE, * LPMUSICTYPE;


typedef enum tagMIDISYNTHTYPE
{
	SYNTH_NATIVE,
	SYNTH_TIMIDITY,
	SYNTH_TINYSOUNDFONT
} MIDISYNTHTYPE, * LPMIDISYNTHTYPE;

#define configStatMark 27

#define iAudioDevice		m_Function_Set[configStatMark + 0] //声音设备号
#define iSurroundOPLOffset	m_Function_Set[configStatMark + 1] //围绕OPL抵消384
#define iAudioChannels		m_Function_Set[configStatMark + 2] //连接数 
#define iSampleRate			m_Function_Set[configStatMark + 3] //采样率 44100
#define iOPLSampleRate		m_Function_Set[configStatMark + 4] //OPL频率 49716
#define iResampleQuality	m_Function_Set[configStatMark + 5] //重新取样质量,重新取样质量最大
#define iMusicVolume		m_Function_Set[configStatMark + 6] //音乐音量
#define iSoundVolume		m_Function_Set[configStatMark + 7] //iSoundVolume
#define eMusicType			m_Function_Set[configStatMark + 8] //音乐类型
#define eOPLCore			m_Function_Set[configStatMark + 9] //OPL核心
#define eOPLChip			m_Function_Set[configStatMark + 10]
#define eMIDISynth			m_Function_Set[configStatMark + 11] // MIDI设置
#define fUseMp3				m_Function_Set[configStatMark + 12] //优先使用MP3
#define fIsUseBig5			m_Function_Set[configStatMark + 13] //UseBig5			
#define fUseSurroundOPL		m_Function_Set[configStatMark + 14] //使用环绕OPL
#define fKeepAspectRatio	m_Function_Set[configStatMark + 15] //保持比例
#define fEnableAviPlay		m_Function_Set[configStatMark + 16] //是否过场动画AVI
#define wAudioBufferSize	m_Function_Set[configStatMark + 17] // 

class CConfig
{
public:
	INT				 iLogLevel = LOGLEVEL::LOGLEVEL_ERROR;//日志级别
	BOOL             fIsWIN95 = 0;//
	BOOL			 fisUSEYJ1DeCompress = 1;
	BOOL             fFullScreen = 0;//满屏
	BOOL             fEnableJoyStick = 0;//使用游戏使操纵杆
	BOOL             fEnableKeyRepeat = 0;//重复按键
	BOOL             fUseTouchOverlay = 0;//触屏辅助
	
	INT			is_KO_ENEMY_FAST{ 0 };
	INT			is_INVINCIBLE{ 0 };
	INT			is_SHOW_OBJECT{ 0 };
	INT			is_TEST{ 0 };
	INT			is_SHOW{ 0 };
	INT			is_ShowScript[20]{ 0 };
	INT32			m_Function_Set[120]{ 0 };//功能设置
	//friend		DWORD32 SDL_GetTicks_New();
public:
	CConfig();
	~CConfig();
	INT loadConfig(class CScript * Pal);
};

DWORD32 SDL_GetTicks_New();

#endif