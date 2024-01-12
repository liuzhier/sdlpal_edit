#ifndef COMMAND_H
#define COMMAND_H

#include "../s/stdafx.h"


#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif

#define _HAS_STD_BYTE 0


#define CP_ACP                    0           // default to ANSI code page

#define PAL_XY(x, y)    (PAL_POS)(((((WORD)(y)) << 16) & 0xFFFF0000) | (((WORD)(x)) & 0xFFFF))
#define PAL_X(xy)       (SHORT)((xy) & 0xFFFF)
#define PAL_Y(xy)       (SHORT)(((xy) >> 16) & 0xFFFF)

#define    FPS             10
#define    FRAME_TIME      (1000 / FPS)
#define       BATTLE_FPS               40
#define       BATTLE_FRAME_TIME        (1000 / BATTLE_FPS)

#define MAX_PARAMETER						32000	//武术、灵力、防御、身法、吉运、体力、真气的最大值，默认为999
#define MAX_PARAMETER_EXTRA					100		//属性值的最大额外值，战前检测，使用物品后检测。
//#define POISON_STATUS_EXPAND // 增加毒的烈度

#define ENEMY_MAX_MOVE						3		//敌人最多行动次数，如果为2则是经典版
#define MAX_POISON_LEVEL					99		//复活后解除的毒的最高等级；99一般为装备技能，如寿葫芦
#define EDIT_DAMAGE_CALC					1		//修改伤害计算公式（包括敌人的属性计算方式）
#define MAX_DAMAGE							32000	//最大伤害值
//#define SHOW_DATA_IN_BATTLE                 1       //在战斗中显示相关数据
#define SHOW_ENEMY_STATUS                   1       //在战斗中查看敌方数据
#define DWORD_EXP                           1       //经验值用DWORD
#define NEW_ScriptOnBattleEnd               1       //修改被覆盖的战后脚本
#define FINISH_GAME_MORE_ONE_TIME           1       // 

#define DONOT_SELECT_TARGET_RANDOMLY_TOTALLY        //不完全是随机选择目标
#define STRENGTHEN_ENEMY							//在后期加强前期的敌人、队伍多于3人，敌人加强
//#define EDIT_RECOVER_VALUE_AFTER_BATTLE             //修改战后恢复百分比（作者已写算法）
//#define USE_LACKY_VALUE								//除了逃跑，使用吉运值
#define EDIT_EXP_CALCULATION //修改经验的计算方式
#define SHOW_MP_DECREASING //当mp减少时也显示数值
#define ADD_SOME_POISONS_SUCCESSFULLY_ANYTIME		//某些毒可以对任何人均命中（无视敌方巫抗或我方毒抗）
#define ADD_SOME_STATUSES_SUCCESSFULLY_ANYTIME		//为敌人添加状态总是成功（无视巫抗）

#define INCREASE_ATTRIBUTE_AFTER_USE_MENGSHE		//灵儿使用梦蛇后，各项属性增加

#define INCREASE_EXTRA_PLAYER_HPMP					//回复体力真气的时候，额外回复一定百分比（根据等级）
#define INCREASE_EXTRA_HPMP_MIN_LEVEL		50
#define INCREASE_EXTRA_HPMP_MAX_LEVEL		70

#define New_EDIT_SCRIPT_OPERATION_0X0034			//修改灵葫炼丹为商店
//解决幻影林天南 BUG
#define LIN_TIAN_NAN_BUG

# define New_HP_MP                      1          //补体力真气时已满返回失败

# define Draw_Chase_Range          1               //显示驱魔香和十里香步数

#define STRENGTHEN_PLAYER							//有特色的加强主角，灵儿初始五灵抗性20%，阿奴毒抗巫抗各30%，林月如额外恢复

//设定控制100%
#define Controlled100 1
//设定夺魂100%
//#define Prodigy100 1
//设定物品掉落100%
#define Drop_Rate100 1

