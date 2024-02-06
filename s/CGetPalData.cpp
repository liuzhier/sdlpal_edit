#include "CGetPalData.h"
#include "CTestData.h"
#include <iterator>
#include <cassert>

LPCSTR p_Script(int i);


VOID ShowMsg(const char* format, ...)
{
	//接收传递信息
	char string[256]{ 0 };
	va_list     argptr;

	va_start(argptr, format);
	vsnprintf(string, 255, format, argptr);
	va_end(argptr);
	MSG bMsg;
	while (PeekMessage(&bMsg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&bMsg);
		DispatchMessage(&bMsg);
	}
	CString* pMsg = new CString(string);

	::PostMessage(AfxGetApp()->m_pMainWnd->m_hWnd, WM_POST_MESSAGE_CSTR, 0, (LPARAM)(pMsg));
}

CGetPalData::CGetPalData()
{

}

CGetPalData::CGetPalData(int save, BOOL noRun)
	:CScript(save,noRun)
{
	PAL_Object_classify();
}


//运行在进程中，输入 测试结构，Pal的指针指向已经载入，未运行的数据
RunInThread::RunInThread(TestData* pt, CGetPalData* Pal)
{
	//
	if ((!pt) || Pal == nullptr)
		return;
	auto t = *pt;
	auto g = Pal->gpGlobals;
	g->viewport = PAL_XY(t[t_ViewportX], t[t_ViewportY]);
	g->wMaxPartyMemberIndex = t[t_nPartyMember];
	g->wNumScene = t[t_NumScene];
	g->fNightPalette = t[t_PaletteOffset];
	g->wPartyDirection = t[t_PartyDirection];
	g->wNumMusic = t[t_NumMusic];
	g->wNumBattleMusic = t[t_NumBattleMusic];
	g->wNumBattleField = t[t_NumBattleField];
	g->wScreenWave = t[t_ScreenWave];
	g->wCollectValue = t[t_CollectValue];
	g->wLayer = t[t_Layer];
	g->wChaseRange = t[t_ChaseRange];
	g->wChasespeedChangeCycles = t[t_ChasespeedChangeCycles];
	g->nFollower = t[t_nFollower];
	g->dwCash = t[t_Cash];
	if (t[0] < 0)
		SDL_SetWindowTitle(Pal->gpWindow, "运行事件对象测试，不允许存档");
	else
		SDL_SetWindowTitle(Pal->gpWindow, "运行测试");

	Pal->gConfig->is_INVINCIBLE = t[t_INVINCIBLE];
	Pal->gConfig->is_KO_ENEMY_FAST = t[t_KO_ENEMY_FAST];
	Pal->gConfig->is_SHOW_OBJECT = t[t_SHOW_OBJECT];
	Pal->gConfig->is_ShowScript[0] = t[t_EquipmentEffect];
	Pal->gConfig->is_ShowScript[1] = t[t_AutoScript];
	Pal->gConfig->is_ShowScript[2] = t[0] < 0;
	Pal->gConfig->is_TEST = TRUE;

	auto save = t[0];
	auto gpGlobals = Pal->gpGlobals;
	if (save > 0)
		Pal->PAL_LoadGame(Pal->va("%s%d%s", Pal->PalDir.c_str(), save, ".rpg"));
	else if (save < 0)
	{
		Pal->PAL_LoadGame(Pal->va("%s%d%s", Pal->PalDir.c_str(), save, ".rpg"));
		// 将事件对象替换成系统缺省
		Pal->PAL_MKFReadChunk((LPBYTE)(gpGlobals->g.lprgEventObject),
			sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject, 0, gpGlobals->f.fpSSS);
	}
	else
		Pal->PAL_LoadDefaultGame();

	//装入参数
	//运行主程序
	DWORD dwTime = SDL_GetTicks_New();
	g->fGameStart = 1;
	Pal->PAL_ProcessEvent();
	Pal->g_InputState.prevdir = kDirUnknown;
	Pal->g_InputState.dir = kDirUnknown;

	Pal->VideoInit();//初始化视屏系统
	Pal->SoundRunThread();//启动声音进程


	while (!Pal->PalQuit)
	{

		//
		// Do some initialization at game start.
		//
		if (gpGlobals->fGameStart)
		{	//Do some initialization work when game starts (new game or load game).
			Pal->PAL_GameStart();
			gpGlobals->fGameStart = FALSE;
			Pal->setColor(255, 255, 255);
			/////
			SDL_Color color = { 10,10,10,255 };
			Pal->RenderBlendCopy(Pal->gpTextureReal, (CGL_Texture*)nullptr, 255, 2, &color);
			Pal->RenderPresent(Pal->gpTextureReal);
		}

		//
		// Load the game resources if needed.
		//
		Pal->PAL_LoadResources();

		//
		// Clear the input state of previous frame.
		//
		Pal->PAL_ClearKeyState();

		//
		// Wait for the time of one frame. Accept input here.
		//
		Pal->PAL_ProcessEvent();
		while (SDL_GetTicks_New() <= dwTime)
		{
			Pal->PAL_ProcessEvent();
			SDL_Delay(1);
		}

		//
		// Set the time of the next frame.
		//
		dwTime = SDL_GetTicks_New() + Pal->gConfig->m_Function_Set[24];

		//
		// Run the main frame routine.        
		//
		Pal->PAL_StartFrame();
	}
	//pCScript->PalQuit = 1;
	//清除已加载的显示部分
	Pal->VideoShutDown();

	return;
}



CGetPalData::~CGetPalData()
{
}


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

#ifdef FINISH_GAME_MORE_ONE_TIME
	BYTE				bFinishGameTime;         //未使用地址，用来记录共完成游戏多少次
	BYTE				rgbReserved[5];			// unused
#else
	WORD             rgwReserved2[3];         // unused
#endif
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


//返回存档文件结构中的 场景结构指针，输入指向存档结构，返回指向场景结构指针
LPSCENE CGetPalData::getSecensPoint(const LPVOID p)
{
	return ((SAVEDGAME*)p)->rgScene;
}



//标记所有有效的脚本地址 入口 脚本地址表
INT CGetPalData::PAL_MarkScriptAll(INT isIncludeSave)
{
	pMark.clear();;
	pMark.resize(gpGlobals->g.nScriptEntry + 1);
	for (int save = 0; save < 10; save++)
	{
		//0=系统缺省 1--9 存储文件
		if (save)
		{
			string sv = va("%s%d.rpg", PalDir.c_str(), save);
			if (!IsFileExist(sv))
				continue;
			PAL_LoadGame(sv.c_str());
		}
		// 
		//标记对象脚本
		for (int n = 0; n < gpGlobals->g.nObject; n++)
		{
			LPWORD p = ((LPOBJECT)gpGlobals->g.rgObject)[n].rgwData;
			//对象结构已经统一，不分win95 不是dos
			for (int j = 2; j < 6; j++)
			{
				if (p[j])
					PAL_MarkScriptEntryAll(p[j], (save << 28) + (1 << 24) + (j << 16) + n, save);
			}
		}
		//第二步，标记场景信息和事件对象 
		for (WORD s = 1, k = 0; s <= MAX_SCENES; s++)
		{
			LPWORD p = (LPWORD)&gpGlobals->g.rgScene[s - 1];
			//标记进入和传送脚本
			if (p[1])//进入脚本
				PAL_MarkScriptEntryAll(p[1], (save << 28) + (2 << 24) + (1 << 16) + s - 1, save);
			if (p[2])//传送脚本
				PAL_MarkScriptEntryAll(p[2], (save << 28) + (2 << 24) + (2 << 16) + s - 1, save);
			for (; k < gpGlobals->g.nEventObject &&
				(k < gpGlobals->g.rgScene[s].wEventObjectIndex ||
					(s == MAX_SCENES && k < gpGlobals->g.nEventObject))
				; k++)
			{
				LPWORD q = (LPWORD)&gpGlobals->g.lprgEventObject[k];
				for (int n = 4; n < 6; n++)
				{
					//4 5	
					if (q[n])
						PAL_MarkScriptEntryAll(q[n], (save << 28) + (3 << 24) + (n << 16) + k, save);
				}
			}
		}
	}
	PAL_LoadDefaultGame();
	return 0;
}

