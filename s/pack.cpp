//
#include "CGetPalData.h"
//#include "CTestData.h"
#include <iterator>
typedef struct tagBakObject
{
	std::vector<SCRIPTENTRY> vpScriptEntry;
	LPSCRIPTENTRY           lprgScriptEntry{};
	int                     nScriptEntry{};

	std::vector<ENEMY>		vpEnemy;
	LPENEMY                 lprgEnemy{};
	int                     nEnemy{};

	std::vector<MAGIC>		vpMagic;
	LPMAGIC                 lprgMagic{};
	int                     nMagic{};

	std::vector<PLAYERROLES> vPlayerRoles;
	PLAYERROLES             PlayerRoles{};

	std::vector<SCENE>		vpScene;
	SCENE                   rgScene[MAX_SCENES]{};

	std::vector<BYTE>		vgObject;
	LPVOID				    rgObject{};
	int						nObject{};

	std::vector<EVENTOBJECT> vgEventObject;
	EVENTOBJECT				rgEventObject[MAX_EVENT_OBJECTS]{};
	int						nEventObject{};

	tagPString		 CpObjectDsc;//

	tagPString       CpWordBuf;

	tagPString       CpMsgBuf;

	LPDWORD			pMsgOffset{};

	MarkScript		pMarkScript;
	MarkMsg			pMarkMsg;

	MAPScript		MapScript;
	MAPScriptLP		MapScriptLP;
	MAPScript		MapEventObject;
	MAPScriptLP		MapEventObjectLP;

	//INT				nScript{};
	INT				nMarkMsg{};
	INT				nMsg{};
	int 			nReturn{};//返回值
	BOOL			isObjEventChange = 0;//事件对象是否变化
	BOOL			isScriptEntry = 0;//脚本入口是否变化 
}BakObject, * LPBakObject;
LPBakObject gpBakObject{};

//准备数据备份空间
INT CGetPalData::PAL_BakObjectSpace(VOID)
{
	LPBakObject p = gpBakObject;
	if (p)
		PAL_DeleteBakObject();

	p = new BakObject;
	if (p == NULL)return -1;
	//memset(p, 0, sizeof(BakObject));
	p->lprgScriptEntry = new SCRIPTENTRY[60000];
	p->lprgEnemy = new ENEMY[gpGlobals->g.nEnemy + 10];
	p->nEnemy = gpGlobals->g.nEnemy;
	p->lprgMagic = new MAGIC[gpGlobals->g.nMagic];
	p->nMagic = gpGlobals->g.nMagic;
	//空间按大的申请
	p->rgObject = UTIL_calloc(sizeof(OBJECT), MAX_OBJECTS);

	if (!(p->lprgEnemy && p->lprgMagic && p->lprgScriptEntry))
	{
		delete[] p->lprgEnemy;
		delete[] p->lprgMagic;
		delete[] p->lprgScriptEntry;
		//memset(p, 0, sizeof(BakObject));
		return -1;
	}
	memset(p->lprgScriptEntry, 0, sizeof(SCRIPTENTRY) * 60000);
	memset(p->rgEventObject, 0, sizeof(p->rgEventObject));
	//已经按大的空间申请了
	memset(p->rgObject, 0, sizeof(OBJECT) * MAX_OBJECTS);

	memset(p->rgScene, 0, sizeof(p->rgScene));
	memcpy(p->lprgEnemy, gpGlobals->g.lprgEnemy, sizeof(ENEMY) * p->nEnemy);
	memcpy(p->lprgMagic, gpGlobals->g.lprgMagic, sizeof(MAGIC) * p->nMagic);
	memcpy(&p->PlayerRoles, &gpGlobals->g.PlayerRoles, sizeof(PLAYERROLES));

	p->nMarkMsg = 0;
	p->nReturn = 0;
	p->nMsg = 0;
	//p->nWords = 0;
	p->nEventObject = 0;
	p->nScriptEntry = 0;
	p->pMsgOffset = NULL;
	gpBakObject = p;
	p->pMarkScript.resize(60000);
	return 0;
}

