#include "cscript.h"
#include "cgl_render.h"
#include <assert.h>
//#ifdef PAL_HAS_AVI
#include "caviplay.h"
//#endif
#include "csdlscreen.h"

static int      g_iCurEquipPart = -1;

#define  AVOID_SCRIPT_CRASH 1 
#define  PAL_ITEM_DESC_BOTTOM	(1 << 15)

enum  tagTestEnum
{
	t_SaveFile = 0,
	t_ViewportX,//位置
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
void
UTIL_LogOutput(
	const char* fmt,
	...
);

CScript::CScript()
{
	pCScript = this;
	gConfig = new CConfig;
	if (InitPalBase())
	{
		PalQuit = 1;
		return;
	}
	pCScript = this;
	//PalQuit = 1;
	//运行主程序
	VideoInit();
	SoundRunThread();
	run();
	PalQuit = 1;
};

CScript::~CScript()
{
	while (PalSoundIn)
		Sleep(1);
	dstroyPalBase();
	if (gConfig)
		delete gConfig;
	gConfig = nullptr;
};


CScript::CScript(int save, BOOL noRun)
	//:CPalEvent(noRun)
{
	pCScript = this;

	gConfig = new CConfig;

	if (InitPalBase())
	{
		PalQuit = 1;
		return;
	}
	auto g = gpGlobals;

	//t[0]>0 运行存档文件 =0 运行系统缺省 <0 运行存档文件 但事件对象使用系统缺省的
	if (save > 0)
		PAL_LoadGame(va("%s%d%s", PalDir.c_str(), save, ".rpg"));
	else if (save < 0)
	{
		PAL_LoadGame(va("%s%d%s", PalDir.c_str(), save, ".rpg"));
		// 将事件对象替换成系统缺省
		PAL_MKFReadChunk((LPBYTE)(gpGlobals->g.lprgEventObject),
			sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject, 0, gpGlobals->f.fpSSS);
	}
	else
		PAL_LoadDefaultGame();
	//装入参数
	//运行主程序
	DWORD dwTime = SDL_GetTicks_New();
	g->fGameStart = 1;
	PAL_ProcessEvent();
	g_InputState.prevdir = kDirUnknown;
	g_InputState.dir = kDirUnknown;
	if (noRun)
	{
		PalQuit = TRUE;//不运行游戏循环
		//PAL_LoadDefaultGame();
		if ((!gConfig->fIsWIN95) && (!gpGlobals->lpObjectDesc))
			gpGlobals->lpObjectDesc = PAL_LoadObjectDesc("desc.dat");
		return;
	}

	VideoInit();//视频初始化
	SoundRunThread();//声音初始化

	while (!PalQuit)
	{

		//
		// Do some initialization at game start.
		//
		if (gpGlobals->fGameStart)
		{	//Do some initialization work when game starts (new game or load game).
			PAL_GameStart();
			gpGlobals->fGameStart = FALSE;
			setColor(255, 255, 255);
			/////
			SDL_Color color = { 10,10,10,255 };
			RenderBlendCopy(gpTextureReal, (CGL_Texture*)nullptr, 255, 2, &color);
			RenderPresent(gpTextureReal);
		}

		//
		// Load the game resources if needed.
		//
		PAL_LoadResources();

		//
		// Clear the input state of previous frame.
		//
		PAL_ClearKeyState();

		//
		// Wait for the time of one frame. Accept input here.
		//
		PAL_ProcessEvent();
		while (SDL_GetTicks_New() <= dwTime)
		{
			PAL_ProcessEvent();
			SDL_Delay(1);
		}

		//
		// Set the time of the next frame.
		//
		dwTime = SDL_GetTicks_New() + gConfig->m_Function_Set[24];

		//
		// Run the main frame routine.        
		//
		PAL_StartFrame();
	}
	//pCScript->PalQuit = 1;
}

WORD CScript::PAL_InterpretInstruction(
	WORD           wScriptEntry,
	WORD           wEventObjectID
)
/*++
  Purpose:

  Interpret and execute one instruction in the script.

  Parameters:

  [IN]  wScriptEntry - The script entry to execute.

  [IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

  The address of the next script instruction to execute.

  --*/
{
	LPEVENTOBJECT		pEvtObj, pCurrent;
	LPSCRIPTENTRY		pScript;
	INT					iPlayerRole, i, j, x, y;
	WORD				w, wCurEventObjectID;
	WORD				wPlayerRole;

	pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);

	if (wEventObjectID != 0)
	{
		pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);
	}
	else
	{
		pEvtObj = NULL;
	}

	if (pScript->rgwOperand[0] == 0 || pScript->rgwOperand[0] == 0xFFFF)
	{
		pCurrent = pEvtObj;
		wCurEventObjectID = wEventObjectID;
	}
	else
	{
		i = pScript->rgwOperand[0] - 1;
		if (i > 0x9000)
		{
			// HACK for Dream 2.11 to avoid crash
			i -= 0x9000;
		}
		pCurrent = &(gpGlobals->g.lprgEventObject[i]);
		wCurEventObjectID = pScript->rgwOperand[0];
	}

	if (pScript->rgwOperand[0] < MAX_PLAYABLE_PLAYER_ROLES)
	{
		iPlayerRole = gpGlobals->rgParty[pScript->rgwOperand[0]].wPlayerRole;
	}
	else
	{
		iPlayerRole = gpGlobals->rgParty[0].wPlayerRole;
	}

	switch (pScript->wOperation)
	{
		// walk one step
	case 0x000B:
	case 0x000C:
	case 0x000D:
	case 0x000E:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pEvtObj == NULL)
		{
			break;
		}
#endif
		pEvtObj->wDirection = pScript->wOperation - 0x000B;
		PAL_NPCWalkOneStep(wEventObjectID, 2);
		break;
	}

	// Set the direction and/or gesture for event object
	case 0x000F:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pEvtObj == NULL)
		{
			break;
		}
#endif
		if (pScript->rgwOperand[0] != 0xFFFF)
		{
			pEvtObj->wDirection = pScript->rgwOperand[0];
		}

		if (pScript->rgwOperand[1] != 0xFFFF)
		{
			pEvtObj->wCurrentFrameNum = pScript->rgwOperand[1];
		}

		break;
	}

	// Walk straight to the specified position
	case 0x0010:
	{
		if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 3))
		{
			wScriptEntry--;
		}
		break;
	}

	// Walk straight to the specified position, at a lower speed
	case 0x0011:
	{
		if ((wEventObjectID & 1) ^ (gpGlobals->dwFrameNum & 1))
		{
			if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
				pScript->rgwOperand[2], 2))
			{
				wScriptEntry--;
			}
		}
		else
		{
			wScriptEntry--;
		}
		break;
	}

	// Set the position of the event object, relative to the party
	case 0x0012:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		pCurrent->x =
			pScript->rgwOperand[1] + PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		pCurrent->y =
			pScript->rgwOperand[2] + PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);
		break;
	}

	// Set the position of the event object
	case 0x0013:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		pCurrent->x = pScript->rgwOperand[1];
		pCurrent->y = pScript->rgwOperand[2];
		break;
	}

	// Set the gesture of the event object
	case 0x0014:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pEvtObj == NULL)
		{
			break;
		}
#endif
		pEvtObj->wCurrentFrameNum = pScript->rgwOperand[0];
		pEvtObj->wDirection = kDirSouth;
		break;
	}

	// Set the direction and gesture for a party member
	case 0x0015:
	{
		gpGlobals->wPartyDirection = pScript->rgwOperand[0];
		gpGlobals->rgParty[pScript->rgwOperand[2]].wFrame =
			gpGlobals->wPartyDirection * 3 + pScript->rgwOperand[1];
		break;
	}

	// Set the direction and gesture for an event object
	case 0x0016:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wDirection = pScript->rgwOperand[1];
			pCurrent->wCurrentFrameNum = pScript->rgwOperand[2];
		}
		break;
	}

	// set the player's extra attribute // 若装备不存在，则战斗后不保留
	case 0x0017:
		/*
		17 参数1 参数2 参数3	   装备引起的属性增加指令
		参数1：主角属性地址——装备位置
		0B：头部
		0C：身体
		0D：肩部
		0E：武器
		0F：脚部
		10：配戴

		参数2：主角属性地址——基本属性
		00：状态表情图像
		01：战斗模型
		02：地图模型
		03：名字
		04：可否攻击全体
		05：无效？
		06：等级
		07：体力最大值
		08：真气最大值
		09：体力
		0A：真气
		11：武术
		12：灵力
		13：防御
		14：身法
		15：吉运
		16：毒抗
		17：风抗
		18：雷抗
		19：水抗
		1A：火抗
		1B：土抗
		......
		41：合体法术
		指向data.mkf的第4个子文件
		指向的地址 = 该主角的地址偏移 + 参数2*12
		主角地址偏移：
		李逍遥 0 赵灵儿 2 林月如 4
		巫后 6 阿奴 8 盖罗娇10
		参数3：整数
		作用	   若该主角参数1的地址指向的位置没有装备东西，则将参数3的值增加到参数2指向的地址
		*/
	{
		WORD* p;

		i = pScript->rgwOperand[0] - 0xB;

		p = (WORD*)(&gpGlobals->rgEquipmentEffect[i]); // HACKHACK

		p[pScript->rgwOperand[1] * MAX_PLAYER_ROLES + wEventObjectID] =
			(SHORT)pScript->rgwOperand[2];
		break;
	}

	// Equip the selected item
	case 0x0018:
	{
		i = pScript->rgwOperand[0] - 0x0B;
		g_iCurEquipPart = i;

		//
		// The wEventObjectID parameter here should indicate the player role
		//
		PAL_RemoveEquipmentEffect(wEventObjectID, i);

		if (gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID] != pScript->rgwOperand[1])
		{
			w = gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID];
			gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID] = pScript->rgwOperand[1];

			PAL_AddItemToInventory(pScript->rgwOperand[1], -1);

			if (w != 0)
			{
				PAL_AddItemToInventory(w, 1);
			}

			gpGlobals->wLastUnequippedItem = w;
		}
		break;
	}

	// Increase/decrease the player's attribute // 永久改变
	case 0x0019:
		/*
		19 参数1 参数2 参数3	   属性增加指令
		参数1：主角属性地址——基本属性
		00：状态表情图像
		01：战斗模型
		02：地图模型
		03：名字
		04：可否攻击全体
		05：无效？
		06：等级
		07：体力最大值
		08：真气最大值
		09：体力
		0A：真气
		11：武术
		12：灵力
		13：防御
		14：身法
		15：吉运
		16：毒抗
		17：风抗
		18：雷抗
		19：水抗
		1A：火抗
		1B：土抗
		......
		41：合体法术
		参数2：整数
		参数3：对象主角（可省略）
		01：李逍遥
		02：赵灵儿
		03：林月如
		04：巫后
		05：阿奴
		06：盖罗娇
		作用		   将参数2的数值增加到参数3指向的主角或选定主角（参数3省略时）的参数1的位置
		*/
	{
		WORD* p = (WORD*)(&gpGlobals->g.PlayerRoles); // HACKHACK

		if (pScript->rgwOperand[2] == 0)
		{
			iPlayerRole = wEventObjectID;
		}
		else
		{
			iPlayerRole = pScript->rgwOperand[2] - 1;
		}

		p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] +=
			(SHORT)pScript->rgwOperand[1];

			p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] =
				min(p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole], MAX_PARAMETER + MAX_PARAMETER_EXTRA);
		break;
	}

	// Set player's stat
	case 0x001A:
		/*
		1A 参数1 参数2 参数3	   属性写入指令
		参数1：主角属性地址
		参数2：整数
		参数3：对象主角（可省略）
		作用	   将参数2的数值直接写入参数3指向的主角或选定主角（参数3省略时）的参数1的位置
		*/
	{
		WORD* p = (WORD*)(&gpGlobals->g.PlayerRoles); // HACKHACK

		if (g_iCurEquipPart != -1)
		{
			//
			// In the progress of equipping items
			//
			p = (WORD*)&(gpGlobals->rgEquipmentEffect[g_iCurEquipPart]);
		}

		if (pScript->rgwOperand[2] == 0)
		{
			//
			// Apply to the current player. The wEventObjectID should
			// indicate the player role.
			//
			iPlayerRole = wEventObjectID;
		}
		else
		{
			iPlayerRole = pScript->rgwOperand[2] - 1;
		}

		p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] =
			(SHORT)pScript->rgwOperand[1];
		break;
	}

	// Increase/decrease player's HP
	case 0x001B:
		/*
		1B 参数1 参数2	   恢复体力指令
		参数1：是否全体
		0：单体
		1：全体
		参数2：整数
		作用	   给指定主角或全体恢复参数2大小的体力
		*/
	{
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_IncreaseHPMP(w, (SHORT)(pScript->rgwOperand[1]), 0);
			}
		}
		else
		{
			//
			// Apply to one player. The wEventObjectID parameter should indicate the player role.
			//
			if (!PAL_IncreaseHPMP(wEventObjectID, (SHORT)(pScript->rgwOperand[1]), 0))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;
	}

	// Increase/decrease player's MP
	case 0x001C:
	{
		/*
		1C 参数1 参数2	   恢复真气指令
		参数1：是否全体
		参数2：整数
		作用	   给指定主角或全体恢复参数2大小的真气
		*/
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_IncreaseHPMP(w, 0, (SHORT)(pScript->rgwOperand[1]));
				/////
				if ((SHORT)(pScript->rgwOperand[1]) <= (SHORT)(-50) && g_Battle.fEnemyMoving)
				{
					INT32 upLeverExp = PAL_New_GetLevelUpExp(gpGlobals->Exp.rgPrimaryExp[w].wLevel) / 25;
					gpGlobals->Exp.rgDefenseExp[w].wExp += upLeverExp ;
					gpGlobals->Exp.rgDexterityExp[w].wExp += upLeverExp ;
					gpGlobals->Exp.rgFleeExp[w].wExp += upLeverExp ;
					gpGlobals->Exp.rgMagicExp[w].wExp += upLeverExp * 2;
				}
			}
		}
		else
		{
			//
			// Apply to one player. The wEventObjectID parameter should indicate the player role.
			//
			if (!PAL_IncreaseHPMP(wEventObjectID, 0, (SHORT)(pScript->rgwOperand[1])))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;
	}

	//Increase / decrease player's HP and MP
	case 0x001D:
	{
		/*
		1D 参数1 参数2  	   恢复体力和真气指令
		参数1：是否全体
		参数2：整数
		作用	  给指定主角或全体恢复参数2大小的体力和真气
		*/
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_IncreaseHPMP(w, (SHORT)(pScript->rgwOperand[1]), (SHORT)(pScript->rgwOperand[1]));
			}
		}
		else
		{
			//
			// Apply to one player. The wEventObjectID parameter should indicate the player role.
			//
			if (!PAL_IncreaseHPMP(wEventObjectID, (SHORT)(pScript->rgwOperand[1]), (SHORT)(pScript->rgwOperand[1])))
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;
	}

	// Increase or decrease cash by the specified amount
	case 0x001E:
	{
		/*
		1E 参数1 参数2	   金钱改变指令
		参数1：整数
		参数2：指令地址（可省略）
		作用	   金钱改变参数1的数值（增加或减少）
		若参数1为负数，并且钱不够，则跳转参数2指向的地址
		*/
		if ((SHORT)(pScript->rgwOperand[0]) < 0 &&
			gpGlobals->dwCash < (WORD)(-(SHORT)(pScript->rgwOperand[0])))
		{
			//
			// not enough cash
			//
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		else
		{
			gpGlobals->dwCash += (SHORT)(pScript->rgwOperand[0]);
		}
		break;
	}

	// Add item to inventory
	case 0x001F:
	{
		PAL_AddItemToInventory(pScript->rgwOperand[0], (SHORT)(pScript->rgwOperand[1]));
		break;
	}

	// Remove item from inventory
	case 0x0020:
	{
		if (!PAL_AddItemToInventory(pScript->rgwOperand[0],
			-((pScript->rgwOperand[1] == 0) ? 1 : pScript->rgwOperand[1])))
		{
			//
			// Try removing equipped item
			//
			x = pScript->rgwOperand[1];
			if (x == 0)
			{
				x = 1;
			}

			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex ; i++)
			{
				if (i < 5)
					w = gpGlobals->rgParty[i].wPlayerRole;

				for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
				{
					if (gpGlobals->g.PlayerRoles.rgwEquipment[j][w] == pScript->rgwOperand[0])
					{
						PAL_RemoveEquipmentEffect(w, j);
						gpGlobals->g.PlayerRoles.rgwEquipment[j][w] = 0;

						if (--x == 0)
						{
							i = 9999;
							break;
						}
					}
				}
			}

			if (x > 0 && pScript->rgwOperand[2] != 0)
			{
				wScriptEntry = pScript->rgwOperand[2] - 1;
			}
		}
		break;
	}

	// Inflict damage to the enemy
	case 0x0021:
		/*
		21 参数1 参数2	   伤敌指令
		参数1：是否全体
		参数2：整数
		作用	   对选定敌人或全体造成参数2数量的伤害（伤害值为负时变为增加血量）
		*/
	{
		SHORT sDamage = (SHORT)pScript->rgwOperand[1];//可正可负
		if (pScript->rgwOperand[0])
		{
			// Inflict damage to all enemies
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				PAL_New_DecreaseHPForEnemy(i, sDamage);
			}
		}
		else
		{
			// Inflict damage to one enemy
			PAL_New_DecreaseHPForEnemy(wEventObjectID, sDamage);
		}
		break;
	}

	// Revive player 角色复活
	case 0x0022:
	{
		if (pScript->rgwOperand[0])
		{
			//
			// Apply to everyone
			//
			g_fScriptSuccess = FALSE;

			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
				{
					gpGlobals->g.PlayerRoles.rgwHP[w] =
						gpGlobals->g.PlayerRoles.rgwMaxHP[w] * pScript->rgwOperand[1] / 10;

					PAL_CurePoisonByLevel(w, MAX_POISON_LEVEL);
					for (x = 0; x < kStatusAll; x++)
					{
						PAL_RemovePlayerStatus(w, x);
					}

					g_fScriptSuccess = TRUE;
				}
			}
		}
		else
		{
			//
			// Apply to one player
			//
			if (gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] == 0)
			{
				gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] =
					gpGlobals->g.PlayerRoles.rgwMaxHP[wEventObjectID] * pScript->rgwOperand[1] / 10;

				PAL_CurePoisonByLevel(wEventObjectID, MAX_POISON_LEVEL);
				for (x = 0; x < kStatusAll; x++)
				{
					PAL_RemovePlayerStatus(wEventObjectID, x);
				}
			}
			else
			{
				g_fScriptSuccess = FALSE;
			}
		}
		break;
	}

	// Remove equipment from the specified player
	case 0x0023:
	{
		iPlayerRole = pScript->rgwOperand[0];

		if (pScript->rgwOperand[1] == 0)
		{
			//
			// Remove all equipments
			//
			for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
			{
				w = gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole];
				if (w != 0)
				{
					PAL_AddItemToInventory(w, 1);
					gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole] = 0;
				}
				PAL_RemoveEquipmentEffect(iPlayerRole, i);
			}
		}
		else
		{
			w = gpGlobals->g.PlayerRoles.rgwEquipment[pScript->rgwOperand[1] - 1][iPlayerRole];
			if (w != 0)
			{
				PAL_RemoveEquipmentEffect(iPlayerRole, pScript->rgwOperand[1] - 1);
				PAL_AddItemToInventory(w, 1);
				gpGlobals->g.PlayerRoles.rgwEquipment[pScript->rgwOperand[1] - 1][iPlayerRole] = 0;
		}
	}
		break;
	}

	// Set the autoscript entry address for an event object
	case 0x0024:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wAutoScript = pScript->rgwOperand[1];
		}
		break;
	}

	// Set the trigger script entry address for an event object
	case 0x0025:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wTriggerScript = pScript->rgwOperand[1];
		}
		break;
	}

	// Show the buy item menu
	case 0x0026:
	{
		PAL_MakeScene();
		VIDEO_UpdateScreen(gpTextureReal);
		PAL_BuyMenu(pScript->rgwOperand[0]);
		break;
	}

	// Show the sell item menu
	case 0x0027:
	{
		PAL_MakeScene();
		VIDEO_UpdateScreen(gpTextureReal);
		PAL_SellMenu();
		break;
	}

	// Apply poison to enemy
	case 0x0028:
		/*
		28 参数1 参数2	   敌方中毒指令
		参数1 是否全体
		参数2 中毒代号
		作用	   敌方选定对象或全体中参数2代表的毒
		*/
	{
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		WORD wPoisonID = pScript->rgwOperand[1];
		BOOL fJump = FALSE;
		BOOL fAlwaysSuccess = FALSE;
		WORD wSorceryResistance = 0;
		INT iSuccessRate = 0;
		WORD wBaseSuccessRate = 100;

		if (g_Battle.fPlayerMoving)
		{
			wPlayerRole = gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
			wPlayerRole = PAL_NEW_CheckAndGetLegalPlayerTarget(wPlayerRole);
			wBaseSuccessRate += PAL_New_GetPlayerSorceryStrength(wPlayerRole);//巫攻越高下毒成功率越高
		}

		//#ifdef ADD_SOME_POISONS_SUCCESSFULLY_ANYTIME //某些毒可以对任何人均命中（无视敌方巫抗或我方毒抗）
		if (gConfig->m_Function_Set[18])
			if (pScript->rgwOperand[2] == 0xffff
				|| ((LPOBJECT)((LPOBJECT)gpGlobals->g.rgObject))[pScript->rgwOperand[1]].poison.wPoisonLevel >= 99)
			{
				fAlwaysSuccess = TRUE;
			}
		//#endif

		if (fAll)
		{	// 全体
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				w = g_Battle.rgEnemy[i].wObjectID;
				if (w != 0)
				{
					wSorceryResistance = PAL_New_GetEnemySorceryResistance(i);
					iSuccessRate = wBaseSuccessRate - wSorceryResistance;
					if (fAlwaysSuccess || PAL_New_GetTrueByPercentage(iSuccessRate))
					{
						PAL_New_AddPoisonForEnemy(i, wPoisonID);
					}
#ifdef ADD_POISON_FAIL_THEN_JUMP
					else if (Pal->rgwOperand[2] != 0x0000 && Pal->rgwOperand[2] != 0xffff)
					{
						fJump = TRUE;
					}
#endif
				}
			}
		}
		else
		{	//单体
			wEventObjectID = PAL_NEW_CheckAndGetLegalEnemyTarget(wEventObjectID);
			wSorceryResistance = PAL_New_GetEnemySorceryResistance(wEventObjectID);
			iSuccessRate = wBaseSuccessRate - wSorceryResistance;

			if (fAlwaysSuccess || PAL_New_GetTrueByPercentage(iSuccessRate))
			{
				PAL_New_AddPoisonForEnemy(wEventObjectID, wPoisonID);
			}
#ifdef ADD_POISON_FAIL_THEN_JUMP
			else if (Pal->rgwOperand[2] != 0x0000 && Pal->rgwOperand[2] != 0xffff)
			{
				fJump = TRUE;
			}
		}
		if (fJump == TRUE)
		{
			wScriptEntry = Pal->rgwOperand[2] - 1;
		}
