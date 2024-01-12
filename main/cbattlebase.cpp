#include "cbattlebase.h"

CBattleBase::CBattleBase()
{

}
/*++
功能：在场景缓存中生成新的战斗场景。
参数：    无。
返回值：    无。
--*/
//使用紋理取代SDL表面
#define BattleSceneUseText 
VOID CBattleBase::PAL_BattleMakeScene(VOID)
{
	int          i;
	PAL_POS      pos;				// 用typedef重命名的DWORD类型。DWORD是用typedef重命名的unsigned long类型。
	LPCBITMAPRLE lBMR = NULL;

#ifndef BattleSceneUseText 
	SDL_Surface* mSurf = SDL_CreateRGBSurface(0, 320, 200, 8, 0, 0, 0, 0);
	SDL_SetSurfacePalette(mSurf, gpPalette);
#endif // !BattleSceneUseText 


	//
	//ClearScreen();
	//
	// Draw the background	画背景
	//
#ifndef BattleSceneUseText 

	pSrc = (LPBYTE)g_Battle.lpBackground->pixels;
	pDst = (LPBYTE)mSurf->pixels;

	for (i = 0; i < mSurf->pitch * mSurf->h; i++)
	{
		b = (*pSrc & 0x0F);
		b += g_Battle.sBackgroundColorShift;

		if (b & 0x80)
		{
			b = 0;
		}
		else if (b & 0x70)
		{
			b = 0x0F;
		}

		*pDst = (b | (*pSrc & 0xF0));

		++pSrc;
		++pDst;
	}

	RenderBlendCopy(g_Battle.lpSceneBuf, mSurf);
#else
	{
		RenderBlendCopy(g_Battle.lpSceneBuf, g_Battle.lpBackground);
		if (g_Battle.sBackgroundColorShift)
		{
			//颜色偏移
			SDL_Color ColorShift = { 240,240,240, 255 };
			if (g_Battle.sBackgroundColorShift < 0)
				ColorShift = { 10,10,10,255 };
			RenderBlendCopy(g_Battle.lpSceneBuf, g_Battle.lpSceneBuf,
				abs(g_Battle.sBackgroundColorShift) * 8, 4, &ColorShift);
		}
	}
#endif // !BattleSceneUseText 

	PAL_ApplyWave(g_Battle.lpSceneBuf);
	
#ifdef 	BattleSceneUseText
	CGL_Texture * mSurf = g_Battle.lpSceneBuf;
#define  PAL_RLEBlitToSurface PAL_RLEBlitToTexture
#endif //BattleSceneUseText

	//
	// Draw the enemies	

	//SDL_FillRect(mSurf, nullptr, *((UINT*)&mSurf->format->palette->colors[0]));

	//绘制敌人

	for (i = g_Battle.wMaxEnemyIndex; i >= 0; i--)
	{
		pos = g_Battle.rgEnemy[i].pos;

		if (g_Battle.rgEnemy[i].rgwStatus[kStatusConfused] > 0 &&
			g_Battle.rgEnemy[i].rgwStatus[kStatusSleep] == 0 &&
			g_Battle.rgEnemy[i].rgwStatus[kStatusParalyzed] == 0)
		{
			//
			// Enemy is confused 
			//敌人陷入混乱状态s
			pos = PAL_XY(PAL_X(pos) + RandomLong(-1, 1), PAL_Y(pos));
		}
		lBMR = PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame);

		pos = PAL_XY(PAL_X(pos) - PAL_RLEGetWidth(lBMR) / 2, PAL_Y(pos) - PAL_RLEGetHeight(lBMR));

		if (g_Battle.rgEnemy[i].wObjectID != 0)
		{
			if (g_Battle.rgEnemy[i].iColorShift)
			{
				PAL_RLEBlitWithColorShift(lBMR, mSurf, pos, g_Battle.rgEnemy[i].iColorShift, 0);
			}
			else
			{
				PAL_RLEBlitToSurface(lBMR, mSurf, pos, 0, 0);
			}

			if (g_Battle.rgEnemy[i].iColorShift)
			{
				PAL_RLEBlitWithColorShift(lBMR, mSurf, pos, g_Battle.rgEnemy[i].iColorShift, 0);
			}
			else
			{
				PAL_RLEBlitToSurface(lBMR, mSurf, pos, 0, 0);
			}
		}
	}

	if (g_Battle.lpSummonSprite != NULL)
	{
		//
		// Draw the summoned god	绘制召唤神的画面
		//
		lBMR = PAL_SpriteGetFrame(g_Battle.lpSummonSprite, g_Battle.iSummonFrame);
		pos = PAL_XY(PAL_X(g_Battle.posSummon) - PAL_RLEGetWidth(lBMR) / 2, PAL_Y(g_Battle.posSummon) - PAL_RLEGetHeight(lBMR));

		PAL_RLEBlitToSurface(lBMR, mSurf, pos, 0, 0);
	}
	else
	{
		//
		// Draw the players	
		//绘制玩家
		for (i = gpGlobals->wMaxPartyMemberIndex; i >= 0; i--)
		{
			pos = g_Battle.rgPlayer[i].pos;

			if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusConfused] != 0 &&
				gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusSleep] == 0 &&
				gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusParalyzed] == 0 &&
				gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0)
			{
				//
				// Player is confused	
				//玩家陷入混乱状态
				continue;
			}

			lBMR = PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame);
			pos = PAL_XY(PAL_X(pos) - PAL_RLEGetWidth(lBMR) / 2, PAL_Y(pos) - PAL_RLEGetHeight(lBMR));

			if (g_Battle.rgPlayer[i].iColorShift != 0)
			{
				PAL_RLEBlitWithColorShift(lBMR, mSurf, pos, g_Battle.rgPlayer[i].iColorShift, 0);
			}
			else if (g_Battle.iHidingTime == 0)
			{
				PAL_RLEBlitToSurface(lBMR, mSurf, pos, 0, 0);
			}
		}

		//
		// Confused players should be drawn on top of normal players
		//玩家的混乱状态应该画在普通状态上层
		for (i = gpGlobals->wMaxPartyMemberIndex; i >= 0; i--)
		{
			if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusConfused] != 0 &&
				gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusSleep] == 0 &&
				gpGlobals->rgPlayerStatus[gpGlobals->rgParty[i].wPlayerRole][kStatusParalyzed] == 0 &&
				gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0)
			{
				//
				// Player is confused
				//玩家陷入混乱状态
				lBMR = PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame);
				pos = PAL_XY(PAL_X(g_Battle.rgPlayer[i].pos), PAL_Y(g_Battle.rgPlayer[i].pos) + RandomLong(-1, 1));
				pos = PAL_XY(PAL_X(pos) - PAL_RLEGetWidth(lBMR) / 2, PAL_Y(pos) - PAL_RLEGetHeight(lBMR));

				if (g_Battle.rgPlayer[i].iColorShift != 0)
				{
					PAL_RLEBlitWithColorShift(lBMR, mSurf, pos, g_Battle.rgPlayer[i].iColorShift, 0);
				}
				else if (g_Battle.iHidingTime == 0)
				{
					PAL_RLEBlitToSurface(lBMR, mSurf, pos, 0, 0);
				}
			}
		}
	}
	

