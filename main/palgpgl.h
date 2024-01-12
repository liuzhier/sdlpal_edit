#pragma once
#ifndef PALGPGL_H
#define PALGPGL_H

#include <stdio.h>
#include "command.h"
//#include "palgpgl.h"

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
typedef CHAR*				LPSTR;
typedef unsigned int        UINT;
typedef const CHAR* LPCSTR;
typedef const VOID* LPCVOID;
typedef VOID* LPVOID;
typedef float				FLOAT;
typedef unsigned int        UINT32, * PUINT32;
typedef LPBYTE				LPSPRITE, LPBITMAPRLE;
typedef const BYTE* LPCSPRITE, * LPCBITMAPRLE;

#define CP_ACP                    0           // default to ANSI code page


#define SDL_Colour SDL_Color
#define PAL_XY(x, y)    (PAL_POS)(((((WORD)(y)) << 16) & 0xFFFF0000) | (((WORD)(x)) & 0xFFFF))
#define PAL_X(xy)       (SHORT)((xy) & 0xFFFF)
#define PAL_Y(xy)       (SHORT)(((xy) >> 16) & 0xFFFF)


#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SWAP16(X)    (X)
#define SWAP32(X)    (X)
#define DO_SWAP 0
#else
#define SWAP16(X)    SDL_Swap16(X)
#define SWAP32(X)    SDL_Swap32(X)
#define DO_SWAP 1
#endif

// maximum number of players in party
#define     MAX_PLAYERS_IN_PARTY         5
//typedef const BYTE *LPCBYTE;


// total number of possible player roles
#define     MAX_PLAYER_ROLES             6

// totally number of playable player roles
#define     MAX_PLAYABLE_PLAYER_ROLES    5

// maximum entries of inventory
#define     MAX_INVENTORY                256

// maximum items in a store
#define     MAX_STORE_ITEM               9

// total number of magic attributes
#define     NUM_MAGIC_ELEMENTAL          5

// maximum number of enemies in a team
#define     MAX_ENEMIES_IN_TEAM          5

// maximum number of equipments for a player
#define     MAX_PLAYER_EQUIPMENTS        6

// maximum number of magics for a player
#define     MAX_PLAYER_MAGICS            32

// maximum number of scenes
#define     MAX_SCENES                   300

// maximum number of objects
#define     MAX_OBJECTS                  600

// maximum number of event objects (should be somewhat more than the original,
// as there are some modified versions which has more)
#define     MAX_EVENT_OBJECTS            5500

// maximum number of effective poisons to players
#define     MAX_POISONS                  16

// maximum number of level
#define     MAX_LEVELS                   99

// 物品的最大数量
#define MAX_ITEM_NUM_IN_INVENTORY			99

#define PAL_LARGE           /* */

// status of characters
typedef enum tagSTATUS
{
	kStatusConfused = 0,  // attack friends randomly        //				疯魔

	kStatusParalyzed,     // paralyzed										定身

	kStatusSleep,         // not allowed to move							昏睡
	kStatusSilence,       // cannot use magic								咒封
	kStatusPuppet,        // for dead players only, continue attacking		死者继续攻击
	kStatusBravery,       // more power for physical attacks				普通攻击加强
	kStatusProtect,       // more defense value								防御加强
	kStatusHaste,         // faster											身法加强
	kStatusDualAttack,    // dual attack									两次攻击
	kStatusAll
} STATUS;



// body parts of equipments
typedef enum tagBODYPART
{
	kBodyPartHead = 0,
	kBodyPartBody,
	kBodyPartShoulder,
	kBodyPartHand,
	kBodyPartFeet,
	kBodyPartWear,
	kBodyPartExtra,
} BODYPART;

// state of event object, used by the sState field of the EVENTOBJECT struct
typedef enum tagOBJECTSTATE
{
	kObjStateHidden = 0,
	kObjStateNormal = 1,
	kObjStateBlocker = 2
} OBJECTSTATE, *LPOBJECTSTATE;

typedef enum tagTRIGGERMODE
{
	kTriggerNone = 0,
	kTriggerSearchNear = 1,
	kTriggerSearchNormal = 2,
	kTriggerSearchFar = 3,
	kTriggerTouchNear = 4,
	kTriggerTouchNormal = 5,
	kTriggerTouchFar = 6,
	kTriggerTouchFarther = 7,
	kTriggerTouchFarthest = 8
} TRIGGERMODE;



typedef struct tagFont
{
	LPWORD           lpBufChar;
	LPBYTE           lpBufGlyph;
	INT              nChar;
} FONT, *LP_FONT;


