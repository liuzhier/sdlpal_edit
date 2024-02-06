
//#include "cpalapp.h"
//本类定义文件系统相关

#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0
 
#include <sys/stat.h>
#include <SDL2\SDL.h>
#include <iostream>
#include <cassert>
#include "cpalapp.h"
#include "cgl_render.h"
#include "CConfig.h"
#include <memory>
using namespace std;

const LPCSTR PalFileName[] =
{
	"fbp.mkf",
	"mgo.mkf",
	"ball.mkf",
	"data.mkf",
	"f.mkf",
	"fire.mkf",
	"rgm.mkf",
	"sss.mkf",
	"map.mkf",
	"gop.mkf",
	nullptr,
};
 
CPalApp::CPalApp()
{
}

CPalApp::~CPalApp()
{
}

INT CPalApp::InitPalBase()
{
	char buf[512];
	GetCurrentDirectoryA(500, buf);
	BasePath = buf;
	
	if (PalDir.empty())
		PalDir = BasePath + "\\";
	
	if (!isCorrectDir(PalDir))
	{
		PalDir += "pal\\";
		if (!isCorrectDir(PalDir))
		{
			//MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, L"打开文件错！"
			//	, CString(PalDir.c_str()).GetBuffer(), MB_OK);
			wcerr << " 打开文件错！" << endl;
			return 1;;
		}
	}
	 
	//以下装入游戏数据
	if (!gpGlobals)
	{
		gpGlobals = new GLOBALVARS;
		SDL_memset(gpGlobals, 0, sizeof(GLOBALVARS));
		PAL_InitGlobals();
	}
	 
	//load config file
	gConfig->loadConfig(pCScript);

	gConfig->fisUSEYJ1DeCompress = is_Use_YJ1_Decompress();

	gConfig->fIsWIN95 = PAL_IsPalWIN();
	//save config file
	//gConfig->saveConfig(PalDir.c_str());

	PAL_InitGlobalGameData();
	gpGlobals->g.rgObject = UTIL_calloc(sizeof(OBJECT), MAX_OBJECTS);
	DWORD len = PAL_MKFGetChunkSize(2, gpGlobals->f.fpSSS);
	if (gConfig->fIsWIN95)
		gpGlobals->g.nObject = len / sizeof(OBJECT);
	else
		gpGlobals->g.nObject = len / sizeof(OBJECT_DOS);

	PAL_InitText();

	PAL_InitUI();

	PAL_LoadDefaultGame();
	gpGlobals->bIsBig5 = ((PAL_IsBig5(gConfig->m_Function_Set[49]) == 1));
	gpGlobals->lpObjectDesc = PAL_LoadObjectDesc("desc.dat");
	
	return 0;
}

VOID CPalApp::dstroyPalBase()
{
	PAL_FreeUI();
	PAL_FreeText();
	PAL_FreeGlobals();
}
// 判断文件是否存在
BOOL CPalApp::IsFileExist(const string& csFile)
{
    struct stat buf;
    stat(csFile.c_str(), &buf);
	int ret= buf.st_mode & _S_IFREG;
	return ret;
}

// 判断文件是否存在
BOOL CPalApp::IsDirExist(const string& csFile)
{
    struct stat buf;
    stat(csFile.c_str(), &buf);
    return buf.st_mode & _S_IFDIR;
}

BOOL CPalApp::isCorrectDir(const std::string & dir)
{
	
	string cdir;
	for (int i = 0; PalFileName[i]; i++)
	{
		cdir = dir;
		cdir += PalFileName[i];
		if(!IsFileExist(cdir))
			return FALSE;
	}

	//是正确的目录，往下进行
	return 1;
}