#ifndef BattleSceneUseText
	RenderBlendCopy(g_Battle.lpSceneBuf, mSurf, 255, 3, 0);
	SDL_FreeSurface(mSurf);
#endif // !BattleSceneUseText

}

VOID CBattleBase::PAL_BattleBackupScene(VOID)
/*++
  目的：    Backup the scene buffer.
  备份场景缓存。
  参数：    无。
  返回值：    无。
  --*/
{	//其意义是将一个平面的一部分或全部图像整块从这个平面复制到另一个平面。
	RenderBlendCopy(gpTextureRealBak, g_Battle.lpSceneBuf);
	//SDL_BlitSurface(g_Battle.lpSceneBuf, NULL, gpScreenBak, NULL);
}


VOID CBattleBase::PAL_BattleUpdateFighters(
	VOID
)
/*++
  功能：    Update players' and enemies' gestures and locations in battle.
  参数：    None.
  返回值：  None.
  --*/
{
	int        i;
	WORD       wPlayerRole;

	// Update the gesture for all players
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

		g_Battle.rgPlayer[i].pos = g_Battle.rgPlayer[i].posOriginal;
		g_Battle.rgPlayer[i].iColorShift = 0;

		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
		{
			if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 2; // dead		//死亡
			}
			else
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 0; // puppet	//傀儡虫作用
			}
		}
		else
		{
			if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] != 0 ||
				PAL_IsPlayerDying(wPlayerRole))
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 1;
			}
			else if (g_Battle.rgPlayer[i].fDefending && !g_Battle.fEnemyCleared)
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 3;
			}
			else
			{
				g_Battle.rgPlayer[i].wCurrentFrame = 0;
			}
		}
	}

	// Update the gesture for all enemies
	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}

		g_Battle.rgEnemy[i].pos = g_Battle.rgEnemy[i].posOriginal;
		g_Battle.rgEnemy[i].iColorShift = 0;

		if (g_Battle.rgEnemy[i].rgwStatus[kStatusSleep] > 0 ||
			g_Battle.rgEnemy[i].rgwStatus[kStatusParalyzed] > 0)
		{
			g_Battle.rgEnemy[i].wCurrentFrame = 0;
			continue;
		}

		if (--g_Battle.rgEnemy[i].e.wIdleAnimSpeed == 0)
		{
			g_Battle.rgEnemy[i].wCurrentFrame++;
			g_Battle.rgEnemy[i].e.wIdleAnimSpeed =
				gpGlobals->g.lprgEnemy[((LPOBJECT)((LPOBJECT)gpGlobals->g.rgObject))
				[g_Battle.rgEnemy[i].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
		}

		if (g_Battle.rgEnemy[i].wCurrentFrame >= g_Battle.rgEnemy[i].e.wIdleFrames)
		{
			g_Battle.rgEnemy[i].wCurrentFrame = 0;
		}
	}
}

