#ifndef CPALBASEIO_H
#define CPALBASEIO_H

#include "csdlscreen.h"
#include <functional>

#define MENUITEM_VALUE_CANCELLED		   0xFFFF

using namespace std;
typedef std::function<void(WORD)>LPITEMCHANGED_CALLBACK;

//typedef VOID(CGameUI::*LPITEMCHANGED_CALLBACK)(WORD);

typedef enum tagLOADRESFLAG
{
	kLoadScene = (1 << 0),    // load a scene
	kLoadPlayerSprite = (1 << 1),    // load player sprites
} LOADRESFLAG, * LPLOADRESFLAG;

typedef struct tagPALMAP
{
	DWORD          Tiles[128][64][2];
	LPSPRITE       pTileSprite;
	INT            iMapNum;

} PALMAP, * LPPALMAP;

typedef const PALMAP* LPCPALMAP;

typedef struct tagMENUITEM
{
	INT32          wValue;
	INT32          wNumWord;
	BOOL          fEnabled;
	PAL_POS       pos;
} MENUITEM, * LPMENUITEM;
//typedef DWORD PAL_POS;

typedef struct tagBOX
{
	PAL_POS        pos;
	WORD           wWidth, wHeight;
	CGL_Texture* lpSavedArea;
} BOX, * LPBOX;

#define MENUITEM_VALUE_CANCELLED      0xFFFF




typedef enum tagBATTLEUISTATE
{
	kBattleUIWait,
	kBattleUISelectMove,
	kBattleUISelectTargetEnemy,
	kBattleUISelectTargetPlayer,
	kBattleUISelectTargetEnemyAll,
	kBattleUISelectTargetPlayerAll,
} BATTLEUISTATE;

typedef enum tagBATTLEMENUSTATE
{
	kBattleMenuMain,
	kBattleMenuMagicSelect,
	kBattleMenuUseItemSelect,
	kBattleMenuThrowItemSelect,
	kBattleMenuMisc,
	kBattleMenuMiscItemSubMenu,
} BATTLEMENUSTATE;

typedef enum //tagBATTLEUIACTION
{
	kBattleUIActionAttack,
	kBattleUIActionMagic,
	kBattleUIActionCoopMagic,
	kBattleUIActionMisc,
} BATTLEUIACTION;

#define SPRITENUM_BATTLEICON_ATTACK      40
#define SPRITENUM_BATTLEICON_MAGIC       41
#define SPRITENUM_BATTLEICON_COOPMAGIC   42
#define SPRITENUM_BATTLEICON_MISCMENU    43

#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER           69
#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER_RED       68

#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER          67
#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER_RED      66

#define BATTLEUI_LABEL_ITEM              5


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

typedef struct tagSHOWNUM
{
	WORD             wNum;
	PAL_POS          pos;
	DWORD            dwTime;
	NUMCOLOR         color;
} SHOWNUM;

typedef enum tagBATTLERESULT
{
	kBattleResultWon = 3,			// player won the battle
	kBattleResultLost = 1,			// player lost the battle
	kBattleResultEnemyFleed = 0x7FFF,// 敌人逃跑
	kBattleResultFleed = 0xFFFF,	// player fleed from the battle
	kBattleResultTerminated = 0,	// battle terminated with scripts
	kBattleResultOnGoing = 1000,	// the battle is ongoing
	kBattleResultPreBattle = 1001,	// running pre-battle scripts
	kBattleResultPause = 1002,		// battle pause

} BATTLERESULT;


typedef enum tagFIGHTERSTATE
{
	kFighterWait,  // waiting time
	kFighterCom,   // accepting command
	kFighterAct,   // doing the actual move
} FIGHTERSTATE;

typedef enum tagBATTLEACTIONTYPE
{
	kBattleActionPass,          // do nothing						无动作
	kBattleActionDefend,        // defend							防御
	kBattleActionAttack,        // physical attack					物攻
	kBattleActionMagic,         // use magic							使用法术
	kBattleActionCoopMagic,     // use cooperative magic				使用合体法术
	kBattleActionFlee,          // flee from the battle				逃跑
	kBattleActionThrowItem,     // throw item onto enemy				投掷物品
	kBattleActionUseItem,       // use item							使用物品	
	kBattleActionAttackMate,    // attack teammate (confused only)	攻击队友（疯魔状态下）
} BATTLEACTIONTYPE;