#define GAIN_MORE_HIDDEN_EXP 1              // 更多的隐藏经验
#define PAL_HAS_AVI		


#ifndef __WINPHONE__
#define PAL_HAS_NATIVEMIDI  1
#endif

#ifdef _DEBUG
//#define INVINCIBLE				//无敌模式，受攻击后不减hp不受不良状态影响
//#define KO_ENEMY_FAST
#endif // _DEBUG


#define SPRITENUM_SLASH                    39
#define SPRITENUM_ITEMBOX                  70
#define SPRITENUM_CURSOR_YELLOW            68
#define SPRITENUM_CURSOR                   69
#define SPRITENUM_PLAYERINFOBOX            18
#define SPRITENUM_PLAYERFACE_FIRST         48

#define GAMEMENU_LABEL_STATUS              3
#define GAMEMENU_LABEL_MAGIC               4
#define GAMEMENU_LABEL_INVENTORY           5
#define GAMEMENU_LABEL_SYSTEM              6

#define SYSMENU_LABEL_SAVE                 11
#define SYSMENU_LABEL_LOAD                 12
#define SYSMENU_LABEL_MUSIC                13
#define SYSMENU_LABEL_SOUND                14
#define SYSMENU_LABEL_QUIT                 15
#define SYSMENU_LABEL_BATTLEMODE           (PAL_ADDITIONAL_WORD_FIRST)

#define BATTLESPEEDMENU_LABEL_1            (PAL_ADDITIONAL_WORD_FIRST + 1)
#define BATTLESPEEDMENU_LABEL_2            (PAL_ADDITIONAL_WORD_FIRST + 2)
#define BATTLESPEEDMENU_LABEL_3            (PAL_ADDITIONAL_WORD_FIRST + 3)
#define BATTLESPEEDMENU_LABEL_4            (PAL_ADDITIONAL_WORD_FIRST + 4)
#define BATTLESPEEDMENU_LABEL_5            (PAL_ADDITIONAL_WORD_FIRST + 5)

#define INVMENU_LABEL_USE                  23
#define INVMENU_LABEL_EQUIP                22

#define SWITCHMENU_LABEL_DISABLE           17
#define SWITCHMENU_LABEL_ENABLE            18


#define CASH_LABEL                         21

#define ITEMUSEMENU_COLOR_STATLABEL        0xBB

#define DESCTEXT_COLOR                     0x2E

#define STATUS_BACKGROUND_FBPNUM           0
#define STATUS_LABEL_EXP                   2
#define STATUS_LABEL_LEVEL                 48
#define STATUS_LABEL_HP                    49
#define STATUS_LABEL_MP                    50
#define STATUS_LABEL_ATTACKPOWER           51
#define STATUS_LABEL_MAGICPOWER            52
#define STATUS_LABEL_RESISTANCE            53
#define STATUS_LABEL_DEXTERITY             54
#define STATUS_LABEL_FLEERATE              55
#define STATUS_COLOR_EQUIPMENT             0xBE

#define BUYMENU_LABEL_CURRENT              35
#define SELLMENU_LABEL_PRICE               25

#define EQUIPMENU_BACKGROUND_FBPNUM        1
#define CHUNKNUM_SPRITEUI                  9

#define MENUITEM_COLOR                     0x4F
#define MENUITEM_COLOR_INACTIVE            0x1C
#define MENUITEM_COLOR_CONFIRMED           0x2C
#define MENUITEM_COLOR_SELECTED_INACTIVE   0x1F
#define MENUITEM_COLOR_SELECTED_FIRST      0xF9
#define MENUITEM_COLOR_SELECTED_TOTALNUM   6

#define MENUITEM_COLOR_SELECTED                                    \
   (MENUITEM_COLOR_SELECTED_FIRST +                                \
      SDL_GetTicks_New() / (600 / MENUITEM_COLOR_SELECTED_TOTALNUM)    \
      % MENUITEM_COLOR_SELECTED_TOTALNUM)

