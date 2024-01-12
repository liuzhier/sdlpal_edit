//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#include "cpalbaseio.h"
#include <assert.h>



//复活后不解除的毒的ID，0x0233是寿葫芦毒，0x0245是忘剑五诀 
const WORD ExcludePoisonIDAfterRevive[] = { 0x0233, 0x0245 };


BOOL CPalBaseIO::PAL_AddItemToInventory(
	WORD          wObjectID,
	INT           iNum
)
/*++
  Purpose:  Add or remove the specified kind of item in the inventory.

  Parameters:  [IN]  wObjectID - object number of the item.
  [IN]  iNum - number to be added (positive value) or removed (negative value).
  Return value:  TRUE if succeeded, FALSE if failed.
  --*/
{
	int          index;
	BOOL         fFound;


	if (wObjectID == 0)
	{
		return FALSE;
	}

	if (iNum == 0)
	{
		iNum = 1;
	}

	index = 0;
	fFound = FALSE;

	//
	// Search for the specified item in the inventory
	//
	while (index < MAX_INVENTORY)
	{
		if (gpGlobals->rgInventory[index].wItem == wObjectID)
		{
			fFound = TRUE;
			break;
		}
		else if (gpGlobals->rgInventory[index].wItem == 0)
		{
			break;
		}
		index++;
	}

	if (iNum > 0)
	{
		//
		// Add item
		//
		if (index >= MAX_INVENTORY)
		{
			//
			// inventory is full. cannot add item
			//
			return FALSE;
		}

		if (fFound)
		{
			gpGlobals->rgInventory[index].nAmount += iNum;
			if (gpGlobals->rgInventory[index].nAmount > MAX_ITEM_NUM_IN_INVENTORY)
			{
				//
				// Maximum number is 99
				//
				gpGlobals->rgInventory[index].nAmount = MAX_ITEM_NUM_IN_INVENTORY;
			}
		}
		else
		{
			gpGlobals->rgInventory[index].wItem = wObjectID;
			if (iNum > MAX_ITEM_NUM_IN_INVENTORY)
			{
				iNum = MAX_ITEM_NUM_IN_INVENTORY;
			}
			gpGlobals->rgInventory[index].nAmount = iNum;
		}

		return TRUE;
	}
	else
	{
		//
		// Remove item
		//
		if (fFound)
		{
			iNum *= -1;
			if (gpGlobals->rgInventory[index].nAmount < iNum)
			{
				//
				// This item has been run out
				//
				gpGlobals->rgInventory[index].nAmount = 0;
				return FALSE;
			}

			gpGlobals->rgInventory[index].nAmount -= iNum;
			return TRUE;
		}

		return FALSE;
	}
}

INT CPalBaseIO::PAL_GetItemAmount( 
	WORD        wItem
)
/*++
  Purpose:    Get the amount of the specified item in the inventory.
  Parameters:    [IN]  wItem - the object ID of the item.
  Return value:    The amount of the item in the inventory.
  --*/
{
	int i;

	for (i = 0; i < MAX_INVENTORY; i++)
	{
		if (gpGlobals->rgInventory[i].wItem == 0)
		{
			break;
		}

		if (gpGlobals->rgInventory[i].wItem == wItem)
		{
			return gpGlobals->rgInventory[i].nAmount;
		}
	}

	return 0;
}


BOOL CPalBaseIO::PAL_IncreaseHPMP(
	WORD          wPlayerRole,
	SHORT         sHP,
	SHORT         sMP
)
/*++
  Purpose:

  Increase or decrease player's HP and/or MP.

  Parameters:

  [IN]  wPlayerRole - the number of player role.

  [IN]  sHP - number of HP to be increased (positive value) or decrased
  (negative value).

  [IN]  sMP - number of MP to be increased (positive value) or decrased
  (negative value).

  Return value:

  TRUE if the operation is succeeded, FALSE if not.

  --*/
{
	INT iHP, iMP = 0;

#ifdef INCREASE_EXTRA_PLAYER_HPMP
	WORD extraLevel = 0;
	WORD levelDifference = 0;
	FLOAT times = 1;
#endif

//#ifdef  New_HP_MP
	BOOL   hp_Max, mp_Max;
	if (gConfig->m_Function_Set[7])
	{
		//在此增加代码，当HP，MP达到最大值时返回失败

		//
		// Only care about alive players
		//
		//测试原HP,MP是否已经最大，sHP >0 sMP == 0
		hp_Max = ((gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] ==
			gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole]) && sHP > 0);
		mp_Max = ((gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] ==
			gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole]) && sMP > 0);
		if ((hp_Max && sMP == 0) || (mp_Max && sHP == 0) || (hp_Max && mp_Max))
		{
			return FALSE;
		}
	}