typedef struct tagEVENTOBJECT
{
	SHORT        sVanishTime;         // vanish time (?)
	WORD         x;                   // X coordinate on the map
	WORD         y;                   // Y coordinate on the map
	SHORT        sLayer;              // layer value
	WORD         wTriggerScript;      // Trigger script entry
	WORD         wAutoScript;         // Auto script entry
	SHORT        sState;              // state of this object
	WORD         wTriggerMode;        // trigger mode
	WORD         wSpriteNum;          // number of the sprite
	USHORT       nSpriteFrames;       // total number of frames of the sprite
	WORD         wDirection;          // direction
	WORD         wCurrentFrameNum;    // current frame number
	USHORT       nScriptIdleFrame;    // count of idle frames, used by trigger script
	WORD         wSpritePtrOffset;    // FIXME: ???
	USHORT       nSpriteFramesAuto;   // total number of frames of the sprite, used by auto script
	WORD         wScriptIdleFrameCountAuto;     // count of idle frames, used by auto script
} EVENTOBJECT, *LPEVENTOBJECT;

typedef struct tagSCENE
{
	WORD         wMapNum;         // number of the map
	WORD         wScriptOnEnter;  // when entering this scene, execute script from here
	WORD         wScriptOnTeleport;  // when teleporting out of this scene, execute script from here
	WORD         wEventObjectIndex;  // event objects in this scene begins from number wEventObjectIndex + 1
} SCENE, *LPSCENE;

// object including system strings, players, items, magics, enemies and poison scripts.

// system strings and players
typedef struct tagOBJECT_PLAYER
{
	WORD         wReserved[2];    // always zero
	WORD         wScriptOnFriendDeath; // when friends in party dies, execute script from here
	WORD         wScriptOnDying;  // when dying, execute script from here
} OBJECT_PLAYER;

typedef enum tagITEMFLAG
{
	kItemFlagUsable = (1 << 0),
	kItemFlagEquipable = (1 << 1),
	kItemFlagThrowable = (1 << 2),
	kItemFlagConsuming = (1 << 3),
	kItemFlagApplyToAll = (1 << 4),
	kItemFlagSellable = (1 << 5),
	kItemFlagEquipableByPlayerRole_First = (1 << 6)
} ITEMFLAG;

// items
typedef struct tagOBJECT_ITEM
{
	WORD         wBitmap;         // bitmap number in BALL.MKF
	WORD         wPrice;          // price
	WORD         wScriptOnUse;    // script executed when using this item
	WORD         wScriptOnEquip;  // script executed when equipping this item
	WORD         wScriptOnThrow;  // script executed when throwing this item to enemy
	WORD         wScriptDesc;     // description script
	WORD         wFlags;          // flags
} OBJECT_ITEM;

typedef enum tagMAGICFLAG
{
	kMagicFlagUsableOutsideBattle = (1 << 0),
	kMagicFlagUsableInBattle = (1 << 1),
	kMagicFlagUsableToEnemy = (1 << 3),
	kMagicFlagApplyToAll = (1 << 4),
} MAGICFLAG;

// magics
typedef struct tagOBJECT_MAGIC
{
	WORD         wMagicNumber;      // magic number, according to DATA.MKF #3
	WORD         wReserved1;        // always zero
	WORD         wScriptOnSuccess;  // when magic succeed, execute script from here
	WORD         wScriptOnUse;      // when use this magic, execute script from here
	WORD         wScriptDesc;       // description script
	WORD         wReserved2;        // always zero
	WORD         wFlags;            // flags
} OBJECT_MAGIC;

// enemies
typedef struct tagOBJECT_ENEMY
{
	WORD         wEnemyID;        // ID of the enemy, according to DATA.MKF #1.
	// Also indicates the bitmap number in ABC.MKF.
	WORD         wResistanceToSorcery;  // resistance to sorcery and poison (0 min, 10 max)
	WORD         wScriptOnTurnStart;    // script executed when turn starts
	WORD         wScriptOnBattleEnd;    // script executed when battle ends
	WORD         wScriptOnReady;        // script executed when the enemy is ready
} OBJECT_ENEMY;

// poisons (scripts executed in each round)
typedef struct tagOBJECT_POISON
{
	WORD         wPoisonLevel;    // level of the poison
	WORD         wColor;          // color of avatars
	WORD         wPlayerScript;   // script executed when player has this poison (per round)
	WORD         wReserved;       // always zero
	WORD         wEnemyScript;    // script executed when enemy has this poison (per round)
} OBJECT_POISON;