#define MENUITEM_COLOR_EQUIPPEDITEM        0xC8

#define DESCTEXT_COLOR                     0x2E

#define RIX_NUM_OPENINGMENU                4
#define MAINMENU_LABEL_NEWGAME             7
#define MAINMENU_LABEL_LOADGAME            8

#define LOADMENU_LABEL_SLOT_FIRST          43

#define CONFIRMMENU_LABEL_NO               19
#define CONFIRMMENU_LABEL_YES              20

#define INVMENU_LABEL_USE                  23
#define INVMENU_LABEL_EQUIP                22

#define BATTLEWIN_GETEXP_LABEL             30
#define BATTLEWIN_BEATENEMY_LABEL          9
#define BATTLEWIN_DOLLAR_LABEL             10
#define BATTLEWIN_LEVELUP_LABEL            32
#define BATTLEWIN_ADDMAGIC_LABEL           33
#define BATTLEWIN_LEVELUP_LABEL_COLOR      0x39
#define SPRITENUM_ARROW                    47

#define BATTLE_LABEL_ESCAPEFAIL            31



#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#define PAL_CLASSIC 1
#define VOID void
#define FALSE 0
#define TRUE  1
#define CP_UTF8                   65001       // UTF-8 translation

    typedef char                CHAR;
    typedef short               SHORT;
    typedef long                LONG;
    typedef unsigned char       BYTE, * LPBYTE;

    typedef unsigned long       ULONG, * PULONG;
    typedef unsigned short      USHORT, * PUSHORT;
    typedef unsigned char       UCHAR, * PUCHAR;

    typedef unsigned short      WORD, * LPWORD;
    typedef unsigned long        DWORD, * LPDWORD;
    typedef int                 INT, * LPINT;
    typedef int					BOOL;
    typedef DWORD				PAL_POS;
    typedef CHAR* LPSTR;
    typedef unsigned int        UINT;
    typedef const CHAR* LPCSTR;
    typedef const VOID* LPCVOID;
    typedef VOID* LPVOID;
    typedef float				FLOAT;
    typedef unsigned int        UINT32, * PUINT32;
    typedef LPBYTE				LPSPRITE, LPBITMAPRLE;
    typedef const BYTE* LPCSPRITE, * LPCBITMAPRLE;
    typedef signed char         INT8, * PINT8;
    typedef signed short        INT16, * PINT16;
    typedef signed int          INT32, * PINT32;
    typedef signed __int64      INT64, * PINT64;
    typedef unsigned char       UINT8, * PUINT8;
    typedef unsigned short      UINT16, * PUINT16;
    typedef unsigned int        UINT32, * PUINT32;
    typedef unsigned __int64    UINT64, * PUINT64;

    typedef enum tagPlayerRoleID
    {
        RoleID_LiXiaoYao = 0,
        RoleID_ZhaoLingEr = 1,
        RoleID_LinYueRu = 2,
        RoleID_WuHou = 3,
        RoleID_ANu = 4,
        RoleID_GaiLuoJiao = 5,
    } PlayerRoleID;

    typedef enum tagPlayerPara
    {
        Para_Level = 0x0006,
        Para_MaxHP,
        Para_MaxMP,
        Para_HP,
        Para_MP,
        Para_AttackStrength = 0x0011,
        Para_MagicStrength,
        Para_Defense,
        Para_Dexterity,
        Para_FleeRate,
        Para_PoisonResistance = 0x0016,
        Para_CloudResistance = 0x0017,
        Para_ThunderResistance,
        Para_WaterResistance,
        Para_FireResistance,
        Para_EarthResistance,
        Para_SorceryResistance = 0x001c,
    }PlayerPara;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SWAP16(X)    (X)
#define SWAP32(X)    (X)
#else
#define SWAP16(X)    SDL_Swap16(X)
#define SWAP32(X)    SDL_Swap32(X)
#endif

#ifndef _MSC_VER


#if !defined(fmax) || !defined(fmin)
# include <math.h>
#endif