//#endif // New_HP_MP

	//
	// Only care about alive players
	//
	if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
	{
		return FALSE;
	}

	iHP = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
	iMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];
	//
	// change HP
	//
	iHP += sHP;

#ifdef INCREASE_EXTRA_PLAYER_HPMP
#ifdef Difficulty_Level
	if (gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] > 900 && sHP > 0)
	{
		iHP += sHP * gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] / 900.0 - sHP;
	}
#else
	if (gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] > 900 && sHP > 0)
	{
		iHP += sHP * gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] / 900.0 - sHP;
	}
/*
	if (gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] >= INCREASE_EXTRA_HPMP_MIN_LEVEL && sHP > 0)
	{
		//levelDifference = INCREASE_EXTRA_HPMP_MAX_LEVEL - INCREASE_EXTRA_HPMP_MIN_LEVEL;
		//levelDifference = max(levelDifference, 1);
		//extraLevel = gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] - INCREASE_EXTRA_HPMP_MIN_LEVEL;
		//extraLevel = min(levelDifference, extraLevel);
		times = ((float)max(max(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole], MAX_PARAMETER) - 1000, 0) / 1000.0)
			//* ((float)extraLevel / (float)levelDifference);
			;
#ifdef FINISH_GAME_MORE_ONE_TIME
		times = min(times, gpGlobals->bFinishGameTime + 1);
#endif
		iHP += sHP * times;
	}
	*/
#endif //Difficulty_Level
#endif

	iHP = max(iHP, 0);
	iHP = min(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole], iHP);
	gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] = iHP;

	//
	// Change MP
	//
	iMP += sMP;

#ifdef INCREASE_EXTRA_PLAYER_HPMP
#ifdef Difficulty_Level
	if (gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] > 900 && sMP > 0)
	{
		iMP += sMP * gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] / 900.0 - sMP;
	}
#else

	//if (gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] >= INCREASE_EXTRA_HPMP_MIN_LEVEL && sMP > 0)
	if (gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] > 900 && sMP > 0)

	{
		//levelDifference = INCREASE_EXTRA_HPMP_MAX_LEVEL - INCREASE_EXTRA_HPMP_MIN_LEVEL;
		//levelDifference = max(levelDifference, 1);
		//extraLevel = gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] - INCREASE_EXTRA_HPMP_MIN_LEVEL;
		//extraLevel = min(levelDifference, extraLevel);
		//times = ((float)max(max(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole], MAX_PARAMETER) - 1000, 0) / 1000.0)
			//* ((float)extraLevel / (float)levelDifference);
			;
#ifdef FINISH_GAME_MORE_ONE_TIME
		times = min(times, gpGlobals->bFinishGameTime + 1);
#endif
		iMP += sMP * gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] / 900.0 - sMP;
		//iMP += sMP * times;
	}
#endif // Difficulty_Level
#endif //INCREASE_EXTRA_PLAYER_HPMP

	iMP = max(iMP, 0);
	iMP = min(iMP, gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole]);
	gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] = iMP;

	return TRUE;

}
	


VOID CPalBaseIO::PAL_RemoveEquipmentEffect(
	WORD			wPlayerRole,
	WORD			wEquipPart
)
/*++
  Purpose:

  Remove all the effects of the equipment for the player.

  Parameters:

  [IN]  wPlayerRole - the player role.

  [IN]  wEquipPart - the part of the equipment.

  Return value:

  None.

  --*/
{
	WORD* p;
	int         i;

	p = (WORD*)(&gpGlobals->rgEquipmentEffect[wEquipPart]); // HACKHACK

	for (i = 0; i < sizeof(PLAYERROLES) / sizeof(PLAYERS); i++)
	{
		p[i * MAX_PLAYER_ROLES + wPlayerRole] = 0;//临时属性清除
	}

	//
	// Reset some parameters to default when appropriate
	//
	if (wEquipPart == kBodyPartHand)
	{
		//
		// reset the dual attack status
		//
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] = 0;
	}
	else if (wEquipPart == kBodyPartWear)
	{   //解除寿葫芦毒
		PAL_CurePoisonByKind(wPlayerRole, 0x0233);
	}
}