VOID CGetPalData::PAL_ReloadPAL(BOOL isDelete)
{
	//以下卸载游戏数据后，拷贝文件再重新载入 
	PAL_FreeText();
	if (gpGlobals == nullptr)
	{
		gpGlobals = new GLOBALVARS;
	}
	PAL_FreeObjectDesc(gpGlobals->lpObjectDesc);
	gpGlobals->lpObjectDesc = NULL;
	PAL_FreeGlobals();

	string s = PalDir;
	LPCSTR  FileNames[] =
	{
	"abc.mkf",
	"ball.mkf",
	"data.mkf",
	"f.mkf",
	"fbp.mkf",
	"fire.mkf",
	"gop.mkf",
	"map.mkf",
	"mgo.mkf",
	"rgm.mkf",
	"sss.mkf",
	"word.dat",
	"m.msg",
	"DESC.DAT",
	nullptr,
	};

	BOOL bSucceed;
	for (int n = 0; FileNames[n]; n++)
	{
		s = PalDir + FileNames[n];
		string s1 = s + (".NEW");
		if (IsFileExist(s) && IsFileExist(s1))
		{
			bSucceed = DeleteFile(CString( s.c_str()));

			bSucceed = MoveFile(CString(s1.c_str()), CString(s.c_str()));
		}
	}
	if (isDelete)
	{

		LPCSTR textFileName[3] =
		{
			"MAKMKF\\Event.TXT",
			"MAKMKF\\Object.TXT",
			"MAKMKF\\ScriptEntry.TXT"
		};
		for (int n = 0; n < 3; n++)
		{
			//删除使用过的TXT文件
			s = PalDir + textFileName[n];
			string s1 = s + (".TXT");
			//			bSucceed = DeleteFile(s1);
			bSucceed = CopyFile(CString(s.c_str()),CString( s1.c_str()), FALSE);
			DeleteFile(CString(s.c_str()));
		}
	}
	gpGlobals = new GLOBALVARS;
	memset(gpGlobals, 0, sizeof(GLOBALVARS));
	memset(&gpGlobals->g, 0, sizeof(gpGlobals->g));
	PAL_InitGlobals();
	PAL_InitGlobalGameData();
	gpGlobals->g.rgObject = UTIL_calloc(sizeof(OBJECT), MAX_OBJECTS);
	DWORD len = PAL_MKFGetChunkSize(2,gpGlobals->f.fpSSS ); 
	if (gConfig->fIsWIN95)
		gpGlobals->g.nObject = len / sizeof(OBJECT);
	else
		gpGlobals->g.nObject = len / sizeof(OBJECT_DOS);

	gpGlobals->bIsBig5 = (PAL_IsBig5(gConfig->m_Function_Set[49]) == 1);
	PAL_InitText();
	PAL_LoadDefaultGame();
	if (gpGlobals->lpObjectDesc == NULL)
		gpGlobals->lpObjectDesc = PAL_LoadObjectDesc("desc.dat");
	//PAL_Object_classify();
	return VOID();
}

typedef struct tagCellDataStruct//表单元数据结构
{
	int i;
	double d;
	std::string  s;
}CellDataStruct, * LPCellDataStruct;