BOOL  CBattleBase::PAL_IsPlayerDying(
	WORD		wPlayerRole
)
/*++
  功能：    Check if the player is dying.
  检查角色是否濒死了
  参数：    [IN]  wPlayerRole - the player role ID.
  角色id
  返回值：  TRUE if the player is dying, FALSE if not.
  濒死返回TRUE，否则返回FALSE
  --*/
{
	return gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] < gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] / 5
		&& gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] < 200;
}

INT CBattleBase::PAL_BattleSelectAutoTarget(VOID)
/*++
  功能：		Pick an enemy target automatically.
  自动选择一个目标敌人
  参数：		None.
  返回值：	The index of enemy. -1 if failed.
  怪物的序号，如果选择失败返回-1
  --*/
{
	int          i;

	i = (int)g_Battle.UI.wPrevEnemyTarget;

	if (i >= 0 && i <= g_Battle.wMaxEnemyIndex &&
		g_Battle.rgEnemy[i].wObjectID != 0 &&
		g_Battle.rgEnemy[i].dwActualHealth > 0)
	{
		return i;
	}

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID != 0 &&
			g_Battle.rgEnemy[i].dwActualHealth > 0)
		{
			return i;
		}
	}

	return -1;
}

INT  CBattleBase::PAL_CalcBaseDamage(
	DWORD wAttackStrength, DWORD wDefense
)
/*++
  功能：    Calculate the base damage value of attacking.
  计算基础伤害值
  参数：    [IN]  wAttackStrength - attack strength of attacker.
  [IN]  wDefense - defense value of inflictor.
  攻击者武术值，被攻击者防御值
  返回值：   The base damage value of the attacking.
  基础伤害值
  --*/
{
	INT            iDamage;

	//
	// Formula courtesy of palxex and shenyanduxing
	//
	if (gConfig->m_Function_Set[1]) {// #ifdef EDIT_DAMAGE_CALC
		if (wAttackStrength > 2 * wDefense)
		{
			iDamage = wAttackStrength * 1.5 - wDefense * 0.5;
		}
		else if (wAttackStrength > wDefense)
		{
			iDamage = wAttackStrength * 2 - wDefense * 1.5;
		}
		else if (wAttackStrength > wDefense * 0.5)
		{
			iDamage = wAttackStrength - wDefense * 0.5;
		}
		else
		{
			iDamage = 0;
		}
	}
	else //#else
	{ 
		if (wAttackStrength > wDefense)
		{
			iDamage = (INT)(wAttackStrength * 2 - wDefense * 1.6 + 0.5);
		}
		else if (wAttackStrength > wDefense * 0.6)
		{
			iDamage = (INT)(wAttackStrength - wDefense * 0.6 + 0.5);
		}
		else
		{
			iDamage = 0;
		}
	}//#endif

	return iDamage;
}