#else
		}
#endif
		break;
	}

	// Apply poison to player
	case 0x0029:
		/*
		29 参数1 参数2	   我方中毒指令
		参数1 是否全体
		参数2 中毒代号
		作用	   我方选定对象或全体中参数2代表的毒
		*/
	{
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		WORD wPoisonID = pScript->rgwOperand[1];
		BOOL fJump = FALSE;
		BOOL fAlwaysSuccess = FALSE;
		WORD wPoisonResistance = 0;

//#ifdef ADD_SOME_POISONS_SUCCESSFULLY_ANYTIME 
//某些毒可以对任何人均命中（无视敌方巫抗或我方毒抗）
		if(gConfig->m_Function_Set[18])
			if (pScript->rgwOperand[2] == 0xffff || 
				((LPOBJECT)gpGlobals->g.rgObject)
				[pScript->rgwOperand[1]].poison.wPoisonLevel >= 99)
			{
				fAlwaysSuccess = TRUE;
			}
//#endif
		if (fAll)
		{	// Apply to everyone
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				wPoisonResistance = PAL_GetPlayerPoisonResistance(w);
				if (fAlwaysSuccess || !PAL_New_GetTrueByPercentage(wPoisonResistance))
				{
					PAL_AddPoisonForPlayer(w, pScript->rgwOperand[1]);
				}
#ifdef ADD_POISON_FAIL_THEN_JUMP
				else if (Pal->rgwOperand[2] != 0x0000 && Pal->rgwOperand[2] != 0xffff)
				{
					fJump = TRUE;
				}
#endif
			}
		}
		else
		{	// Apply to one player
			wPoisonResistance = PAL_GetPlayerPoisonResistance(wEventObjectID);
			if (fAlwaysSuccess || !PAL_New_GetTrueByPercentage(wPoisonResistance))
			{
				PAL_AddPoisonForPlayer(wEventObjectID, pScript->rgwOperand[1]);
			}
#ifdef ADD_POISON_FAIL_THEN_JUMP
			else if (Pal->rgwOperand[2] != 0x0000 && Pal->rgwOperand[2] != 0xffff)
			{
				fJump = TRUE;
			}
		}

		if (fJump == TRUE)
		{
			wScriptEntry = Pal->rgwOperand[2] - 1;
		}
#else
		}