typedef struct tagBATTLEUI
{
	BATTLEUISTATE    state;
	BATTLEMENUSTATE  MenuState;

	CHAR             szMsg[256];           // message to be shown on the screen
	CHAR             szNextMsg[256];       // next message to be shown on the screen
	DWORD            dwMsgShowTime;        // the end time of showing the message
	WORD             wNextMsgDuration;     // duration of the next message

	WORD             wCurPlayerIndex;      // index of the current player
	WORD             wSelectedAction;      // current selected action
	WORD             wSelectedIndex;       // current selected index of player or enemy
	WORD             wPrevEnemyTarget;     // previous enemy target

	BATTLEACTIONTYPE wActionType;          // type of action to be performed
	WORD             wObjectID;            // object ID of the item or magic to use

	BOOL             fAutoAttack;          // TRUE if auto attack

	SHOWNUM          rgShowNum[BATTLEUI_MAX_SHOWNUM];
} BATTLEUI;


typedef struct tagBATTLEACTION
{
	BATTLEACTIONTYPE   ActionType;
	WORD               wActionID;   // item/magic to use
	SHORT              sTarget;     // -1 for everyone
	FLOAT              flRemainingTime;  // remaining waiting time before the action start
} BATTLEACTION;

typedef struct tagBATTLEENEMY
{
	WORD               wObjectID;              // Object ID of this enemy
	ENEMY              e;                      // detailed data of this enemy
	WORD               rgwStatus[kStatusAll];  // status effects
	FLOAT              flTimeMeter;            // time-charging meter (0 = empty, 100 = full).
	POISONSTATUS       rgPoisons[MAX_POISONS]; // poisons
	LPSPRITE           lpSprite = NULL;
	PAL_POS            pos;                    // current position on the screen
	PAL_POS            posOriginal;            // original position on the screen
	WORD               wCurrentFrame;          // current frame number
	FIGHTERSTATE       state;                  // state of this enemy
	BOOL               fTurnStart;
	BOOL               fFirstMoveDone;
	BOOL               fDualMove;
	WORD               wScriptOnTurnStart;	//战前脚本,一般为战前的对话
	WORD               wScriptOnBattleEnd;	//战后脚本，例如获得某某物品
	WORD               wScriptOnReady;		//战时脚本，一般为敌人的出招脚本
	DWORD              wPrevHP;				// HP value prior to action（改成DWORD类型了）
	INT                iColorShift;

	DWORD              dwActualHealth;
	DWORD              dwMaxHealth;
	DWORD			   dwExp;//
} BATTLEENEMY;

// We only put some data used in battle here; other data can be accessed in the global data.
typedef struct tagBATTLEPLAYER
{
	INT                iColorShift;
	FLOAT              flTimeMeter;          // time-charging meter (0 = empty, 100 = full).
	FLOAT              flTimeSpeedModifier;
	WORD               wHidingTime;          // remaining hiding time
	LPSPRITE           lpSprite;
	PAL_POS            pos;                  // current position on the screen
	PAL_POS            posOriginal;          // original position on the screen
	WORD               wCurrentFrame;        // current frame number
	FIGHTERSTATE       state;                // state of this player
	BATTLEACTION       action;               // action to perform
	BOOL               fDefending;           // TRUE if player is defending
	INT				   fDefenAction;		 // The player action is defend 主动防御
	INT				   fNotKillImmediately;  // Not Kill the player immediately
	INT				   fBadStatus;			 //Set the bad status for player
	WORD               wPrevHP;              // HP value prior to action
	WORD               wPrevMP;              // MP value prior to action
#ifndef PAL_CLASSIC
	SHORT              sTurnOrder;           // turn order
#endif

#ifdef EDIT_EXP_CALCULATION
	DWORD				dwExpGained;
#endif

} BATTLEPLAYER;