INT CBattleBase::PAL_CalcMagicDamage(
	DWORD wMagicStrength, DWORD wDefense,

	SHORT			sElementalResistance[NUM_MAGIC_ELEMENTAL],

	SHORT			sPoisonResistance,
	WORD			wMagicID
)
/*++
	功能：     Calculate the damage of magic.
	计算法术伤害值
	参数：     [IN]  wMagicStrength - magic strength of attacker.
	[IN]  wDefense - defense value of inflictor.
	[IN]  rgwElementalResistance - inflictor's resistance to the elemental magics.
	[IN]  wPoisonResistance - inflictor's resistance to poison.
	[IN]  wMagicID - object ID of the magic.
	攻击者灵力值，被攻击者防御值，被攻击者对各属性伤害的抗性，被攻击者毒抗，法术id
	返回值： The damage value of the magic attack.
	法术伤害值
	--*/
{
	INT             iDamage;
	WORD            wElem;
	WORD	        wBaseDamage;
	WORD			wMagicNumber;

	wMagicNumber = ((LPOBJECT)gpGlobals->g.rgObject)[wMagicID].magic.wMagicNumber;
	wBaseDamage = gpGlobals->g.lprgMagic[wMagicNumber].wBaseDamage;

	if ((SHORT)wBaseDamage < 0)
	{
		return 0;
	}

	// Formula courtesy of palxex and shenyanduxing
	wMagicStrength *= RandomFloat(1, 1.1);

	iDamage = PAL_CalcBaseDamage(wMagicStrength, wDefense);

	wElem = gpGlobals->g.lprgMagic[wMagicNumber].wElemental;

	if (gConfig->m_Function_Set[1]) //#ifdef EDIT_DAMAGE_CALC
	{
		if (wElem >= 1 && wElem <= 6)
		{
			iDamage /= 2;
			iDamage += wBaseDamage * 2 / 3;
		}
		else
		{
			iDamage /= 3;
			iDamage += wBaseDamage;
		}
	}
	else//#else
	{
		iDamage /= 4;
		iDamage += wBaseDamage;
	}//#endif	


	switch (wElem)
	{
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	{
		iDamage *= (100.0 - static_cast<float> (sElementalResistance[wElem - 1])) / 100.0;
		SHORT rgsMagicEffect = 10 + gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].rgsMagicEffect[wElem - 1];
		iDamage *= rgsMagicEffect / 10.0;
		break;
	}

	case 6: //毒系
	{
		iDamage *= (100.0 - static_cast<float>(sPoisonResistance)) / 100.0;
		break;
	}

	case 0:
	default:
		break;
	}
	return iDamage;
}