#endif
		break;
	}

	// Cure poison by object ID for enemy
	case 0x002A:
	{
		/*
		2A 参数1 参数2	   敌方解毒指令
		参数1 是否全体
		参数2 中毒代号
		作用	   敌方选定对象或全体解除参数2代表的毒
		*/
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		WORD wPoisonID = pScript->rgwOperand[1];

		if (fAll)
		{	// Apply to all enemies
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID != 0)
				{
					PAL_New_CurePoisonForEnemyByKind(i, wPoisonID);
				}
			}
	}
		else
		{	// Apply to one enemy
			PAL_New_CurePoisonForEnemyByKind(wEventObjectID, wPoisonID);
		}
		break;
	}

	// Cure poison by object ID for player
	case 0x002B:
	{
		/*
		我方解毒指令
		参数1 是否全体
		参数2 中毒代号
		作用:		我方选定对象或全体解除参数2代表的毒
		*/
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		WORD wPoisonID = pScript->rgwOperand[1];

		if (fAll)
		{
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_CurePoisonByKind(w, wPoisonID);
			}
		}
		else
		{
			PAL_CurePoisonByKind(wEventObjectID, wPoisonID);
		}
		break;
		}

	// Cure poisons by level
	case 0x002C:
	{
		/*
		2C 参数1 参数2		我方多重解毒指令
		参数1 是否全体
		参数2 整数
		作用		我方选定对象或全体解除毒性不高于参数2的毒
		*/
		BOOL fAll = (pScript->rgwOperand[0] == 0) ? FALSE : TRUE;
		WORD wPoisonID = pScript->rgwOperand[1];

		if (fAll)
		{
			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				w = gpGlobals->rgParty[i].wPlayerRole;
				PAL_CurePoisonByLevel(w, wPoisonID);
			}
		}
		else
		{
			PAL_CurePoisonByLevel(wEventObjectID, wPoisonID);
		}
		break;
	}

	// Set the status for player
	case 0x002D:
		/*
		2D 参数1 参数2		   我方特殊状态指令
		参数1 状态代号		    00：疯魔
		01：定身
		02：昏睡
		03：咒封
		04：死者继续攻击
		05：普通攻击加强
		06：防御加强
		07：身法加强
		08：两次攻击
		参数2 整
		作用		   我方选定对象获得参数1代表特殊状态，持续参数2回合
		*/
	{

//#ifdef INVINCIBLE
		if (gConfig->is_INVINCIBLE)//无敌模式
			if (pScript->rgwOperand[0] < 4)
			{
				break;
			}
//#endif
		BOOL fAlwaysSuccess = FALSE;
		WORD wSorceryResistance = PAL_New_GetPlayerSorceryResistance(wEventObjectID);

		if (fInBattle)
		{
			WORD wPlayerIndex = PAL_New_GetPlayerIndex(wEventObjectID);
			if (wPlayerIndex != 0xffff && g_Battle.rgPlayer[wPlayerIndex].fDefending)
			{
				wSorceryResistance += 10;
			}
		}

		INT iSuccessRate = 0;
		WORD wBaseSuccessRate = 100;
		WORD wStatusID = pScript->rgwOperand[0];
		WORD wNumRound = pScript->rgwOperand[1];

//#ifdef ADD_SOME_STATUSES_SUCCESSFULLY_ANYTIME
		if(gConfig->m_Function_Set[6])
		if (wStatusID >= 4 || pScript->rgwOperand[2] == 0xffff)	//有益状态总是成功
		{
			fAlwaysSuccess = TRUE;
		}
//#endif
		iSuccessRate = wBaseSuccessRate - wSorceryResistance;
		//对特殊状态进行处理，如我方多次被控，无法过关
		if (gpGlobals->wMaxPartyMemberIndex == 0)
			iSuccessRate = 10;
		if (g_Battle.rgPlayer[wEventObjectID].fBadStatus < 10 && fInBattle)
		{
			g_Battle.rgPlayer[wEventObjectID].fBadStatus += wNumRound;
		}
		else iSuccessRate = 10;

		if (fAlwaysSuccess || PAL_New_GetTrueByPercentage(iSuccessRate))
		{   
			
			//仅对不良状态有抗性  //有益状态直接加
			PAL_SetPlayerStatus(wEventObjectID, wStatusID, wNumRound);
			/////
			if (wStatusID < 4)
				gpGlobals->Exp.rgDefenseExp[wEventObjectID].wCount += wNumRound * 2;
		}
		else if (pScript->rgwOperand[2] != 0x0000 && pScript->rgwOperand[2] != 0xffff)
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
		
		break;
	}

	// Set the status for enemy
	case 0x002E:
		/*
		2E 参数1 参数2 参数3	   敌方特殊状态指令
		参数1 状态代号
		参数2 整数
		参数3 脚本地址
		作用	   敌方选定对象获得参数1代表特殊状态，持续参数2回合，若不受该状态影响，则
		跳转参数3指向的脚本地址
		*/
	{
		BOOL fAlwaysSuccess = FALSE;
		BOOL fSorceryIsFull = FALSE;
		WORD wSorceryResistance = 0;
		INT iSuccessRate = 0;
		WORD wBaseSuccessRate = 100;
		WORD wStatusID = pScript->rgwOperand[0];
		WORD wNumRound = pScript->rgwOperand[1];

		wEventObjectID = PAL_NEW_CheckAndGetLegalEnemyTarget(wEventObjectID);
		wSorceryResistance = PAL_New_GetEnemySorceryResistance(wEventObjectID);
		fSorceryIsFull = wSorceryResistance >= 100 ? TRUE : FALSE;

		if (g_Battle.fPlayerMoving)
		{
			wPlayerRole = gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
			wPlayerRole = PAL_NEW_CheckAndGetLegalPlayerTarget(wPlayerRole);	//如果
			wBaseSuccessRate += PAL_New_GetPlayerSorceryStrength(wPlayerRole);//巫攻越高设置状态成功率越高
		}

#ifdef ADD_SOME_STATUSES_SUCCESSFULLY_ANYTIME
		if(gConfig->m_Function_Set[6])
		if (pScript->rgwOperand[2] == 0xffff)
		{
			fAlwaysSuccess = TRUE;
		}
#endif
		iSuccessRate = wBaseSuccessRate - wSorceryResistance;
		if (fAlwaysSuccess || !fSorceryIsFull && PAL_New_GetTrueByPercentage(iSuccessRate))
		{
			PAL_New_SetEnemyStatus(wEventObjectID, wStatusID, wNumRound);
		}
		else
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
		break;
	}

	// Remove player's status
	case 0x002F:
		/*
		2F 参数1	   我方解除特殊状态指令
		参数1 状态代号
		00：疯魔
		01：定身
		02：昏睡
		03：咒封
		04：死者继续攻击
		05：普通攻击加强
		06：防御加强
		07：身法加强
		08：两次攻击
		作用	   我方选定对象解除参数1代表的特殊状态
		*/
	{
		PAL_RemovePlayerStatus(wEventObjectID, pScript->rgwOperand[0]);
		break;
	}

	// Increase player's stat temporarily by percent // 按百分比暂时增加角色属性
	case 0x0030:
		/*
		11：武术
		12：灵力
		13：防御
		14：身法
		15：吉运
		16：毒抗
		17：风抗
		18：雷抗
		19：水抗
		1A：火抗
		1B：土抗
		*/
	{
		WORD* p = (WORD*)(&gpGlobals->rgEquipmentEffect[kBodyPartExtra]); // HACKHACK
		WORD* p1 = (WORD*)(&gpGlobals->g.PlayerRoles);

		if (pScript->rgwOperand[2] == 0)
		{
			iPlayerRole = wEventObjectID;
		}
		else
		{
			iPlayerRole = pScript->rgwOperand[2] - 1;
	}

		p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] =
			p1[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] *
			(SHORT)pScript->rgwOperand[1] / 100;

		break;
		}

	// Change battle sprite temporarily for player
	case 0x0031:
	{
		gpGlobals->rgEquipmentEffect[kBodyPartExtra].rgwSpriteNumInBattle[wEventObjectID] =
			pScript->rgwOperand[0];
		break;
	}

	// collect the enemy for items // 收集妖怪炼丹
	case 0x0033:
	{
		if (g_Battle.rgEnemy[wEventObjectID].e.wCollectValue != 0)
		{
			gpGlobals->wCollectValue +=
				g_Battle.rgEnemy[wEventObjectID].e.wCollectValue;
		}
		else
		{
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		break;
	}

	// Transform collected enemies into items // 灵葫炼丹
	case 0x0034:
		/*
		34 参数1	   灵葫炼丹指令
		参数1：脚本地址
		作用,调用灵葫炼丹程序，根据现有灵葫值，从data.mkf第1个子文件的第一组数据中随
		机炼成一种物品，如灵葫值为0，则跳转参数1指向的脚本地址
		*/
	{
//#ifdef  New_EDIT_SCRIPT_OPERATION_0X0034
		if (gConfig->m_Function_Set[4])
		{
			PAL_BuyMenu(0);
			break;
		}
//#else
		else
		{
			if (gpGlobals->wCollectValue > 0)
			{
				char s[256];

				i = RandomLong(1, gpGlobals->wCollectValue);
				if (i > 9)
				{
					i = 9;
				}
				gpGlobals->wCollectValue -= i;
				i--;
				PAL_AddItemToInventory(gpGlobals->g.lprgStore[0].rgwItems[i], 1);
				PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
				strcpy(s, PAL_GetWord(42));
				strcat(s, PAL_GetWord(gpGlobals->g.lprgStore[0].rgwItems[i]));
				PAL_ShowDialogText(s);
			}
			else
			{
				wScriptEntry = pScript->rgwOperand[0] - 1;
			}
			break;
		}
//#endif
	}
	// Shake the screen // 摇动屏幕
	case 0x0035:
	{
		i = pScript->rgwOperand[1];
		if (i == 0)
		{
			i = 4;
	}
		VIDEO_ShakeScreen(pScript->rgwOperand[0], i);
		if (!pScript->rgwOperand[0])
		{
			VIDEO_UpdateScreen(gpTextureReal);
		}
		break;
		}

	// Set the current playing RNG animation // 设置当前正在播放的RNG动画
	case 0x0036:
	{
		gpGlobals->iCurPlayingRNG = pScript->rgwOperand[0];
		break;
	}

	// Play RNG animation
	case 0x0037:
	{
		PAL_RNGPlay(gpGlobals->iCurPlayingRNG,
			pScript->rgwOperand[0],
			pScript->rgwOperand[1] > 0 ? pScript->rgwOperand[1] : 999,
			pScript->rgwOperand[2] > 0 ? pScript->rgwOperand[2] : 16);
		break;
	}

	// Teleport the party out of the scene
	case 0x0038:
	{
		if (!fInBattle &&
			gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wScriptOnTeleport != 0)
		{
			///BUG 运行过程中场景号发生变化，导致后续跳转失败
			//int wNumScent = gpGlobals->wNumScene - 1;
			//gpGlobals->g.rgScene[wNumScent].wScriptOnTeleport =
				PAL_RunTriggerScript(gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wScriptOnTeleport, 0xFFFF);
		}
		else
		{
			//
			// failed
			//
			g_fScriptSuccess = FALSE;
			wScriptEntry = pScript->rgwOperand[0] - 1;
			}
		break;
		}

	// Drain HP from enemy // 吸取生命
	case 0x0039:
		/*
		吸取生命指令
		参数0：整数
		作用:		吸取选定对象"参数0"点生命，用于补充动作对象的体力
		*/
	{
		SHORT sDamage = min(max(0, (SHORT)pScript->rgwOperand[0]), g_Battle.rgEnemy[wEventObjectID].dwActualHealth);
		g_Battle.rgEnemy[wEventObjectID].dwActualHealth -= sDamage;

		if (g_Battle.fPlayerMoving)
		{
			wPlayerRole = gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
		}
		else
		{
			int index = PAL_New_GetPlayerIndexByHealth(TRUE);
			wPlayerRole = gpGlobals->rgParty[index].wPlayerRole;
			wPlayerRole = PAL_NEW_CheckAndGetLegalPlayerTarget(wPlayerRole);
		}

		PAL_IncreaseHPMP(wPlayerRole, sDamage, 0);
		break;
	}

	// Player flee from the battle // 战斗中逃走
	case 0x003A:
	{
		if (g_Battle.fIsBoss)
		{
			//
			// Cannot flee from bosses
			//
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		else
		{
			PAL_BattlePlayerEscape();
		}
		break;
	}

	// Ride the event object to the specified position, at a low speed
	case 0x003F:
	{
		PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 2);
		break;
	}

	// set the trigger method for a event object
	case 0x0040:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		if (pScript->rgwOperand[0] != 0)
		{
			pCurrent->wTriggerMode = pScript->rgwOperand[1];
		}
		break;
	}

	// Mark the script as failed // 令脚本执行失败
	case 0x0041:
	{
		g_fScriptSuccess = FALSE;
		break;
	}

	// Simulate a magic for player // 模拟法术
	case 0x0042:
	{
		i = (SHORT)(pScript->rgwOperand[2]) - 1;
		if (i < 0)
		{
			i = wEventObjectID;
		}
		PAL_BattleSimulateMagic(i, pScript->rgwOperand[0], pScript->rgwOperand[1], TRUE);
		break;
	}

	// Set background music
	case 0x0043:
	{
		gpGlobals->wNumMusic = pScript->rgwOperand[0];
  		PAL_PlayMUS(pScript->rgwOperand[0], (pScript->rgwOperand[0] != 0x3D), pScript->rgwOperand[1]);
		break;
	}

	// Ride the event object to the specified position, at the normal speed
	case 0x0044:
	{
		PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 4);
		break;
	}

	// Set battle music
	case 0x0045:
	{
		gpGlobals->wNumBattleMusic = pScript->rgwOperand[0];
		break;
	}

	// Set the party position on the map
	case 0x0046:
	{
		int xOffset, yOffset, x, y;

		xOffset =
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
				? 16 : -16);
		yOffset =
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
				? 8 : -8);

		x = pScript->rgwOperand[0] * 32 + pScript->rgwOperand[2] * 16;
		y = pScript->rgwOperand[1] * 16 + pScript->rgwOperand[2] * 8;

		x -= PAL_X(gpGlobals->partyoffset);
		y -= PAL_Y(gpGlobals->partyoffset);

		gpGlobals->viewport = PAL_XY(x, y);

		x = PAL_X(gpGlobals->partyoffset);
		y = PAL_Y(gpGlobals->partyoffset);

		for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
		{
			gpGlobals->rgParty[i].x = x;
			gpGlobals->rgParty[i].y = y;
			gpGlobals->rgTrail[i].x = x + PAL_X(gpGlobals->viewport);
			gpGlobals->rgTrail[i].y = y + PAL_Y(gpGlobals->viewport);
			gpGlobals->rgTrail[i].wDirection = gpGlobals->wPartyDirection;

			x += xOffset;
			y += yOffset;
		}

		break;
	}

	// Play sound effect
	case 0x0047:
	{
		SOUND_Play(pScript->rgwOperand[0]);
		break;
	}

	// Set the state of event object
	case 0x0049:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		pCurrent->sState = pScript->rgwOperand[1];
		break;
	}

	// Set the current battlefield
	case 0x004A:
	{
		gpGlobals->wNumBattleField = pScript->rgwOperand[0];
		break;
	}

	// Nullify the event object for a short while
	case 0x004B:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pEvtObj == NULL)
		{
			break;
		}
#endif
		pEvtObj->sVanishTime = -15;
		break;
	}

	// chase the player
	case 0x004C:
	{
		WORD wMaxDistance = pScript->rgwOperand[0]; // max. distance
		WORD wSpeed = pScript->rgwOperand[1]; // speed

		if (wMaxDistance == 0)
		{
			wMaxDistance = 8;
		}
		if (wSpeed == 0)
		{
			wSpeed = 4;
		}

		PAL_MonsterChasePlayer(wEventObjectID, wSpeed, wMaxDistance, pScript->rgwOperand[2]);
		break;
	}

	// wait for any key
	case 0x004D:
	{
		PAL_WaitForKey(0);
		break;
	}

	// Load the last saved game
	case 0x004E:
	{
		PAL_FadeOut(1);
		PAL_InitGameData(gpGlobals->bCurrentSaveSlot);
		return 0; // don't go further
	}

	// Fade the screen to red color (game over)
	case 0x004F:
	{
		PAL_FadeToRed();
		break;
	}

	// screen fade out
	case 0x0050:
	{
		VIDEO_UpdateScreen(gpTextureReal);
		PAL_FadeOut(pScript->rgwOperand[0] ? pScript->rgwOperand[0] : 1);
		gpGlobals->fNeedToFadeIn = TRUE;
		break;
		}

	// screen fade in
	case 0x0051:
	{
		VIDEO_UpdateScreen(gpTextureReal);
		PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
			((SHORT)(pScript->rgwOperand[0]) > 0) ? pScript->rgwOperand[0] : 1);
		gpGlobals->fNeedToFadeIn = FALSE;
		break;
	}

	// hide the event object for a while, default 800 frames
	case 0x0052:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pEvtObj == NULL)
		{
			break;
		}
#endif
		pEvtObj->sState *= -1;
		pEvtObj->sVanishTime = (pScript->rgwOperand[0] ? pScript->rgwOperand[0] : 800);
		break;
		}

	// use the day palette
	case 0x0053:
	{
		gpGlobals->fNightPalette = FALSE;
		break;
	}

	// use the night palette
	case 0x0054:
	{
		gpGlobals->fNightPalette = TRUE;
		break;
	}

	//Add magic to a player
	case 0x0055:
	{
		i = pScript->rgwOperand[1];
		if (i == 0)
		{
			i = wEventObjectID;
		}
		else
		{
			i--;
		}
		PAL_AddMagic(i, pScript->rgwOperand[0]);
		break;
	}

	// 移除仙术
	case 0x0056:
	{
		i = pScript->rgwOperand[1];
		if (i == 0)
		{
			i = wEventObjectID;
		}
		else
		{
			i--;
		}
		PAL_RemoveMagic(i, pScript->rgwOperand[0]);
		break;
	}

	//根据真气设定仙术的基础伤害值
	case 0x0057:
	{
		WORD wBaseDamage = 0;
		WORD wCostMP = gpGlobals->g.PlayerRoles.rgwMP[wEventObjectID];

#ifdef FINISH_GAME_MORE_ONE_TIME
		if (gpGlobals->bFinishGameTime < 2)
		{
			wCostMP = min(wCostMP, MAX_PARAMETER + MAX_PARAMETER_EXTRA);
		}
#endif
		i = ((pScript->rgwOperand[1] == 0) ? 8 : pScript->rgwOperand[1]);
		j = ((LPOBJECT)gpGlobals->g.rgObject)[pScript->rgwOperand[0]].magic.wMagicNumber;
		wBaseDamage = min(wCostMP * i, MAX_DAMAGE);
		gpGlobals->g.lprgMagic[j].wBaseDamage = wBaseDamage;
		gpGlobals->g.PlayerRoles.rgwMP[wEventObjectID] -= wCostMP;
		break;
	}

	// Jump if there is less than the specified number of the specified items in the inventory
	case 0x0058:
	{
		if (PAL_GetItemAmount(pScript->rgwOperand[0]) < (SHORT)(pScript->rgwOperand[1]))
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
		break;
	}

	// Change to the specified scene
	case 0x0059:
	{
		if (pScript->rgwOperand[0] > 0 && pScript->rgwOperand[0] <= MAX_SCENES &&
			gpGlobals->wNumScene != pScript->rgwOperand[0])
		{
			//
			// Set data to load the scene in the next frame
			//
			gpGlobals->wNumScene = pScript->rgwOperand[0];
			PAL_SetLoadFlags(kLoadScene);
			gpGlobals->fEnteringScene = TRUE;
			gpGlobals->wLayer = 0;
		}
		break;
	}

	// Halve the player's HP
	case 0x005A:
	{
		wPlayerRole = wEventObjectID;
		gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] /= 2;
		break;
	}

	// Halve the enemy's HP
	case 0x005B:
	{
		WORD wMaxDamage = pScript->rgwOperand[0];
		DWORD dwActualHealth = g_Battle.rgEnemy[wEventObjectID].dwActualHealth;
		INT iDamage = min(dwActualHealth / 2 + 1, wMaxDamage);
		iDamage = min(iDamage, dwActualHealth);
		g_Battle.rgEnemy[wEventObjectID].dwActualHealth -= iDamage;
		break;
	}

	// Hide for a while
	case 0x005C:
	{
		g_Battle.iHidingTime = (INT)(pScript->rgwOperand[0]);
		break;
	}

	// Jump if player doesn't have the specified poison // 如果角色没有中某一种特定的毒，则跳转
	case 0x005D:
	{
		if (PAL_New_GetPoisonIndexForPlayer(wEventObjectID, pScript->rgwOperand[0]) == -1)
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;
	}

	// Jump if enemy doesn't have the specified poison
	case 0x005E:
	{
		if (PAL_New_GetPoisonIndexForEnemy(wEventObjectID, pScript->rgwOperand[0]) == -1)
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;
	}

	// Kill the player immediately
	case 0x005F:
	{
		//设置冷却值 在之后的n回合内夺魂无效
		if (g_Battle.rgPlayer[wEventObjectID].fNotKillImmediately)
		{
			g_Battle.rgPlayer[wEventObjectID].fNotKillImmediately--;
		}
		else
		{
			gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] = 0;
			g_Battle.rgPlayer[wEventObjectID].fNotKillImmediately = gConfig->m_Function_Set[10];
		}
		gpGlobals->Exp.rgFleeExp[wEventObjectID].wCount += RandomLong(1, 2);

		break;
	}

	// Kill the enemy immediately
	case 0x0060:
	{
		g_Battle.rgEnemy[wEventObjectID].dwActualHealth = 0;
		break;
	}

	// Jump if player is not poisoned
	case 0x0061:
	{
		if (PAL_New_IsPlayerPoisoned(wEventObjectID) == FALSE) //之前用的函数不能正确判断等级为0的毒，比如赤毒
		{
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		break;
	}

	// Pause enemy chasing for a while
	case 0x0062:
	{
		gpGlobals->wChasespeedChangeCycles = pScript->rgwOperand[0];
		gpGlobals->wChaseRange = 0;
		break;
	}

	// Speed up enemy chasing for a while
	case 0x0063:
	{
		gpGlobals->wChasespeedChangeCycles = pScript->rgwOperand[0];
		gpGlobals->wChaseRange = 3;
		break;
	}

	// Jump if enemy's HP is more than the specified percentage
	case 0x0064:
	{
		if ((g_Battle.rgEnemy[wEventObjectID].dwActualHealth) * 100 >
			(g_Battle.rgEnemy[wEventObjectID].dwMaxHealth) * pScript->rgwOperand[0])
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;
	}

	// Set the player's sprite
	case 0x0065:
	{
		gpGlobals->g.PlayerRoles.rgwSpriteNum[pScript->rgwOperand[0]] = pScript->rgwOperand[1];
		if (!fInBattle && pScript->rgwOperand[2])
		{
			PAL_SetLoadFlags(kLoadPlayerSprite);
			PAL_LoadResources();
		}
		break;
	}

	// Throw weapon to enemy
	case 0x0066:
	{
		WORD wBaseDamage = pScript->rgwOperand[1] * 5;
		wBaseDamage += RandomLong(0, 4);

		if (g_Battle.fPlayerMoving)
		{
			wPlayerRole = gpGlobals->rgParty[g_Battle.wMovingPlayerIndex].wPlayerRole;
			wBaseDamage += gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole];
			wBaseDamage += PAL_GetPlayerAttackStrength(wPlayerRole);
			/////
			if (gConfig->m_Function_Set[20])
				if (g_Battle.rgPlayer[g_Battle.wMovingPlayerIndex].action.ActionType == kBattleActionThrowItem)
					if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusBravery] && wBaseDamage > 50)
						wBaseDamage *= 3;
		}
		else
		{
			wBaseDamage += 100;
		}