typedef struct tagSUMMON
{
	LPSPRITE           lpSprite;
	WORD               wCurrentFrame;
} SUMMON;

#define MAX_BATTLE_ACTIONS    256
#define MAX_KILLED_ENEMIES    256

#ifdef PAL_CLASSIC

typedef enum tabBATTLEPHASE
{
	kBattlePhaseSelectAction,
	kBattlePhasePerformAction
} BATTLEPHASE;

typedef enum tabACTIONQUEUETYPE
{
	kActionQueueTypeEmpty,
	kActionQueueTypeEnemy,
	kActionQueueTypePlayer,
} ACTIONQUEUETYPE;

typedef struct tagACTIONQUEUE
{
	BOOL		fIsEmpty;
	BOOL       fIsEnemy;
	DWORD       wDexterity;
	WORD       wIndex;
} ACTIONQUEUE;

#define MAX_ACTIONQUEUE_ITEMS (MAX_PLAYERS_IN_PARTY + MAX_ENEMIES_IN_TEAM * ENEMY_MAX_MOVE)

#endif

typedef struct tagMENGSHECOUNT
{
	WORD			MaxHP;
	WORD			MaxMP;
	WORD			AttackStrength;
	WORD			MagicStrength;
	WORD			Defense;
	WORD			Dexterity;
	WORD			FleeRate;
} MENGSHECOUNT;

typedef struct tagBATTLE
{
	BATTLEPLAYER     rgPlayer[MAX_PLAYERS_IN_PARTY];
	BATTLEENEMY      rgEnemy[MAX_ENEMIES_IN_TEAM];

	WORD             wMaxEnemyIndex;

	CGL_Texture * lpSceneBuf;			//战斗画面的图层，包括角色、敌人
	SDL_Surface* lpBackground;			//战斗中地图画面的图层

	SHORT            sBackgroundColorShift;

	LPSPRITE         lpSummonSprite;       // sprite of summoned god
	PAL_POS          posSummon;
	INT              iSummonFrame;         // current frame of the summoned god

	INT              iExpGained;           // total experience value gained
	INT              iCashGained;          // total cash gained

	BOOL             fIsBoss;              // TRUE if boss fight
	BOOL             fEnemyCleared;        // TRUE if enemies are cleared
	BATTLERESULT     BattleResult;

	FLOAT            flTimeChargingUnit;   // the base waiting time unit

	BATTLEUI         UI;

	LPBYTE           lpEffectSprite;

	BOOL             fEnemyMoving;         // TRUE if enemy is moving

	BOOL             fPlayerMoving;         // TRUE if player is moving

	INT              iHidingTime;          // Time of hiding

	WORD             wMovingPlayerIndex;   // current moving player index

	WORD             wMovingEnemyIndex;   // new, current moving Enemy index

	int              iBlow;

#ifdef PAL_CLASSIC
	BATTLEPHASE      Phase;
	ACTIONQUEUE      ActionQueue[MAX_ACTIONQUEUE_ITEMS];
	int              iCurAction;
	BOOL             fRepeat;              // TRUE if player pressed Repeat
	BOOL             fForce;               // TRUE if player pressed Force
	BOOL             fFlee;                // TRUE if player pressed Flee
#endif
	BOOL    g_ifUseMenshe[MAX_PLAYER_ROLES]{ 0 };//使用梦蛇后，各项属性增加
} BATTLE;



//本类是游戏底层与上层之间的中间层
class CPalBaseIO : public CSdlScreen
{
public:
    struct MAGICITEM
    {
        WORD         wMagic;
        WORD         wMP;
        BOOL         fEnabled;
    } rgMagicItem[MAX_PLAYER_MAGICS];
	BATTLE g_Battle;
	int     g_iCurrentItem = 0;
	int     g_iNumMagic = 0;
	WORD	g_wCurEffectSprite = 0;
	BOOL    g_fScriptSuccess = TRUE;
	WORD    g_wPlayerMP = 0;
	BOOL	g_fUpdatedInBattle = 0;

public:
    CPalBaseIO();

	~CPalBaseIO();