VOID CGetPalData::PAL_PackObject(VOID)
{
	//从修改后的系统解包文件生成系统对

	//USES_CONVERSION;
	//第一步读入脚本文件
	//第一步，生成对象
	//生成备份数据空间
	if (PAL_BakObjectSpace())
	{
		//备份数据失败，返回
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("内存不足，返回"), NULL, MB_OK);
		return;
	}
	LPBakObject p = gpBakObject;
	string s;
	FILE* f;
	SCRIPTENTRY mScript;
	INT ScriptLine = 0;
	INT ScriptLingOld = 0;
	s = (PalDir + ("MAKMKF\\ScriptEntry.TXT"));

	string ss = s;
	if (!(IsFileExist(ss)))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("\t返回？"), _T("文件不存在"), MB_OK);
		return;
	}
	if (fopen_s(&f, ss.c_str(), "r"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法打开文件"), CString(s.c_str()), MB_OK);
		return;
	}

	WORD oldScriptN;//记录旧的脚本地址
	while (1)
	{
		CHAR  buf[512];
		if (ScriptLine >= ScriptLingOld + 1000)
		{
			ScriptLingOld = ScriptLine;
			ShowMsg("正在处理 %s的\t%5.5d 行", ss.c_str(), ScriptLingOld);
		}
		if (FreadBuf(buf, f))
		{
			break;
		}
		if (buf[0] == '|')
			continue;
		//跳过第一个字符读取数据
		//读入新行必须是指令
		int readScript = sscanf_s(&buf[1], "%hx%hx%hx%hx%hx", &oldScriptN, &mScript.wOperation, &mScript.rgwOperand[0],
			&mScript.rgwOperand[1], &mScript.rgwOperand[2]);

		assert(readScript == 5);
		p->lprgScriptEntry[ScriptLine] = mScript;
		p->MapScript[oldScriptN] = ScriptLine;
		//assert(oldScriptN < gpGlobals->g.nScriptEntry);
		//取脚本地址
		LPWORD mpOperand[3]{ 0 };
		for (int j = 0; j < 3; j++)
		{
			mpOperand[j] = &p->lprgScriptEntry[ScriptLine].rgwOperand[j];
		}
		switch (mScript.wOperation)
		{
		case 0:
		{
			break;
		}
		case 0x0006://条件跳转  参数1 概率  满足执行下一条，不满足跳转到参数2指向的地址
		case 0x000a://跳转到 参数1 指定地址，如玩家选择 No
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
			if (mScript.rgwOperand[1] && mScript.rgwOperand[1] != 0xffff)
				p->MapScriptLP[mpOperand[1]] = mpOperand[1];
			break;
		}
		case 0x0002://停止运行，用参数1替换，用于下一轮运行,条件参数 2 = 0 或小于对象空闲帧数
		case 0x0003://无条件跳转到参数 1位置,参数2不等于0和未达到参数2指定的帧数 运行下一条
		case 0x0004://运行 参数1 指向的子脚本
		case 0x0033://收集妖怪炼丹,参数1 失败转到
		case 0x0034://灵壶炼丹指令,参数1 失败转到
		case 0x0038://将队伍从现场传送出去 失败跳到参数1 指向地址
		case 0x003a://将队伍从现场传送出去 失败跳到参数1 指向地址
		case 0x0061://没有中毒，跳转，参数1 跳转到
		case 0x0068://如敌方行动，跳转到参数1
		case 0x0074://如不是全体满血，跳到参数1
		case 0x0091://如果敌方不是孤单，则跳转 ，参数1 地址
		{
			if (mScript.rgwOperand[0] && mScript.rgwOperand[0] != 0xffff)
				p->MapScriptLP[mpOperand[0]] = mpOperand[0];
			if (mScript.wOperation == 0x0003 && mScript.rgwOperand[1] == 0)//不运行下一条
				break;
			break;
		}
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
		{
			if (mScript.rgwOperand[2] && mScript.rgwOperand[2] != 0xffff)
				p->MapScriptLP[mpOperand[2]] = mpOperand[2];
			break;
		}
		case 0x0007://开始战斗  参数1 队伍号， 参数3 为 0 BOSS 参数2 战斗失败执行 参数3 战斗中逃跑执行
		case 0x006d://设定一个场景的进入和传送脚本，参数1，场景号，参数2 进入脚本，参数3 传送脚本，参数2 和参数3同时为0 清除脚本
		{
			if (mScript.rgwOperand[2])
				p->MapScriptLP[mpOperand[2]] = mpOperand[2];
			if (mScript.rgwOperand[1])
				p->MapScriptLP[mpOperand[1]] = mpOperand[1];
			break;
		}
		case 0xffff://显示文字 参数1 文字地址 打印该文字
		{
			CHAR str[512];
			FreadBuf(str, f);
			if (p->nMarkMsg > (int)p->pMarkMsg.size() - 2)
				p->pMarkMsg.resize(p->nMarkMsg + 100);
			if (p->nMsg > (int)p->CpMsgBuf.size() - 2)
				p->CpMsgBuf.resize(p->nMsg + 100);
			int n;
			for (n = 0; n < p->nMarkMsg; n++)
			{
				if (p->pMarkMsg[n].o == mScript.rgwOperand[0])
				{
					mScript.rgwOperand[0] = p->pMarkMsg[n].n;
					break;
				}
			}
			if (n >= p->nMarkMsg)
			{
				p->pMarkMsg[p->nMarkMsg].o = mScript.rgwOperand[0];
				p->pMarkMsg[p->nMarkMsg].n = p->nMsg;
				p->nMarkMsg++;
				mScript.rgwOperand[0] = p->nMsg;
				p->CpMsgBuf[p->nMsg] = str;
				p->nMsg++;
			}
			p->lprgScriptEntry[ScriptLine] = mScript;
			break;
		}
		default:
			break;
		}
		//以下标记事件对象变动
		switch (mScript.wOperation)
		{
		case 0x0012://设置对象到相对于队伍的位置 ,参数2 X，参数3 Y
		case 0x0013://设置对象到指定的位置 ,参数2 X，参数3 Y";
		case 0x0016://"设置对象的，方向和（ 手势），参数1 不为0 ，参数2，方向，参数3，形象";
		case 0x0024://"设置对象自动脚本地址，参数1 不等于0 参数2 地址"
		case 0x0025://"设置对象触发脚本地址，参数1 不等于0 参数2 地址"
		case 0x0040://"设置对象触发模式 如参数1 ！= 0 ，参数2 设置";
		case 0x0049://"设置对象状态，参数2 状态";
		case 0x006C://NPC走一步，参数2 X，参数3 Y
		case 0x006F://将当前事件对象状态与·另一个事件对象同步
		case 0x007D://移动对象位置，参数2 X，参数3 Y"
		case 0x007E://设置对象的层，参数2 
		case 0x0081://跳过，如果没有面对对象，参数1 对象，参数2 改变触发方式，参数3地址
		case 0x0083://如果事件对象不在当前事件对象的指定区域中，则跳转
		case 0x0094://"如果事件对象的状态是指定的，则跳转,参数2 状态，参数3 地址";
		{
			//assert(*mpOperand[0]);
			if (mScript.rgwOperand[0] && mScript.rgwOperand[0] != 0xffff)
				p->MapEventObjectLP[mpOperand[0]] = mpOperand[0];
			break;
		}
		case 0x009A://1,2
		{
			if (*mpOperand[0])
				p->MapEventObjectLP[mpOperand[0]] = mpOperand[0];
			if (*mpOperand[1])
				p->MapEventObjectLP[mpOperand[1]] = mpOperand[1];
			break;
		}
		default:
			break;
		}
		ScriptLine++;
	}
	p->nScriptEntry = ScriptLine;
	fclose(f);
	//第二步生成对象数据
	s = PalDir + ("MAKMKF\\Object.txt");
	ss = s;
	if (!(IsFileExist(s)))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("\t返回？"), _T("文件不存在"), MB_OK);
		return;
	}
	if (fopen_s(&f, ss.c_str(), "r"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法打开文件"), CString(s.c_str()), MB_OK);
		return;
	}

	WORD tObject;
	WORD atObject = 0;
	p->CpObjectDsc.resize(MAX_OBJECTS);
	p->CpWordBuf.resize(MAX_OBJECTS);
	while (p->nReturn == 0)
	{
		CHAR buf[512];
		//WORD inScript[3];
		if (FreadBuf(buf, f))break;

		int q = buf[0];

		switch (q)
		{
		case '|':
			continue;
		case 'K'://空
		case 'M'://魔法
		case 'W'://我方
		case 'O'://毒药
		case 'L'://敌方
		case 'V'://物品
		{
			CHAR mWordBuf[50];
			CHAR mDescBuf[250];
			sscanf_s(&buf[1], "%hd", &tObject);
			if (tObject != atObject)
			{
				//错误处理
				ShowMsg("对象号读取错，应该是 %3.3d 读取是 %3.3d", atObject, tObject);
				p->nReturn = -1;
				continue;
			}
			atObject++;

			LPWORD obj;

			obj = (LPWORD)gpGlobals->g.rgObject;

			//= p->rgObject[tObject].rgwData;

			if (gConfig->fIsWIN95)
			{
				int incount = sscanf_s(&buf[1], "%*hd%hd%hd%hx%hx%hx%hx%hx", &obj[0], &obj[1], &obj[2],
					&obj[3], &obj[4], &obj[5], &obj[6]);
				if (incount != 7)
				{
					ShowMsg("对象信息读取错，应该是 6项 读取是 %d 项", incount);
					p->nReturn = -1;
					continue;
				}
				for (int n = 0; n < 7; n++)
					((LPOBJECT)p->rgObject)[tObject].rgwData[n] = obj[n];
			}
			else
			{
				int incount = sscanf_s(&buf[1], "%*hd%hd%hd%hx%hx%hx%hx", &obj[0], &obj[1], &obj[2],
					&obj[3], &obj[4], &obj[5]);
				if (incount != 6)
				{
					ShowMsg("对象信息读取错，应该是 6项 读取是 %d 项", incount);
					p->nReturn = -1;
					continue;
				}
				for (int n = 0; n < 6; n++)
					((LPOBJECT_DOS)p->rgObject)[tObject].rgwData[n] = obj[n];
			}

			ShowMsg("进度，对象号\t%3.3d", tObject);
			//处理脚本数据
			for (int k = 2; k < 5; k++)
			{
				if (obj[k])
				{
					LPOBJECT lpObj = (LPOBJECT)p->rgObject;
					p->MapScriptLP[&lpObj[tObject].rgwData[k]] = &lpObj[tObject].rgwData[k];
				}
			}
			if (obj[5] && gConfig->fIsWIN95)
			{
				LPOBJECT lpObj = (LPOBJECT)p->rgObject;
				p->MapScriptLP[&lpObj[tObject].rgwData[5]] = &lpObj[tObject].rgwData[5];
			}
			sscanf_s(buf, "%*[^{]{%[^}] %*[^{]{%[^}]", mWordBuf, 29, mDescBuf, 249);
			//以下处理文字信息
			trim(mDescBuf);
			if (strlen(mDescBuf))
				p->CpObjectDsc[tObject] = mDescBuf;
			trim(mWordBuf);
			if (strlen(mWordBuf))
			{
				p->CpWordBuf[tObject] = mWordBuf;
			}
			break;
		}
		default:
			p->nReturn = -1;
			MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, NULL, _T("错误"), MB_OK);
			break;
		}
	}
	fclose(f);
	if (p->nReturn == -1)
		return;
	//第三步，生成事件对象
	s = PalDir + ("MAKMKF\\Event.TXT");
	ss = s;
	if (!(IsFileExist(s)))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("\t返回？"), _T("文件不存在"), MB_OK);
		return;
	}
	if (fopen_s(&f, ss.c_str(), "r"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法打开文件"), CString(s.c_str()), MB_OK);
		return;
	}
	WORD  nScene = 0;
	WORD atScene = 1;
	while (p->nReturn == 0)
	{
		CHAR buf[512];
		if (FreadBuf(buf, f))
		{
			p->nReturn = 1;
			continue;

		}
		switch (buf[0])
		{
		case '|':
		case '#':
			continue;
		case 'S':
		{
			//处理场景信息
			sscanf_s(&buf[1], "%hd", &nScene);
			if (nScene != atScene)
			{
				ShowMsg("场景号错，应为 %d 实为  %d", nScene, atScene);
				p->nReturn = -1;
				continue;
				//出错，处理
			}
			atScene++;
			ShowMsg("正在处理 场景号\t%3.3d", nScene);
			LPWORD pScene = (LPWORD)&p->rgScene[nScene - 1];
			int ret = sscanf_s(&buf[1], "%*hd%hd%hx%hx%hx", &pScene[0], &pScene[1], &pScene[2], &pScene[3]);
			if (4 != ret)
			{
				//错误处理
				ShowMsg("场景信息读取错，应读 4项 实际读取 %d 返回", ret);
				p->nReturn = -1;
				continue;
			}
			if (pScene[3] == 0 && p->nEventObject)
			{
				//结束
				p->nReturn = 1;
				continue;
			}
			if (pScene[3] != p->nEventObject)
			{
				ShowMsg("场景 %3.3hd 事件对象索引变化 由 %4.4hd 到  %4.4hd", nScene, pScene[3], p->nEventObject);
				pScene[3] = p->nEventObject;
				p->isObjEventChange = TRUE;
			}
			if (pScene[1])//进入脚本
			{
				p->MapScriptLP[&pScene[1]] = &pScene[1];
			}
			if (pScene[2])//传送脚本
			{
				p->MapScriptLP[&pScene[2]] = &pScene[2];
			}
			break;
		}
		case 'K':
		{
			//处理事件对象
			LPWORD obj = (LPWORD)&p->rgEventObject[p->nEventObject];
			WORD oldEventObject;
			sscanf_s(&buf[1], "%hx%hd%hd%hd%hd%hx%hx%hd%hd%hd%hd%hd%hd%hd%hd%hd%hd", &oldEventObject,
				&obj[0], &obj[1], &obj[2], &obj[3], &obj[4], &obj[5], &obj[6], &obj[7], &obj[8],
				&obj[9], &obj[10], &obj[11], &obj[12], &obj[13], &obj[14], &obj[15]);

			p->MapEventObject[oldEventObject] = p->nEventObject + 1;//场景编号从1开始，需要加1

			if (obj[4])
				p->MapScriptLP[&obj[4]] = &obj[4];//目标脚本
			if (obj[5])
				p->MapScriptLP[&obj[5]] = &obj[5];//自动脚本
			p->nEventObject++;
			break;
		}
		default:
			//到这里出错
		{
			ShowMsg("数据读取错，返回");
			p->nReturn = -1;
			continue;
		}
		}
	}

	fclose(f);
	//生成文件
	//修改脚本入口地址
	MAPScriptLP::iterator pair;
	ShowMsg("修改脚本入口地址 共有 %d 条脚本入口", p->MapScriptLP.size());
	int count = 0;
	for (pair = p->MapScriptLP.begin(); pair != p->MapScriptLP.end(); pair++)
	{
		LPWORD pScript = pair->first;
		MAPScript::iterator pw;
		pw = p->MapScript.find(*pScript);
		if (pw == p->MapScript.end())
		{
			//没有找到出错
			ShowMsg("数据错，原有脚本入口 %4.4X 被删除", *pScript);
			p->nReturn = -1;
			break;
		}
		if (pw->first != pw->second)
		{
			//新旧入口不一致，需要修改，用新入口替换旧入口
			p->isScriptEntry = TRUE;
			*pScript = pw->second;
			count++;
		}
	}

	ShowMsg("修改事件对象地址 共有 %d 条事件对象调用入口", p->MapEventObjectLP.size());
	int ChangeEventCount = 0;
	for (pair = p->MapEventObjectLP.begin(); pair != p->MapEventObjectLP.end(); pair++)
	{
		LPWORD pScript = pair->first;
		MAPScript::iterator pw;
		pw = p->MapEventObject.find(*pScript);
		if (pw == p->MapEventObject.end() && *pair->first < 0xE000)
		{
			//没有找到出错
			ShowMsg("数据错，原有 %4.4X 事件对象被删除", *pair->first);
			p->nReturn = -1;
			gpGlobals->g.nEventObject;
			break;
		}
		if (pw->first != pw->second)
		{
			//新旧入口不一致，需要修改，用新入口替换旧入口
			p->isObjEventChange = TRUE;
			*pScript = pw->second;
			ChangeEventCount++;
		}
	}
	if (p->nReturn == -1)
		return;
	ShowMsg("共有 %d 条脚本入口地址被修改", count);
	ShowMsg("共有 %d 条事件对象地址被修改", ChangeEventCount);
	ShowMsg("共生成事件对象%d 条 ，原有%d 条", p->nEventObject, gpGlobals->g.nEventObject);
	ShowMsg("共生成脚本入口 %d 条，原来的脚本入口 %d 条", p->nScriptEntry, gpGlobals->g.nScriptEntry);

	ShowMsg("生成对话 %d 条，原有对话 %d 条", p->nMsg, g_TextLib.nMsgs);

	ShowMsg("正在生成新的对话文件");

	Pal_Print_NoUseMsg();
	Pal_Make_NewMsg();
	if (p->nScriptEntry > gpGlobals->g.nScriptEntry)
	{
		gpGlobals->g.lprgScriptEntry = (LPSCRIPTENTRY)realloc(gpGlobals->g.lprgScriptEntry,
			gpBakObject->nScriptEntry * sizeof(SCRIPTENTRY));
		if (gpGlobals->g.lprgScriptEntry == NULL)
		{
			::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("内存错，退出"), NULL, MB_OK);
			exit(-1);
		}
	}
	gpGlobals->g.nScriptEntry = p->nScriptEntry;
	free(gpGlobals->g.lprgScriptEntry);
	gpGlobals->g.lprgScriptEntry = (LPSCRIPTENTRY)UTIL_calloc(gpGlobals->g.nScriptEntry, sizeof(SCRIPTENTRY));
	//memset(gpGlobals->g.lprgScriptEntry, 0, gpGlobals->g.nScriptEntry * sizeof(SCRIPTENTRY));
	memcpy(gpGlobals->g.lprgScriptEntry, p->lprgScriptEntry,
		gpGlobals->g.nScriptEntry * sizeof(SCRIPTENTRY));
	//更新事件对象
	gpGlobals->g.nEventObject = p->nEventObject;
	free(gpGlobals->g.lprgEventObject);
	gpGlobals->g.lprgEventObject = (LPEVENTOBJECT)UTIL_calloc(p->nEventObject, sizeof(EVENTOBJECT));
	memcpy(gpGlobals->g.lprgEventObject, p->rgEventObject,
		gpGlobals->g.nEventObject * sizeof(EVENTOBJECT));
	//更新对象
	if (gConfig->fIsWIN95)
		memcpy(gpGlobals->g.rgObject, p->rgObject, sizeof(OBJECT) * MAX_OBJECTS);
	else
	{
		//已经按WIN格式更改了原数据，需要转换
		memcpy(gpGlobals->g.rgObject, p->rgObject, sizeof(OBJECT_DOS) * MAX_OBJECTS);

	}
	//更新场景
	memcpy(gpGlobals->g.rgScene, p->rgScene, sizeof(gpBakObject->rgScene));
	//更新对话索引

	g_TextLib.nMsgs = gpBakObject->nMsg;
	if (g_TextLib.lpMsgOffset)
		free(g_TextLib.lpMsgOffset);
	g_TextLib.lpMsgOffset = gpBakObject->pMsgOffset;
	gpBakObject->pMsgOffset = NULL;
	ShowMsg("正在生成新的脚本文件");

	//PAL_Make_MKF();
	//更新词索引文件WOR16.ASC
	ShowMsg("正在生成新的字库索引文件 WOR16.ASC");
	if (fopen_s(&f, string(PalDir + ("WOR16.ASC")).c_str(), "rb") == 0)
	{
		FILE* f1;
		if (fopen_s(&f1, string(PalDir + ("WOR16.ASC.NEW")).c_str(), "w") == 0)
		{
			fseek(f, 0, SEEK_END);
			DWORD len = ftell(f);
			LPSTR allstr = (LPSTR)alloca(len);
			fseek(f, 0, SEEK_SET);
			Cls_Iconv Iconv1{}, Iconv2{};
			CHAR p1[3]{ 0 };
			for (DWORD n = 0; n < len / 2; n++)
			{
				p1[2] = 0;
				fread_s(p1, 2, 1, 2, f);
				LPSTR wWord;
				if (gpGlobals->bIsBig5)
					wWord = Iconv1.Gb2312ToBig5(Iconv2.Big5ToGb2312(p1));
				else
					wWord = p1;
				fwrite(wWord, 2, 1, f1);
			}
			fclose(f1);
		}
		fclose(f);
	}

	PAL_Make_MKF();
	PAL_ReloadPAL(TRUE);

	//检查是否需要修改存档文件
	do
	{
		int rpgn;
		//修改存档文件
		for (rpgn = 1; rpgn <= 5; rpgn++)
		{
			string rpgName = va("%s%1.1d.rpg", PalDir.c_str(), rpgn);

			if (IsFileExist(rpgName))
			{
				string name = (rpgName);
				PAL_ChangeSaveFile(name.c_str());
			}
		}
	} while (0);

	//清理工作
	PAL_DeleteBakObject();
	::PostMessage(AfxGetApp()->m_pMainWnd->m_hWnd, WM_OKReturn, 0, 1);
	ShowMsg("打包完成");
}