#ifdef REMOVE_TEMPORARY_BASEDAMAGE_WHEN_SIMULATE_MAGIC
		wBaseDamage += RandomLong(200, 205);
		PAL_BattleSimulateMagic((SHORT)wEventObjectID, Pal->rgwOperand[0], wBaseDamage, FALSE);
#else
		PAL_BattleSimulateMagic((SHORT)wEventObjectID, pScript->rgwOperand[0], wBaseDamage, TRUE);
#endif
		break;
	}

	// Enemy use magic
	case 0x0067:
	{
		g_Battle.rgEnemy[wEventObjectID].e.wMagic = pScript->rgwOperand[0];
		g_Battle.rgEnemy[wEventObjectID].e.wMagicRate =
			((pScript->rgwOperand[1] == 0) ? 10 : pScript->rgwOperand[1]);
		break;
	}

	// Jump if it's enemy's turn
	case 0x0068:
	{
		if (g_Battle.fEnemyMoving)
		{
			wScriptEntry = pScript->rgwOperand[0] - 1;
		}
		break;
	}

	// Enemy escape in battle
	case 0x0069:
	{
		PAL_BattleEnemyEscape();
		break;
	}

	// Steal from the enemy
	case 0x006A:
	{
		PAL_BattleStealFromEnemy(wEventObjectID, pScript->rgwOperand[0]);
		break;
	}

	// Blow away enemies
	case 0x006B:
	{
		g_Battle.iBlow = (SHORT)(pScript->rgwOperand[0]);
		break;
	}

	// Walk the NPC in one step
	case 0x006C:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		pCurrent->x += (SHORT)(pScript->rgwOperand[1]);
		pCurrent->y += (SHORT)(pScript->rgwOperand[2]);
		PAL_NPCWalkOneStep(wCurEventObjectID, 0);
		break;
	}

	// Set the enter script and teleport script for a scene
	case 0x006D:
	{
		if (pScript->rgwOperand[0])
		{
			if (pScript->rgwOperand[1])
			{
				gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnEnter =
					pScript->rgwOperand[1];
			}

			if (pScript->rgwOperand[2])
			{
				gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnTeleport =
					pScript->rgwOperand[2];
			}

			if (pScript->rgwOperand[1] == 0 && pScript->rgwOperand[2] == 0)
			{
				gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnEnter = 0;
				gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnTeleport = 0;
			}
		}
		break;
	}

	// Move the player to the specified position in one step
	case 0x006E:
	{
		for (i = MAX_PLAYABLE_PLAYER_ROLES - 2; i >= 0; i--)
		{
			gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
		}
		gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
		gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		gpGlobals->viewport = PAL_XY(
			PAL_X(gpGlobals->viewport) + (SHORT)(pScript->rgwOperand[0]),
			PAL_Y(gpGlobals->viewport) + (SHORT)(pScript->rgwOperand[1]));

		gpGlobals->wLayer = pScript->rgwOperand[2] * 8;

		if (pScript->rgwOperand[0] != 0 || pScript->rgwOperand[1] != 0)
		{
			PAL_UpdatePartyGestures(TRUE);
		}
		break;
	}

	// Sync the state of current event object with another event object
	case 0x006F:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pEvtObj == NULL || pCurrent == NULL)
		{
			break;
		}