typedef struct tagOBJECT_ITEM_DOS
{
	WORD         wBitmap;         // bitmap number in BALL.MKF
	WORD         wPrice;          // price
	WORD         wScriptOnUse;    // script executed when using this item
	WORD         wScriptOnEquip;  // script executed when equipping this item
	WORD         wScriptOnThrow;  // script executed when throwing this item to enemy
	WORD         wFlags;          // flags
} OBJECT_ITEM_DOS;

typedef struct tagOBJECT_MAGIC_DOS
{
	WORD         wMagicNumber;      // magic number, according to DATA.MKF #3
	WORD         wReserved1;        // always zero
	WORD         wScriptOnSuccess;  // when magic succeed, execute script from here
	WORD         wScriptOnUse;      // when use this magic, execute script from here
	WORD         wReserved2;        // always zero
	WORD         wFlags;            // flags
} OBJECT_MAGIC_DOS;

typedef union tagOBJECT_DOS
{
	WORD              rgwData[6];
	OBJECT_PLAYER     player;
	OBJECT_ITEM_DOS   item;
	OBJECT_MAGIC_DOS  magic;
	OBJECT_ENEMY      enemy;
	OBJECT_POISON     poison;
} OBJECT_DOS, * LPOBJECT_DOS;

typedef union tagOBJECT//win95
{
	WORD              rgwData[7];
	OBJECT_PLAYER     player;
	OBJECT_ITEM       item;
	OBJECT_MAGIC      magic;
	OBJECT_ENEMY      enemy;
	OBJECT_POISON     poison;
} OBJECT, *LPOBJECT;

typedef struct tagSCRIPTENTRY
{
	WORD          wOperation = 0;     // operation code
	WORD          rgwOperand[3]{ 0 };  // operands
} SCRIPTENTRY, * LPSCRIPTENTRY;

typedef struct tagINVENTORY
{
	WORD          wItem;             // item object code
	USHORT        nAmount;           // amount of this item
	USHORT        nAmountInUse;      // in-use amount of this item
} INVENTORY, *LPINVENTORY;

typedef struct tagSTORE
{
	WORD          rgwItems[MAX_STORE_ITEM];
} STORE, *LPSTORE;

typedef struct tagENEMY
{
	WORD        wIdleFrames;         // total number of frames when idle
	WORD        wMagicFrames;        // total number of frames when using magics
	WORD        wAttackFrames;       // total number of frames when doing normal attack
	WORD        wIdleAnimSpeed;      // speed of the animation when idle
	WORD        wActWaitFrames;      // FIXME: ???
	WORD        wYPosOffset;
	WORD        wAttackSound;        // sound played when this enemy uses normal attack
	WORD        wActionSound;        // FIXME: ???
	WORD        wMagicSound;         // sound played when this enemy uses magic
	WORD        wDeathSound;         // sound played when this enemy dies
	WORD        wCallSound;          // sound played when entering the battle
	WORD        wHealth;             // total HP of the enemy
	WORD        wExps;                // How many EXPs we'll get for beating this enemy
	WORD        wCash;               // how many cashes we'll get for beating this enemy
	WORD        wLevel;              // this enemy's level
	WORD        wMagic;              // this enemy's magic number
	WORD        wMagicRate;          // chance for this enemy to use magic
	WORD        wAttackEquivItem;    // equivalence item of this enemy's normal attack
	WORD        wAttackEquivItemRate;// chance for equivalence item
	WORD        wStealItem;          // which item we'll get when stealing from this enemy
	USHORT      nStealItem;          // total amount of the items which can be stolen
	WORD        wAttackStrength;     // normal attack strength
	WORD        wMagicStrength;      // magical attack strength
	WORD        wDefense;            // resistance to all kinds of attacking
	WORD        wDexterity;          // dexterity
	WORD        wFleeRate;           // chance for successful fleeing
	WORD        wPoisonResistance;   // resistance to poison
	WORD        wElemResistance[NUM_MAGIC_ELEMENTAL]; // resistance to elemental magics
	WORD        wPhysicalResistance; // resistance to physical attack
	WORD        wDualMove;           // whether this enemy can do dual move or not
	WORD        wCollectValue;       // value for collecting this enemy for items
} ENEMY, *LPENEMY;

typedef struct tagENEMYTEAM
{
	WORD        rgwEnemy[MAX_ENEMIES_IN_TEAM];
} ENEMYTEAM, *LPENEMYTEAM;