VOID CPalBaseIO::PAL_AddPoisonForPlayer(
	WORD			wPlayerRole,
	WORD			wPoisonID
)
/*++
  Purpose:    Add the specified poison to the player.
  对我方角色增加指定的毒
  Parameters:    [IN]  wPlayerRole - the player role ID.
  [IN]  wPoisonID - the poison to be added.
  角色id，毒的id
  Return value:    None.
  注：不可以在该函数内进行毒排序，会造成脚本执行混乱
  --*/
{
	int			i, index, iPoisonIndex;
	WORD			w;

	index = PAL_New_GetPlayerIndex(wPlayerRole);
	WORD wPoisonLevel = ((LPOBJECT)gpGlobals->g.rgObject)[wPoisonID].poison.wPoisonLevel;

	if (index == -1 || gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
	{
		return;
	}

	iPoisonIndex = PAL_New_GetPoisonIndexForPlayer(wPlayerRole, wPoisonID);
	//-1 已中毒
	if (iPoisonIndex != -1)
	{
		// already poisoned
		if (gConfig->m_Function_Set[0])
			//#ifdef POISON_STATUS_EXPAND // 增加毒的烈度
		{
			gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity++;
			switch (wPoisonLevel)
			{
			case 0:
			case 1:
				gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity =
					min(gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity, 3);
				break;

			case 2:
				gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity =
					min(gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity, 2);
				break;
			case 3:
				
				gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity =
					min(gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity, 1);
				break;
				
			default:
				gpGlobals->rgPoisonStatus[iPoisonIndex][index].wPoisonIntensity = 0;
				break;
			}
		}//#endif
		return;
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;
		if (w == wPoisonID)
		{	// already poisoned
			return;
		}
		else if (w == 0)
		{
			break;
		}
	}

	i = min(i, MAX_POISONS - 1);
	gpGlobals->rgPoisonStatus[i][index].wPoisonID = wPoisonID;
	gpGlobals->rgPoisonStatus[i][index].wPoisonScript = ((LPOBJECT)gpGlobals->g.rgObject)[wPoisonID].poison.wPlayerScript;

	//#ifdef POISON_STATUS_EXPAND // 增加毒的烈度
	if (gConfig->m_Function_Set[0])
	{
#ifdef FINISH_GAME_MORE_ONE_TIME
		if (wPoisonLevel < 3)
		{
			gpGlobals->rgPoisonStatus[i][index].wPoisonIntensity += gpGlobals->bFinishGameTime;
		}
#endif
	}
	//#endif
}

VOID CPalBaseIO::PAL_CurePoisonByKind(
	WORD			wPlayerRole,
	WORD			wPoisonID
)
/*++
  Purpose:

  Remove the specified poison from the player.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  [IN]  wPoisonID - the poison to be removed.

  Return value:

  None.

  --*/
{
	int i, index;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)
	{
		return; // don't go further
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
		{

			if (gConfig->m_Function_Set[0])//#ifdef POISON_STATUS_EXPAND
				memset(&gpGlobals->rgPoisonStatus[i][index], 0, sizeof(gpGlobals->rgPoisonStatus[i][index]));

			else //#else
			{
				gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
				gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
			}//#endif
		}
	}
}