typedef struct tagSAVEDGAME_DOS
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower{};

	WORD	    	 bFinishGameTime{};         //未使用地址，用来记录共完成游戏多少次
	WORD			 rgbReserved[2]{};			// unused

	DWORD            dwCash;                  // amount of cash
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	BYTE			 rgbReserved2[320]{};		// poison status // 现在已经弃用
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
	OBJECT_DOS       rgObject[MAX_OBJECTS];
	EVENTOBJECT      rgEventObject[MAX_EVENT_OBJECTS];
	inline static const size_t getOffset(int ob,int isWin95 = 0)
	{
		switch (ob)
		{
		case 0://Object
			return sizeof(tagSAVEDGAME_DOS) - sizeof(rgEventObject) - sizeof(rgObject);
		case 1://EventObject
			if (isWin95)
				return sizeof(tagSAVEDGAME_DOS) - sizeof(rgEventObject) + MAX_OBJECTS * sizeof(WORD);
			else
				return sizeof(tagSAVEDGAME_DOS) - sizeof(rgEventObject);
		case 2://Scene
			return sizeof(tagSAVEDGAME_DOS) - sizeof(rgEventObject) - sizeof(rgObject) - sizeof(rgScene);
		default://Error
			return 0;
		}
	};
} SAVEDGAME_DOS, * LPSAVEDGAME_DOS;

typedef struct tagSAVEDGAME
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;

	WORD			 bFinishGameTime{};         //未使用地址，用来记录共完成游戏多少次
	WORD			 rgbReserved[2]{};			// unused
	
	DWORD            dwCash;                  // amount of cash
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	BYTE				rgbReserved2[320];		// poison status // 现在已经弃用
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
	OBJECT           rgObject[MAX_OBJECTS];
	EVENTOBJECT      rgEventObject[MAX_EVENT_OBJECTS];
} SAVEDGAME, * LPSAVEDGAME;

typedef struct tagSAVEDGAME_COMMON
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;

	WORD				bFinishGameTime{};         //未使用地址，用来记录共完成游戏多少次
	WORD				rgbReserved[2]{};			// unused

	DWORD            dwCash;                  // amount of cash

	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	BYTE			 rgbReserved2[320]{};		// poison status // 现在已经弃用
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
} SAVEDGAME_COMMON, * LPSAVEDGAME_COMMON;


INT CPalApp::PAL_LoadGame(LPCSTR szFileName)