    VOID PAL_RNGPlay(INT iNumRNG, INT iStartFrame, INT iEndFrame, INT iSpeed);
	INT PAL_RLEBlitMonoColor(LPCBITMAPRLE lpBitmapRLE, CGL_Texture* lpDstSurface, PAL_POS pos, BYTE bColor, INT iColorShift);
    INT PAL_RLEBlitMonoColor(LPCBITMAPRLE lpBitmapRLE, SDL_Surface* lpDstSurface, PAL_POS pos, BYTE bColor, INT iColorShift);
	VOID PAL_StartDialog(BYTE bDialogLocation, BYTE bFontColor, INT iNumCharFace, BOOL fPlayingRNG);
	VOID PAL_DialogWaitForKey(VOID);
	VOID PAL_ShowDialogText(LPCSTR lpszText);
	VOID PAL_ShowWideDialogText(LPCWSTR lpszText);
	VOID PAL_ClearDialog(BOOL fWaitForKey);
	VOID PAL_EndDialog(VOID);
	BOOL PAL_IsInDialog(VOID);
	BOOL PAL_DialogIsPlayingRNG(VOID);
	VOID PAL_ApplyWave(CGL_Texture* lpSurface);
	VOID PAL_ApplyWave(SDL_Surface* lpSurface);
	VOID PAL_DialogSetDelayTime(INT iDelayTime);


	//cmenu.cpp
	INT PAL_OpeningMenu(VOID);
	VOID PAL_DrawOpeningMenuBackground(VOID);
	WORD PAL_ReadMenu(LPMENUITEM rgMenuItem, INT nMenuItem, WORD wDefaultItem,
		BYTE bLabelColor, LPITEMCHANGED_CALLBACK lpfnMenuItemChanged);

	INT PAL_SaveSlotMenu(WORD wDefaultSlot);
	LPBOX PAL_CreateBox(PAL_POS pos, INT nRows, INT nColumns, INT iStyle, BOOL fSaveScreen);
	VOID PAL_DeleteBox(LPBOX  lpBox);
	BOOL PAL_ConfirmMenu(LPCSTR text = NULL);
	LPBOX PAL_CreateSingleLineBox(PAL_POS pos, INT nLen, BOOL fSaveScreen);

	//cres.cpp
	typedef struct tagRESOURCES
	{
		BYTE             bLoadFlags;

		LPPALMAP         lpMap;                                      // current loaded map
		LPSPRITE*		 lppEventObjectSprites;                      // event object sprites
		int              nEventObject;                               // number of event objects

		LPSPRITE         rglpPlayerSprite[MAX_PLAYERS_IN_PARTY + 1]; // player sprites
	} RESOURCES, * LPRESOURCES;
public:
	LPRESOURCES gpResources = NULL;

public:
	//cres.cpp
	VOID PAL_InitResources(VOID);
	VOID PAL_FreeResources(VOID);
	VOID PAL_SetLoadFlags(BYTE bFlags);
	VOID PAL_LoadResources(VOID);
	LPPALMAP PAL_GetCurrentMap(VOID);
	LPSPRITE PAL_GetPlayerSprite(BYTE bPlayerIndex);
	LPSPRITE PAL_GetEventObjectSprite(WORD wEventObjectID);
	WORD PAL_SpriteGetNumFrames(LPCSPRITE lpSprite);
private:
public:
	VOID PAL_FreeEventObjectSprites(VOID);
	VOID PAL_FreePlayerSprites(VOID);