#endif
		if (pCurrent->sState == (SHORT)(pScript->rgwOperand[1]))
		{
			pEvtObj->sState = (SHORT)(pScript->rgwOperand[1]);
		}
		break;
	}

	// Walk the party to the specified position
	case 0x0070:
	{
		PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 2);
		break;
	}

	// Wave the screen
	case 0x0071:
	{
		gpGlobals->wScreenWave = pScript->rgwOperand[0];
		gpGlobals->sWaveProgression = (SHORT)(pScript->rgwOperand[1]);
		break;
	}

	// Fade the screen to scene
	case 0x0073:
	{
		VIDEO_BackupScreen();
		PAL_MakeScene();
		VIDEO_FadeScreen(pScript->rgwOperand[0]);
		break;
		}

	// Jump if not all players are full HP
	case 0x0074:
	{
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			if (gpGlobals->g.PlayerRoles.rgwHP[w] < gpGlobals->g.PlayerRoles.rgwMaxHP[w])
			{
				wScriptEntry = pScript->rgwOperand[0] - 1;
				break;
			}
		}
		break;
	}

	// Set the player party
	case 0x0075:
	{
		gpGlobals->wMaxPartyMemberIndex = 0;
		for (i = 0; i < 3; i++)
		{
			if (pScript->rgwOperand[i] != 0)
			{
				gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex].wPlayerRole =
					pScript->rgwOperand[i] - 1;

				g_Battle.rgPlayer[gpGlobals->wMaxPartyMemberIndex].action.ActionType =
					kBattleActionAttack;

				gpGlobals->wMaxPartyMemberIndex++;
			}
		}

		if (gpGlobals->wMaxPartyMemberIndex == 0)
		{
			// HACK for Dream 2.11
			gpGlobals->rgParty[0].wPlayerRole = 0;
			gpGlobals->wMaxPartyMemberIndex = 1;
		}

		gpGlobals->wMaxPartyMemberIndex--;

		//
		// Reload the player sprites
		//
		PAL_SetLoadFlags(kLoadPlayerSprite);
		PAL_LoadResources();

		memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
		PAL_UpdateEquipments();
		break;
	}

	// Show FBP picture
	case 0x0076:
	{
/*#ifdef PAL_WIN95
		SDL_FillRect(gpScreen, NULL, 0);
		VIDEO_UpdateScreen(gpTextureReal);
#else
		PAL_EndingSetEffectSprite(0);
		PAL_ShowFBP(pScript->rgwOperand[0], pScript->rgwOperand[1]);
#endif*/
		if (gConfig->fIsWIN95)
		{
			//SDL_FillRect(gpScreen, NULL, 0);
			VIDEO_UpdateScreen(NULL);
		}
		else
		{
			PAL_EndingSetEffectSprite(0);
			PAL_ShowFBP(pScript->rgwOperand[0], pScript->rgwOperand[1]);
		}
		break;
	}

	// Stop current playing music
	case 0x0077:
	{
		PAL_PlayMUS(0, FALSE,
			(pScript->rgwOperand[0] == 0) ? 2.0f : (FLOAT)(pScript->rgwOperand[0]) * 2);
		gpGlobals->wNumMusic = 0;
		break;
	}

	// FIXME: ???
	case 0x0078:
	{
		break;
	}

	// Jump if the specified player is in the party
	case 0x0079:
	{
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			if (gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole] ==
				pScript->rgwOperand[0])
			{
				wScriptEntry = pScript->rgwOperand[1] - 1;
				break;
			}
		}
		break;
	}

	// Walk the party to the specified position, at a higher speed
	case 0x007A:
	{
		PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 4);
		break;
	}

	// Walk the party to the specified position, at the highest speed
	case 0x007B:
	{
		PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 8);
		break;
	}

	// Walk straight to the specified position
	case 0x007C:
	{
		if ((wEventObjectID & 1) ^ (gpGlobals->dwFrameNum & 1))
		{
			if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
				pScript->rgwOperand[2], 4))
			{
				wScriptEntry--;
			}
		}
		else
		{
			wScriptEntry--;
		}
		break;
	}

	// Move the event object
	case 0x007D:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		pCurrent->x += (SHORT)(pScript->rgwOperand[1]);
		pCurrent->y += (SHORT)(pScript->rgwOperand[2]);
		break;
	}

	// Set the layer of event object
	case 0x007E:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		pCurrent->sLayer = (SHORT)(pScript->rgwOperand[1]);
		break;
	}

	// Move the viewport
	case 0x007F:
	{
		if (pScript->rgwOperand[0] == 0 && pScript->rgwOperand[1] == 0)
		{
			//
			// Move the viewport back to normal state
			//
			x = gpGlobals->rgParty[0].x - 160;
			y = gpGlobals->rgParty[0].y - 112;

			gpGlobals->viewport =
				PAL_XY(PAL_X(gpGlobals->viewport) + x, PAL_Y(gpGlobals->viewport) + y);
			gpGlobals->partyoffset = PAL_XY(160, 112);

			for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
			{
				gpGlobals->rgParty[i].x -= x;
				gpGlobals->rgParty[i].y -= y;
			}

			if (pScript->rgwOperand[2] != 0xFFFF)
			{
				PAL_MakeScene();
				VIDEO_UpdateScreen(gpTextureReal);
			}
		}
		else
		{
			DWORD time;

			i = 0;

			x = (SHORT)(pScript->rgwOperand[0]);
			y = (SHORT)(pScript->rgwOperand[1]);

			time = SDL_GetTicks_New() + gConfig->m_Function_Set[24];

			do
			{
				if (pScript->rgwOperand[2] == 0xFFFF)
				{
					x = PAL_X(gpGlobals->viewport);
					y = PAL_Y(gpGlobals->viewport);

					gpGlobals->viewport =
						PAL_XY(pScript->rgwOperand[0] * 32 - 160, pScript->rgwOperand[1] * 16 - 112);

					x -= PAL_X(gpGlobals->viewport);
					y -= PAL_Y(gpGlobals->viewport);

					for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
					{
						gpGlobals->rgParty[j].x += x;
						gpGlobals->rgParty[j].y += y;
					}
				}
				else
				{
					gpGlobals->viewport =
						PAL_XY(PAL_X(gpGlobals->viewport) + x, PAL_Y(gpGlobals->viewport) + y);
					gpGlobals->partyoffset =
						PAL_XY(PAL_X(gpGlobals->partyoffset) - x, PAL_Y(gpGlobals->partyoffset) - y);

					for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
					{
						gpGlobals->rgParty[j].x -= x;
						gpGlobals->rgParty[j].y -= y;
					}
				}

				if (pScript->rgwOperand[2] != 0xFFFF)
				{
					PAL_GameUpdate(FALSE);
				}

				PAL_MakeScene();
				VIDEO_UpdateScreen(gpTextureReal);

				//
				// Delay for one frame
				//
				PAL_ProcessEvent();
				while (SDL_GetTicks_New() < time)
				{
					PAL_ProcessEvent();
					SDL_Delay(1);
				}
				time = SDL_GetTicks_New() + gConfig->m_Function_Set[24];
			} while (++i < (SHORT)(pScript->rgwOperand[2]));
		}
		break;
				}

	// Toggle day/night palette
	case 0x0080:
	{
		gpGlobals->fNightPalette = !(gpGlobals->fNightPalette);
		PAL_PaletteFade(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
			!(pScript->rgwOperand[0]));
		break;
	}

	// Jump if the player is not facing the specified event object
	case 0x0081:
	{
		if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
			pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
		{
			//
			// The event object is not in the current scene
			//
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
			break;
		}
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		x = pCurrent->x;
		y = pCurrent->y;

		x +=
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
				? 16 : -16);
		y +=
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
				? 8 : -8);

		x -= PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		y -= PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		if (abs(x) + abs(y * 2) < pScript->rgwOperand[1] * 32 + 16)
		{
			if (pScript->rgwOperand[1] > 0)
			{
				//
				// Change the trigger mode so that the object can be triggered in next frame
				//
#ifdef AVOID_SCRIPT_CRASH
				if (pCurrent == NULL)
				{
					break;
				}
#endif
				pCurrent->wTriggerMode = kTriggerTouchNormal + pScript->rgwOperand[1];
			}
		}
		else
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
		}

		break;
	}

	// Walk straight to the specified position, at a high speed
	case 0x0082:
	{
		if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 8))
		{
			wScriptEntry--;
		}
		break;
	}

	// Jump if event object is not in the specified zone of the current event object
	case 0x0083:
	{
		if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
			pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
		{
			//
			// The event object is not in the current scene
			//
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
			break;
		}

#ifdef AVOID_SCRIPT_CRASH
		if (pEvtObj == NULL || pCurrent == NULL)
		{
			break;
		}
#endif
		x = pEvtObj->x - pCurrent->x;
		y = pEvtObj->y - pCurrent->y;

		if (abs(x) + abs(y * 2) >= pScript->rgwOperand[1] * 32 + 16)
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
		}
		break;
	}

	// Place the item which player used as an event object to the scene
	case 0x0084:
	{
		if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
			pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
		{
			//
			// The event object is not in the current scene
			//
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
			break;
		}

		x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		x +=
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
				? -16 : 16);
		y +=
			((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
				? -8 : 8);

		if (PAL_CheckObstacle(PAL_XY(x, y), FALSE, 0))
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
			g_fScriptSuccess = FALSE;
		}
		else
		{
#ifdef AVOID_SCRIPT_CRASH
			if (pCurrent == NULL)
			{
				break;
			}
#endif
			pCurrent->x = x;
			pCurrent->y = y;
			pCurrent->sState = (SHORT)(pScript->rgwOperand[1]);
		}
		break;
	}

	// Delay for a period
	case 0x0085:
	{
		PAL_Delay(pScript->rgwOperand[0] * 80);
		break;
	}

	// Jump if the specified item is not equipped
	case 0x0086:
	{
		y = FALSE;
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			w = gpGlobals->rgParty[i].wPlayerRole;
			for (x = 0; x < MAX_PLAYER_EQUIPMENTS; x++)
			{
				if (gpGlobals->g.PlayerRoles.rgwEquipment[x][w] == pScript->rgwOperand[0])
				{
					y = TRUE;
					i = 999;
					break;
				}
			}
		}
		if (!y)
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
		break;
	}

	// Animate the event object
	case 0x0087:
	{
		PAL_NPCWalkOneStep(wCurEventObjectID, 0);
		break;
	}

	// Set the base damage of magic according to amount of money // 根据所剩金钱设置基础伤害值
	case 0x0088:
	{
		i = ((gpGlobals->dwCash > 5000) ? 5000 : gpGlobals->dwCash);
		gpGlobals->dwCash -= i;
		j = ((LPOBJECT)gpGlobals->g.rgObject)[pScript->rgwOperand[0]].magic.wMagicNumber;
		gpGlobals->g.lprgMagic[j].wBaseDamage = i * 2 / 5;
		break;
		}

	// Set the battle result
	case 0x0089:
	{
		g_Battle.BattleResult = (BATTLERESULT)pScript->rgwOperand[0];
		break;
	}

	// Enable Auto-Battle for next battle
	case 0x008A:
	{
		gpGlobals->fAutoBattle = TRUE;
		break;
	}

	// change the current palette
	case 0x008B:
	{
		gpGlobals->wNumPalette = pScript->rgwOperand[0];
		if (!gpGlobals->fNeedToFadeIn)
		{
			PAL_SetPalette(gpGlobals->wNumPalette, FALSE);
		}
		break;
	}

	// Fade from/to color
	case 0x008C:
	{
		PAL_ColorFade(pScript->rgwOperand[1], (BYTE)(pScript->rgwOperand[0]),
			pScript->rgwOperand[2]);
		gpGlobals->fNeedToFadeIn = FALSE;
		break;
	}

	// Increase player's level
	case 0x008D:
	{
		PAL_PlayerLevelUp(wEventObjectID, pScript->rgwOperand[0]);
		break;
	}

	// Halve the cash amount
	case 0x008F:
	{
		gpGlobals->dwCash /= 2;
		break;
	}

	// Set the object script
	case 0x0090:
	{
		((LPOBJECT)gpGlobals->g.rgObject)[pScript->rgwOperand[0]].rgwData[2 + pScript->rgwOperand[2]] =
			pScript->rgwOperand[1];
		break;
	}

	// Jump if the enemy is not alone
	case 0x0091:
	{
		if (fInBattle)
		{
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (i != wEventObjectID &&
					g_Battle.rgEnemy[i].wObjectID == g_Battle.rgEnemy[wEventObjectID].wObjectID)
				{
					wScriptEntry = pScript->rgwOperand[0] - 1;
					break;
				}
			}
		}
		break;
	}

	// Show a magic-casting animation for a player in battle
	case 0x0092:
	{
		if (fInBattle)
		{
			if (pScript->rgwOperand[0] != 0)
			{
				PAL_BattleShowPlayerPreMagicAnim(pScript->rgwOperand[0] - 1, FALSE);
				g_Battle.rgPlayer[pScript->rgwOperand[0] - 1].wCurrentFrame = 6;
			}

			for (i = 0; i < 5; i++)
			{
				for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
				{
					g_Battle.rgPlayer[j].iColorShift = i * 2;
				}
				PAL_BattleDelay(1, 0, TRUE);
			}
			PAL_BattleBackupScene();
			PAL_BattleUpdateFighters();
			PAL_BattleMakeScene();
			RenderBlendCopy(gpTextureReal, g_Battle.lpSceneBuf);;
			PAL_BattleFadeScene();
		}
		break;
	}

	// Fade the screen. Update scene in the process.
	case 0x0093:
	{
		PAL_SceneFade(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
			(SHORT)(pScript->rgwOperand[0]));
		gpGlobals->fNeedToFadeIn = ((SHORT)(pScript->rgwOperand[0]) < 0);
		break;
	}

	// Jump if the state of event object is the specified one
	case 0x0094:
	{
#ifdef AVOID_SCRIPT_CRASH
		if (pCurrent == NULL)
		{
			break;
		}
#endif
		if (pCurrent->sState == (SHORT)(pScript->rgwOperand[1]))
		{
			wScriptEntry = pScript->rgwOperand[2] - 1;
		}
		break;
	}

	// Jump if the current scene is the specified one
	case 0x0095:
	{
		if (gpGlobals->wNumScene == pScript->rgwOperand[0])
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;
	}

	// Show the ending animation
	case 0x0096:
	{
//#ifndef PAL_WIN95
		if(!gConfig->fIsWIN95)
			PAL_NEW_EndingScreen1();
		//PAL_EndingAnimation();
//#endif
		break;
	}

	// Ride the event object to the specified position, at a higher speed
	case 0x0097:
	{
		PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
			pScript->rgwOperand[2], 8);
		break;
	}

	// Set follower of the party
	case 0x0098:
	{
		if (pScript->rgwOperand[0] > 0 && gpGlobals->wMaxPartyMemberIndex < 4)
		{
			gpGlobals->nFollower = 1;
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].wPlayerRole = pScript->rgwOperand[0];

			PAL_SetLoadFlags(kLoadPlayerSprite);
			PAL_LoadResources();

			//
			// Update the position and gesture for the follower
			//
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].x =
				gpGlobals->rgTrail[gpGlobals->wMaxPartyMemberIndex].x - PAL_X(gpGlobals->viewport);
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].y =
				gpGlobals->rgTrail[gpGlobals->wMaxPartyMemberIndex].y - PAL_Y(gpGlobals->viewport);
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].wFrame =
				gpGlobals->rgTrail[gpGlobals->wMaxPartyMemberIndex].wDirection * 3;
		}
		else
		{
			gpGlobals->nFollower = 0;
		}
		break;
	}

	// Change the map for the specified scene
	case 0x0099:
	{
		if (pScript->rgwOperand[0] == 0xFFFF)
		{
			gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wMapNum = pScript->rgwOperand[1];
			PAL_SetLoadFlags(kLoadScene);
			PAL_LoadResources();
		}
		else
		{
			gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wMapNum = pScript->rgwOperand[1];
		}
		break;
	}

	// Set the state for multiple event objects
	case 0x009A:
	{
		for (i = pScript->rgwOperand[0]; i <= pScript->rgwOperand[1]; i++)
		{
			gpGlobals->g.lprgEventObject[i - 1].sState = pScript->rgwOperand[2];
		}
		break;
	}

	// Fade to the current scene // FIXME: This is obviously wrong
	case 0x009B:
	{
		VIDEO_BackupScreen();
		PAL_MakeScene();
		VIDEO_FadeScreen(2);
		break;
	}

	// Enemy duplicate itself // 怪物复制自身
	case 0x009C:
	{
		w = 0;

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID != 0)
			{
				w++;
			}
		}

		if (w != 1)
		{
			//
			// Duplication is only possible when only 1 enemy left
			//
			if (pScript->rgwOperand[1] != 0)
			{
				wScriptEntry = pScript->rgwOperand[1] - 1;
			}
			break;
		}

		w = pScript->rgwOperand[0];
		if (w == 0)
		{
			w = 1;
		}

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (w > 0 && g_Battle.rgEnemy[i].wObjectID == 0)
			{
				w--;
				//需要释放图像
				if (g_Battle.rgEnemy[i].lpSprite)
					free(g_Battle.rgEnemy[i].lpSprite);

				memset(&(g_Battle.rgEnemy[i]), 0, sizeof(BATTLEENEMY));

				g_Battle.rgEnemy[i].wObjectID = g_Battle.rgEnemy[wEventObjectID].wObjectID;
				g_Battle.rgEnemy[i].e = g_Battle.rgEnemy[wEventObjectID].e;
				g_Battle.rgEnemy[i].dwMaxHealth = g_Battle.rgEnemy[wEventObjectID].dwMaxHealth;
				
				if(gConfig->m_Function_Set[26])
					g_Battle.rgEnemy[wEventObjectID].dwActualHealth /=2;
				
				g_Battle.rgEnemy[i].dwActualHealth = g_Battle.rgEnemy[wEventObjectID].dwActualHealth;

				g_Battle.rgEnemy[i].dwExp = g_Battle.rgEnemy[wEventObjectID].dwExp;

				g_Battle.rgEnemy[i].wScriptOnTurnStart = g_Battle.rgEnemy[wEventObjectID].wScriptOnTurnStart;
				g_Battle.rgEnemy[i].wScriptOnBattleEnd = g_Battle.rgEnemy[wEventObjectID].wScriptOnBattleEnd;
				g_Battle.rgEnemy[i].wScriptOnReady = g_Battle.rgEnemy[wEventObjectID].wScriptOnReady;

				g_Battle.rgEnemy[i].state = kFighterWait;
				g_Battle.rgEnemy[i].flTimeMeter = 50;
				g_Battle.rgEnemy[i].iColorShift = 0;
			}
		}

		PAL_LoadBattleSprites();

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID == 0)
			{
				continue;
			}
			g_Battle.rgEnemy[i].pos = g_Battle.rgEnemy[wEventObjectID].pos;
		}

		for (i = 0; i < 10; i++)
		{
			for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
			{
				x = (PAL_X(g_Battle.rgEnemy[j].pos) + PAL_X(g_Battle.rgEnemy[j].posOriginal)) / 2;
				y = (PAL_Y(g_Battle.rgEnemy[j].pos) + PAL_Y(g_Battle.rgEnemy[j].posOriginal)) / 2;

				g_Battle.rgEnemy[j].pos = PAL_XY(x, y);
			}

			PAL_BattleDelay(1, 0, TRUE);
		}

		PAL_BattleUpdateFighters();
		PAL_BattleDelay(1, 0, TRUE);
		break;
	}

	// Enemy summons another monster
	case 0x009E:
	{
		WORD wEmptyEnemySiteNum = 0;
		WORD wNewEnemyID = pScript->rgwOperand[0];//召唤怪物的id
		WORD wNewEnemyNum = max(pScript->rgwOperand[1], 1);//召唤怪物的数量

		if (wNewEnemyID == 0 || wNewEnemyID == 0xFFFF)
		{
			wNewEnemyID = g_Battle.rgEnemy[wEventObjectID].wObjectID;
		}

		for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
		{
			if (g_Battle.rgEnemy[i].wObjectID == 0)
			{
				wEmptyEnemySiteNum++;
			}
		}

		if (wEmptyEnemySiteNum < wNewEnemyNum || !PAL_New_IfEnemyCanMove(wEventObjectID))
		{
			if (pScript->rgwOperand[2] != 0)
			{	//召唤失败，则跳转
				wScriptEntry = pScript->rgwOperand[2] - 1;
			}
		}
		else
		{
			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				if (g_Battle.rgEnemy[i].wObjectID == 0)
				{
					//需要释放图像
					if (g_Battle.rgEnemy[i].lpSprite)
						free(g_Battle.rgEnemy[i].lpSprite);
					memset(&(g_Battle.rgEnemy[i]), 0, sizeof(BATTLEENEMY));

					g_Battle.rgEnemy[i].wObjectID = wNewEnemyID;
					g_Battle.rgEnemy[i].e = gpGlobals->g.lprgEnemy[((LPOBJECT)gpGlobals->g.rgObject)[wNewEnemyID].enemy.wEnemyID];
					g_Battle.rgEnemy[i].dwMaxHealth = g_Battle.rgEnemy[i].e.wHealth;
					g_Battle.rgEnemy[i].dwActualHealth = g_Battle.rgEnemy[i].e.wHealth;
					g_Battle.rgEnemy[i].dwExp = g_Battle.rgEnemy[i].e.wExps;
//#ifdef STRENGTHEN_ENEMY
					if (gConfig->m_Function_Set[3])
						g_Battle.rgEnemy[i] = PAL_New_StrengthenEnemy(g_Battle.rgEnemy[i]);
//#endif
					g_Battle.rgEnemy[i].state = kFighterWait;
					g_Battle.rgEnemy[i].wScriptOnTurnStart = ((LPOBJECT)gpGlobals->g.rgObject)[wNewEnemyID].enemy.wScriptOnTurnStart;
					g_Battle.rgEnemy[i].wScriptOnBattleEnd = ((LPOBJECT)gpGlobals->g.rgObject)[wNewEnemyID].enemy.wScriptOnBattleEnd;
					g_Battle.rgEnemy[i].wScriptOnReady = ((LPOBJECT)gpGlobals->g.rgObject)[wNewEnemyID].enemy.wScriptOnReady;
					g_Battle.rgEnemy[i].flTimeMeter = 50;
					g_Battle.rgEnemy[i].iColorShift = 8;

					wNewEnemyNum--;
					if (wNewEnemyNum == 0)
					{
						break;
					}
				}
			}

			PAL_BattleDelay(2, 0, TRUE);

			PAL_BattleBackupScene();
			PAL_LoadBattleSprites();
			PAL_BattleMakeScene();
			RenderBlendCopy(gpTextureReal, g_Battle.lpSceneBuf);;
			SOUND_Play(212);
			PAL_BattleFadeScene();

			for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
			{
				g_Battle.rgEnemy[i].iColorShift = 0;
			}

			VIDEO_BackupScreen();
			PAL_BattleMakeScene();
			RenderBlendCopy(gpTextureReal, g_Battle.lpSceneBuf);;
			PAL_BattleFadeScene();
		}
		break;
	}

	// Enemy transforms into something else
	case 0x009F:
	{
		WORD wEnemyIndex = wEventObjectID;
		WORD wNewEnemyID = pScript->rgwOperand[0];

		if (PAL_New_IfEnemyCanMove(wEnemyIndex))
		{
			WORD wPrveLevel = g_Battle.rgEnemy[wEnemyIndex].e.wLevel;
			WORD wPrevCollectValue = g_Battle.rgEnemy[wEnemyIndex].e.wCollectValue;
			DWORD wPrevExp = g_Battle.rgEnemy[wEnemyIndex].dwExp;

			g_Battle.rgEnemy[wEnemyIndex].wObjectID = wNewEnemyID;
			g_Battle.rgEnemy[wEnemyIndex].e =
				gpGlobals->g.lprgEnemy[((LPOBJECT)gpGlobals->g.rgObject)[wNewEnemyID].enemy.wEnemyID];

			g_Battle.rgEnemy[wEnemyIndex].e.wLevel = max(g_Battle.rgEnemy[wEnemyIndex].e.wLevel, wPrveLevel);
			g_Battle.rgEnemy[wEnemyIndex].e.wCollectValue = max(g_Battle.rgEnemy[wEnemyIndex].e.wCollectValue, wPrevCollectValue);
			g_Battle.rgEnemy[wEnemyIndex].dwExp = max(g_Battle.rgEnemy[wEnemyIndex].dwExp, wPrevExp);

			g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame = 0;
			g_Battle.rgEnemy[wEnemyIndex].wScriptOnBattleEnd =
				((LPOBJECT)gpGlobals->g.rgObject)[wNewEnemyID].enemy.wScriptOnBattleEnd; //修正怪物变身后的战后脚本

			for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
			{
				g_Battle.rgEnemy[wEnemyIndex].iColorShift = i;
				PAL_BattleDelay(1, 0, FALSE);
			}

			g_Battle.rgEnemy[wEnemyIndex].iColorShift = 0;

			PAL_BattleBackupScene();
			PAL_LoadBattleSprites();
			PAL_BattleMakeScene();
			RenderBlendCopy(gpTextureReal, g_Battle.lpSceneBuf);
			PAL_BattleFadeScene();
		}
		break;
	}

	// Quit game
	case 0x00A0:
	{
		if (gConfig->fIsWIN95)
			PAL_EndingScreen();
		PalQuit = TRUE;
		break;
	}

	//Set the positions of all party members to the same as the first one
	case 0x00A1:
	{

		for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
		{
			gpGlobals->rgTrail[i].wDirection = gpGlobals->wPartyDirection;
			gpGlobals->rgTrail[i].x = gpGlobals->rgParty[0].x + PAL_X(gpGlobals->viewport);
			gpGlobals->rgTrail[i].y = gpGlobals->rgParty[0].y + PAL_Y(gpGlobals->viewport);
		}
		for (i = 1; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			gpGlobals->rgParty[i].x = gpGlobals->rgParty[0].x;
			gpGlobals->rgParty[i].y = gpGlobals->rgParty[0].y - 1;
		}
		PAL_UpdatePartyGestures(FALSE);
		break;
	}

	// Jump to one of the following instructions randomly
	case 0x00A2:
	{
		wScriptEntry += RandomLong(0, pScript->rgwOperand[0] - 1);
		break;
	}

	// Play CD music. Use the RIX music for fallback.
	case 0x00A3:
	{
		//if (!SOUND_PlayCDA(pScript->rgwOperand[0]))
		{
			PAL_PlayMUS(pScript->rgwOperand[1], TRUE, 0);
	}
		break;
		}

	// Scroll FBP to the screen
	case 0x00A4:
	{
		//#ifndef PAL_WIN95
		if (!gConfig->fIsWIN95)
			if (pScript->rgwOperand[0] == 68)
			{
				//
				// HACKHACK: to make the ending picture show correctly
				//
				PAL_ShowFBP(69, 0);
				PAL_ScrollFBP(pScript->rgwOperand[0], pScript->rgwOperand[2], TRUE);
			}
			else
			{
				PAL_ScrollFBP(pScript->rgwOperand[0], pScript->rgwOperand[2], pScript->rgwOperand[1]);
			}
		//#endif
		break;
	}

	// Show FBP picture with sprite effects
	case 0x00A5:
	{
//#ifndef PAL_WIN95
		if (!gConfig->fIsWIN95)
		{
			if (pScript->rgwOperand[1] != 0xFFFF)
			{
				PAL_EndingSetEffectSprite(pScript->rgwOperand[1]);
			}
			PAL_ShowFBP(pScript->rgwOperand[0], pScript->rgwOperand[2]);
		}
//#endif
		break;
	}

	// backup screen
	case 0x00A6:
	{
		VIDEO_BackupScreen();
		break;
	}

	//接下来是新增加的命令

	// 结尾1：消灭水魔兽
	case 0x00B0:
	{
//#ifdef PAL_HAS_AVI
		if (gConfig->m_Function_Set[22])
			if (IsFileExist(va("%s/avi/4.AVI", PalDir.c_str())) && IsFileExist(va("%s/avi/5.AVI", PalDir.c_str())))
			{
				CAviPlay m;
				m.PAL_PlayAVI(va("%s/avi/4.AVI", PalDir.c_str()));
				m.PAL_PlayAVI(va("%s/avi/5.AVI", PalDir.c_str()));
			}
			else
			{
				PAL_NEW_EndingScreen1();
			}
		//#else
		else
			PAL_NEW_EndingScreen1();
//#endif		
		break;
	}

	// 结尾2：全剧终&主角人物诗&工作人员介绍
	case 0x00B1:
	{
//#ifdef PAL_HAS_AVI
		if (gConfig->m_Function_Set[22]) {
			CAviPlay m;
			if (!m.PAL_PlayAVI(va("%s/avi/6.AVI", PalDir.c_str())))
				PAL_NEW_EndingScreen2();
		}
		//#else
		else
			PAL_NEW_EndingScreen2();
//#endif		
		break;
	}

	// 穿越回最初场景
	case 0x00B2:
	{
#ifdef FINISH_GAME_MORE_ONE_TIME

		PAL_InitGameData(-1);
		return 0; // don't go further
#else
		break;
#endif

	}

	// 增加队中人员（3人以上），参考命令0075
	case 0x0100:
	{
		for (i = 0; i < 3; i++)
		{
			if (pScript->rgwOperand[i] != 0)
			{
				gpGlobals->wMaxPartyMemberIndex++;

				if (gpGlobals->wMaxPartyMemberIndex > 5)
				{
					break;
				}

				gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex].wPlayerRole =
					pScript->rgwOperand[i] - 1;

				g_Battle.rgPlayer[gpGlobals->wMaxPartyMemberIndex].action.ActionType =
					kBattleActionAttack;
			}
		}
		//
		// Reload the player sprites
		//
		PAL_SetLoadFlags(kLoadPlayerSprite);
		PAL_LoadResources();
		PAL_UpdateEquipments();
		break;
	}

	// 新命令：移除我方角色的额外装备效果
	case 0x0101:
	{
		PAL_RemoveEquipmentEffect(wEventObjectID, kBodyPartExtra);

		PAL_BattleBackupScene();
		PAL_LoadBattleSprites();
		PAL_BattleMakeScene();
		RenderBlendCopy(gpTextureReal, g_Battle.lpSceneBuf);;
		PAL_BattleFadeScene();
		break;
	}

	// 新命令：参考命令0075，设置队伍成员（适用于3人以上，顺序固定）领队人物延续
	case 0x0102:
	{
		WORD wLeaderPlayerRole = gpGlobals->rgParty[0].wPlayerRole;
		gpGlobals->wMaxPartyMemberIndex = 0;
		for (i = 0; i < MAX_PLAYER_ROLES; i++)
		{
			if (pScript->rgwOperand[0] & (1 << i) && gpGlobals->wMaxPartyMemberIndex < MAX_PLAYERS_IN_PARTY)
			{
				gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex].wPlayerRole = i;
				g_Battle.rgPlayer[gpGlobals->wMaxPartyMemberIndex].action.ActionType = kBattleActionAttack;
				gpGlobals->wMaxPartyMemberIndex++;
			}
		}
		if (gpGlobals->wMaxPartyMemberIndex != 0)
		{
			gpGlobals->wMaxPartyMemberIndex--;
		}

		int index = PAL_New_GetPlayerIndex(wLeaderPlayerRole);
		if (index != -1 && index != 0)
		{
			WORD temp = gpGlobals->rgParty[0].wPlayerRole;
			gpGlobals->rgParty[0].wPlayerRole = gpGlobals->rgParty[index].wPlayerRole;
			gpGlobals->rgParty[index].wPlayerRole = temp;
		}
		//
		// Reload the player sprites
		//
		PAL_SetLoadFlags(kLoadPlayerSprite);
		PAL_LoadResources();
		PAL_UpdateEquipments();
		break;
		}

	// 如果队伍中没有指定的角色则跳转（参考0079）
	case 0x0103:
	{
		BOOL jumpFlag = TRUE;
		for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
		{
			if (gpGlobals->rgParty[i].wPlayerRole == pScript->rgwOperand[0] - 1)
			{
				jumpFlag = FALSE;
				break;
			}
		}
		if (jumpFlag)
		{
			wScriptEntry = pScript->rgwOperand[1] - 1;
		}
		break;
		}

	//到这里说明出错了
	default:
	{
		TerminateOnError("SCRIPT: Invalid Instruction at %4x: (%4x - %4x, %4x, %4x)",
			wScriptEntry, pScript->wOperation, pScript->rgwOperand[0],
			pScript->rgwOperand[1], pScript->rgwOperand[2]);
		break;
	}
	}

	return wScriptEntry + 1;
	}