/*++
  Purpose:

  Load a saved game.

  Parameters:

  [IN]  szFileName - file name of saved game.

  Return value:

  0 if success, -1 if failed.

  --*/
{
	FILE* fp;
	fp = fopen(szFileName, "rb");
	if (fp == NULL)
	{
		return -1;
	}
	SAVEDGAME_COMMON* s = NULL;
	size_t                   i;

	if (gConfig->fIsWIN95)
	{
		SAVEDGAME* p = (SAVEDGAME*)malloc(sizeof(SAVEDGAME));

		memset(p, 0, sizeof(SAVEDGAME));
		
		s = (SAVEDGAME_COMMON*)p;
		if (!s)
		{
			fclose(fp);
			return -1;
		}
		i = fread(s, 1, sizeof(SAVEDGAME), fp);
		fclose(fp);

		if (i < sizeof(SAVEDGAME) - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS)
		{
			free(s);
			return 1;
		}
	}
	else
	{
		SAVEDGAME_DOS* p = (SAVEDGAME_DOS*)UTIL_calloc(sizeof(SAVEDGAME_DOS), 1);
		
		//memset(p, 0, sizeof(*p));
		
		s = (SAVEDGAME_COMMON*)p;
		if (!s)
		{
			fclose(fp);
			return -1;
		}

		i = fread(s, 1, sizeof(SAVEDGAME_DOS), fp);
		fclose(fp);

		if (i < sizeof(SAVEDGAME_DOS) - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS)
		{
			free(s);
			return 1;
		}
	}
	//
	// Adjust endianness
	//
	DO_BYTESWAP(s, i);

	//
	// Cash amount is in DWORD, so do a wordswap in Big-Endian.
	//
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	s->dwCash = ((s->dwCash >> 16) | (s->dwCash << 16));
#endif


	gpGlobals->viewport = PAL_XY(s->wViewportX, s->wViewportY);
	gpGlobals->wMaxPartyMemberIndex = s->nPartyMember;
	gpGlobals->wNumScene = s->wNumScene;
	gpGlobals->fNightPalette = (s->wPaletteOffset != 0);
	gpGlobals->wPartyDirection = s->wPartyDirection;
	gpGlobals->wNumMusic = s->wNumMusic;
	gpGlobals->wNumBattleMusic = s->wNumBattleMusic;
	gpGlobals->wNumBattleField = s->wNumBattleField;
	gpGlobals->wScreenWave = s->wScreenWave;
	gpGlobals->sWaveProgression = 0;
	gpGlobals->wCollectValue = s->wCollectValue;
	gpGlobals->wLayer = s->wLayer;
	gpGlobals->wChaseRange = s->wChaseRange;
	gpGlobals->wChasespeedChangeCycles = s->wChasespeedChangeCycles;
	gpGlobals->nFollower = s->nFollower;
	gpGlobals->dwCash = s->dwCash;
	if (!gConfig->fIsWIN95)
		gpGlobals->bFinishGameTime = s->bFinishGameTime;
	else
		gpGlobals->bFinishGameTime = 0;
	memcpy(gpGlobals->rgParty, s->rgParty, sizeof(gpGlobals->rgParty));
	memcpy(gpGlobals->rgTrail, s->rgTrail, sizeof(gpGlobals->rgTrail));
	//memcpy(gpGlobals->, s->rgTrail, sizeof(gpGlobals->rgTrail));

	gpGlobals->Exp = s->Exp;
	gpGlobals->g.PlayerRoles = s->PlayerRoles;
	memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
	memcpy(gpGlobals->rgInventory, s->rgInventory, sizeof(gpGlobals->rgInventory));
	memcpy(gpGlobals->g.rgScene, s->rgScene, sizeof(gpGlobals->g.rgScene));

	gpGlobals->fEnteringScene = FALSE;

	PAL_CompressInventory();
	if (gConfig->fIsWIN95)
	{
		SAVEDGAME* p = (SAVEDGAME*)s;
		memcpy(((LPOBJECT)gpGlobals->g.rgObject), p->rgObject, sizeof(p->rgObject));
		memcpy(gpGlobals->g.lprgEventObject, p->rgEventObject, sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

	}
	else
	{
		SAVEDGAME_DOS* p = (SAVEDGAME_DOS*)s;
		LPOBJECT op =(LPOBJECT)gpGlobals->g.rgObject;
		for (i = 0; i < MAX_OBJECTS; i++)
		{
			memcpy(&op[i], &p->rgObject[i], sizeof(OBJECT_DOS));
			op[i].rgwData[6] = p->rgObject[i].rgwData[5];     // wFlags
			op[i].rgwData[5] = 0;                            // wScriptDesc or wReserved2
		}
		memcpy(gpGlobals->g.lprgEventObject, p->rgEventObject, sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);
	}
	gpGlobals->bSavedTimes = s->wSavedTimes;
	free(s);
	return 0;
}

//get Object displacement
//取对象结构在存储文件中的位移 0 对象，2 场景 1 场景对象
const size_t CPalApp::getSaveFileOffset(int ob)
{
	return tagSAVEDGAME_DOS::getOffset(ob, gConfig->fIsWIN95);
}

VOID CPalApp::PAL_SaveGame(
	LPCSTR         szFileName,
	WORD           wSavedTimes
)
/*++
  Purpose:

  Save the current game state to file.

  Parameters:

  [IN]  szFileName - file name of saved game.

  Return value:

  None.

  --*/
{
	gpGlobals->bSavedTimes = wSavedTimes;
	//如果不请允许存盘，返回
	if (gConfig->is_ShowScript[2]) return;

	FILE* fp;
	PAL_LARGE SAVEDGAME_COMMON*       s = NULL;
	size_t                    i;
	if (gConfig->fIsWIN95)
	{
		SAVEDGAME* p = (SAVEDGAME*)UTIL_calloc(sizeof(SAVEDGAME), 1);
		s = (SAVEDGAME_COMMON*)p;
		memcpy(&p->rgObject, ((LPOBJECT)gpGlobals->g.rgObject), sizeof(p->rgObject));
		memcpy(&p->rgEventObject, gpGlobals->g.lprgEventObject, sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

		i = sizeof(*p);
	}
	else
	{
		SAVEDGAME_DOS* p = (SAVEDGAME_DOS*)UTIL_calloc(sizeof(SAVEDGAME_DOS), 1);
		s = (SAVEDGAME_COMMON*)p;
		LPOBJECT op = (LPOBJECT)gpGlobals->g.rgObject;
		for (i = 0; i < MAX_OBJECTS; i++)
		{
			memcpy(&p->rgObject[i], &op[i], sizeof(OBJECT_DOS));
			p->rgObject[i].rgwData[5] = op[i].rgwData[6];     // wFlags
		}
		memcpy(p->rgEventObject, gpGlobals->g.lprgEventObject, sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

		i = sizeof(*p);
	}
	s->wSavedTimes = wSavedTimes;
	s->wViewportX = PAL_X(gpGlobals->viewport);
	s->wViewportY = PAL_Y(gpGlobals->viewport);
	s->nPartyMember = gpGlobals->wMaxPartyMemberIndex;
	s->wNumScene = gpGlobals->wNumScene;
	s->wPaletteOffset = (gpGlobals->fNightPalette ? 0x180 : 0);
	s->wPartyDirection = gpGlobals->wPartyDirection;
	s->wNumMusic = gpGlobals->wNumMusic;
	s->wNumBattleMusic = gpGlobals->wNumBattleMusic;
	s->wNumBattleField = gpGlobals->wNumBattleField;
	s->wScreenWave = gpGlobals->wScreenWave;
	s->wCollectValue = gpGlobals->wCollectValue;
	s->wLayer = gpGlobals->wLayer;
	s->wChaseRange = gpGlobals->wChaseRange;
	s->wChasespeedChangeCycles = gpGlobals->wChasespeedChangeCycles;
	s->nFollower = gpGlobals->nFollower;
	s->dwCash = gpGlobals->dwCash;

#ifdef FINISH_GAME_MORE_ONE_TIME
	s->bFinishGameTime = gpGlobals->bFinishGameTime;
#endif

#ifndef PAL_CLASSIC
	s->wBattleSpeed = gpGlobals->bBattleSpeed;
#else
	s->wBattleSpeed = 2;
#endif

	memcpy(s->rgParty, gpGlobals->rgParty, sizeof(gpGlobals->rgParty));
	memcpy(s->rgTrail, gpGlobals->rgTrail, sizeof(gpGlobals->rgTrail));
	s->Exp = gpGlobals->Exp;
	s->PlayerRoles = gpGlobals->g.PlayerRoles;
	//memcpy(s->rgPoisonStatus, gpGlobals->rgPoisonStatus, sizeof(gpGlobals->rgPoisonStatus));
	memcpy(s->rgInventory, gpGlobals->rgInventory, sizeof(gpGlobals->rgInventory));
	memcpy(s->rgScene, gpGlobals->g.rgScene, sizeof(gpGlobals->g.rgScene));

	//
	// Adjust endianness
	//
	DO_BYTESWAP(s, i);

	//
	// Cash amount is in DWORD, so do a wordswap in Big-Endian.
	//
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	s->dwCash = ((s->dwCash >> 16) | (s->dwCash << 16));
#endif

	if (fopen_s(&fp, szFileName, "wb"))
		return ;
	i += PAL_MKFGetChunkSize(0, gpGlobals->f.fpSSS) - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS;

	fwrite(s, i, 1, fp);

	fclose(fp);
	free(s);
}


size_t CPalApp::getSaveFileLen()
{

	if (gConfig->fIsWIN95)
		return sizeof(SAVEDGAME) + gpGlobals->g.nEventObject * sizeof(EVENTOBJECT) -
		sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS;
	else
		return sizeof(SAVEDGAME_DOS) + gpGlobals->g.nEventObject * sizeof(EVENTOBJECT) -
		sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS;
};