INT CBattleBase::PAL_CalcPhysicalAttackDamage(
	DWORD wAttackStrength, DWORD wDefense, DWORD wAttackResistance
)
/*++
  功能：    Calculate the damage value of physical attacking.
  计算物理攻击伤害值
  参数：    [IN]  wAttackStrength - attack strength of attacker.
  [IN]  wDefense - defense value of inflictor.
  [IN]  wAttackResistance - inflictor's resistance to physical attack.
  攻击者武术值，被攻击者防御值，被攻击者物抗
  返回值：    The damage value of the physical attacking.
  物理攻击伤害值
  --*/
{
	INT             iDamage;

	iDamage = PAL_CalcBaseDamage(wAttackStrength, wDefense);
	//wAttackResistance = min(wAttackResistance, 20);
	if (wAttackResistance != 0)
	{
		//iDamage *= iDamage * 10 / wAttackResistance;
		iDamage *= (110.0 - wAttackResistance) / 100.0;
	}

	return iDamage;
}

WORD CBattleBase::PAL_GetPlayerActualDexterity(
	WORD			wPlayerRole
)
/*++
  功能：    Get player's actual dexterity value in battle.
  获得在战斗中角色的实际的身法值
  参数：    [IN]  wPlayerRole - the player role ID.
  角色id
  返回值：    The player's actual dexterity value.
  角色的实际的身法值
  --*/
{
	DWORD wDexterity = PAL_GetPlayerDexterity(wPlayerRole);

	if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] != 0)
	{
#ifdef PAL_CLASSIC
		if (gpGlobals->wMaxPartyMemberIndex < 3)
		{
			wDexterity *= 3;
		}
		else
		{
			wDexterity *= 6;
		}
#else
		wDexterity *= 6;
		wDexterity /= 5;
#endif
	}
#ifndef PAL_CLASSIC
	else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] != 0)
	{
		wDexterity *= 2;
		wDexterity /= 3;
	}
#endif
	if (PAL_IsPlayerDying(wPlayerRole))
	{
		//
		// player who is low of HP should be slower
		//
#ifdef PAL_CLASSIC
		wDexterity /= 2;
#else
		wDexterity *= 4;
		wDexterity /= 5;
#endif
	}

	return min(0xffff,wDexterity);
}

VOID CBattleBase::PAL_BattlePlayerCheckReady(VOID)
/*++
  功能：

  Check if there are player who is ready.

  参数：

  None.

  返回值：

  None.

  --*/
{
	float   flMax = 0;
	int     iMax = 0, i;

	//
	// Start the UI for the fastest and ready player
	//
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (g_Battle.rgPlayer[i].state == kFighterCom ||
			(g_Battle.rgPlayer[i].state == kFighterAct && g_Battle.rgPlayer[i].action.ActionType == kBattleActionCoopMagic))
		{
			flMax = 0;
			break;
		}
		else if (g_Battle.rgPlayer[i].state == kFighterWait)
		{
			if (g_Battle.rgPlayer[i].flTimeMeter > flMax)
			{
				iMax = i;
				flMax = g_Battle.rgPlayer[i].flTimeMeter;
			}
		}
	}

	if (flMax >= 100.0f)
	{
		g_Battle.rgPlayer[iMax].state = kFighterCom;
		g_Battle.rgPlayer[iMax].fDefending = FALSE;
	}
}

VOID CBattleBase::PAL_LoadBattleBackground(
	VOID
)
/*++
  功能：    Load the screen background picture of the battle.
  载入屏幕战斗背景图片。
  参数：    无。
  返回值：    无。
  --*/
{
	PAL_LARGE BYTE           buf[320 * 200];

	//
	// Create the surface
	//
	g_Battle.lpBackground =	SDL_CreateRGBSurface(0, 320, 200, 8, 0, 0, 0, 0);

	if (g_Battle.lpBackground == NULL)
	{
		TerminateOnError("PAL_LoadBattleBackground(): failed to create surface!");
	}

	SDL_SetSurfacePalette(g_Battle.lpBackground, gpPalette);

	//
	// Load the picture
	//读取图片
	PAL_MKFDecompressChunk(buf, 320 * 200, gpGlobals->wNumBattleField, gpGlobals->f.fpFBP);

	//
	// Draw the picture to the surface.
	//
	PAL_FBPBlitToSurface(buf, g_Battle.lpBackground);
}