INT CGetPalData::FreadBuf(const LPSTR buf, FILE* f)
{
	while (true)
	{
		//mLocation = ftell(f);
		if (fgets(buf, 512, f) == NULL)
			return -1;
		trim(buf);
		if (buf[0] == 0)continue;
		break;
	}
	//ShowMsg(buf);
	return 0;
}

VOID CGetPalData::PAL_DeleteBakObject(VOID)
{
	LPBakObject p = gpBakObject;
	if (p == NULL)
		return;

	p->CpMsgBuf.resize(0);
	p->CpObjectDsc.resize(0);
	p->pMarkMsg.resize(0);
	p->pMarkScript.resize(0);
	if (p->lprgEnemy)
		delete[] p->lprgEnemy;
	if (p->lprgMagic)
		delete[] p->lprgMagic;
	if (p->lprgScriptEntry)
		delete[] p->lprgScriptEntry;
	if (p->pMsgOffset)
		free(p->pMsgOffset);

	free(p->rgObject);
	delete p;
	gpBakObject = NULL;
}


VOID CGetPalData::PAL_ChangeSaveFile(LPCSTR FileName)
{

	//装入存档文件
	PAL_LoadGame(FileName);
	ShowMsg("正在更新存档文件  %s", FileName);
	LPBakObject p = gpBakObject;
	p->MapScriptLP.clear();
	for (int n = 0; n < MAX_OBJECTS; n++)
	{
		LPWORD pObj;
		if (gpObject_classify[n] == 0)
			continue;
		if (gConfig->fIsWIN95)
		{
			pObj = ((LPOBJECT)gpGlobals->g.rgObject)[n].rgwData;
			for (int k = 2; k < 6; k++)
			{
				if (pObj[k])
				{
					p->MapScriptLP[&((LPOBJECT)gpGlobals->g.rgObject)[n].rgwData[k]] =
						&((LPOBJECT)gpGlobals->g.rgObject)[n].rgwData[k];
				}
			}
		}
		else
		{
			pObj = ((LPOBJECT_DOS)gpGlobals->g.rgObject)[n].rgwData;
			for (int k = 2; k < 5; k++)
			{
				if (pObj[k])
				{
					p->MapScriptLP[&((LPOBJECT_DOS)gpGlobals->g.rgObject)[n].rgwData[k]] =
						&((LPOBJECT_DOS)gpGlobals->g.rgObject)[n].rgwData[k];
				}
			}
		}
	}
	for (int n = 1; n <= MAX_SCENES; n++)
	{
		for (int j = 1; j <= 2; j++)
		{
			LPWORD pScene = (LPWORD)&gpGlobals->g.rgScene[n - 1];
			if (pScene[j])
				p->MapScriptLP[&pScene[j]] = &pScene[j];
		}
	}
	for (int n = 0; n < gpGlobals->g.nEventObject; n++)
	{
		for (int j = 4; j <= 5; j++)
		{
			LPWORD pObj = (LPWORD)&gpGlobals->g.lprgEventObject[n];
			assert(&pObj[j] == pObj + j);
			if (*(pObj + j))
				p->MapScriptLP[pObj + j] = pObj + j;
		}
	}
	//修改脚本入口地址
	MAPScriptLP::iterator pair;
	ShowMsg("修改脚本入口地址 共有 %d 条脚本入口", p->MapScriptLP.size());
	int count = 0;
	for (pair = p->MapScriptLP.begin(); pair != p->MapScriptLP.end(); pair++)
	{
		LPWORD pScript = pair->first;
		MAPScript::iterator pw;
		pw = p->MapScript.find(*pScript);
		if (pw == p->MapScript.end())
		{
			//没有找到出错
			ShowMsg("数据错，原有脚本入口 %4.4X 被删除", *pScript);
			p->nReturn = -1;
			break;
		}
		if (pw->first != pw->second)
		{
			//新旧入口不一致，需要修改，用新入口替换旧入口
			p->isScriptEntry = TRUE;
			*pScript = pw->second;
			count++;
		}
	}
	ShowMsg("共有 %d 条脚本入口地址被修改", count);
	//修改事件对象地址

	PAL_SaveGame(FileName, 1);
	return VOID();
}