VOID CPalBaseIO::PAL_CurePoisonByLevel(
	WORD			wPlayerRole,
	WORD			wMaxLevel
)
/*++
  Purpose:    Remove the poisons which have a maximum level of wMaxLevel from the player.

  Parameters:    [IN]  wPlayerRole - the player role ID.
  [IN]  wMaxLevel - the maximum level of poisons to be removed.

  Return value:    None.
  --*/
{
	int        i, index;
	WORD       w;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)
	{
		return; // don't go further
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;

		if (PAL_New_IsAnItemInArray(w, ExcludePoisonIDAfterRevive))
		{
			continue;
		}
		if (((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wPoisonLevel <= wMaxLevel)
		{
			if (gConfig->m_Function_Set[0])//#ifdef POISON_STATUS_EXPAND
				memset(&gpGlobals->rgPoisonStatus[i][index], 0, sizeof(gpGlobals->rgPoisonStatus[i][index]));
			else {//#else
				gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
				gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
			}//#endif
		}
	}
}

BOOL CPalBaseIO::PAL_New_IsAnItemInArray(
	WORD			item,
	const WORD		items[]
)
{
	int i = 0;
	int length = sizeof(items) / sizeof(WORD);
	BOOL result = FALSE;

	for (i = 0; i < length; i++)
	{
		if (item == items[i])
		{
			result = TRUE;
			break;
		}
	}
	return result;
}

BOOL CPalBaseIO::PAL_IsPlayerPoisonedByLevel(
	WORD			wPlayerRole,
	WORD			wMinLevel
)
/*++
  Purpose:

  Check if the player is poisoned by poisons at a minimum level of wMinLevel.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  [IN]  wMinLevel - the minimum level of poison.

  Return value:

  TRUE if the player is poisoned by poisons at a minimum level of wMinLevel;
  FALSE if not.

  --*/
{
	int         i, index;
	WORD        w;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
		{
			break;
		}
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)
	{
		return FALSE; // don't go further
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;
		w = ((LPOBJECT)gpGlobals->g.rgObject)[w].poison.wPoisonLevel;

		if (w >= 99)
		{
			//
			// Ignore poisons which has a level of 99 (usually effect of equipment)
			//
			continue;
		}

		if (w >= wMinLevel)
		{
			return TRUE;
		}
	}

	return FALSE;
}

INT CPalBaseIO::PAL_New_GetPoisonIndexForPlayer(
	WORD			wPlayerRole,
	WORD			wPoisonID
)
/*++
  Purpose:    Check if the player is poisoned by the specified poison.

  Parameters:		[IN]  wPlayerRole - the player role ID.
  [IN]  wPoisonID - the poison to be checked.

  Return value:		如果没有中这种毒，返回-1；
  如果中毒，则返回该毒在该角色的rgPoisonStatus的位置序号
  --*/
{
	int i, index;

	index = PAL_New_GetPlayerIndex(wPlayerRole);
	if (index == -1)
	{
		return -1;
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
		{
			return i;
		}
	}
	return -1;
}

