#pragma once
#ifndef CTESTDATA_H
#define CTESTDATA_H
#include "../main/cpalevent.h"
#include "CFileDir.h"
#include "CPalMapEdit.h"
#include <vector>
//using namespace  CPalEvent;
enum  tagTestEnum
{
	t_SaveFile = 0,
	t_ViewportX ,//位置
	t_ViewportY,//位置
	t_nPartyMember,//队伍人数-1
	t_NumScene,//场景号
	t_PaletteOffset,//夜间调色版
	t_PartyDirection,//队伍方向
	t_NumMusic,//音乐号
	t_NumBattleMusic,//战斗音乐号
	t_NumBattleField,//战斗场景
	t_ScreenWave,// 屏幕波动
	t_CollectValue,//灵葫值
	t_Layer,//排列
	t_ChaseRange,// 引怪值 0 驱魔 3 十里 1 正常
	t_ChasespeedChangeCycles,//引怪剩余周期
	t_nFollower,//跟随
	t_Cash,//金钱
	t_INVINCIBLE,				//无敌模式，受攻击后不减hp不受不良状态影响
	t_KO_ENEMY_FAST,			//快速灭怪
	t_SHOW_OBJECT,				//显示对象
	t_EquipmentEffect,			//显示装备脚本
	t_AutoScript,				//显示自动脚本
	t_noSave,
	t_End,//结束标志
};

typedef std::vector<INT32>TestData;
typedef std::vector<TestData>allTestData;
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
} SAVEDGAME_COMMON, * LPSAVEDGAME_COMMON;

class CTestData :public CFileDir//,public CPalEvent
{
	//class CPalEdit* pal;
	class CScript * pal;
public:
	allTestData sTestData;


	//CTestData(CPalEdit* Pal) :pal(Pal)
	CTestData(CScript* Pal) : pal(Pal)
	{
		LoadTestData();
	};

	~CTestData() 
	{
		saveTestData();
	};

	int LoadTestData()
	{
		//文件不存在 ，建立新的数据
		sTestData.resize(1);
		sTestData[0].resize(t_End);
		auto g = pal->gpGlobals;
		sTestData[0][t_SaveFile] = 0;
		sTestData[0][t_ViewportX] = PAL_X(g->viewport);
		sTestData[0][t_ViewportY] = PAL_Y(g->viewport);
		sTestData[0][t_nPartyMember] = g->wMaxPartyMemberIndex;
		sTestData[0][t_NumScene] = g->wNumScene;
		sTestData[0][t_PaletteOffset] = g->fNightPalette;
		sTestData[0][t_PartyDirection] = g->wPartyDirection;
		sTestData[0][t_NumMusic] = g->wNumMusic;
		sTestData[0][t_NumBattleMusic] = g->wNumBattleMusic;
		sTestData[0][t_NumBattleField] = g->wNumBattleField;
		sTestData[0][t_ScreenWave] = g->wScreenWave;
		sTestData[0][t_CollectValue] = g->wCollectValue;
		sTestData[0][t_Layer] = g->wLayer;
		sTestData[0][t_ChaseRange] = g->wChaseRange;
		sTestData[0][t_ChasespeedChangeCycles] = g->wChasespeedChangeCycles;
		sTestData[0][t_nFollower] = g->nFollower;
		sTestData[0][t_Cash] = g->dwCash;
		sTestData[0][t_INVINCIBLE] = 1;
		sTestData[0][t_KO_ENEMY_FAST] = 1;
		sTestData[0][t_SHOW_OBJECT] = 1;
		sTestData[0][t_EquipmentEffect] = 0;
		//以下读入存档文件
		{
			std::vector<BYTE> pSaveData;
			pSaveData.resize(sizeof(SAVEDGAME_COMMON));
			SAVEDGAME_COMMON* p = (SAVEDGAME_COMMON*)&pSaveData[0];
			int n = sTestData.size();
			for (int i = 1; i <= 5; i++)
			{
				FILE* f = nullptr;
				string filename = pal->PalDir;
				char str[10]{ 0 };
				sprintf(str, "%d.rpg", i);
				filename += str;
				f = fopen(filename.c_str(), "rb");
				if (!f)continue;
				fread_s(p, sizeof(SAVEDGAME_COMMON), sizeof(SAVEDGAME_COMMON), 1, f);
				TestData s;
				s.resize(t_End);
				s[t_SaveFile] = i;
				s[t_ViewportX] = p->wViewportX;
				s[t_ViewportY] = p->wViewportY;
				s[t_nPartyMember] = p->nPartyMember;
				s[t_NumScene] = p->wNumScene;
				s[t_PaletteOffset] = p->wPaletteOffset;
				s[t_PartyDirection] = p->wPartyDirection;
				s[t_NumMusic] = p->wNumMusic;
				s[t_NumBattleMusic] = p->wNumBattleMusic;
				s[t_NumBattleField] = p->wNumBattleField;
				s[t_ScreenWave] = p->wScreenWave;
				s[t_CollectValue] = p->wCollectValue;
				s[t_Layer] = p->wLayer;
				s[t_ChaseRange] = p->wChaseRange;
				s[t_ChasespeedChangeCycles] = p->wChasespeedChangeCycles;
				s[t_nFollower] = p->nFollower;
				s[t_Cash] = p->dwCash;
				s[t_INVINCIBLE] = 1;
				s[t_KO_ENEMY_FAST] = 1;
				s[t_SHOW_OBJECT] = 1;
				s[t_EquipmentEffect] = 0;
				sTestData.push_back(move(s));

				fclose(f);
			}
			//free(p);
		}
		return TRUE;
	}

	int  saveTestData()
	{
		return TRUE;
	}
};



#endif //CTESTDATA_H

