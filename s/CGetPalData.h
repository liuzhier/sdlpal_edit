#pragma once
#include "../main/cscript.h"

typedef struct tagMark//用于标记对象入口结构
{
	WORD n;//新入口
	WORD o;//旧入口
}rMark, * lpMark;

using  tagPString = vector <string>;
using  MarkScript = vector <WORD>;
using  MarkMsg = vector <rMark>;
//using  ListMsg = vector <DWORD>;
using  MAPScriptLP = map<WORD*, WORD*>;
using  MAPScript = map<WORD, WORD>;

typedef enum tagObject_class
{
	kIsPlayer = (1 << 0),
	kIsItem = (1 << 1),
	kIsMagic = (1 << 2),
	kIsEnemy = (1 << 3),
	kIsPoison = (1 << 4)
} Object_Class;



/// <summary>
/// PMARK 构成二维数组，用于记录脚本调用来源，INT32由三部分组成，高8位=1来自对象。 
/// =2 来自场景scene。 =3 来自事件对象 EventObject。 =4 来自脚本 。=0 出错
/// 第九到十六位对象的位置
/// 后十六位脚本原行号，前十六位固定的，后十六位可能会随脚本插入变动
/// 第一维长度= 脚本记录长度
/// </summary>
struct	sgMark { vector<DWORD32> s; };
typedef vector<sgMark> PMARK;
typedef vector<BYTE> PDATABUF;

//发送和显示信息
VOID ShowMsg(const char* format, ...);

using TTestRunData = vector<SHORT>;
using JUMPS = int[3];

class CGetPalData : public CScript
{
public:
	CGetPalData();
	~CGetPalData();
	CGetPalData(int save, BOOL noRun = 0);

	VOID PAL_Make_MKF();
	LPSCENE getSecensPoint(const LPVOID p);

	//pack.cpp
	//VOID PAL_PackObject(VOID);
	//INT FreadBuf(const LPSTR buf, FILE* f);
	//备份打包对象
	//INT PAL_BakObjectSpace(VOID);
	//VOID PAL_DeleteBakObject(VOID);
	//VOID PAL_ChangeSaveFile(LPCSTR FileName);
	//INT Pal_Make_NewMsg(VOID);
	//VOID Pal_Print_NoUseMsg(VOID);
	//


	VOID PAL_ReloadPAL(BOOL isDelete);
	VOID PAL_UnPackObject(VOID);
	//将存档文件载入至内存变量中
	INT loadSaveFile();
	//存储存档文件
	INT saveSaveFile();

public:
	INT  gpObject_classify[MAX_OBJECTS]{ 0 };
	INT nEnemy{};
	INT nItem{};
	INT nPoisonID{};
	INT nMagic{};

	//存档文件备份空间
	PDATABUF pSaveData[10];

	PMARK pMark;

public:



	VOID PAL_MarkScriptEntry(WORD Entry, LPBYTE AllMark);
	VOID PAL_MarkScriptEntryAll(WORD Entry, DWORD32 AllMark,int save = 0);

	INT MarkSprictJumpsAddress(WORD sprictentry, MAPScript & jumps);

	INT SingleScriptChange(int entry,int _old, int _new);


	VOID PAL_Object_classify(VOID);
	//将UTF8 转到 系统汉字
	VOID Utf8ToSys(string& s);

	INT PAL_MarkScriptAll(INT isIncludeSave = FALSE);

	// 替换mkf文件中的一个片段，
	int replaceMKFOne(LPCSTR fileName, int nNum, LPCVOID buf, int BufLen);

	INT EncodeRLE(const void* Source, const UINT8 TransparentColor, INT32 Stride, INT32 Width, INT32 Height, void*& Destination, INT32& Length);

};

class RunInThread
{
public:
	RunInThread(TestData* t, CGetPalData* Pal);
};