BOOL CPalBaseIO::PAL_New_IsPlayerPoisoned(
	WORD			wPlayerRole
)
/*++
  Purpose:    Check if the player is poisoned.
  Parameters:    [IN]  wPlayerRole - the player role ID.
  Return value:    TRUE if player is poisoned;
  FALSE if not.
  --*/
{
	int i, index;

	index = PAL_New_GetPlayerIndex(wPlayerRole);
	if (index == -1)
	{
		return FALSE;
	}

	for (i = 0; i < MAX_POISONS; i++)
	{
		if (gpGlobals->rgPoisonStatus[i][index].wPoisonID != 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

VOID CPalBaseIO::PAL_New_SortPoisonsForPlayerByLevel(
	WORD			wPlayerRole
)
{
	int         i, j, index, PoisonNum;
	WORD        wPoisonID1, wPoisonID2;
	WORD        wPoisonLevel1, wPoisonLevel2;
	POISONSTATUS	tempPoison;

	for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
	{
		if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)break;
	}

	if (index > gpGlobals->wMaxPartyMemberIndex)return; // don't go further

	for (j = 0, PoisonNum = 0; j < MAX_POISONS; j++)
	{
		wPoisonID1 = gpGlobals->rgPoisonStatus[j][index].wPoisonID;
		if (wPoisonID1 == 0) gpGlobals->rgPoisonStatus[j][index].wPoisonScript = 0;
		else PoisonNum++;
	}

	if (PoisonNum < 2)	return;	//中毒数目小于2不用排序


	for (i = 0; i < MAX_POISONS - 1; i++)
	{
		for (j = 0; j < MAX_POISONS - i - 1; j++)
		{
			wPoisonID1 = gpGlobals->rgPoisonStatus[j][index].wPoisonID;
			wPoisonLevel1 = ((LPOBJECT)gpGlobals->g.rgObject)[wPoisonID1].poison.wPoisonLevel;
			wPoisonID2 = gpGlobals->rgPoisonStatus[j + 1][index].wPoisonID;
			wPoisonLevel2 = ((LPOBJECT)gpGlobals->g.rgObject)[wPoisonID2].poison.wPoisonLevel;

			if (wPoisonLevel1 < wPoisonLevel2)
			{
				tempPoison = gpGlobals->rgPoisonStatus[j][index];
				gpGlobals->rgPoisonStatus[j][index] = gpGlobals->rgPoisonStatus[j + 1][index];
				gpGlobals->rgPoisonStatus[j + 1][index] = tempPoison;
			}
		}
	}
}


WORD CPalBaseIO::PAL_GetPlayerAttackStrength(
	WORD			wPlayerRole
)
/*++
  Purpose:

  Get the player's attack strength, count in the effect of equipments.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  The total attack strength of the player.

  --*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwAttackStrength[wPlayerRole];
	}

	return w;
}

WORD CPalBaseIO::PAL_GetPlayerMagicStrength(
	WORD			wPlayerRole
)
/*++
  Purpose:

  Get the player's magic strength, count in the effect of equipments.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  The total magic strength of the player.

  --*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwMagicStrength[wPlayerRole];
	}

	return w;
}

WORD CPalBaseIO::PAL_GetPlayerDefense(
	WORD			wPlayerRole
)
/*++
  Purpose:

  Get the player's defense value, count in the effect of equipments.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  The total defense value of the player.

  --*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwDefense[wPlayerRole];
	}

	return w;
}

WORD CPalBaseIO::PAL_GetPlayerDexterity(
	WORD           wPlayerRole
)
/*++
  Purpose:

  Get the player's dexterity, count in the effect of equipments.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  The total dexterity of the player.

  --*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole];

#ifdef PAL_CLASSIC
	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
#else
	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS - 1; i++)
#endif
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwDexterity[wPlayerRole];
	}

	return w;
}

WORD CPalBaseIO::PAL_GetPlayerFleeRate(
	WORD           wPlayerRole
)
/*++
  Purpose:

  Get the player's flee rate, count in the effect of equipments.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  The total flee rate of the player.

  --*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwFleeRate[wPlayerRole];
	}

	return w;
}

WORD CPalBaseIO::PAL_GetPlayerPoisonResistance(
	WORD           wPlayerRole
)
/*++
  Purpose:

  Get the player's resistance to poisons, count in the effect of equipments.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  The total resistance to poisons of the player.

  --*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwPoisonResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwPoisonResistance[wPlayerRole];
	}

	return min(w, 100);
}

WORD CPalBaseIO::PAL_New_GetPlayerSorceryResistance(
	WORD			wPlayerRole
)
/*++
	Purpose:Get the player's resistance to Sorcery, count in the effect of equipments.
	Parameters:[IN]  wPlayerRole - the player role ID.
	Return value:The total resistance to Sorcery of the player.
	--*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwSorceryResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwSorceryResistance[wPlayerRole];
	}

	return min(100, w);
}

WORD CPalBaseIO::PAL_New_GetPlayerSorceryStrength(
	WORD			wPlayerRole
)
/*++
Purpose:Get the player's resistance to Sorcery, count in the effect of equipments.
Parameters:[IN]  wPlayerRole - the player role ID.
Return value:The total resistance to Sorcery of the player.
--*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwSorceryStrength[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwSorceryStrength[wPlayerRole];
	}

	return min(100, w);
}

WORD CPalBaseIO::PAL_GetPlayerElementalResistance(
	WORD			wPlayerRole,
	INT				iAttrib
)
/*++
  Purpose:

  Get the player's resistance to attributed magics, count in the effect
  of equipments.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  [IN]  iAttrib - the attribute of magics.

  Return value:

  The total resistance to the attributed magics of the player.

  --*/
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwElementalResistance[iAttrib][wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwElementalResistance[iAttrib][wPlayerRole];
	}

	return min(w, 100);
}

WORD CPalBaseIO::PAL_New_GetPlayerPhysicalResistance(
	WORD			wPlayerRole
)
{
	WORD       w;
	int        i;

	w = gpGlobals->g.PlayerRoles.rgwPhysicalResistance[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		w += gpGlobals->rgEquipmentEffect[i].rgwPhysicalResistance[wPlayerRole];
	}

	return min(100, w);
}

WORD CPalBaseIO::PAL_GetPlayerBattleSprite(
	WORD             wPlayerRole
)
/*++
  Purpose:

  Get player's battle sprite.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  Number of the player's battle sprite.

  --*/
{
	int       i;
	WORD      w;

	w = gpGlobals->g.PlayerRoles.rgwSpriteNumInBattle[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		if (gpGlobals->rgEquipmentEffect[i].rgwSpriteNumInBattle[wPlayerRole] != 0)
		{
			w = gpGlobals->rgEquipmentEffect[i].rgwSpriteNumInBattle[wPlayerRole];
		}
	}

	return w;
}

WORD CPalBaseIO::PAL_GetPlayerCooperativeMagic(
	WORD             wPlayerRole
)
/*++
  Purpose:

  Get player's cooperative magic.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  Object ID of the player's cooperative magic.

  --*/
{
	int       i;
	WORD      w;

	w = gpGlobals->g.PlayerRoles.rgwCooperativeMagic[wPlayerRole];

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		if (gpGlobals->rgEquipmentEffect[i].rgwCooperativeMagic[wPlayerRole] != 0)
		{
			w = gpGlobals->rgEquipmentEffect[i].rgwCooperativeMagic[wPlayerRole];
		}
	}

	return w;
}