WORD CScript::PAL_RunAutoScript(WORD wScriptEntry, WORD wEventObjectID)

/*++
  Purpose:

  Runs the autoscript of the specified event object.

  Parameters:

  [IN]  wScriptEntry - The script entry to execute.

  [IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

  The address of the next script instruction to execute.

  --*/
{
	LPSCRIPTENTRY          pScript;
	LPEVENTOBJECT          pEvtObj;
//#ifdef PAL_WIN95
	int                    iDescLine = 0;
//#endif

begin:
	pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);
	pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);
	if (gConfig->is_ShowScript[1] && AfxGetApp())
	{
		//显示自动脚本则不显示普通脚本
		SendMessage(AfxGetApp()->m_pMainWnd->m_hWnd, WM_POST_MESSAGE_SCRIPT, 
			PAL_XY(wScriptEntry, wEventObjectID), (LPARAM)pScript);
	}

	//
	// For autoscript, we should interpret one instruction per frame (except
	// jumping) and save the address of next instruction.
	//
	switch (pScript->wOperation)
	{
	case 0x0000:
		//
		// Stop running
		//
		break;

	case 0x0001:
		//
		// Stop running and replace the entry with the next line
		//
		wScriptEntry++;
		break;

	case 0x0002:
		//
		// Stop running and replace the entry with the specified one
		//
		if (pScript->rgwOperand[1] == 0 ||
			++(pEvtObj->wScriptIdleFrameCountAuto) < pScript->rgwOperand[1])
		{
			wScriptEntry = pScript->rgwOperand[0];
		}
		else
		{
			pEvtObj->wScriptIdleFrameCountAuto = 0;
			wScriptEntry++;
		}
		break;

	case 0x0003:
		//
		// unconditional jump
		//
		if (pScript->rgwOperand[1] == 0 ||
			++(pEvtObj->wScriptIdleFrameCountAuto) < pScript->rgwOperand[1])
		{
			wScriptEntry = pScript->rgwOperand[0];
			goto begin;
		}
		else
		{
			pEvtObj->wScriptIdleFrameCountAuto = 0;
			wScriptEntry++;
		}
		break;

	case 0x0004:
		//
		// Call subroutine
		//
		PAL_RunTriggerScript(pScript->rgwOperand[0],
			pScript->rgwOperand[1] ? pScript->rgwOperand[1] : wEventObjectID);
		wScriptEntry++;
		break;

	case 0x0006:
		//
		// jump to the specified address by the specified rate
		//
		if (RandomLong(1, 100) >= pScript->rgwOperand[0] && pScript->rgwOperand[1] != 0)
		{
			wScriptEntry = pScript->rgwOperand[1];
			goto begin;
		}
		else
		{
			wScriptEntry++;
		}
		break;

	case 0x0009:
		//
		// Wait for a certain number of frames
		//
		if (++(pEvtObj->wScriptIdleFrameCountAuto) >= pScript->rgwOperand[0])
		{
			//
			// waiting ended; go further
			//
			pEvtObj->wScriptIdleFrameCountAuto = 0;
			wScriptEntry++;
		}
		break;

	case 0xFFFF:
//#ifdef PAL_WIN95
		if (gConfig->fIsWIN95)
		{
			int XBase = (wEventObjectID & PAL_ITEM_DESC_BOTTOM) ? 71 : 102;
			int YBase = (wEventObjectID & PAL_ITEM_DESC_BOTTOM) ? 151  : 3;
			int iDescLine = (wEventObjectID & ~PAL_ITEM_DESC_BOTTOM);

			{
				LPCSTR s = PAL_GetMsg(pScript->rgwOperand[0]);
				PAL_DrawText(s, PAL_XY(XBase, iDescLine * 16 + YBase), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
				wScriptEntry++;
			}
		}
		else
		{
			wScriptEntry++;
		}
		break;
		//#endif
		wScriptEntry++;
		break;

//#ifdef PAL_WIN95
	case 0x00A7:
		wScriptEntry++;
		break;
//#endif

	default:
		//
		// Other operations
		//
		wScriptEntry = PAL_InterpretInstruction(wScriptEntry, wEventObjectID);
		break;
	}

	return wScriptEntry;
}

WORD CScript::PAL_RunTriggerScript(WORD wScriptEntry, WORD wEventObjectID
)
/*++
  Purpose:

  Runs a trigger script.

  Parameters:

  [IN]  wScriptEntry - The script entry to execute.

  [IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

  The entry point of the script.

  --*/
{
	static WORD       wLastEventObject = 0;

	WORD              wNextScriptEntry;
	BOOL              fEnded;
	LPSCRIPTENTRY     pScript;
	LPEVENTOBJECT     pEvtObj = NULL;
	int               i;
	int               randomNum;
	//extern BOOL       g_fUpdatedInBattle; // HACKHACK

	wNextScriptEntry = wScriptEntry;
	fEnded = FALSE;
	g_fUpdatedInBattle = FALSE;

	if (wEventObjectID == 0xFFFF)
	{
		wEventObjectID = wLastEventObject;
	}

	wLastEventObject = wEventObjectID;

	if (wEventObjectID != 0)
	{
		pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);
	}

	g_fScriptSuccess = TRUE;

	//
	// Set the default dialog speed.
	//
	PAL_DialogSetDelayTime(3);

	while (wScriptEntry != 0 && !fEnded && PalQuit == FALSE)
	{
		pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);

		if (gConfig->is_TEST && !gConfig->is_ShowScript[1] && AfxGetApp())
		{
			//如果显示自动脚本则不显示普通脚本
			SendMessage(AfxGetApp()->m_pMainWnd->m_hWnd, WM_POST_MESSAGE_SCRIPT, 
				PAL_XY(wScriptEntry, wEventObjectID), (LPARAM)pScript);
		}
		switch (pScript->wOperation)
		{
		case 0x0000:
			//
			// Stop running
			//
			fEnded = TRUE;
			break;

		case 0x0001:
			//
			// Stop running and replace the entry with the next line
			//
			fEnded = TRUE;
			wNextScriptEntry = wScriptEntry + 1;
			break;

		case 0x0002:
			//
			// Stop running and replace the entry with the specified one
			//
			if (pScript->rgwOperand[1] == 0 || ( pEvtObj &&
				++(pEvtObj->nScriptIdleFrame) < pScript->rgwOperand[1]))
			{
				fEnded = TRUE;
				wNextScriptEntry = pScript->rgwOperand[0];
			}
			else
			{
				//
				// failed
				//
				if (pEvtObj)
					pEvtObj->nScriptIdleFrame = 0;
				wScriptEntry++;
			}
			break;

		case 0x0003:
			//
			// unconditional jump
			//
			if (pScript->rgwOperand[1] == 0 ||(pEvtObj &&
				++(pEvtObj->nScriptIdleFrame) < pScript->rgwOperand[1]))
			{
				wScriptEntry = pScript->rgwOperand[0];
			}
			else
			{
				//
				// failed
				//
				pEvtObj->nScriptIdleFrame = 0;
				wScriptEntry++;
			}
			break;

		case 0x0004:
			//
			// Call script
			//
			PAL_RunTriggerScript(pScript->rgwOperand[0],
				((pScript->rgwOperand[1] == 0) ? wEventObjectID : pScript->rgwOperand[1]));
			wScriptEntry++;
			break;

		case 0x0005:
			//
			// Redraw screen
			//
			PAL_ClearDialog(TRUE);

			if (PAL_DialogIsPlayingRNG())
			{
				VIDEO_RestoreScreen();
			}
			else if (fInBattle)
			{
				PAL_BattleMakeScene();
				RenderBlendCopy(gpTextureReal, g_Battle.lpSceneBuf);;
				
				VIDEO_UpdateScreen(gpTextureReal);
			}
			else
			{
				if (pScript->rgwOperand[2])
				{
					PAL_UpdatePartyGestures(FALSE);
				}
				/////
				PAL_MakeScene();

				VIDEO_UpdateScreen(gpTextureReal);
				PAL_Delay((pScript->rgwOperand[1] == 0) ? 60 : (pScript->rgwOperand[1] * 60));
			}

			wScriptEntry++;
			break;

		case 0x0006:
			//
			// Jump to the specified address by the specified rate
			//
			/*
			06 参数1 参数2		  判断、跳转指令
			参数1：整数
			参数2：脚本地址（可以省略）		  指向sss.mkf的第5个子文件		  指向的地址 = 参数2 * 8
			作用		  脚本执行中，遇到06，则系统产生一个0到100的随机数，若该随机数不大于参数1
			，则继续执行06之后的内容，否则：
			若参数2被省略，则中断脚本执行，并将06指令所在的脚本地址写入脚本调用处
			若参数2存在，则跳转参数2指向的脚本地址，继续执行该地址之后的内容
			*/
			randomNum = RandomLong(1, 100);
			if (randomNum > pScript->rgwOperand[0])
			{
				wScriptEntry = pScript->rgwOperand[1];
				continue;
			}
			else
			{
				wScriptEntry++;
			}
			break;

		case 0x0007:
			//
			// Start battle
			//
			i = PAL_StartBattle(pScript->rgwOperand[0], !pScript->rgwOperand[2]);

			if (i == kBattleResultLost && pScript->rgwOperand[1] != 0)
			{
				wScriptEntry = pScript->rgwOperand[1];
			}
			else if (i == kBattleResultFleed && pScript->rgwOperand[2] != 0)
			{
				wScriptEntry = pScript->rgwOperand[2];
			}
			else
			{
				wScriptEntry++;
			}
			gpGlobals->fAutoBattle = FALSE;
			break;

		case 0x0008:
			//
			// Replace the entry with the next instruction
			//
			wScriptEntry++;
			wNextScriptEntry = wScriptEntry;
			break;

			// wait for the specified number of frames
		case 0x0009:
		{
			DWORD        time;

			PAL_ClearDialog(TRUE);

			time = SDL_GetTicks_New() + gConfig->m_Function_Set[24];

			for (i = 0; i < (pScript->rgwOperand[0] ? pScript->rgwOperand[0] : 1); i++)
			{
				PAL_ProcessEvent();
				while (SDL_GetTicks_New() < time)
				{
					PAL_ProcessEvent();
					SDL_Delay(1);
				}

				time = SDL_GetTicks_New() + gConfig->m_Function_Set[24];

				if (pScript->rgwOperand[2])
				{
					PAL_UpdatePartyGestures(FALSE);
				}

				PAL_GameUpdate(pScript->rgwOperand[1] ? TRUE : FALSE);
				PAL_MakeScene();
				VIDEO_UpdateScreen(gpTextureReal);
			}

			wScriptEntry++;
			break;
		}

		// Goto the specified address if player selected no
		case 0x000A:
		{
			PAL_ClearDialog(FALSE);

			if (!PAL_ConfirmMenu())
			{
				wScriptEntry = pScript->rgwOperand[0];
			}
			else
			{
				wScriptEntry++;
			}
			break;
		}

		// Show dialog in the middle part of the screen
		case 0x003B:
		{
			PAL_ClearDialog(TRUE);
			PAL_StartDialog(kDialogCenter, (BYTE)pScript->rgwOperand[0], 0,
				pScript->rgwOperand[2] ? TRUE : FALSE);
			wScriptEntry++;
			break;
		}

		case 0x003C:
			//
			// Show dialog in the upper part of the screen
			//
			PAL_ClearDialog(TRUE);
			PAL_StartDialog(kDialogUpper, (BYTE)pScript->rgwOperand[1],
				pScript->rgwOperand[0], pScript->rgwOperand[2] ? TRUE : FALSE);
			wScriptEntry++;
			break;

		case 0x003D:
			//
			// Show dialog in the lower part of the screen
			//
			PAL_ClearDialog(TRUE);
			PAL_StartDialog(kDialogLower, (BYTE)pScript->rgwOperand[1],
				pScript->rgwOperand[0], pScript->rgwOperand[2] ? TRUE : FALSE);
			wScriptEntry++;
			break;

		case 0x003E:
			//
			// Show text in a window at the center of the screen
			//
			PAL_ClearDialog(TRUE);
			PAL_StartDialog(kDialogCenterWindow, (BYTE)pScript->rgwOperand[0], 0, FALSE);
			wScriptEntry++;
			break;

		case 0x008E:
			//
			// Restore the screen
			//
			PAL_ClearDialog(TRUE);
			VIDEO_RestoreScreen();
			VIDEO_UpdateScreen(gpTextureReal);
			wScriptEntry++;
			break;

		case 0xFFFF:
			//
			// Print dialog text
			//
			PAL_ShowDialogText(PAL_GetMsg(pScript->rgwOperand[0]));
			wScriptEntry++;
			break;

			//接下来是新增加的命令
		case 0x00FF:
		{
			// 新命令：随机跳转到之后的n条后的地址中的一条，
			// 执行这一条后，从这条命令开始处的n条后继续执行
			// n为 pScript->rgwOperand[0]的值
			WORD wNewScriptEntry = wScriptEntry + RandomLong(1, pScript->rgwOperand[0]);
			WORD wJumpScriptEntry = PAL_InterpretInstruction(wNewScriptEntry, wEventObjectID);

			/*
			if (wJumpScriptEntry < pScript->rgwOperand[0] + 1 && wJumpScriptEntry > wScriptEntry)
			{//若新指令没有跳转或跳转在该命令则旧条命令开始处的n条后继续执行

			wScriptEntry += pScript->rgwOperand[0] + 1;
			}
			else
			{//若该条指令有跳转，则从跳转处开始执行
			//若新指令有跳转但跳转地址为新指令地址的下一条，无法跳转
			//会进入上面的if处
			wScriptEntry = wJumpScriptEntry;
			}
			*/
			//还是设置成不允许中途跳转吧，以免发生奇怪的事情
			wScriptEntry += pScript->rgwOperand[0] + 1;

			break;
		}

		default:
			PAL_ClearDialog(TRUE);
			wScriptEntry = PAL_InterpretInstruction(wScriptEntry, wEventObjectID);
			break;
		}
	}

	PAL_EndDialog();
	g_iCurEquipPart = -1;

	return wNextScriptEntry;
}