#include <float.h>

#ifndef max
# define max fmax
#endif

#ifndef min
# define min fmin
#endif

#else

#ifndef max
#define max(a, b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)    (((a) < (b)) ? (a) : (b))
#endif

#endif // !_MSC_VER



#if !defined (CYGWIN) && !defined (DINGOO) && !defined (GPH) && !defined (GEKKO) && !defined (__WINPHONE__)
//#define PAL_HAS_MP3           1
#endif

#define vsnprintf _vsnprintf

#ifdef _MSC_VER
#pragma warning (disable:4018)
#pragma warning (disable:4028)
#pragma warning (disable:4244)
#pragma warning (disable:4305)
#pragma warning (disable:4761)
#pragma warning (disable:4996)
#endif

    typedef char CHAR;
    typedef unsigned char BYTE;
    typedef const BYTE* LPCBYTE;


#define PAL_LARGE           /* */

#define SPRITENUM_BATTLEICON_ATTACK      40
#define SPRITENUM_BATTLEICON_MAGIC       41
#define SPRITENUM_BATTLEICON_COOPMAGIC   42
#define SPRITENUM_BATTLEICON_MISCMENU    43

#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER           69
#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER_RED       68

#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER          67
#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER_RED      66

#define BATTLEUI_LABEL_ITEM              5

#define PAL_ADDITIONAL_WORD_SECOND			20001

//#ifdef SHOW_DATA_IN_BATTLE
#define BATTLEUI_LABEL_DATA				PAL_ADDITIONAL_WORD_SECOND
//#endif

#define BATTLEUI_LABEL_ENEMY_STATUS		PAL_ADDITIONAL_WORD_SECOND + 1
#define STATUS_LABEL_SORCERYRESISTANCE	PAL_ADDITIONAL_WORD_SECOND + 2
#define STATUS_LABEL_POISONRESISTANCE	PAL_ADDITIONAL_WORD_SECOND + 3
#define STATUS_LABEL_PHYSICALRESISTANCE	PAL_ADDITIONAL_WORD_SECOND + 4
#define STATUS_LABEL_COLLECTVALUE		PAL_ADDITIONAL_WORD_SECOND + 5
#define STATUS_LABEL_STEALITEM			PAL_ADDITIONAL_WORD_SECOND + 6
#define STATUS_LABEL_ATTACKEQUIVITEM	PAL_ADDITIONAL_WORD_SECOND + 7
#define LABEL_NOTHING					PAL_ADDITIONAL_WORD_SECOND + 8

#define BATTLEUI_LABEL_DEFEND            58
#define BATTLEUI_LABEL_AUTO              56
#define BATTLEUI_LABEL_INVENTORY         57
#define BATTLEUI_LABEL_FLEE              59
#define BATTLEUI_LABEL_STATUS            60

#define BATTLEUI_LABEL_USEITEM           23
#define BATTLEUI_LABEL_THROWITEM         24

#define TIMEMETER_COLOR_DEFAULT          0x1B
#define TIMEMETER_COLOR_SLOW             0x5B
#define TIMEMETER_COLOR_HASTE            0x2A

#define BATTLEUI_MAX_SHOWNUM             16

//#define SOUND_Play(i) SOUND_PlayChannel((i), (g_iCurrChannel ^= 1))
#define SDLK_KP1     SDLK_KP_1
#define SDLK_KP2     SDLK_KP_2
#define SDLK_KP3     SDLK_KP_3
#define SDLK_KP4     SDLK_KP_4
#define SDLK_KP5     SDLK_KP_5
#define SDLK_KP6     SDLK_KP_6
#define SDLK_KP7     SDLK_KP_7
#define SDLK_KP8     SDLK_KP_8
#define SDLK_KP9     SDLK_KP_9
#define SDLK_KP0     SDLK_KP_0

#define SDL_HWSURFACE     0

#ifdef __cplusplus
}
#endif

#endif // COMMAND_H