	//cplayer.cpp
	BOOL PAL_AddItemToInventory(WORD wObjectID, INT iNum);
	INT PAL_GetItemAmount(WORD wItem);
	BOOL PAL_IncreaseHPMP(WORD wPlayerRole, SHORT sHP, SHORT sMP);
	VOID PAL_RemoveEquipmentEffect(WORD wPlayerRole, WORD wEquipPart);
	VOID PAL_AddPoisonForPlayer(WORD wPlayerRole, WORD wPoisonID);
	VOID PAL_CurePoisonByKind(WORD wPlayerRole, WORD wPoisonID);
	VOID PAL_CurePoisonByLevel(WORD wPlayerRole, WORD wMaxLevel);
	BOOL PAL_New_IsAnItemInArray(WORD item, const WORD items[]);
	BOOL PAL_IsPlayerPoisonedByLevel(WORD wPlayerRole, WORD wMinLevel);
	INT PAL_New_GetPoisonIndexForPlayer(WORD wPlayerRole, WORD wPoisonID);
	BOOL PAL_New_IsPlayerPoisoned(WORD wPlayerRole);
	VOID PAL_New_SortPoisonsForPlayerByLevel(WORD wPlayerRole);
	WORD PAL_GetPlayerAttackStrength(WORD wPlayerRole);
	WORD PAL_GetPlayerMagicStrength(WORD wPlayerRole);
	WORD PAL_GetPlayerDefense(WORD wPlayerRole);
	WORD PAL_GetPlayerDexterity(WORD wPlayerRole);
	WORD PAL_GetPlayerFleeRate(WORD wPlayerRole);
	WORD PAL_GetPlayerPoisonResistance(WORD wPlayerRole);
	WORD PAL_New_GetPlayerSorceryResistance(WORD wPlayerRole);
	WORD PAL_New_GetPlayerSorceryStrength(WORD wPlayerRole);
	WORD PAL_GetPlayerElementalResistance(WORD wPlayerRole, INT iAttrib);
	WORD PAL_New_GetPlayerPhysicalResistance(WORD wPlayerRole);
	WORD PAL_GetPlayerBattleSprite(WORD wPlayerRole);
	WORD PAL_GetPlayerCooperativeMagic(WORD wPlayerRole);
	BOOL PAL_PlayerCanAttackAll(WORD wPlayerRole);
	BOOL PAL_AddMagic(WORD wPlayerRole, WORD wMagic);
	VOID PAL_RemoveMagic(WORD wPlayerRole, WORD wMagic);
	VOID PAL_SetPlayerStatus(WORD wPlayerRole, WORD wStatusID, WORD wNumRound);
	VOID PAL_RemovePlayerStatus(WORD wPlayerRole, WORD wStatusID);
	VOID PAL_New_RemovePlayerAllStatus(WORD wPlayerRole);
	VOID PAL_ClearAllPlayerStatus(VOID);
	VOID PAL_PlayerLevelUp(WORD wPlayerRole, WORD wNumLevel);
	INT PAL_New_GetPlayerIndex(WORD wPlayerRole);
	WORD PAL_New_GetPlayerID(WORD wPlayerIndex);
	INT PAL_New_GetPlayerIndexByPara(PlayerPara pp, BOOL fGetLowest);
	INT PAL_New_GetPlayerIndexByHealth(BOOL fGetLowest);
	BOOL PAL_New_GetTrueByPercentage(WORD wPercentage);
	DWORD PAL_New_GetLevelUpBaseExp(WORD wLevel);
	DWORD PAL_New_GetLevelUpExp(WORD wLevel);
	BOOL PAL_New_IfPlayerCanMove(WORD wPlayerRole);

	//cmap.cpp
	LPPALMAP  PAL_LoadMap(INT iMapNum, FILE* fpMapMKF, FILE* fpGopMKF);
	VOID PAL_FreeMap(LPPALMAP lpMap);
	LPCBITMAPRLE PAL_MapGetTileBitmap(BYTE x, BYTE y, BYTE h, BYTE ucLayer, LPCPALMAP lpMap);
	BOOL PAL_MapTileIsBlocked(BYTE x, BYTE y, BYTE h, LPCPALMAP lpMap);
	BYTE PAL_MapGetTileHeight(BYTE x, BYTE y, BYTE h, BYTE ucLayer, LPCPALMAP lpMap);
	VOID PAL_MapBlitToSurface(LPCPALMAP lpMap, SDL_Surface* lpSurface, const SDL_Rect* lpSrcRect, BYTE ucLayer);
	VOID PAL_MapBlitToSurface(LPCPALMAP lpMap, CGL_Texture* lpSurface, const SDL_Rect* lpSrcRect, BYTE ucLayer);

};

#endif // CPALBASEIO_H