VOID CScript::PAL_ShowFBP(
	WORD         wChunkNum,
	WORD         wFade
)
/*++
  Purpose:

  Draw an FBP picture to the screen.

  Parameters:

  [IN]  wChunkNum - number of chunk in fbp.mkf file.

  [IN]  wFade - fading speed of showing the picture.

  Return value:

  None.

  --*/
{
	BYTE* buf = (BYTE*)malloc(320 * 200);
	BYTE* bufSprite = (BYTE*)malloc(320 * 200);
	CGL_Texture* mText = new CGL_Texture(320, 200, 32);

	if (PAL_MKFDecompressChunk(buf, 320 * 200, wChunkNum, gpGlobals->f.fpFBP) <= 0)
	{
		if(buf)memset(buf, 0, (320 * 200));
	}

	if (g_wCurEffectSprite != 0)
	{
		PAL_MKFDecompressChunk(bufSprite, 320 * 200, g_wCurEffectSprite, gpGlobals->f.fpMGO);
	}

	PAL_FBPBlitToSurface(buf, mText);
	VIDEO_BackupScreen();
	if (wFade)
	{
		wFade++;
		wFade *= 10;

		for (int i = 0; i < 64; i++)
		{
			RenderBlendCopy(gpTextureReal, gpTextureRealBak);
			RenderBlendCopy(gpTextureReal, mText, i << 2, 0);

			PAL_Delay(wFade);
			if (g_wCurEffectSprite != 0)
			{
				int f = SDL_GetTicks_New() / 150;
				PAL_RLEBlitToTexture(PAL_SpriteGetFrame(bufSprite, f % PAL_SpriteGetNumFrames(bufSprite)),
					gpTextureReal, PAL_XY(0, 0));
			}
			VIDEO_UpdateScreen(gpTextureReal);
		}
	}

	//
	// HACKHACK: to make the ending show correctly
	//
	if (wChunkNum != (gConfig->fIsWIN95 ? 68 : 49))
	{
		PAL_FBPBlitToSurface(buf, gpTextureReal);
	}

	free( buf);
	free(bufSprite);
	delete mText;
	VIDEO_UpdateScreen(gpTextureReal);
}

VOID CScript::PAL_PartyRideEventObject(
	WORD           wEventObjectID,
	INT            x,
	INT            y,
	INT            h,
	INT            iSpeed
)
/*++
  Purpose:

  Move the party to the specified position, riding the specified event object.

  Parameters:

  [IN]  wEventObjectID - the event object to be ridden.

  [IN]  x - Column number of the tile.

  [IN]  y - Line number in the map.

  [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
  (See map.h for details.)

  [IN]  iSpeed - the speed to move.

  Return value:

  TRUE if the party and event object has successfully moved to the specified
  position, FALSE if still need more moving.

  --*/
{
	int              xOffset, yOffset, dx, dy, i;
	DWORD            t;
	LPEVENTOBJECT    p;

	p = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

	xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
	yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);

	t = 0;

	while (xOffset != 0 || yOffset != 0)
	{
		PAL_ProcessEvent();
		while (SDL_GetTicks_New() <= t)
		{
			PAL_ProcessEvent();
			SDL_Delay(1);
		}

		t = SDL_GetTicks_New() + gConfig->m_Function_Set[24];

		if (yOffset < 0)
		{
			gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
		}
		else
		{
			gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirSouth : kDirEast);
		}

		if (abs(xOffset) > iSpeed * 2)
		{
			dx = iSpeed * (xOffset < 0 ? -2 : 2);
		}
		else
		{
			dx = xOffset;
		}

		if (abs(yOffset) > iSpeed)
		{
			dy = iSpeed * (yOffset < 0 ? -1 : 1);
		}
		else
		{
			dy = yOffset;
		}

		//
		// Store trail
		//
		for (i = 3; i >= 0; i--)
		{
			gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
		}

		gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
		gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + dx + PAL_X(gpGlobals->partyoffset);
		gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + dy + PAL_Y(gpGlobals->partyoffset);

		//
		// Move the viewport
		//
		gpGlobals->viewport =
			PAL_XY(PAL_X(gpGlobals->viewport) + dx, PAL_Y(gpGlobals->viewport) + dy);

		p->x += dx;
		p->y += dy;

		PAL_GameUpdate(FALSE);
		PAL_MakeScene();
		VIDEO_UpdateScreen(gpTextureReal);

		xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
		yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);
	}
}

VOID CScript::PAL_PaletteFade(
	INT         iPaletteNum,
	BOOL        fNight,
	BOOL        fUpdateScene
)
/*++
  Purpose:

	Fade from the current palette to the specified one.
	从当前调色板淡出到指定的调色板。

	Parameters:

	[IN]  iPaletteNum - number of the palette.

	[IN]  fNight - whether use the night palette or not.

	[IN]  fUpdateScene - TRUE if update the scene in the progress.

  Return value:

	None.

--*/
{
	PAL_SetPalette(iPaletteNum, fNight);
	VIDEO_BackupScreen();
	PAL_MakeScene();
	for (int i = 0; i < 32; i++)
	{
		DWORD time = SDL_GetTicks_New() + (fUpdateScene ? gConfig->m_Function_Set[24] : gConfig->m_Function_Set[24] / 4);
		if (fUpdateScene)
		{
			PAL_ClearKeyState();
			g_InputState.dir = kDirUnknown;
			g_InputState.prevdir = kDirUnknown;
			PAL_GameUpdate(FALSE);
			PAL_MakeScene();
		}
		RenderBlendCopy(gpGL->gpRenderTexture,gpTextureRealBak);
		RenderBlendCopy(gpGL->gpRenderTexture,gpTextureReal, i << 3, 0);
		RenderPresent(gpGL->gpRenderTexture);

		//while (PAL_PollEvent(NULL));
		PAL_ProcessEvent();
		while (SDL_GetTicks_New() < time)
		{
			PAL_ProcessEvent();
			SDL_Delay(5);
		}
	}
}

VOID CScript::PAL_MonsterChasePlayer(
	WORD         wEventObjectID,
	WORD         wSpeed,
	WORD         wChaseRange,
	BOOL         fFloating
)
/*++
  Purpose:

  Make the specified event object chase the players.

  Parameters:

  [IN]  wEventObjectID - the event object ID of the monster.

  [IN]  wSpeed - the speed of chasing.

  [IN]  wChaseRange - sensitive range of the monster.

  [IN]  fFloating - TRUE if monster is floating (i.e., ignore the obstacles)

  Return value:

  None.

  --*/
{
	LPEVENTOBJECT    pEvtObj = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];
	WORD             wMonsterSpeed = 0, prevx, prevy;
	int              x, y, i, j, l;

	if (gpGlobals->wChaseRange != 0)
	{
		x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - pEvtObj->x;
		y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - pEvtObj->y;

		if (x == 0)
		{
			x = RandomLong(0, 1) ? -1 : 1;
		}

		if (y == 0)
		{
			y = RandomLong(0, 1) ? -1 : 1;
		}

		prevx = pEvtObj->x;
		prevy = pEvtObj->y;

		i = prevx % 32;
		j = prevy % 16;

		prevx /= 32;
		prevy /= 16;
		l = 0;

		if (i + j * 2 >= 16)
		{
			if (i + j * 2 >= 48)
			{
				prevx++;
				prevy++;
			}
			else if (32 - i + j * 2 < 16)
			{
				prevx++;
			}
			else if (32 - i + j * 2 < 48)
			{
				l = 1;
			}
			else
			{
				prevy++;
			}
		}

		prevx = prevx * 32 + l * 16;
		prevy = prevy * 16 + l * 8;

		//
		// Is the party near to the event object?
		//
		if (abs(x) + abs(y) * 2 < wChaseRange * 32 * gpGlobals->wChaseRange)
		{
			if (x < 0)
			{
				if (y < 0)
				{
					pEvtObj->wDirection = kDirWest;
				}
				else
				{
					pEvtObj->wDirection = kDirSouth;
				}
			}
			else
			{
				if (y < 0)
				{
					pEvtObj->wDirection = kDirNorth;
				}
				else
				{
					pEvtObj->wDirection = kDirEast;
				}
			}

			if (x != 0)
			{
				x = pEvtObj->x + x / abs(x) * 16;
			}
			else
			{
				x = pEvtObj->x;
			}

			if (y != 0)
			{
				y = pEvtObj->y + y / abs(y) * 8;
			}
			else
			{
				y = pEvtObj->y;
			}

			if (fFloating)
			{
				wMonsterSpeed = wSpeed;
			}
			else
			{
				if (!PAL_CheckObstacle(PAL_XY(x, y), TRUE, wEventObjectID))
				{
					wMonsterSpeed = wSpeed;
				}
				else
				{
					pEvtObj->x = prevx;
					pEvtObj->y = prevy;
				}

				for (l = 0; l < 4; l++)
				{
					switch (l)
					{
					case 0:
						pEvtObj->x -= 4;
						pEvtObj->y += 2;
						break;

					case 1:
						pEvtObj->x -= 4;
						pEvtObj->y -= 2;
						break;

					case 2:
						pEvtObj->x += 4;
						pEvtObj->y -= 2;
						break;

					case 3:
						pEvtObj->x += 4;
						pEvtObj->y += 2;
						break;
					}

					if (PAL_CheckObstacle(PAL_XY(pEvtObj->x, pEvtObj->y), FALSE, 0))
					{
						pEvtObj->x = prevx;
						pEvtObj->y = prevy;
					}
				}
			}
		}
	}

	PAL_NPCWalkOneStep(wEventObjectID, wMonsterSpeed);
}

VOID CScript::PAL_EndingSetEffectSprite(
	WORD         wSpriteNum
)
/*++
  Purpose:

  Set the effect sprite of the ending.

  Parameters:

  [IN]  wSpriteNum - the number of the sprite.

  Return value:

  None.

  --*/
{
	g_wCurEffectSprite = wSpriteNum;
}