VOID CGetPalData::PAL_UnPackObject(VOID)
{
	//解开系统文件，用于修改
	USES_CONVERSION;
	LPSTR obj, name;
	LPBYTE Mark = new BYTE[60000];
	memset(Mark, 0, 60000);
	FILE* f;

	string ss;
	ss = PalDir + ("MAKMKF\\Object.TXT");
	if (IsFileExist(ss))
	{
		if (::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("\t继续吗？"), _T("文件已存在"), MB_OKCANCEL) == IDCANCEL)
			return;
	}
	if (fopen_s(&f, ss.c_str(), "w"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法建立文件"),CString( ss.c_str() ), MB_OK);
		return;
	}
	//第一步，打印对象信息
	for (int n = 0; n < MAX_OBJECTS; n++)
	{
		ShowMsg("进度，对象号\t%3.3d", n);

		switch (gpObject_classify[n])
		{
		case kIsPlayer:
		{
			obj = "W";
			name = "我方";
			break;
		}
		case kIsEnemy:
		{
			obj = "L";
			name = "敌方";
			break;
		}
		case kIsMagic:
		{
			obj = "M";
			name = "魔法";
			break;
		}
		case kIsPoison:
		{
			obj = "O";
			name = "毒药";
			break;
		}
		case kIsItem:
			obj = "V";
			name = "物品";
			break;
		case 0:
		{
			obj = "K";
			name = "空的";
			break;
		}
		default:
			obj = "K";
			name = "空的";
		}
		LPWORD p = nullptr;

		{
			p = ((LPOBJECT)gpGlobals->g.rgObject)[n].rgwData;
			fprintf_s(f, "%s %3.3d\t%d\t%d\t%4.4X\t%4.4X\t%4.4X\t%4.4X\t%X\t%s\t{%16.16s}\t{%s}\n", obj,
				n, p[0], p[1], p[2], p[3], p[4], p[5], p[6], name, PAL_GetWord(n),
				"");
		}
		//以下标记脚本
		if (gpObject_classify[n])
		{
			for (int k = 2; k <= 5; k++)
			{
				if (p[k])
				{
					PAL_MarkScriptEntry(p[k], Mark);
				}
			}
		}
	}
	fclose(f);

	ShowMsg("已经生成 %s", (ss.c_str()));

	ss = PalDir + ("MAKMKF\\Event.TXT");
	if (fopen_s(&f, ss.c_str(), "w"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法建立文件"), CString(ss.c_str()), MB_OK);
		return;
	}

	fprintf_s(f, "\n|以下打印事件对象\n");

	//第二步，打印场景信息和事件对象 

	for (WORD s = 1, k = 0; s <= MAX_SCENES; s++)
	{
		ShowMsg("进度，场景号\t%3.3d", s);
		fprintf_s(f, "|场景号\t地图号\t进入脚本\t传送脚本\t索引\n");
		LPWORD p = (LPWORD)&gpGlobals->g.rgScene[s - 1];
		fprintf_s(f, "S%3.3d\t%3.3d\t%4.4X\t%4.4X\t%4.4X\n", s, p[0], p[1], p[2], p[3]);
		//标记进入和传送脚本
		if (p[1])//进入脚本
			PAL_MarkScriptEntry(p[1], Mark);
		if (p[2])//传送脚本
			PAL_MarkScriptEntry(p[2], Mark);

		fprintf_s(f, "|序号\t隐时间\t方位X\t方位Y\t起始层\t目标脚本\t自动脚本\t状态\t触发模式\t形象号\t形象数\t方向\t当前帧数\t空闲帧数\t无效\t总帧数\t空闲计数\n");
		for (; k < gpGlobals->g.nEventObject &&
			(k < gpGlobals->g.rgScene[s].wEventObjectIndex ||
				(s == 300 && k < gpGlobals->g.nEventObject))
			; k++)
		{
			LPWORD q = (LPWORD)&gpGlobals->g.lprgEventObject[k];
			fprintf_s(f, "K%4.4X\t", k + 1);//序号从1开始，1需要加1 
			for (int n = 0; n < 16; n++)
			{
				if (n == 4 || n == 5)
				{
					fprintf_s(f, "%4.4X\t", q[n]);
					PAL_MarkScriptEntry(q[n], Mark);
				}
				else
					fprintf_s(f, "%d\t", (SHORT)q[n]);
			}
			fprintf_s(f, "\n");
		}
	}
	fclose(f);

	ShowMsg("已经生成 %s", ss.c_str());
	//第三步标记脚本
	ss = PalDir + ("MAKMKF\\ScriptEntry.TXT");

	ShowMsg("正在生成 %s", ss.c_str());

	if (fopen_s(&f, ss.c_str(), "w"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法建立文件"), CString(ss.c_str()), MB_OK);
		return;
	}
	int ScriptCont = 0;
	for (int n = 0; n < gpGlobals->g.nScriptEntry; n++)
	{
		LPSCRIPTENTRY	p = &gpGlobals->g.lprgScriptEntry[n];
		CellDataStruct ColUnion{};
		ColUnion.i = p->wOperation;
		fprintf_s(f, "%1s%4.4X\t", Mark[n] ? "Y" : "N", n);
		if (Mark[n])
			ScriptCont++;
		fprintf_s(f, "%4.4X\t%4.4X\t%4.4X\t%4.4X\t//%s\n", p->wOperation, p->rgwOperand[0],
			p->rgwOperand[1], p->rgwOperand[2], p_Script(ColUnion.i));
		if (p->wOperation == 0xFFFF)
			fprintf_s(f, "%s\n", PAL_GetMsg(p->rgwOperand[0]));
	}
	fclose(f);
	delete[] Mark;
	ShowMsg("原有脚本数为 %d，其中有效脚本数为 %d", gpGlobals->g.nScriptEntry, ScriptCont);
	ShowMsg("解包完成");
}

INT CGetPalData::loadSaveFile()
{
	for (int n = 1; n < 10; n++)
	{
		pSaveData[n].clear();
		string sV = va("%s%1d.rpg", PalDir.c_str(), n);
		if (!IsFileExist(sV))
		{
			continue;
		}
		FILE* f = fopen(sV.c_str(), "rb");
		if (!f)
			continue;
		fseek(f, 0, SEEK_END);
		size_t len = ftell(f);
		pSaveData[n].resize(len);
		fseek(f, 0, SEEK_SET);
		fread(&pSaveData[n][0], len, 1, f);
		fclose(f);
	}
	return 0;
}

INT CGetPalData::saveSaveFile()
{
	for (int n = 1; n < 10; n++)
	{
		if (pSaveData[n].size())
		{
			string sV = va("%s%1d.rpg", PalDir.c_str(), n);
			FILE* f = fopen(sV.c_str(), "wb");
			fwrite(&pSaveData[n][0], pSaveData[n].size(), 1, f);
			fclose(f);
		}
	}
	return 0;
}


VOID CGetPalData::PAL_Make_MKF()
{
	//重新生成sss.mkf   和 date.mkf
	VOID* pp;
	UINT32 c;
	FILE* f;
	string s;
	s = PalDir + ("SSS.MKF.NEW");
	string s1;
	s1 = PalDir + "SSS.MKF";
	fopen_s(&f, s.c_str(), "wb");
	UINT j;
	LPGAMEDATA p = &gpGlobals->g;
	if (f == NULL)
	{
		return;
	}
	c = 6 * sizeof(INT);
	fwrite(&c, sizeof(INT), 1, f);
	int cLen[16]{ 0 };
	for (j = 0; j < 5; j++)
	{
		switch (j)
		{
		case 0://
			cLen[j] = p->nEventObject * sizeof(EVENTOBJECT);
			break;
		case 1://
			cLen[j] = sizeof(p->rgScene);
			break;
		case 2://
			if (gConfig->fIsWIN95)
				cLen[j] = sizeof(OBJECT) * gpGlobals->g.nObject;
			else
			{
				cLen[j] = sizeof(OBJECT_DOS) * gpGlobals->g.nObject;
			}
			break;
		case 3:
			cLen[j] = (g_TextLib.nMsgs + 1) * sizeof(DWORD);
			//3 信息字段数
			break;
		case 4://
			cLen[j] = p->nScriptEntry * sizeof(SCRIPTENTRY);
			break;
		default:
			break;
		}
		long dLen = PAL_MKFGetChunkSize(j, gpGlobals->f.fpSSS);
		//assert(dLen == cLen[j]);
		c += cLen[j];
		fwrite(&c, sizeof(INT), 1, f);
	}
	for (j = 0; j < 5; j++)
	{
		c = cLen[j];
		switch (j)
		{
		case 0:
			fwrite(p->lprgEventObject, 1, c, f);
			break;
		case 1:
			fwrite(p->rgScene, 1, c, f);
			break;
		case 2:
			//需要对数据进行修改 
			if (gConfig->fIsWIN95)
				fwrite(p->rgObject, 1, c, f);
			else
			{
				LPOBJECT_DOS sObject = new OBJECT_DOS[p->nObject];
				memset(sObject, 0, c);
				LPOBJECT pObject = (LPOBJECT)p->rgObject;
				for (int m = 0; m < p->nObject; m++)
				{
					memcpy(&sObject[m], &pObject[m], sizeof(OBJECT_DOS));
					sObject[m].rgwData[5] = pObject[m].rgwData[6];     // wFlags
				}

				fwrite(sObject, 1, c, f);
				delete [] sObject;
			}
			break;
		case 3:
			//3 信息字段数
			fwrite(g_TextLib.lpMsgOffset, 1, c, f);
			break;
		case 4:
			fwrite(p->lprgScriptEntry, 1, c, f);//4
			break;
		default:
			break;
		}
	}

	//SSS.MKF建立完成
	fclose(f);
	CString cs1(s1.c_str());
	CString cs(s.c_str());
	DeleteFile(cs1);
	MoveFile(cs, cs1);

	auto sData = PalDir + "DATA.MKF.NEW";
	auto s1Data = PalDir + "DATA.MKF";
	fopen_s(&f, sData.c_str(), "wb");

	if (f == NULL)
		return;
	c = 17 * sizeof(int);
	fwrite(&c, sizeof(INT32), 1, f);
	for (j = 0; j < 16; j++)
	{
		switch (j)
		{
		case 0:
			cLen[j] = sizeof(STORE) * p->nStore;
			break;
		case 1:
			cLen[j] = sizeof(ENEMY) * p->nEnemy;
			//fwrite(p->lprgEnemy, 1, c, f);
			break;
		case 2:
			cLen[j] = sizeof(ENEMYTEAM) * p->nEnemyTeam;
			//fwrite(p->lprgEnemyTeam, 1, c, f);
			break;
		case 3:
			cLen[j] = sizeof(PLAYERROLES);
			//fwrite(&p->PlayerRoles, 1, c, f);
			break;
		case 4:
			cLen[j] = sizeof(MAGIC) * p->nMagic;
			//fwrite(p->lprgMagic, 1, c, f);
			break;
		case 5:
			cLen[j] = sizeof(BATTLEFIELD) * p->nBattleField;
			//fwrite(p->lprgBattleField, 1, c, f);
			break;
		case 6:
			cLen[j] = sizeof(LEVELUPMAGIC_ALL) * p->nLevelUpMagic;
			//fwrite(p->lprgLevelUpMagic, 1, c, f);
			break;
		case 7:
		case 8:
		case 9://gpSpriteUI
		case 10://g_Battle.lpEffectSprite
		case 12://g_TextLib.bufDialogIcons
			cLen[j] = PAL_MKFGetChunkSize(j, gpGlobals->f.fpDATA);
			//PAL_MKFReadChunk((LPBYTE)pp, c, j, gpGlobals->f.fpDATA);
			break;
		case 11:
			cLen[j] = sizeof(p->rgwBattleEffectIndex);
			//fwrite(p->rgwBattleEffectIndex, 1, c, f);
			break;
		case 13:
			cLen[j] = sizeof(p->EnemyPos);
			//fwrite(&p->EnemyPos, 1, c, f);
			break;
		case 14:
			cLen[j] = sizeof(p->rgLevelUpExp);
			//fwrite(p->rgLevelUpExp, 1, c, f);
			break;
		case 15:
			cLen[j] = sizeof(gConfig->m_Function_Set)  + 1;
			break;

		default:
			break;
		}
		LONG dlen = PAL_MKFGetChunkSize(j, gpGlobals->f.fpDATA);
		//assert(cLen[j] == dlen);
		c += cLen[j];
		fwrite(&c, sizeof(INT32), 1, f);
	}

	for (j = 0; j < 16; j++)
	{
		DWORD off;
		off = ftell(f);
		c = cLen[j];
		switch (j)
		{
		case 0:
			fwrite(p->lprgStore, 1, c, f);
			break;
		case 1:
			fwrite(p->lprgEnemy, 1, c, f);
			break;
		case 2:
			fwrite(p->lprgEnemyTeam, 1, c, f);
			break;
		case 3:
			fwrite(&p->PlayerRoles, 1, c, f);
			break;
		case 4:
			fwrite(p->lprgMagic, 1, c, f);
			break;
		case 5:
			fwrite(p->lprgBattleField, 1, c, f);
			break;
		case 6:
			fwrite(p->lprgLevelUpMagic, 1, c, f);
			break;
		case 7:
		case 8:
		case 9://gpSpriteUI
		case 10://g_Battle.lpEffectSprite
		case 12://g_TextLib.bufDialogIcons
			pp = new BYTE[(c)];
			PAL_MKFReadChunk((LPBYTE)pp, c, j, gpGlobals->f.fpDATA);
			fwrite(pp, 1, c, f);
			delete[] pp;
			break;
		case 11:
			fwrite(p->rgwBattleEffectIndex, 1, c, f);
			break;
		case 13:
			fwrite(&p->EnemyPos, 1, c, f);
			break;
		case 14:
			fwrite(p->rgLevelUpExp, 1, c, f);
			break;
		case 15:
			fwrite(gConfig->m_Function_Set, 1, c, f);
			break;
		default:
			break;
		}
	}
	fclose(f);
	PAL_FreeGlobals();
	gpGlobals = NULL;
	DeleteFile(CString( s1.c_str()));
	MoveFile(CString(s.c_str()), CString(s1.c_str()));
	DeleteFile(CString(s1Data.c_str()));
	MoveFile(CString(sData.c_str()), CString(s1Data.c_str()));
	//存档文件
	for (int n = 0; n < 10; n++)
	{
		if (pSaveData[n].size())
		{
			s = va("%s%1d.rpg", PalDir.c_str(), n);
			fopen_s(&f, s.c_str(), "wb");
			if (f == nullptr)
				continue;;
			fwrite(&pSaveData[n][0], pSaveData[n].size(), 1, f);
			fclose(f);
		}
	}
	//重新装入系统数据
	
}




//标记脚本入口
VOID CGetPalData::PAL_MarkScriptEntry(WORD Entry, LPBYTE AllMark)
{
	//已经标记过返回
	if (AllMark[Entry])
		return;
	while (1)
	{
		//标记
		AllMark[Entry] = 1;
		LPSCRIPTENTRY p = &gpGlobals->g.lprgScriptEntry[Entry];
		switch (p->wOperation)
		{
		case 0:
			return;
			//参数2 跳转地址
		case 0x0006://条件跳转  参数1 概率  满足执行下一条，不满足跳转到参数2指向的地址
		case 0x001e://金钱改变，参数1 = 值，如参数1为且钱不足时，跳到参数2执行
		case 0x0024://设置对象自动脚本地址，参数1 对象不等于0 参数2 地址
		case 0x0025://设置对象触发脚本地址，参数1 不等于0 参数2 地址
		case 0x005d://如我方没有中特定的毒，则跳转  参数1 毒种类，参数2 跳转到
		case 0x005e://如敌方没有中特定的毒，则跳转  参数1 毒种类，参数2 跳转到
		case 0x0064://跳转，如敌方生命值高于 设定的百分比 ，参数1 比值，参数2 目标
		case 0x0079://如果指定成员在队伍中，则跳到参数2
		case 0x0090://设置对象脚本，参数1 对象，参数3 偏移，参数2 值
		case 0x0095://如果当前场景等于参数1，则跳转,参数2
		case 0x009c://怪物复制自身,失败跳转到 参数2
		case 0x0103://队伍中没有指定角色则跳转，参数1 角色，参数2 跳转目标
			PAL_MarkScriptEntry(Entry + 1, AllMark);
			if (p->rgwOperand[1])
				PAL_MarkScriptEntry(p->rgwOperand[1], AllMark);
			return;
			//参数1 为跳转地址
		case 0x0002://停止运行，用参数1替换，用于下一轮运行,条件是参数 2 = 0 或小于对象空闲帧数
		case 0x0003://无条件跳转到参数 1位置,参数2等于0和未达到参数2指定的帧数 运行下一条
		case 0x0004://运行 参数1 指向的子脚本
		case 0x000a://跳转到 参数1 指定地址，如玩家选择 No
		case 0x0033://收集妖怪炼丹,参数1 失败转到
		case 0x0034://灵壶炼丹指令,参数1 失败转到
		case 0x0038://将队伍从现场传送出去 失败跳到参数1 指向地址
		case 0x003a://将队伍从现场传送出去 失败跳到参数1 指向地址
		case 0x0061://没有中毒，跳转，参数1 跳转到
		case 0x0068://如敌方行动，跳转到参数1
		case 0x0074://如不是全体满血，跳到参数1
		case 0x0091://如果敌方不是孤单，则跳转 ，参数1 地址
			if (p->wOperation == 0x0003 && p->rgwOperand[1] == 0)
			{
				PAL_MarkScriptEntry(p->rgwOperand[0], AllMark);
				return;
			}
			PAL_MarkScriptEntry(Entry + 1, AllMark);
			if (p->rgwOperand[0])
				PAL_MarkScriptEntry(p->rgwOperand[0], AllMark);
			return;
			//参数3为跳转地址
		case 0x0020://从库存中移除物品，参数1 品种，参数2 数量，如库存没有移除已装备物品，物品不足跳到参数3
		case 0x0028://敌方中毒指令 参数1 ！=0 全体，参数2 毒,失败跳转到参数3
		case 0x0029://我方中毒指令 参数1 ！=0 全体，参数2 毒,失败跳转到参数3
		case 0x002d://我方特殊状态指令，参数1 =0 疯、睡、定、封、傀儡、攻、防、身、双击，参数2 回合数。失败跳到参数3
		case 0x002e://敌方特殊状态指令，参数1 =0 疯、睡、定、傀儡、攻、防、身、双击，参数2 回合数，参数3 如不成功 跳转到参数3指向的地址。
		case 0x0058://如果库存数量不足则跳过，参数1 品种，参数2 要求数量，参数3 跳转到
		case 0x0081://跳过，如果没有面对对象，参数1 对象，参数2 改变触发方式，参数3地址
		case 0x0083://如果事件对象不在当前事件对象的指定区域，则跳过,参数1，参数3地址
		case 0x0084://将玩家用作事件对象的物品放置到场景中,参数1，参数2，失败跳到参数3
		case 0x0086://如果未装备指定物品则跳过，参数1 物品，参数3 地址
		case 0x0094://如果事件对象的状态是指定的，则跳转,参数2 状态，参数3 地址
		case 0x009e://怪物召唤，参数1 召唤的怪物ID，参数2 数量，参数3 失败跳转
			PAL_MarkScriptEntry(Entry + 1, AllMark);
			if (p->rgwOperand[2] && p->rgwOperand[2] != 0xFFFF)
				PAL_MarkScriptEntry(p->rgwOperand[2], AllMark);
			return;
			//参数2、3为跳转地址
		case 0x0007://开始战斗  参数1 队伍号， 参数3 为 0 BOSS 参数2 战斗失败执行 参数3 战斗中逃跑执行
		case 0x006d://设定一个场景的进入和传送脚本，参数1，场景号，参数2 进入脚本，参数3 传送脚本，参数2 和参数3同时为0 清除脚本
			PAL_MarkScriptEntry(Entry + 1, AllMark);
			if (p->rgwOperand[1])
				PAL_MarkScriptEntry(p->rgwOperand[1], AllMark);
			if (p->rgwOperand[2])
				PAL_MarkScriptEntry(p->rgwOperand[2], AllMark);
			return;
		case 0xffff://显示文字 参数1 文字地址 打印该文字
			//fprintf_s(f, "%s\n", p_Pal->PAL_GetMsg(p->rgwOperand[0]));
		default:
			break;
		}
		Entry++;
		if (Entry >= gpGlobals->g.nScriptEntry)
		{
			//fprintf_s(f, "0000\t0000\t0000\t0000\n");
			return;
		}

	}
	return VOID();
}

VOID CGetPalData::PAL_Object_classify(VOID)
{

	memset(&gpObject_classify, 0, sizeof(gpObject_classify));
	//标记我方角色
	for (int n = 0; n < MAX_PLAYER_ROLES; n++)
	{
		WORD player = gpGlobals->g.PlayerRoles.rgwName[n];
		gpObject_classify[player] = kIsPlayer;
	}

	//标记敌方
	nEnemy = 0;
	for (int n = 0; n < gpGlobals->g.nEnemyTeam; n++)
	{
		for (int s = 0; s < MAX_ENEMIES_IN_TEAM; s++)
		{
			WORD enemy = gpGlobals->g.lprgEnemyTeam[n].rgwEnemy[s];
			if (enemy != 0xffff && enemy != 0 && enemy < MAX_OBJECTS)
			{
				string s = PAL_GetWord(enemy);
				if (s.size() < 2)
					continue;
				if (!(gpObject_classify[enemy] & kIsEnemy))
				{
					gpObject_classify[enemy] |= kIsEnemy;
					nEnemy++;
				}
			}
		}
	}
	//标记召唤的敌人
	for (int n = 0; n < gpGlobals->g.nScriptEntry; n++)
	{
		WORD wOperation = gpGlobals->g.lprgScriptEntry[n].wOperation;
		if (wOperation == 0x009e || wOperation == 0x009f)
		{
			WORD enemy = gpGlobals->g.lprgScriptEntry[n].rgwOperand[0];
			if (enemy && !(gpObject_classify[enemy] & kIsEnemy))
			{
				gpObject_classify[enemy] |= kIsEnemy;
				nEnemy++;
			}
		}
	}
	assert(nEnemy <= gpGlobals->g.nEnemy);
	//标记毒
	nPoisonID = 0;
	for (int n = 0; n < gpGlobals->g.nScriptEntry; n++)
	{
		WORD wOperation = gpGlobals->g.lprgScriptEntry[n].wOperation;
		if (wOperation == 0x0028 || wOperation == 0x0029)
		{
			WORD  poisonID = gpGlobals->g.lprgScriptEntry[n].rgwOperand[1];
			assert(poisonID < MAX_OBJECTS);
			LPWORD lpData;

			LPOBJECT lpObj = (LPOBJECT)gpGlobals->g.rgObject;
			lpData = lpObj[poisonID].rgwData;

			if (lpData[3] || lpData[5])
				continue;

			if (!(gpObject_classify[poisonID] & kIsPoison))
			{
				nPoisonID++;
				gpObject_classify[poisonID] |= kIsPoison;
			}
		}
	}

	//标记物品
	nItem = 0;
	//标记获得物品
	for (int n = 0; n < gpGlobals->g.nScriptEntry; n++)
	{
		if (gpGlobals->g.lprgScriptEntry[n].wOperation == 0x001f
			&& gpGlobals->g.lprgScriptEntry[n].rgwOperand[0])
		{
			WORD ItemID = gpGlobals->g.lprgScriptEntry[n].rgwOperand[0];
			assert(ItemID && ItemID < MAX_OBJECTS);
			if (!(gpObject_classify[ItemID] & kIsItem))
			{
				nItem++;
				gpObject_classify[ItemID] |= kIsItem;
			}
		}
	}
	//标记商店物品
	for (int n = 0; n < gpGlobals->g.nStore; n++)
	{
		for (int t = 0; t < MAX_STORE_ITEM; t++)
		{
			WORD ItemID = gpGlobals->g.lprgStore[n].rgwItems[t];
			if (ItemID == 0)
				continue;
			assert(ItemID && ItemID < MAX_OBJECTS);
			if ((gpObject_classify[ItemID] ^ kIsItem))
			{
				nItem++;
				gpObject_classify[ItemID] |= kIsItem;
			}
		}
	}
	//标记装备物品
	for (int n = 0; n < MAX_PLAYER_ROLES; n++)
	{
		for (int t = 0; t < MAX_PLAYER_EQUIPMENTS; t++)
		{
			WORD ItemID = gpGlobals->g.PlayerRoles.rgwEquipment[n][t];
			if (ItemID == 0)continue;
			assert(ItemID && ItemID < MAX_OBJECTS);
			if (ItemID && !(gpObject_classify[ItemID] & kIsItem))
			{
				nItem++;
				gpObject_classify[ItemID] |= kIsItem;
			}
		}
	}
	//标记可偷物品
	for (int n = 0; n < MAX_OBJECTS; n++)
	{
		if (gpObject_classify[n] != kIsEnemy)
			continue;
		WORD EnemyID;

		if (gConfig->fIsWIN95)
		{
			EnemyID = ((LPOBJECT)gpGlobals->g.rgObject)[n].enemy.wEnemyID;
		}
		else
		{
			EnemyID = ((LPOBJECT_DOS)gpGlobals->g.rgObject)[n].enemy.wEnemyID;
		}
		if (EnemyID > gpGlobals->g.nEnemy)
			continue;
		assert(EnemyID < gpGlobals->g.nEnemy);
		if (gpGlobals->g.lprgEnemy[EnemyID].nStealItem == 0)
			continue;
		WORD ItemID = gpGlobals->g.lprgEnemy[EnemyID].wStealItem;
		if (ItemID == 0)continue;
		assert(ItemID && ItemID < MAX_OBJECTS);
		if (ItemID && !(gpObject_classify[ItemID] & kIsItem))
		{
			nItem++;
			gpObject_classify[ItemID] |= kIsItem;
		}
	}

	//标记有标志的物品
	for (int n = 0; n < MAX_OBJECTS; n++)
	{
		WORD pp;
		//
		pp = ((LPOBJECT)gpGlobals->g.rgObject)[n].rgwData[6];
		if (pp && gpObject_classify[n] == 0)
		{
			nItem++;
			gpObject_classify[n] |= kIsItem;
		}
	}
	
	//assert(nItem == );

	//标记魔法
	nMagic = 0;
#if 1
	//标记脚本魔法
	for (int n = 0; n < MAX_OBJECTS; n++)
	{
		//OBJECT_MAGIC sMagic;
		WORD flags = ((LPOBJECT)gpGlobals->g.rgObject)[n].magic.wFlags;
		if (!(gpObject_classify[n] & kIsMagic) && (flags == 2 || flags == 0xa || flags == 0x1a || flags == 0x12))
		{
			gpObject_classify[n] = kIsMagic;
			nMagic++;
		}

	}
#endif
	for (int n = 0; n < MAX_PLAYER_ROLES; n++)
	{
		for (int pMagic = 0; pMagic < MAX_PLAYER_MAGICS; pMagic++)
		{
			//标记初始魔法
			WORD sMagic = gpGlobals->g.PlayerRoles.rgwMagic[pMagic][n];
			if (sMagic && sMagic < MAX_OBJECTS && !(gpObject_classify[sMagic] & kIsMagic))
			{
				gpObject_classify[sMagic] = kIsMagic;
				nMagic++;
			}
		}
		//标记合体魔法
		{
			WORD sMagic = gpGlobals->g.PlayerRoles.rgwCooperativeMagic[n];
			if (sMagic && !(gpObject_classify[sMagic] & kIsMagic))
			{
				gpObject_classify[sMagic] = kIsMagic;
				nMagic++;
			}
		}
	}
	//标记升级魔法
	for (int n = 0; n < MAX_PLAYER_ROLES - 1; n++)
	{
		for (int pMagic = 0; pMagic < gpGlobals->g.nLevelUpMagic; pMagic++)
		{
			if (gpGlobals->g.lprgLevelUpMagic[pMagic].m[n].wLevel == 0)
				continue;
			WORD sMagic = gpGlobals->g.lprgLevelUpMagic[pMagic].m[n].wMagic;
			if (sMagic && !(gpObject_classify[sMagic] & kIsMagic))
			{
				gpObject_classify[sMagic] = kIsMagic;
				nMagic++;
			}
		}
	}
	//标记获得魔法
	for (int n = 0; n < gpGlobals->g.nScriptEntry; n++)
	{
		SCRIPTENTRY p = gpGlobals->g.lprgScriptEntry[n];
		switch (p.wOperation)
		{
		case 0x0055:
		case 0x0056:
		case 0x0067:
			//case 0x0092:
		{
			WORD sMagic = p.rgwOperand[0];
			if (sMagic && sMagic < MAX_OBJECTS && !(gpObject_classify[sMagic] & kIsMagic))
			{
				gpObject_classify[sMagic] = kIsMagic;
				nMagic++;
			}
			break;
		}
		default:
			break;
		}
	}

	assert(nMagic <= gpGlobals->g.nMagic);
	return;
}

VOID CGetPalData::Utf8ToSys(string& s)
{
	//去除尾部空格
	s.erase(s.find_last_not_of(" ") + 1);
	Cls_Iconv Istr{};
	//
	if (!gConfig->fIsUseBig5)
	{
		s = Istr.UTF8toGBK(s.c_str(), 936);
		//936 to 950
		if (gpGlobals->bIsBig5)
			s = Istr.Gb2312ToBig5(s.c_str());
	}
	else
	{
		s = Istr.UTF8toGBK(s.c_str(), 950);
		//950 to 936
		if (!gpGlobals->bIsBig5)
			s = Istr.Big5ToGb2312(s.c_str());
	}
}


//标记具体的有效脚本地址
//参数1 入口，参数2 要标记的脚本号列表
VOID CGetPalData::PAL_MarkScriptEntryAll(WORD Entry, DWORD32 mark,int save)
{
	if (Entry == 0)
		return;

	pMark[Entry].s.push_back(mark);//标记
	// 已经标记过 返回
	if (pMark[Entry].s.size() > 1)
		return;
	LPSCRIPTENTRY p = &gpGlobals->g.lprgScriptEntry[Entry];
	switch (p->wOperation)
	{
	case 0:
		return;
		//以下参数二跳转地址
	case 0x0006://条件跳转  参数1 概率  满足执行下一条，不满足跳转到参数2指向的地址
	case 0x001e://金钱改变，参数1 = 值，如参数1为且钱不足时，跳到参数2执行
	case 0x0024://设置对象自动脚本地址，参数1 不等于0 参数2 地址
	case 0x0025://设置对象触发脚本地址，参数1 不等于0 参数2 地址
	case 0x005d://如我方没有中特定的毒，则跳转  参数1 毒种类，参数2 跳转到
	case 0x005e://如敌方没有中特定的毒，则跳转  参数1 毒种类，参数2 跳转到
	case 0x0064://跳转，如敌方生命值高于 设定的百分比 ，参数1 比值，参数2 目标
	case 0x0079://如果指定成员在队伍中，则跳到参数2
	case 0x0090://设置对象脚本，参数1 对象，参数3 偏移，参数2 值
	case 0x0095://如果当前场景等于参数1，则跳转,参数2
	case 0x009c://怪物复制自身,失败跳转到 参数2
	case 0x0103://队伍中没有指定角色则跳转，参数1 角色，参数2 跳转目标
	{
		if (p->rgwOperand[1])
		{
			PAL_MarkScriptEntryAll(p->rgwOperand[1], (save << 28) + (4 << 24) + (2 << 16) + Entry, save);
		}
		break;
	}
	case 0x0002://停止运行，用参数1替换，用于下一轮运行,条件是参数 2 = 0 或小于对象空闲帧数
	case 0x0003://无条件跳转到参数 1位置,参数2等于0和未达到参数2指定的帧数 运行下一条
		if (p->rgwOperand[1] == 0)
		{
			//到此结束
			PAL_MarkScriptEntryAll(p->rgwOperand[0], (save << 28) + (4 << 24) + (1 << 16) + Entry, save);
			return;
		}
	case 0x0004://运行 参数1 指向的子脚本
	case 0x0033://收集妖怪炼丹,参数1 失败转到
	case 0x000a://跳转到 参数1 指定地址，如玩家选择 No
	case 0x0034://灵壶炼丹指令,参数1 失败转到
	case 0x0038://将队伍从现场传送出去 失败跳到参数1 指向地址
	case 0x003a://将队伍从现场传送出去 失败跳到参数1 指向地址
	case 0x0061://没有中毒，跳转，参数1 跳转到
	case 0x0068://如敌方行动，跳转到参数1
	case 0x0074://如不是全体满血，跳到参数1
	case 0x0091://如果敌方不是孤单，则跳转 ，参数1 地址
		if (p->rgwOperand[0])
			PAL_MarkScriptEntryAll(p->rgwOperand[0], (save << 28) + (4 << 24) + (1 << 16) + Entry, save);
		break;
		//以下参数一跳转地址
	case 0x0020://从库存中移除物品，参数1 品种，参数2 数量，如库存没有移除已装备物品，物品不足跳到参数3
	case 0x0028://敌方中毒指令 参数1 ！=0 全体，参数2 毒,失败跳转到参数3
	case 0x0029://我方中毒指令 参数1 ！=0 全体，参数2 毒,失败跳转到参数3
	case 0x002d://我方特殊状态指令，参数1 =0 疯、睡、定、封、傀儡、攻、防、身、双击，参数2 回合数。失败跳到参数3
	case 0x002e://敌方特殊状态指令，参数1 =0 疯、睡、定、傀儡、攻、防、身、双击，参数2 回合数，参数3 如不成功 跳转到参数3指向的地址。
	case 0x0058://如果库存数量不足则跳过，参数1 品种，参数2 要求数量，参数3 跳转到
	case 0x0081://跳过，如果没有面对对象，参数1 对象，参数2 改变触发方式，参数3地址
	case 0x0083://如果事件对象不在当前事件对象的指定区域，则跳过,参数1，参数3地址
	case 0x0084://将玩家用作事件对象的物品放置到场景中,参数1，参数2，失败跳到参数3
	case 0x0086://如果未装备指定物品则跳过，参数1 物品，参数3 地址
	case 0x0094://如果事件对象的状态是指定的，则跳转,参数2 状态，参数3 地址
	case 0x009e://怪物召唤，参数1 召唤的怪物ID，参数2 数量，参数3 失败跳转
		if (p->rgwOperand[2] && p->rgwOperand[2] != 0xFFFF)
			PAL_MarkScriptEntryAll(p->rgwOperand[2], (save << 28) + (4 << 24) + (3 << 16) + Entry, save);
		break;
		//以下双跳转地址2 和 3
	case 0x0007://开始战斗  参数1 队伍号， 参数3 为 0 BOSS 参数2 战斗失败执行 参数3 战斗中逃跑执行
	case 0x006d://设定一个场景的进入和传送脚本，参数1，场景号，参数2 进入脚本，参数3 传送脚本，参数2 和参数3同时为0 清除脚本
		if (p->rgwOperand[1])
			PAL_MarkScriptEntryAll(p->rgwOperand[1], (save << 28) + (4 << 24) + (2 << 16) + Entry, save);
		if (p->rgwOperand[2])
			PAL_MarkScriptEntryAll(p->rgwOperand[2], (save << 28) + (4 << 24) + (3 << 16) + Entry, save);
		break;
	case 0xffff://显示文字 参数1 文字地址 打印该文字
		//fprintf_s(f, "%s\n", p_Pal->PAL_GetMsg(p->rgwOperand[0]));
	default:
		break;
	}
	if (Entry + 1 >= gpGlobals->g.nScriptEntry || pMark[Entry + 1].s.size())
	{
		return;
	}
	PAL_MarkScriptEntryAll(Entry + 1, (save << 28) + (4 << 24) + Entry, save);//进行下一行标注
	return;
}

//标记脚本入口位置
//Mark the entry point of the scrip0t

//标记脚本跳转地址
//Mark The script jumps to the address
//输入 跳转地址引用
//返回下一跳地址，为0 结束，jumps 不为0返回三个跳转地址
INT CGetPalData::MarkSprictJumpsAddress(WORD spriptEntry, MAPScript& mMaps)
{
	while (TRUE)
	{
		MAPScript::iterator pw;
		pw = mMaps.find(spriptEntry);
		if (pw != mMaps.end())
			return 0;
		mMaps[spriptEntry] = spriptEntry;
		auto p = &gpGlobals->g.lprgScriptEntry[spriptEntry];
		if (p->wOperation == 0)
			return 0;
		switch (p->wOperation)
		{
			//以下参数二跳转地址
		case 0x0006://条件跳转  参数1 概率  满足执行下一条，不满足跳转到参数2指向的地址
		case 0x001e://金钱改变，参数1 = 值，如参数1为且钱不足时，跳到参数2执行
		case 0x0024://设置对象自动脚本地址，参数1 不等于0 参数2 地址
		case 0x0025://设置对象触发脚本地址，参数1 不等于0 参数2 地址
		case 0x005d://如我方没有中特定的毒，则跳转  参数1 毒种类，参数2 跳转到
		case 0x005e://如敌方没有中特定的毒，则跳转  参数1 毒种类，参数2 跳转到
		case 0x0064://跳转，如敌方生命值高于 设定的百分比 ，参数1 比值，参数2 目标
		case 0x0079://如果指定成员在队伍中，则跳到参数2
		case 0x0090://设置对象脚本，参数1 对象，参数3 偏移，参数2 值
		case 0x0095://如果当前场景等于参数1，则跳转,参数2
		case 0x009c://怪物复制自身,失败跳转到 参数2
		case 0x0103://队伍中没有指定角色则跳转，参数1 角色，参数2 跳转目标
			MarkSprictJumpsAddress(p->rgwOperand[1], mMaps);
			break;
		case 0x0002://停止运行，用参数1替换，用于下一轮运行,条件是参数 2 = 0 或小于对象空闲帧数
		case 0x0003://无条件跳转到参数 1位置,参数2等于0和未达到参数2指定的帧数 运行下一条
		case 0x0004://运行 参数1 指向的子脚本
		case 0x000a://跳转到 参数1 指定地址，如玩家选择 No
		case 0x0033://收集妖怪炼丹,参数1 失败转到
		case 0x0034://灵壶炼丹指令,参数1 失败转到
		case 0x0038://将队伍从现场传送出去 失败跳到参数1 指向地址
		case 0x003a://将队伍从现场传送出去 失败跳到参数1 指向地址
		case 0x0061://没有中毒，跳转，参数1 跳转到
		case 0x0068://如敌方行动，跳转到参数1
		case 0x0074://如不是全体满血，跳到参数1
		case 0x0091://如果敌方不是孤单，则跳转 ，参数1 地址
		{
			if (p->wOperation == 0x0003 && p->rgwOperand[1] == 0)
			{
				MarkSprictJumpsAddress(p->rgwOperand[0], mMaps);
				return 0;
			}
			else
			{
				MarkSprictJumpsAddress(p->rgwOperand[0], mMaps);
			}
			break;
		}
			//以下参数一跳转地址
		case 0x0020://从库存中移除物品，参数1 品种，参数2 数量，如库存没有移除已装备物品，物品不足跳到参数3
		case 0x0028://敌方中毒指令 参数1 ！=0 全体，参数2 毒,失败跳转到参数3
		case 0x0029://我方中毒指令 参数1 ！=0 全体，参数2 毒,失败跳转到参数3
		case 0x002d://我方特殊状态指令，参数1 =0 疯、睡、定、封、傀儡、攻、防、身、双击，参数2 回合数。失败跳到参数3
		case 0x002e://敌方特殊状态指令，参数1 =0 疯、睡、定、傀儡、攻、防、身、双击，参数2 回合数，参数3 如不成功 跳转到参数3指向的地址。
		case 0x0058://如果库存数量不足则跳过，参数1 品种，参数2 要求数量，参数3 跳转到
		case 0x0081://跳过，如果没有面对对象，参数1 对象，参数2 改变触发方式，参数3地址
		case 0x0083://如果事件对象不在当前事件对象的指定区域，则跳过,参数1，参数3地址
		case 0x0084://将玩家用作事件对象的物品放置到场景中,参数1，参数2，失败跳到参数3
		case 0x0086://如果未装备指定物品则跳过，参数1 物品，参数3 地址
		case 0x0094://如果事件对象的状态是指定的，则跳转,参数2 状态，参数3 地址
		case 0x009e://怪物召唤，参数1 召唤的怪物ID，参数2 数量，参数3 失败跳转
			if (p->rgwOperand[2] != 0xffff)
				MarkSprictJumpsAddress(p->rgwOperand[2], mMaps);
			break;
			//以下双跳转地址2 和 3
		case 0x0007://开始战斗  参数1 队伍号， 参数3 为 0 BOSS 参数2 战斗失败执行 参数3 战斗中逃跑执行
		case 0x006d://设定一个场景的进入和传送脚本，参数1，场景号，参数2 进入脚本，参数3 传送脚本，参数2 和参数3同时为0 清除脚本
			MarkSprictJumpsAddress(p->rgwOperand[1], mMaps);
			MarkSprictJumpsAddress(p->rgwOperand[2], mMaps);
			break;
		default:
			break;
		}
		spriptEntry++;
	}
	return 0;
}

//Single script changes
//脚本单行变动处理
// 输入：变动的脚本，原行号，新行号,第一项等于第二项？
//
INT CGetPalData::SingleScriptChange(int entry,int sOld, int sNew)
{
	auto &sgMark = pMark[sOld].s;
	if (sgMark.size() == 0)
		return 0;
	LPWORD obj{};
	for (int n = 0; n < sgMark.size(); n++)
	{
		auto m = sgMark.at(n);
		INT save = (m >> 28);//>0 存储文档号
		INT from = (m >> 24) & 0xf;//1 = 对象 2 = 场景 3 = 事件对象 4 脚本
		INT col = (m >> 16) & 0xff;//所在列
		INT row = m & 0xffff;//所在行
		obj = nullptr;
		switch (from)
		{
		case 1:
			//对象
			if (save == 0)
				obj = &((LPWORD)gpGlobals->g.rgObject)[7 * row + col];
			else if(gConfig->fIsWIN95)
				obj = &((LPWORD)((LPBYTE)&pSaveData[save][getSaveFileOffset(0)]))[7 * row + col];
			else
			{
				obj = &((LPWORD)((LPBYTE)&pSaveData[save][getSaveFileOffset(0)]))[6 * row + col];
				if (col == 5)obj--;
			}
			ASSERT(*obj == sOld);
			break;
		case 2:
			//场景
			if (save == 0)
				obj = &((LPWORD)(&gpGlobals->g.rgScene[row]))[col];
			else
			{
				obj = &((LPWORD)(&(
					(LPSCENE)((LPBYTE)&pSaveData[save][getSaveFileOffset(2)]))[row]))[col];
			}
			ASSERT(*obj == sOld);
			break;
		case 3:
			//事件对象
			if (save == 0)
				obj = &((LPWORD)(&gpGlobals->g.lprgEventObject[row]))[col];
			else
			{
				obj = &((LPWORD)(&(
					(LPEVENTOBJECT)((LPBYTE)&pSaveData[save][getSaveFileOffset(1)]))[row]))[col];
				//	测试用
				//LPSAVEDGAME_DOS saveData = (LPSAVEDGAME_DOS)&pSaveData[save][0];
				//auto obj1 = &((LPWORD)(&saveData->rgEventObject[row]))[col];
				//ASSERT(obj == obj1 );
			}
			ASSERT(*obj == sOld);
			break;
		case 4:
			//脚本
			if (save == 0)
			{
				if (col == 0)
				{
					//新行-旧行
					ASSERT(row == sOld - 1);
					continue;
				}
				auto scriptEntry = &gpGlobals->g.lprgScriptEntry[row + sNew - sOld];
				obj = &((LPWORD)scriptEntry)[col];
				ASSERT(*obj == sOld);
			}
			else
				continue;
			break;
		default:
			ASSERT(FALSE);
			return 1;
			break;
		}
		*obj = sNew;
	}
	return 0;
}

// 替换mkf文件中的一个片段，
int CGetPalData::replaceMKFOne(LPCSTR fileName, int nNum, LPCVOID buf, int BufLen)
{
	// TODO: 在此处添加实现代码.
	if (!fileName || !buf)
		return -1;
	CString cfn = CString(fileName); //+fileName;
	string fn = fileName;
	string fnNew = fn + ".new";
	FILE* fp{}, * fpNew{};
	if (fopen_s(&fp, fn.c_str(), "rb"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("打开文件错"), cfn, MB_OK);
		return -1;
	}

	fpos_t filesize;
	fseek(fp, 0, SEEK_END);
	fgetpos(fp, &filesize);
	LPVOID fileBuf = calloc(filesize, 1);
	if (!fileBuf) exit(10);
	fseek(fp, 0, SEEK_SET);
	fread(fileBuf, filesize, 1, fp);
	fclose(fp);

	if (fopen_s(&fpNew, fnNew.c_str(), "wb"))
	{
		cfn += ".new";
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("打开文件错"), cfn, MB_OK);
		free(fileBuf);
		return -1;
	}
	DWORD32 headlen = ((DWORD32*)fileBuf)[0];
	int cont = headlen / sizeof(DWORD32) - 1;
	//在尾部添加
	if (nNum >= cont)
	{
		nNum = cont++;
		headlen += sizeof(DWORD32);
	}
	//
	fwrite(&headlen, sizeof(DWORD32), 1, fpNew);
	//
	DWORD32 offset = headlen;
	DWORD32* t = (DWORD32*)fileBuf;
	//写数据头
	for (int n = 0; n < cont; n++)
	{
		if (n == nNum)
			offset += BufLen;
		else
		{
			offset += t[n + 1] - t[n];
			assert(t[n + 1] >= t[n]);
		}
		fwrite((LPCVOID)(&offset), sizeof(DWORD32), 1, fpNew);
	}
	//写数据
	for (int n = 0; n < cont; n++)
	{
		if (n == nNum)
			fwrite(buf, BufLen, 1, fpNew);
		else
		{
			offset += t[n + 1] - t[n];
			DWORD32 len = t[n + 1] - t[n];
			LPCVOID p = ((LPBYTE)fileBuf + t[n]);
			if (len)
				fwrite(p, len, 1, fpNew);
		}
	}

	fclose(fpNew);
	free(fileBuf);
	//最后一步 改文件名
	CString oldfilename = CString(fileName);
	CString bakefilename = oldfilename;
	bakefilename += ".bak";
	CString newFilename(fnNew.c_str());
	CopyFile(oldfilename, bakefilename, FALSE);
	if (CopyFile(newFilename, oldfilename, FALSE))
	{
		remove(CStringA(newFilename).GetString());
	}
	return 0;
}

INT CGetPalData::EncodeRLE(const void* Source, const UINT8 TransparentColor, INT32 Stride, INT32 Width, INT32 Height, void*& Destination, INT32& Length)
{
	INT32 i, j, count;
	UINT32 length;
	UINT8* src = (UINT8*)Source;
	UINT8* temp;
	UINT8* ptr;

	if (Source == NULL)
		return EINVAL;
	if ((ptr = temp = (UINT8*)malloc(Width * Height * 2 + 4)) == NULL)
		return ENOMEM;

	for (i = 0, ptr = temp + 4; i < Height; i++)
	{
		for (j = 0; j < Width;)
		{
			for (count = 0; j < Width && *src == TransparentColor; j++, src++, count++);
			while (count > 0)
			{
				*ptr++ = (count > 0x7f) ? 0xff : 0x80 | count;
				count -= 0x7f;
			}
			for (count = 0; j < Width && *src != TransparentColor; j++, src++, count++);
			while (count > 0)
			{
				if (count > 0x7f)
				{
					*ptr++ = 0x7f;
					memcpy(ptr, src - count, 0x7f);
					ptr += 0x7f;
				}
				else
				{
					*ptr++ = count;
					memcpy(ptr, src - count, count);
					ptr += count;
				}
				count -= 0x7f;
			}
		}
		src += Stride - Width;
	}
	if ((ptr - temp) % 2) //smkf subfile must be even; need rle encoder archive it
		*ptr++ = 0;
	length = (UINT32)(ptr - temp);

	if ((Destination = realloc(temp, length)) == NULL)
	{
		free(temp);
		return ENOMEM;
	}
	*((UINT16*)Destination) = (UINT16)Width;
	*((UINT16*)Destination + 1) = (UINT16)Height;
	Length = length;
	return 0;
}