typedef WORD PLAYERS[MAX_PLAYER_ROLES];

typedef struct tagPLAYERROLES
{
	PLAYERS            rgwAvatar;             // avatar (shown in status view)
	PLAYERS            rgwSpriteNumInBattle;  // sprite displayed in battle (in F.MKF)
	PLAYERS            rgwSpriteNum;          // sprite displayed in normal scene (in MGO.MKF)
	PLAYERS            rgwName;               // name of player class (in WORD.DAT)
	PLAYERS            rgwAttackAll;          // whether player can attack everyone in a bulk or not
	PLAYERS            rgwCanBeyondMaxPara;   // 未知属性 //暂时用来作为是否检查超过最大值的标志位
	PLAYERS            rgwLevel;              // level
	PLAYERS            rgwMaxHP;              // maximum HP
	PLAYERS            rgwMaxMP;              // maximum MP
	PLAYERS            rgwHP;                 // current HP
	PLAYERS            rgwMP;                 // current MP
	WORD               rgwEquipment[MAX_PLAYER_EQUIPMENTS][MAX_PLAYER_ROLES]; // equipments
	PLAYERS            rgwAttackStrength;     // normal attack strength
	PLAYERS            rgwMagicStrength;      // magical attack strength
	PLAYERS            rgwDefense;            // resistance to all kinds of attacking
	PLAYERS            rgwDexterity;          // dexterity
	PLAYERS            rgwFleeRate;           // chance of successful fleeing
	PLAYERS            rgwPoisonResistance;   // resistance to poison
	WORD               rgwElementalResistance[NUM_MAGIC_ELEMENTAL][MAX_PLAYER_ROLES]; // resistance to elemental magics
	PLAYERS            rgwSorceryResistance;  // 未知属性 //暂时用来作为巫抗
	PLAYERS            rgwPhysicalResistance; // 未知属性 //暂时用来作为物抗
	PLAYERS            rgwSorceryStrength;    // 未知属性 //暂时用来作为巫攻
	PLAYERS            rgwCoveredBy;          // who will cover me when I am low of HP or not sane
	WORD               rgwMagic[MAX_PLAYER_MAGICS][MAX_PLAYER_ROLES]; // magics
	PLAYERS            rgwWalkFrames;         // walk frame (???)
	PLAYERS            rgwCooperativeMagic;   // cooperative magic
	PLAYERS            rgwUnknown5;           // FIXME: ???
	PLAYERS            rgwUnknown6;           // FIXME: ???
	PLAYERS            rgwDeathSound;         // sound played when player dies
	PLAYERS            rgwAttackSound;        // sound played when player attacks
	PLAYERS            rgwWeaponSound;        // weapon sound (???)
	PLAYERS            rgwCriticalSound;      // sound played when player make critical hits
	PLAYERS            rgwMagicSound;         // sound played when player is casting a magic
	PLAYERS            rgwCoverSound;         // sound played when player cover others
	PLAYERS            rgwDyingSound;         // sound played when player is dying
} PLAYERROLES, *LPPLAYERROLES;

typedef enum tagMAGIC_TYPE
{
	kMagicTypeNormal = 0,
	kMagicTypeAttackAll = 1,  // draw the effect on each of the enemies
	kMagicTypeAttackWhole = 2,  // draw the effect on the whole enemy team
	kMagicTypeAttackField = 3,  // draw the effect on the battle field
	kMagicTypeApplyToPlayer = 4,  // the magic is used on one player
	kMagicTypeApplyToParty = 5,  // the magic is used on the whole party
	kMagicTypeTrance = 8,  // trance the player
	kMagicTypeSummon = 9,  // summon
} MAGIC_TYPE;

typedef struct tagMAGIC
{
	WORD               wEffect;               // effect sprite
	WORD               wType;                 // type of this magic
	WORD               wXOffset;
	WORD               wYOffset;
	WORD               wSummonEffect;         // summon effect sprite (in F.MKF)
	WORD               wSpeed;                // speed of the effect
	WORD               wKeepEffect;           // FIXME: ???
	WORD               wSoundDelay;           // delay of the SFX
	WORD               wEffectTimes;          // total times of effect
	WORD               wShake;                // shake screen
	WORD               wWave;                 // wave screen
	WORD               wUnknown;              // FIXME: ???
	WORD               wCostMP;               // MP cost
	WORD               wBaseDamage;           // base damage
	WORD               wElemental;            // elemental (0 = No Elemental, last = poison)
	WORD               wSound;                // sound played when using this magic
} MAGIC, *LPMAGIC;