VOID CScript::PAL_ScrollFBP(
	WORD         wChunkNum,
	WORD         wScrollSpeed,
	BOOL         fScrollDown
)
/*++
  Purpose:

  Scroll up an FBP picture to the screen.

  Parameters:

  [IN]  wChunkNum - number of chunk in fbp.mkf file.

  [IN]  wScrollSpeed - scrolling speed of showing the picture.

  [IN]  fScrollDown - TRUE if scroll down, FALSE if scroll up.

  Return value:

  None.

  --*/
{
	//SDL_Surface* p;
	PAL_LARGE BYTE        buf[320 * 200]{};
	PAL_LARGE BYTE        bufSprite[320 * 200]{};
	int                   i, l;
	SDL_Rect              rect{ 0 }, dstrect{0};

	if (PAL_MKFDecompressChunk(buf, 320 * 200, wChunkNum, gpGlobals->f.fpFBP) <= 0 || PalQuit)
	{
		return;
	}

	if (g_wCurEffectSprite != 0)
	{
		PAL_MKFDecompressChunk(bufSprite, 320 * 200, g_wCurEffectSprite, gpGlobals->f.fpMGO);
	}

	VIDEO_BackupScreen();

	PAL_FBPBlitToSurface(buf, gpTextureReal);

	if (wScrollSpeed == 0)
	{
		wScrollSpeed = 1;
	}

	rect.x = 0;
	rect.w = 320;
	dstrect.x = 0;
	dstrect.w = 320;

	for (l = 1; l < 220; l++)
	{
		i = l;
		if (i > 200)
		{
			i = 200;
		}

		if (fScrollDown)
		{
			rect.y = 0;
			dstrect.y = i;
			rect.h = 200 - i;
			dstrect.h = 200 - i;
		}
		else
		{
			rect.y = i;
			dstrect.y = 0;
			rect.h = 200 - i;
			dstrect.h = 200 - i;
		}
		{
			SDL_Rect bRect = rect;
			setPictureRatio(&bRect);
			SDL_Rect dbRect = dstrect;
			setPictureRatio(&dbRect);
			RenderBlendCopy(gpGL->gpRenderTexture, gpTextureRealBak, 255, 1, 0, &dbRect, &bRect);
		}
		if (fScrollDown)
		{
			rect.y = 200 - i;
			dstrect.y = 0;
			rect.h = i;
			dstrect.h = i;
		}
		else
		{
			rect.y = 0;
			dstrect.y = 200 - i;
			rect.h = i;
			dstrect.h = i;
		}
		{
			SDL_Rect bRect = rect;
			setPictureRatio(&bRect);
			SDL_Rect dbRect = dstrect;
			setPictureRatio(&dbRect);
			RenderBlendCopy(gpGL->gpRenderTexture, gpTextureReal, 255, 1, 0, &dbRect, &bRect);
		}

		PAL_ApplyWave(gpGL->gpRenderTexture);

		if (g_wCurEffectSprite != 0)
		{
			int f = SDL_GetTicks_New() / 150;
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(bufSprite, f % PAL_SpriteGetNumFrames(bufSprite)),
				gpGL->gpRenderTexture, PAL_XY(0, 0));
		}

		if (!gpGlobals->fNeedToFadeIn) setAlpha(255);

		VIDEO_UpdateScreen(gpGL->gpRenderTexture);

		if (gpGlobals->fNeedToFadeIn)
		{
			PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
			gpGlobals->fNeedToFadeIn = FALSE;
		}

		PAL_Delay(800 / wScrollSpeed);
	}
	VIDEO_UpdateScreen(gpTextureReal);
}


VOID  CScript::PAL_NEW_EndingScreen2(
	VOID
)
/*++
Purpose:

显示结尾处的人物介绍，对应95/98版的第六个avi视频

Parameters:

None.

Return value:

None.

--*/
{
	PAL_SetPalette(5, FALSE);//不加这句有可能会出现白屏的情况

	//1141 00A5 0031 FFFF 0007 淡入FBP图片 49 65535 07
	PAL_EndingSetEffectSprite(0x27b);
	PAL_ShowFBP(49, 7);

	//1142 0076 0031 0006 0000 显示FBP图像 49 06
	PAL_EndingSetEffectSprite(0);
	PAL_ShowFBP(49, 6);

	//1143 004D 0000 0000 0000 等待按键
	PAL_WaitForKey(0);

	//1144 0077 0001 0000 0000 停止播放音乐 01
	PAL_PlayMUS(0, FALSE, 0.5);
	PAL_Delay(500);

	//1145 0043 0009 0003 0000 设置播放音乐 09
	//if (!SOUND_PlayCDA(13))
	{
		PAL_PlayMUS(9, TRUE, 0);
	}

	//1146 00A4 0030 0000 0010 卷动FBP图片 48 00 16
	int i = 48;
	PAL_ScrollFBP(i--, 0xf, TRUE);
	PAL_ScrollFBP(i--, 0xf, TRUE);
	PAL_ScrollFBP(i--, 0xf, TRUE);
	PAL_ScrollFBP(i--, 0xf, TRUE);
	PAL_ScrollFBP(i--, 0xf, TRUE);
	PAL_ScrollFBP(i--, 0xf, TRUE);
	PAL_ScrollFBP(i--, 0xf, TRUE);
	PAL_ScrollFBP(i--, 0xf, TRUE);
	PAL_ScrollFBP(i--, 0xf, TRUE);

	//1150 0077 0001 0000 0000 停止播放音乐 01
	PAL_PlayMUS(0, FALSE, 3);

	//1151 0050 0004 0000 0000 全屏淡出
	PAL_FadeOut(3);
}

VOID CScript::PAL_EndingAnimation(
	VOID
)/*++
  Purpose:

  Show the ending animation.//就是灵儿独自面对合体水魔兽的动画

  Parameters:

  None.

  Return value:

  None.

  --*/
{
	LPBYTE            buf;
	LPBYTE            bufGirl;

	SDL_Rect          srcrect{ 0 }, dstrect{0};

	int               yPosGirl = 180;
	int               i;

	CGL_Texture * mTexture = new CGL_Texture(RealWidth, RealHeight, 32);

	buf = (LPBYTE)UTIL_calloc(1, 64000);
	bufGirl = (LPBYTE)UTIL_calloc(1, 6000);

	PAL_MKFDecompressChunk(buf, 64000, gConfig->fIsWIN95 ? 69 : 61, gpGlobals->f.fpFBP);
	PAL_FBPBlitToSurface(buf, mTexture);

	PAL_MKFDecompressChunk(buf, 64000, gConfig->fIsWIN95 ? 70 : 62, gpGlobals->f.fpFBP);
	PAL_FBPBlitToSurface(buf, gpTextureRealBak);


	PAL_MKFDecompressChunk(buf, 64000, 571, gpGlobals->f.fpMGO);
	PAL_MKFDecompressChunk(bufGirl, 6000, 572, gpGlobals->f.fpMGO);

	srcrect.x = 0;
	dstrect.x = 0;
	srcrect.w = 320 * PictureRatio;
	dstrect.w = 320 * PictureRatio;

	gpGlobals->wScreenWave = 2;

	for (i = 0; i < 400; i++)
	{
		//
		// Draw the background
		//
		srcrect.y = 0;
		srcrect.h = PictureRatio * (200.0 - i / 2.0);

		dstrect.y = PictureRatio * i / 2;
		dstrect.h = PictureRatio * (200.0 - i / 2.0);
		RenderBlendCopy(gpTextureReal, gpTextureRealBak, 255, 1, 0, &dstrect, &srcrect);

		srcrect.y = PictureRatio * (200.0 - i / 2.0);
		srcrect.h = PictureRatio * i / 2.0;

		dstrect.y = 0;
		dstrect.h = PictureRatio * i / 2;
		RenderBlendCopy(gpTextureReal, mTexture, 255, 1, 0, &dstrect, &srcrect);

		PAL_ApplyWave(gpTextureReal);

		//
		// Draw the beast
		//
		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(buf, 0), gpTextureReal, PAL_XY(0, -400 + i));
		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(buf, 1), gpTextureReal, PAL_XY(0, -200 + i));

		//#ifdef PAL_WIN95
		if (gConfig->fIsWIN95)
			PAL_RLEBlitToTexture(buf + 0x8444, gpTextureReal, PAL_XY(0, -200 + i));
		//#else
		else
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(buf, 1), gpTextureReal, PAL_XY(0, -200 + i));
		//#endif

				//
				// Draw the girl
				//
		yPosGirl -= i & 1;
		if (yPosGirl < 80)
		{
			yPosGirl = 80;
		}

		PAL_RLEBlitToTexture(PAL_SpriteGetFrame(bufGirl, (SDL_GetTicks_New() / 50) % 4),
			gpTextureReal, PAL_XY(220, yPosGirl));

		//
		// Update the screen
		//

		if (gpGlobals->fNeedToFadeIn)
		{
			setAlpha(0);
			PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
			gpGlobals->fNeedToFadeIn = FALSE;
		}
		VIDEO_UpdateScreen(gpTextureReal);
		PAL_Delay(20);
	}

	gpGlobals->wScreenWave = 0;

	//SDL_FreeSurface(pUpper);
	//SDL_FreeSurface(pLower);

	free(buf);
	free(bufGirl);
	delete mTexture;
}



VOID CScript::PAL_NEW_EndingScreen1(
	VOID
)/*++
Purpose:

显示结尾处的消灭合体水魔兽的动画，对应95/98版的第4、5个avi视频

Parameters:

None.

Return value:

None.

--*/
{
	//1128 0043 0019 0000 0000 设置播放音乐 25
	PAL_PlayMUS(0x19, TRUE, 0);

	//1129 008B 0005 0000 0000 改变调色板 05
	PAL_SetPalette(5, FALSE);

	//112A 00A4 0044 0000 000F 卷动FBP图片 68 00 15
	PAL_ScrollFBP(68, 0xf, TRUE);

	//112B 0050 0000 0000 0000 全屏淡出
	PAL_FadeOut(1);

	//112C 008B 0004 0000 0000 改变调色板 04
	PAL_SetPalette(4, FALSE);

	setAlpha(255);
	//112D 0096 0000 0000 0000 播放结束动画
	PAL_EndingAnimation();


	gpGlobals->wNumPalette = 4;
	//112E 0077 0001 0000 0000 停止播放音乐 01
	PAL_PlayMUS(0, FALSE, 1);

	//112F 008C 000F 0007 0000 屏幕逐渐褪色 15 07 00
	PAL_ColorFade(7, 15, FALSE);

	//1131 00A3 0002 0011 0000 播放CD音轨 02 若失败 则播放RIX 17
	//if (!SOUND_PlayCDA(2))
	{
		PAL_PlayMUS(0x11, TRUE, 0);
	}

	setAlpha(255);

	//1132 0036 000B 0000 0000 设置动画 11
	//1133 0037 0000 0001 0000 播放动画 00 01 00
	PAL_RNGPlay(0xb, 0, 1, 7);

	//1134 008B 0000 0000 0000 改变调色板 00
	PAL_SetPalette(0, FALSE);

	//1135 0037 0002 0000 0007 播放动画 02 00 07
	PAL_RNGPlay(0xb, 2, 999, 7);

	//1136 0050 0003 0000 0000 全屏淡出
	PAL_FadeOut(3);

	//1137 008B 0008 0000 0000 改变调色板 08
	PAL_SetPalette(8, FALSE);

	setAlpha(255);
	//1138 0036 000A 0000 0000 设置动画 10
	//1139 0037 0000 0000 0006 播放动画 00 00 06
	PAL_RNGPlay(10, 0, 999, 6);

	//113A 0076 0047 000A 0000 显示FBP图像 71 10
	PAL_EndingSetEffectSprite(0);
	PAL_ShowFBP(71, 10);

	//113B 00A6 0000 0000 0000 备份画面
	VIDEO_BackupScreen();

	//113C 00A5 0046 027B 0007 淡入FBP图片 70 635 07
	PAL_EndingSetEffectSprite(0x27b);

	PAL_ShowFBP(70, 7);

	//113D 008B 0005 0000 0000 改变调色板 05
	PAL_SetPalette(5, FALSE);
	//
	VIDEO_UpdateScreen(gpTextureReal);
	//113E 00A5 0043 FFFF 0007 淡入FBP图片 67 65535 07
	PAL_ShowFBP(67, 7);

	//113F 00A4 0042 FFFF 0007 卷动FBP图片 66 65535 07
	PAL_ScrollFBP(66, 0xf, TRUE);

	//1140 00A5 0041 FFFF 0007 淡入FBP图片 65 65535 07
	PAL_ShowFBP(65, 7);
}

VOID CScript::PAL_EndingScreen(
	VOID
)
/*++
 Purpose:

   Show the ending screen for Win95 version.

 Parameters:

   None.

 Return value:

   None.

--*/
{
	//
	// Use AVI & WIN95's music if we can
	// Otherwise, simulate the ending of DOS version
	//
	CAviPlay m;

	BOOL avi_played = m.PAL_PlayAVI("4.avi");

	if (!(avi_played = m.PAL_PlayAVI("5.avi")))
	{
		//BOOL win_music = AUDIO_PlayCDTrack(12);

		//if (!win_music) AUDIO_PlayMusic(0x1a, TRUE, 0);
		PAL_PlayMUS(0x1a, TRUE, 0);
		PAL_RNGPlay(gpGlobals->iCurPlayingRNG, 110, 150, 7);
		PAL_RNGPlay(gpGlobals->iCurPlayingRNG, 150, -1, 9);

		PAL_FadeOut(2);

		//if (!win_music) AUDIO_PlayMusic(0x19, TRUE, 0);
		PAL_PlayMUS(0x19, TRUE, 0);
		PAL_ShowFBP(75, 0);
		PAL_FadeIn(5, FALSE, 1);
		PAL_ScrollFBP(74, 0xf, TRUE);

		PAL_FadeOut(1);

		//SDL_FillRect(gpScreen, NULL, 0);
		ClearScreen();
		gpGlobals->wNumPalette = 4;
		gpGlobals->fNeedToFadeIn = TRUE;
		PAL_EndingAnimation();

		//if (!win_music) AUDIO_PlayMusic(0, FALSE, 2);
		PAL_PlayMUS(0, FALSE, 2);
		PAL_ColorFade(7, 15, FALSE);

		//if (!win_music && !AUDIO_PlayCDTrack(2))
		//{
			//AUDIO_PlayMusic(0x11, TRUE, 0);
		//}
		PAL_PlayMUS(0x11, TRUE, 0);

		//SDL_FillRect(gpScreen, NULL, 0);
		ClearScreen();
		PAL_SetPalette(0, FALSE);
		PAL_RNGPlay(0xb, 0, -1, 7);

		PAL_FadeOut(2);

		//SDL_FillRect(gpScreen, NULL, 0);
		ClearScreen();
		gpGlobals->wNumPalette = 8;
		gpGlobals->fNeedToFadeIn = TRUE;
		PAL_RNGPlay(10, 0, -1, 6);

		PAL_EndingSetEffectSprite(0);
		PAL_ShowFBP(77, 10);

		///VIDEO_BackupScreen(gpScreen);
		VIDEO_BackupScreen();

		PAL_EndingSetEffectSprite(0x27b);
		PAL_ShowFBP(76, 7);

		PAL_SetPalette(5, FALSE);
		PAL_ShowFBP(73, 7);
		PAL_ScrollFBP(72, 0xf, TRUE);

		PAL_ShowFBP(71, 7);
		PAL_ShowFBP(68, 7);

		PAL_EndingSetEffectSprite(0);
		PAL_ShowFBP(68, 6);

		PAL_WaitForKey(0);
		///AUDIO_PlayMusic(0, FALSE, 1);
		PAL_PlayMUS(0, FALSE, 1);
		///UTIL_Delay(500);
		PAL_Delay(500);
	}

	if (!m.PAL_PlayAVI("6.avi"))
	{
		if (avi_played)
		{
			gpGlobals->fNeedToFadeIn = FALSE;
			PAL_SetPalette(5, FALSE);
			PAL_EndingSetEffectSprite(0);
		}

		//if (!AUDIO_PlayCDTrack(13))
		{
			//AUDIO_PlayMusic(9, TRUE, 0);
		}
		PAL_PlayMUS(9, TRUE, 0);

		PAL_ScrollFBP(67, 0xf, TRUE);
		PAL_ScrollFBP(66, 0xf, TRUE);
		PAL_ScrollFBP(65, 0xf, TRUE);
		PAL_ScrollFBP(64, 0xf, TRUE);
		PAL_ScrollFBP(63, 0xf, TRUE);
		PAL_ScrollFBP(62, 0xf, TRUE);
		PAL_ScrollFBP(61, 0xf, TRUE);
		PAL_ScrollFBP(60, 0xf, TRUE);
		PAL_ScrollFBP(59, 0xf, TRUE);

		///AUDIO_PlayMusic(0, FALSE, 6);
		PAL_PlayMUS(0, FALSE, 6);
		PAL_FadeOut(3);
	}
}


void
UTIL_LogOutput(
	const char* fmt,
	...
)
{
	va_list    va;
	char _log_buffer[512]{ 0 };

	va_start(va, fmt);
	vsnprintf(_log_buffer , 512, fmt, va);
	va_end(va);

	CString * ps = new CString;
	*ps = _log_buffer;

	PostMessage(AfxGetApp()->m_pMainWnd->m_hWnd, WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
}