BOOL CPalBaseIO::PAL_PlayerCanAttackAll(
	WORD        wPlayerRole
)
/*++
  Purpose:

  Check if the player can attack all of the enemies in one move.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  Return value:

  TRUE if player can attack all of the enemies in one move, FALSE if not.

  --*/
{
	int       i;
	BOOL      f;

	f = FALSE;

	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		if (gpGlobals->rgEquipmentEffect[i].rgwAttackAll[wPlayerRole] != 0)
		{
			f = TRUE;
			break;
		}
	}

	return f;
}

BOOL CPalBaseIO::PAL_AddMagic(
	WORD           wPlayerRole,
	WORD           wMagic
)
/*++
  Purpose:

  Add a magic to the player.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  [IN]  wMagic - the object ID of the magic.

  Return value:

  TRUE if succeeded, FALSE if failed.

  --*/
{
	int            i;

	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wMagic)
		{
			//
			// already have this magic
			//
			return FALSE;
		}
	}

	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == 0)
		{
			break;
		}
	}

	if (i >= MAX_PLAYER_MAGICS)
	{
		//
		// Not enough slots
		//
		return FALSE;
	}

	gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] = wMagic;
	return TRUE;
}

VOID CPalBaseIO::PAL_RemoveMagic(
	WORD           wPlayerRole,
	WORD           wMagic
)
/*++
  Purpose:

  Remove a magic to the player.

  Parameters:

  [IN]  wPlayerRole - the player role ID.

  [IN]  wMagic - the object ID of the magic.

  Return value:

  None.

  --*/
{
	int            i;

	for (i = 0; i < MAX_PLAYER_MAGICS; i++)
	{
		if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wMagic)
		{
			gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] = 0;
			break;
		}
	}
}

VOID CPalBaseIO::PAL_SetPlayerStatus(
	WORD         wPlayerRole,
	WORD         wStatusID,
	WORD         wNumRound
)
/*++
  Purpose:

  Set one of the statuses for the player.

  Parameters:

  [IN]  wPlayerRole - the player ID.

  [IN]  wStatusID - the status to be set.

  [IN]  wNumRound - the effective rounds of the status.

  Return value:

  None.

  --*/
{
#ifndef PAL_CLASSIC
	if (wStatusID == kStatusSlow &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] > 0)
	{
		//
		// Remove the haste status
		//
		PAL_RemovePlayerStatus(wPlayerRole, kStatusHaste);
		return;
	}

	if (wStatusID == kStatusHaste &&
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] > 0)
	{
		//
		// Remove the slow status
		//
		PAL_RemovePlayerStatus(wPlayerRole, kStatusSlow);
		return;
	}
#endif

	switch (wStatusID)
	{
	case kStatusConfused:
	case kStatusSleep:
	case kStatusSilence:
#ifdef PAL_CLASSIC
	case kStatusParalyzed:
#else
	case kStatusSlow:
#endif
		//
		// for "bad" statuses, don't set the status when we already have it
		//
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 &&
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] == 0)
		{
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
		}
		break;

	case kStatusPuppet:
		//
		// only allow dead players for "puppet" status
		//
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 &&
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
		{
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
		}
		break;

	case kStatusBravery:
	case kStatusProtect:
	case kStatusDualAttack:
	case kStatusHaste:
		//
		// for "good" statuses, reset the status if the status to be set lasts longer
		//
		if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 &&
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
		{
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
		}
		else if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 &&
			gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] != 0 &&
			gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
		{
			if (wStatusID == kStatusBravery || wStatusID == kStatusDualAttack)
			{
				gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
			}
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

VOID CPalBaseIO::PAL_RemovePlayerStatus(
	WORD         wPlayerRole,
	WORD         wStatusID
)
/*++
  Purpose:    Remove one of the status for player.
  对角色解除某一个状态
  Parameters:	[IN]  wPlayerRole - the player ID.
  [IN]  wStatusID - the status to be set.
  Return value:    None.
  --*/
{
	//
	// Don't remove effects of equipments
	//
	if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] <= 999)
	{
		gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = 0;
	}
}