//生成新的对话文件，提示文件，对象名称文件
INT CGetPalData::Pal_Make_NewMsg(VOID)
{
	//第一步建立新对话文件和对话索引
	string s = PalDir + ("M.MSG.NEW");
	FILE* f;
	if (fopen_s(&f, s.c_str(), "w"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法建立文件"), CString(s.c_str()), MB_OK);
		return -1;
	}

	LPBakObject  p = gpBakObject;

	if (p->pMsgOffset)
	{
		free(p->pMsgOffset);
	}
	LPDWORD msgOffList = (LPDWORD)UTIL_calloc(p->nMsg + 1, sizeof(DWORD));
	//msgOffList[0] = 0;
	for (int n = 0; n < p->nMsg; n++)
	{
		Cls_Iconv Istr{};
		string  pstr;
		if (gpGlobals->bIsBig5)
			pstr = Istr.Gb2312ToBig5((LPSTR)p->CpMsgBuf[n].c_str());
		else
			pstr = p->CpMsgBuf[n];
		pstr.erase(0, pstr.find_first_not_of(" "));
		fputs(pstr.c_str(), f);
		msgOffList[n + 1] = ftell(f);
	}
	p->pMsgOffset = msgOffList;
	fclose(f);

	//第二步，建立新字典文件
	s = PalDir + ("WORD.DAT.NEW");
	if (fopen_s(&f, s.c_str(), "w"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法建立文件"), CString(s.c_str()), MB_OK);
		return -1;
	}
	Cls_Iconv str{};

	for (int n = 0; n < MAX_OBJECTS; n++)
	{
		LPSTR pstr = va("%-10.10s", gpGlobals->bIsBig5 ?
			str.Gb2312ToBig5(p->CpWordBuf[n].c_str()) :
			p->CpWordBuf[n].c_str());
		fwrite(pstr, 1, 10, f);
	}
	fclose(f);

	//第三步，建立提示文件
	if (!gConfig->fIsWIN95)
	{
		s = PalDir + ("DESC.DAT.NEW");
		if (fopen_s(&f, s.c_str(), "w"))
		{
			::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法建立文件"), CString(s.c_str()), MB_OK);
			return -1;
		}
		Cls_Iconv str{}, str1{};
		if (gpGlobals->bIsBig5)
			fputs(str.Gb2312ToBig5("本文件由系统根据整理结果生成\r\n"), f);
		else
			fputs("本文件由系统根据整理结果生成\r\n", f);
		for (int n = 1; n < MAX_OBJECTS; n++)
		{
			if (!p->CpObjectDsc[n].empty())
			{
				fprintf_s(f, "%3.3x(%10.10s)=%s\r\n", n,
					gpGlobals->bIsBig5 ?
					str.Gb2312ToBig5(p->CpWordBuf[n].c_str()) :
					p->CpWordBuf[n].c_str(),
					gpGlobals->bIsBig5 ?
					str1.Gb2312ToBig5((LPSTR)p->CpObjectDsc[n].c_str()) :
					p->CpObjectDsc[n].c_str());
			}
		}
		fclose(f);
	}
	return 0;
}

//输出没有被使用的对话
VOID  CGetPalData::Pal_Print_NoUseMsg(VOID)
{
	USES_CONVERSION;
	string s = PalDir + ("MAKMKF\\NoUseMsg.txt");
	FILE* f;
	if (fopen_s(&f, s.c_str(), "w"))
	{
		::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("无法建立文件"), CString(s.c_str()), MB_OK);
		return;
	}
	LPBakObject p = gpBakObject;
	LPSTR bufMsg = new CHAR[g_TextLib.nMsgs + 1];
	memset(bufMsg, 0, g_TextLib.nMsgs + 1);
	for (int n = 0; n < p->nMsg; n++)
	{
		int i = p->pMarkMsg[n].o;
		if (i < g_TextLib.nMsgs)
			bufMsg[i] = 1;
	}
	fputs("以下打印的是未使用的对话\r\n", f);
	fputs("(编号)\t\t 内容\r\n", f);
	for (int n = 0; n < g_TextLib.nMsgs; n++)
	{
		if (!bufMsg[n])
		{
			LPSTR str = PAL_GetMsg(n);
			trim(str);
			fprintf_s(f, "(%4.4x)\t", n);
			fputs(str, f);
			fputs("\r\n", f);
		}
	}
	delete[]bufMsg;
	fclose(f);
}