INT CBattleBase::PAL_New_GetAliveEnemyNum()
{
	int i, num;
	for (i = 0, num = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID != 0 && g_Battle.rgEnemy[i].dwActualHealth != 0)
		{
			num++;
		}
	}
	return num;
}

VOID CBattleBase::PAL_LoadBattleSprites(
	VOID
)
/*++
  目的：    Load all the loaded sprites.
  载入所有已读的字画面。
  参数：    无。
  返回值：    无。
  --*/
{
	int           i, l, x, y, s;
	FILE* fp;

	PAL_FreeBattleSprites();

	fp = UTIL_OpenRequiredFile("abc.mkf");

	//
	// Load battle sprites for players
	//读取玩家的战斗子画面
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		s = PAL_GetPlayerBattleSprite(gpGlobals->rgParty[i].wPlayerRole);

		l = PAL_MKFGetDecompressedSize(s, gpGlobals->f.fpF);

		if (l <= 0)
		{
			continue;
		}

		g_Battle.rgPlayer[i].lpSprite = (LPSPRITE)UTIL_calloc(l, 1);

		PAL_MKFDecompressChunk(g_Battle.rgPlayer[i].lpSprite, l, s, gpGlobals->f.fpF);

		//
		// Set the default position for this player
		//设置此玩家的默认位置
		x = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][i][0];
		y = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][i][1];

		g_Battle.rgPlayer[i].posOriginal = PAL_XY(x, y);
		g_Battle.rgPlayer[i].pos = PAL_XY(x, y);
	}

	//
	// Load battle sprites for enemies
	//读取敌人的战斗子画面
	for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}
		//获取该敌人图像的长度
		l = PAL_MKFGetDecompressedSize(
			((LPOBJECT)gpGlobals->g.rgObject)[g_Battle.rgEnemy[i].wObjectID].enemy.wEnemyID, fp);

		if (l <= 0)
		{
			continue;
		}
		//申请内存
		g_Battle.rgEnemy[i].lpSprite = (LPSPRITE)UTIL_calloc(l, 1);

		//读入图像
		DWORD sEnemyID = ((LPOBJECT)(gpGlobals->g.rgObject))
			[g_Battle.rgEnemy[i].wObjectID].enemy.wEnemyID;
		PAL_MKFDecompressChunk(g_Battle.rgEnemy[i].lpSprite, l, sEnemyID, fp);

		//
		// Set the default position for this enemy
		//设置此敌人的默认位置
		x = gpGlobals->g.EnemyPos.pos[i][g_Battle.wMaxEnemyIndex].x;
		y = gpGlobals->g.EnemyPos.pos[i][g_Battle.wMaxEnemyIndex].y;

		y += g_Battle.rgEnemy[i].e.wYPosOffset;

		g_Battle.rgEnemy[i].posOriginal = PAL_XY(x, y);
		g_Battle.rgEnemy[i].pos = PAL_XY(x, y);
	}

	fclose(fp);
}

VOID CBattleBase::PAL_FreeBattleSprites(
	VOID
)
/*++
  功能：    Free all the loaded sprites.
  释放所有已读取的（战斗）子画面。
  参数：    无。
  返回值：    无。
  --*/
{
	int         i;

	//
	// Free all the loaded sprites
	//释放左右已读取的子画面
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (g_Battle.rgPlayer[i].lpSprite != NULL)
		{
			free(g_Battle.rgPlayer[i].lpSprite);
		}
		g_Battle.rgPlayer[i].lpSprite = NULL;
	}

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].lpSprite != NULL)
		{
			free(g_Battle.rgEnemy[i].lpSprite);
		}
		g_Battle.rgEnemy[i].lpSprite = NULL;
	}

	if (g_Battle.lpSummonSprite != NULL)
	{
		free(g_Battle.lpSummonSprite);
	}
	g_Battle.lpSummonSprite = NULL;
}