VOID CPalBaseIO::PAL_New_RemovePlayerAllStatus(
	WORD         wPlayerRole
)
{
	int x;
	for (x = 0; x < kStatusAll; x++)
	{
		PAL_RemovePlayerStatus(wPlayerRole, x);
	}
}


VOID CPalBaseIO::PAL_ClearAllPlayerStatus(
	VOID
)
/*++
  Purpose:    Clear all player status.
  清除所有角色的状态
  Parameters:    None.
  Return value:    None.
  --*/
{
	int      i, j;

	for (i = 0; i < MAX_PLAYER_ROLES; i++)
	{
		for (j = 0; j < kStatusAll; j++)
		{
			//
			// Don't remove effects of equipments
			//
			if (gpGlobals->rgPlayerStatus[i][j] <= 999)
			{
				gpGlobals->rgPlayerStatus[i][j] = 0;
			}
		}
	}
}

VOID CPalBaseIO::PAL_PlayerLevelUp(
	WORD          wPlayerRole,
	WORD          wNumLevel
)
/*++
  Purpose:

  Increase the player's level by wLevels.

  Parameters:

  [IN]  wPlayerRole - player role ID.

  [IN]  wNumLevel - number of levels to be increased.

  Return value:

  None.

  --*/
{
	WORD          i;

	// Add the level
	gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] += wNumLevel;
	if (gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] > MAX_LEVELS
#if 0

#else
#if FINISH_GAME_MORE_ONE_TIME
		&& !gpGlobals->bFinishGameTime
#endif
#endif
		)
	{
		gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] = MAX_LEVELS;
	}

	for (i = 0; i < wNumLevel; i++)
	{
		//
		// Increase player's stats
		//
		gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] += 12 + RandomLong(0, 2);
		gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] += 9 + RandomLong(0, 1);
		gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole] += 4 + RandomLong(0, 1);
		gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole] += 4 + RandomLong(0, 1);
		gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole] += 2 + RandomLong(0, 1);
		gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole] += 2 + RandomLong(0, 1);
		gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole] += 2 + RandomLong(0, 1);
	}


#define STAT_LIMIT(t) { if ((t) > MAX_PARAMETER) (t) = MAX_PARAMETER; }
		STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole]);
		STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole]);
		STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole]);
		STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole]);
		STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole]);
		STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole]);
		STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole]);
#undef STAT_LIMIT


	//
	// Reset experience points to zero
	//
	gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wExp = 0;
	gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wLevel =
		gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole];
}



INT CPalBaseIO::PAL_New_GetPlayerIndex(
	WORD		wPlayerRole
)
{
	int		i;

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		if (wPlayerRole == gpGlobals->rgParty[i].wPlayerRole)
		{
			break;
		}
	}

	if (i > gpGlobals->wMaxPartyMemberIndex)
	{
		return -1;//没有找到
	}
	else
	{
		return i;
	}


}

WORD CPalBaseIO::PAL_New_GetPlayerID(
	WORD		wPlayerIndex
)
{
	if (wPlayerIndex > MAX_PLAYERS_IN_PARTY)
	{
		return 0xFFFF;
	}
	else
	{
		return gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
	}
}