typedef struct tagBATTLEFIELD
{
	WORD               wScreenWave;                      // level of screen waving
	SHORT              rgsMagicEffect[NUM_MAGIC_ELEMENTAL]; // effect of attributed magics
} BATTLEFIELD, *LPBATTLEFIELD;

// magics learned when level up
typedef struct tagLEVELUPMAGIC
{
	WORD               wLevel;    // level reached
	WORD               wMagic;    // magic learned
} LEVELUPMAGIC, *LPLEVELUPMAGIC;

typedef struct tagLEVELUPMAGIC_ALL
{
	LEVELUPMAGIC       m[MAX_PLAYABLE_PLAYER_ROLES];
} LEVELUPMAGIC_ALL, *LPLEVELUPMAGIC_ALL;

typedef struct tagENEMYPOS
{
	struct {
		WORD      x;
		WORD      y;
	} pos[MAX_ENEMIES_IN_TEAM][MAX_ENEMIES_IN_TEAM];
} ENEMYPOS, *LPENEMYPOS;

// Exp. points needed for the next level
typedef WORD LEVELUPEXP, *LPLEVELUPEXP;

// game data which is available in data files.
typedef struct tagGAMEDATA
{
	LPEVENTOBJECT           lprgEventObject{0};
	int                     nEventObject{0};

	SCENE                   rgScene[MAX_SCENES]{0};
	int						nScene{0};

	LPVOID                  rgObject{nullptr};
	int						nObject{0};

	LPSCRIPTENTRY           lprgScriptEntry{0};
	int                     nScriptEntry{0};

	LPSTORE                 lprgStore{0};
	int                     nStore{0};

	LPENEMY                 lprgEnemy{0};
	int                     nEnemy{0};

	LPENEMYTEAM             lprgEnemyTeam{0};
	int                     nEnemyTeam{0};

	PLAYERROLES             PlayerRoles{0};

	LPMAGIC                 lprgMagic{0};
	int                     nMagic{0};

	LPBATTLEFIELD           lprgBattleField{0};
	int                     nBattleField{0};

	LPLEVELUPMAGIC_ALL      lprgLevelUpMagic{0};
	int                     nLevelUpMagic{0};

	ENEMYPOS                EnemyPos{0};
	LEVELUPEXP              rgLevelUpExp[MAX_LEVELS + 1]{0};

	WORD                    rgwBattleEffectIndex[10][2]{0};
} GAMEDATA, * LPGAMEDATA;

typedef struct tagFILES
{
	FILE            *fpFBP;      // battlefield background images
	FILE            *fpMGO;      // sprites in scenes
	FILE            *fpBALL;     // item bitmaps
	FILE            *fpDATA;     // misc data
	FILE            *fpF;        // player sprites during battle
	FILE            *fpFIRE;     // fire effect sprites
	FILE            *fpRGM;      // character face bitmaps
	FILE            *fpSSS;      // script data
} FILES, *LPFILES;

// player party
typedef struct tagPARTY
{
	WORD             wPlayerRole;         // player role
	SHORT            x, y;                // position
	WORD             wFrame;              // current frame number
	WORD             wImageOffset;        // FIXME: ???
} PARTY, *LPPARTY;

// player trail, used for other party members to follow the main party member
typedef struct tagTRAIL
{
	WORD             x, y;          // position
	WORD             wDirection;    // direction
} TRAIL, *LPTRAIL;

typedef struct tagEXPERIENCE
{
#ifdef DWORD_EXP
	DWORD		wExp;
#else
	WORD         wExp;                // current experience points
	WORD         wReserved;
#endif
	WORD         wLevel;              // current level
	WORD         wCount;
} EXPERIENCE, *LPEXPERIENCE;

typedef struct tagALLEXPERIENCE
{
	EXPERIENCE        rgPrimaryExp[MAX_PLAYER_ROLES];	//主经验
	EXPERIENCE        rgHealthExp[MAX_PLAYER_ROLES];	
	EXPERIENCE        rgMagicExp[MAX_PLAYER_ROLES];		
	EXPERIENCE        rgAttackExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgMagicPowerExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgDefenseExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgDexterityExp[MAX_PLAYER_ROLES];
	EXPERIENCE        rgFleeExp[MAX_PLAYER_ROLES];
} ALLEXPERIENCE, *LPALLEXPERIENCE;