VOID CBattleBase::PAL_BattleBackupStat(
	VOID
)
/*++
  功能：    Backup HP and MP values of all players and enemies.
  备份角色和敌人的hp和mp
  参数：    None.
  返回值：  None.
  --*/
{
	int          i;
	WORD         wPlayerRole;

	for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
	{
		if (g_Battle.rgEnemy[i].wObjectID == 0)
		{
			continue;
		}
		g_Battle.rgEnemy[i].wPrevHP = g_Battle.rgEnemy[i].dwActualHealth;
	}

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

		g_Battle.rgPlayer[i].wPrevHP =
			gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
		g_Battle.rgPlayer[i].wPrevMP =
			gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];
	}
}


INT CBattleBase::PAL_New_GetAlivePlayerNum(VOID)
{
	INT i, num;
	WORD wPlayerRole;
	for (i = 0, num = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0)
		{
			num++;
		}
	}
	return num;
}

INT CBattleBase:: PAL_New_GetHealthyPlayerNum(VOID)
{
	INT i, num;
	WORD wPlayerRole;
	for (i = 0, num = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 && PAL_New_IfPlayerCanMove(wPlayerRole))
		{
			num++;
		}
	}
	return num;
}

BOOL CBattleBase:: PAL_New_IfPlayerCanMove(WORD wPlayerRole)
{
	if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] == 0 &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] == 0 &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CBattleBase::PAL_New_IfEnemyCanMove(WORD wEnemyIndex)
{
	if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusSleep] == 0 &&
		g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusParalyzed] == 0 &&
		g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusConfused] == 0 &&
		g_Battle.iHidingTime <= 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

WORD CBattleBase::PAL_NEW_CheckAndGetLegalPlayerTarget(
	WORD wPlayerRole
)
/*++
  功能：    检查对象是否是一个合法的我方角色目标
  参数：    对象id
  返回值：  一个合法的我方角色目标id
  --*/
{
	INT		i;
	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (wPlayerRole == gpGlobals->rgParty[i].wPlayerRole)
		{
			return wPlayerRole;
		}
	}

	if (i > gpGlobals->wMaxPartyMemberIndex)
	{
		i = RandomLong(0, gpGlobals->wMaxPartyMemberIndex);
		wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
	}
	return wPlayerRole;
}

WORD CBattleBase::PAL_NEW_CheckAndGetLegalEnemyTarget(
	WORD wEnemyIndex
)
/*++
  功能：    检查对象是否是一个合法的敌人目标
  参数：    对象id
  返回值：  一个合法的敌人目标
  --*/
{
	WORD		i = 0;
	if (wEnemyIndex >= MAX_ENEMIES_IN_TEAM || g_Battle.rgEnemy[wEnemyIndex].wObjectID == 0)
	{
		do
		{
			i = (WORD)RandomLong(0, g_Battle.wMaxEnemyIndex);
		} while (g_Battle.rgEnemy[i].wObjectID == 0);

		return i;
	}
	else
	{
		return wEnemyIndex;
	}
}

INT CBattleBase::PAL_New_EnemyNotMoved(WORD wEnemyIndex)
{
	//返回敌方不能活动的轮次
	INT i = 0;
	i = max(g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusSleep], i);
	i = max(g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusParalyzed], i);
	i = max(g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusConfused], i);
	i = max(g_Battle.iHidingTime <= 0 ? 0 : g_Battle.iHidingTime, i);
	return i;
	/*
		if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusSleep] == 0 &&
			g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusParalyzed] == 0 &&
			g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusConfused] == 0 &&
			g_Battle.iHidingTime <= 0)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
		*/
}