//fGetLowest为真找属性最低的角色序号，bLow为假找属性最高的角色序号，
INT CPalBaseIO::PAL_New_GetPlayerIndexByPara(
	PlayerPara	pp,
	BOOL		fGetLowest)
{
	WORD* p = (WORD*)(&gpGlobals->g.PlayerRoles);
	WORD* p1[MAX_PLAYER_EQUIPMENTS + 1] =
	{
		(WORD*)(&gpGlobals->rgEquipmentEffect[0]),
		(WORD*)(&gpGlobals->rgEquipmentEffect[1]),
		(WORD*)(&gpGlobals->rgEquipmentEffect[2]),
		(WORD*)(&gpGlobals->rgEquipmentEffect[3]),
		(WORD*)(&gpGlobals->rgEquipmentEffect[4]),
		(WORD*)(&gpGlobals->rgEquipmentEffect[5]),
		(WORD*)(&gpGlobals->rgEquipmentEffect[6]),
	};
	int i, j, max, maxIndex, min, minIndex, cur;
	WORD w = 0;

	w = gpGlobals->rgParty[0].wPlayerRole;
	cur = p[pp * MAX_PLAYER_ROLES + w];
	for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
	{
		cur += p1[i][pp * MAX_PLAYER_ROLES + w];
	}
	min = cur;
	minIndex = 0;
	max = cur;
	maxIndex = 0;

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;
		cur = p[pp * MAX_PLAYER_ROLES + w];
		for (j = 0; j <= MAX_PLAYER_EQUIPMENTS; j++)
		{
			cur += p1[j][pp * MAX_PLAYER_ROLES + w];
		}

		if (max < cur)
		{
			max = cur;
			maxIndex = i;
		}
		if (min > cur)
		{
			min = cur;
			minIndex = i;
		}
	}

	if (fGetLowest == TRUE)
	{
		return minIndex;
	}
	else
	{
		return maxIndex;
	}
}

INT CPalBaseIO::PAL_New_GetPlayerIndexByHealth(
	BOOL		fGetLowest
)
/*++
功能：    选中生命值最低（高）的角色.
参数：    fLow为真是选择最低，反之选择最高.
返回值：   目标序号.
--*/
{
	int i;
	int maxHP = 0;
	int maxHPIndex = 0;
	int minHP = 0;
	int minHPIndex = 0;
	int curHP = 0;
	WORD w = 0;

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;
		curHP = gpGlobals->g.PlayerRoles.rgwHP[w];

		if (curHP != 0)
		{
			minHP = curHP;
			minHPIndex = i;
			maxHP = curHP;
			maxHPIndex = i;
			break;
		}
	}

	for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
	{
		w = gpGlobals->rgParty[i].wPlayerRole;
		curHP = gpGlobals->g.PlayerRoles.rgwHP[w];
		if (curHP == 0)
		{
			continue;
		}
		if (maxHP < curHP && PAL_New_IfPlayerCanMove(w))
		{
			maxHP = curHP;
			maxHPIndex = i;
		}
		if (minHP > curHP)
		{
			minHP = curHP;
			minHPIndex = i;
		}
	}

	if (fGetLowest == TRUE)
	{
		return minHPIndex;
	}
	else
	{
		return maxHPIndex;
	}
}

BOOL CPalBaseIO::PAL_New_GetTrueByPercentage(
	WORD		wPercentage
)
/*++
Purpose:根据百分比返回真值

Parameters:	[IN]  wPercentage - 百分数

Return value:有（输入%）的可能返回真值，其余返回假
--*/
{
	if (RandomLong(0, 99) < wPercentage)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

DWORD CPalBaseIO::PAL_New_GetLevelUpBaseExp(
	WORD		wLevel
)
{
	DWORD wExp = 0;
/*
	if (wLevel < 52)
	{
		wExp = 25 * wLevel * (wLevel - 1) / 2 + 15;
	}
	else
	{
		wExp = 32000;
	}
	*/
	wExp = 25 * wLevel * (wLevel - 1) / 2 + 15;
	return wExp;
}

DWORD CPalBaseIO::PAL_New_GetLevelUpExp(
	WORD		wLevel
)
{
#if 0
	//WORD wTimes = wLevel / 100;
	//WORD wRemainLevel = wLevel % 100;
	//DWORD dwExp = wTimes * PAL_New_GetLevelUpBaseExp(100) + PAL_New_GetLevelUpBaseExp(wRemainLevel);
#else
	DWORD dwExp = PAL_New_GetLevelUpBaseExp(wLevel);
#endif
	return min(dwExp, 9999999);
}


BOOL CPalBaseIO::PAL_New_IfPlayerCanMove(WORD wPlayerRole)
{
	if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] == 0 &&
#ifdef PAL_CLASSIC
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] == 0 &&
#else
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] == 0 &&
#endif
		gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