typedef enum tagExpType
{
	PrimaryExp = 0,
	HealthExp = 1,
	MagicExp,
	AttackExp,
	MagicPowerExp,
	DefenseExp,
	DexterityExp,
	FleeExp = 7,
} ExpType;


typedef struct tagPOISONSTATUS
{
	WORD			wPoisonID{ 0 };       // kind of the poison
	WORD			wPoisonScript{ 0 };   // script entry
//#ifdef POISON_STATUS_EXPAND
	WORD			wPoisonIntensity{ 0 }; // 毒的烈度
//#endif
} POISONSTATUS, *LPPOISONSTATUS;

typedef struct tagOBJECTDESC
{
	LPSTR p[MAX_OBJECTS];
} OBJECTDESC, *LPOBJECTDESC;


typedef struct tagGLOBALVARS
{
	FILES            f{};
	GAMEDATA         g{};

	int              iCurMainMenuItem{};    // current main menu item number
	int              iCurSystemMenuItem{};  // current system menu item number
	int              iCurInvMenuItem{};     // current inventory menu item number
	int              iCurPlayingRNG{};      // current playing RNG animation
	BYTE             bCurrentSaveSlot{};    // current save slot (1-5)
	BOOL             fInMainGame{};         // TRUE if in main game
	BOOL             fEnteringScene{};      // TRUE if entering a new scene
	BOOL             fNeedToFadeIn{};       // TRUE if need to fade in when drawing scene
	BOOL             fInBattle{};           // TRUE if in battle
	BOOL             fAutoBattle{};         // TRUE if auto-battle
#ifndef PAL_CLASSIC
	BYTE             bBattleSpeed{};        // Battle Speed (1 = Fastest, 5 = Slowest)
#endif
	WORD             wLastUnequippedItem{}; // last unequipped item

	PLAYERROLES      rgEquipmentEffect[MAX_PLAYER_EQUIPMENTS + 1]{}; // equipment effects
	WORD             rgPlayerStatus[MAX_PLAYER_ROLES][kStatusAll]{}; // player status

	PAL_POS          viewport{};            // viewport coordination
	PAL_POS          partyoffset{};
	WORD             wLayer{};
	WORD             wMaxPartyMemberIndex{};// max index of members in party (0 to MAX_PLAYERS_IN_PARTY - 1)
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES]{}; // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES]{}; // player trail
	WORD             wPartyDirection{};     // direction of the party
	WORD             wNumScene{};           // current scene number
	WORD             wNumPalette{};         // current palette number
	BOOL             fNightPalette{};       // TRUE if use the darker night palette
	WORD             wNumMusic{};           // current music number
	WORD             wNumBattleMusic{};     // current music number in battle
	WORD             wNumBattleField{};     // current battle field number
	WORD             wCollectValue{};       // value of "collected" items
	WORD             wScreenWave{};         // level of screen waving
	SHORT            sWaveProgression{};
	WORD             wChaseRange{};
	WORD             wChasespeedChangeCycles{};
	USHORT           nFollower{};

	DWORD            dwCash{};              // amount of cash

	ALLEXPERIENCE    Exp{};                 // experience status
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]{}; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY]{};  // inventory status
	LPOBJECTDESC     lpObjectDesc{ 0 };
	DWORD            dwFrameNum{ 0 };

	/////////
	BOOL fGameStart;
	//LPOBJECTNEWMSG	lpObjectNewMsg;
#ifdef FINISH_GAME_MORE_ONE_TIME
	BYTE				bFinishGameTime;
#endif

//#ifdef SHOW_DATA_IN_BATTLE
	BOOL				fShowDataInBattle;
//#endif

	BOOL			 bIsBig5;
	WORD			 bSavedTimes;
	//BOOL			 isPalWIN;
} GLOBALVARS, * LPGLOBALVARS;


void TerminateOnError(const char *fmt,
    ...
);
class Cls_Iconv
{
	static char OutBuf[1024];
public:
	LPSTR GBKtoUTF8(LPCSTR buf, INT Iconv = CP_ACP);
	LPSTR UTF8toGBK(LPCSTR buf, INT Iconv = CP_ACP);
	LPSTR Iconv(LPCSTR str, INT InIconv, INT OutIconv);
	LPSTR Big5ToGb2312(LPCSTR str);
	LPSTR Gb2312ToBig5(LPCSTR str);
};

typedef enum tagDIALOGPOSITION
{
	kDialogUpper = 0,
	kDialogCenter,
	kDialogLower,
	kDialogCenterWindow
} DIALOGLOCATION;

#endif