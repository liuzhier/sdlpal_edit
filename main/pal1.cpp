

// PalApp.cpp: 定义应用程序的类行为。
//

#include "cpalapp.h"
#include <Windows.h>
#include <WinNls.h>
//#include "util.h"
#include <assert.h>
#include "CConfig.h"
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK , __FILE__ , __LINE__)
//#define new DEBUG_NEW
#endif


// CPalApp

char Cls_Iconv::OutBuf[1024];

LPSTR Cls_Iconv::GBKtoUTF8(LPCSTR szBuf,INT conv)
{
    return Iconv(szBuf, conv, CP_UTF8);
}

LPSTR Cls_Iconv::UTF8toGBK(LPCSTR szBuf,INT conv)
{
    return Iconv(szBuf, CP_UTF8, conv);
}

LPSTR Cls_Iconv::Iconv(LPCSTR szBuf, INT InIconv, INT OutIconv)
{
    //在不同字符集中转化
    //输入buf 被转换字符串，输入字符集，输出字符集
    //输出完成字符串
    //QTextCodec * code = QTextCodec::codecForName("UTF-8");

    int len = MultiByteToWideChar(InIconv, 0, szBuf, -1, NULL, 0);
    LPWSTR p = new WCHAR[len + 1];
    memset(p, 0, sizeof(WCHAR)*(len + 1));
    MultiByteToWideChar(InIconv, 0, szBuf, -1, p, len);
    len = WideCharToMultiByte(OutIconv, 0, p, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(OutIconv, 0, p, -1, OutBuf, len, NULL, NULL);
    OutBuf[len = 0];
    delete[] p;
    return LPSTR(OutBuf);
}

LPSTR Cls_Iconv::Big5ToGb2312(LPCSTR str)
{
    //QTextCodec * code = QTextCodec::codecForName("UTF-8");
    //将繁体转化为简体
    //先将big5 转化为gbk字符集
    Iconv(str, 950, 936);
    //再将GBK转化为简体
    size_t nStrLen = strlen(OutBuf);
    WORD wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_CHINESE_BIG5);
    int nReturn = LCMapStringA(wLCID, LCMAP_SIMPLIFIED_CHINESE, OutBuf, nStrLen, NULL, 0);
    LPSTR pcBuf = new CHAR[(size_t)nReturn + 10];
    {
        wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_CHINESE_BIG5);
        LCMapStringA(wLCID, LCMAP_SIMPLIFIED_CHINESE, OutBuf, nReturn, pcBuf, nReturn + 1);
        strncpy(OutBuf, pcBuf, nReturn);
    }
    delete[](pcBuf);

    return OutBuf;
}

LPSTR Cls_Iconv::Gb2312ToBig5(LPCSTR str)
{
    //将简体转化为繁体
    //将简体转化为gbk字符集
    //Iconv(str, 936, 950);
    strcpy(OutBuf, str);
    //将GBK转化为繁体
    size_t nStrLen = strlen(OutBuf);
    DWORD wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_CHINESE_PRC);
    size_t nReturn = LCMapStringA(wLCID, LCMAP_TRADITIONAL_CHINESE, OutBuf, nStrLen, NULL, 0);
    char *pcBuf = new char[(size_t)nReturn + 10];
    {
        wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_CHINESE_PRC);
        LCMapStringA(wLCID, LCMAP_TRADITIONAL_CHINESE, OutBuf, nReturn, pcBuf, nReturn + 1);
        strncpy(OutBuf, pcBuf, nReturn);
    }
    delete[] pcBuf;

    return Iconv(OutBuf, 936, 950);
}
//---------------------------------------------------------------------------

void CPalApp::trim( char *str )
/*++
  Purpose:

    Remove the leading and trailing spaces in a string.

  Parameters:

    str - the string to proceed.

  Return value:

    None.

--*/
{
    int pos = 0;
    char *dest = str;

    //
    // skip leading blanks
    //
    while (str[pos] <= ' ' && str[pos] > 0)
        pos++;

    while (str[pos])
    {
        *(dest++) = str[pos];
        pos++;
    }
    *(dest--) = '\0'; // store the null
    // remove trailing blanks
    while (dest >= str && *dest <= ' ' && *dest > 0)
        *(dest--) = '\0';
}

//初始化数据打开文件
INT CPalApp::PAL_InitGlobals(VOID)
/*++
  Purpose:    Initialize global data.
  Parameters:    None.
  Return value:    0 = success, -1 = error.
  --*/
{

    //
    // Open files
    //
    gpGlobals->f.fpFBP = UTIL_OpenRequiredFile("fbp.mkf");
    gpGlobals->f.fpMGO = UTIL_OpenRequiredFile("mgo.mkf");
    gpGlobals->f.fpBALL = UTIL_OpenRequiredFile("ball.mkf");
    gpGlobals->f.fpDATA = UTIL_OpenRequiredFile("data.mkf");
    gpGlobals->f.fpF = UTIL_OpenRequiredFile("f.mkf");
    gpGlobals->f.fpFIRE = UTIL_OpenRequiredFile("fire.mkf");
    gpGlobals->f.fpRGM = UTIL_OpenRequiredFile("rgm.mkf");
    gpGlobals->f.fpSSS = UTIL_OpenRequiredFile("sss.mkf");
    //gpGlobals->lpObjectDesc = PAL_LoadObjectDesc("desc.dat");
    gpGlobals->bCurrentSaveSlot = 1;

    return 0;
}


FILE * CPalApp::UTIL_OpenRequiredFile(LPCSTR  lpszFileName)
{

    FILE         *fp;
    std::string csf = PalDir + lpszFileName;
	
    if (fopen_s(&fp, csf.c_str(), "rb"))
    {
        //exit(0);
    }
    return fp;
}


#define SWAP16(X) (X)
//#define SWAP32(x)  (x)



#define LOAD_DATA(buf, size, chunknum, fp)                       \
{                                                             \
    PAL_MKFReadChunk((LPBYTE)(buf), (size), (chunknum), (fp)); \
    DO_BYTESWAP(buf, size);                                    \
}

void
TerminateOnError(
    const char *fmt,
    ...
)
// This function terminates the game because of an error and
// prints the message string pointed to by fmt both in the
// console and in a messagebox.
{
    va_list argptr;
    char string[256];
    //extern VOID PAL_Shutdown(VOID);

    // concatenate all the arguments in one string
    va_start(argptr, fmt);
    vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);


    //fprintf(stderr, "\nFATAL ERROR: %s\n", string);

#ifdef _DEBUG
    //assert(!"TerminateOnError()"); // allows jumping to debugger
#endif
    exit(14);

    //PAL_Shutdown();
}



VOID CPalApp::PAL_ReadGlobalGameData(VOID)
/*++
  Purpose:    Read global game data from data files.
  Parameters:    None.
  Return value:    None.
  --*/
{
    const GAMEDATA    *p = &gpGlobals->g;
    //unsigned int       i;

    LOAD_DATA(p->lprgScriptEntry, p->nScriptEntry * sizeof(SCRIPTENTRY),
        4, gpGlobals->f.fpSSS);

    LOAD_DATA(p->lprgStore, p->nStore * sizeof(STORE), 0, gpGlobals->f.fpDATA);
    LOAD_DATA(p->lprgEnemy, p->nEnemy * sizeof(ENEMY), 1, gpGlobals->f.fpDATA);
    LOAD_DATA(p->lprgEnemyTeam, p->nEnemyTeam * sizeof(ENEMYTEAM),
        2, gpGlobals->f.fpDATA);
    LOAD_DATA(p->lprgMagic, p->nMagic * sizeof(MAGIC), 4, gpGlobals->f.fpDATA);
    LOAD_DATA(p->lprgBattleField, p->nBattleField * sizeof(BATTLEFIELD),
        5, gpGlobals->f.fpDATA);
    LOAD_DATA(p->lprgLevelUpMagic, p->nLevelUpMagic * sizeof(LEVELUPMAGIC_ALL),
        6, gpGlobals->f.fpDATA);
    LOAD_DATA(p->rgwBattleEffectIndex, sizeof(p->rgwBattleEffectIndex),
        11, gpGlobals->f.fpDATA);
    PAL_MKFReadChunk((LPBYTE)&(p->EnemyPos), sizeof(p->EnemyPos),
        13, gpGlobals->f.fpDATA);
    DO_BYTESWAP(&(p->EnemyPos), sizeof(p->EnemyPos));
    PAL_MKFReadChunk((LPBYTE)(p->rgLevelUpExp), sizeof(p->rgLevelUpExp),
        14, gpGlobals->f.fpDATA);
    DO_BYTESWAP(p->rgLevelUpExp, sizeof(p->rgLevelUpExp));
}

VOID CPalApp::PAL_InitGlobalGameData(	VOID)
/*++
  Purpose:
  Initialize global game data.
  Parameters:
  None.
  Return value:
  None.
  --*/
{
    int        len;

#define PAL_DOALLOCATE(fp, num, type, lptype, ptr, n)                            \
    {                                                                             \
    len = PAL_MKFGetChunkSize(num, fp);                                        \
    ptr = (lptype)malloc(len);                                                 \
    n = len / sizeof(type);                                                    \
    if (ptr == NULL)                                                           \
    {                                                                          \
    TerminateOnError("PAL_InitGlobalGameData(): Memory allocation error!"); \
    }                                                                          \
    }

    //
    // If the memory has not been allocated, allocate first.
    //
    if (gpGlobals->g.lprgEventObject == NULL)
    {
        PAL_DOALLOCATE(gpGlobals->f.fpSSS, 0, EVENTOBJECT, LPEVENTOBJECT,
            gpGlobals->g.lprgEventObject, gpGlobals->g.nEventObject);

        PAL_DOALLOCATE(gpGlobals->f.fpSSS, 4, SCRIPTENTRY, LPSCRIPTENTRY,
            gpGlobals->g.lprgScriptEntry, gpGlobals->g.nScriptEntry);


        PAL_DOALLOCATE(gpGlobals->f.fpDATA, 0, STORE, LPSTORE,
            gpGlobals->g.lprgStore, gpGlobals->g.nStore);

        PAL_DOALLOCATE(gpGlobals->f.fpDATA, 1, ENEMY, LPENEMY,
            gpGlobals->g.lprgEnemy, gpGlobals->g.nEnemy);

        PAL_DOALLOCATE(gpGlobals->f.fpDATA, 2, ENEMYTEAM, LPENEMYTEAM,
            gpGlobals->g.lprgEnemyTeam, gpGlobals->g.nEnemyTeam);

        PAL_DOALLOCATE(gpGlobals->f.fpDATA, 4, MAGIC, LPMAGIC,
            gpGlobals->g.lprgMagic, gpGlobals->g.nMagic);

        PAL_DOALLOCATE(gpGlobals->f.fpDATA, 5, BATTLEFIELD, LPBATTLEFIELD,
            gpGlobals->g.lprgBattleField, gpGlobals->g.nBattleField);

        PAL_DOALLOCATE(gpGlobals->f.fpDATA, 6, LEVELUPMAGIC_ALL, LPLEVELUPMAGIC_ALL,
            gpGlobals->g.lprgLevelUpMagic, gpGlobals->g.nLevelUpMagic);

        PAL_ReadGlobalGameData();
    }
#undef PAL_DOALLOCATE
}


INT CPalApp::PAL_MKFGetChunkCount(FILE *fp)
/*++
  Purpose:
  Get the number of chunks in an MKF archive.
  Parameters:
  [IN]  fp - pointer to an fopen'ed MKF file.
  Return value:
  Integer value which indicates the number of chunks in the specified MKF file.
  --*/
{
    INT iNumChunk;
    if (fp == NULL)
    {
        return 0;
    }

    fseek(fp, 0, SEEK_SET);
    fread(&iNumChunk, sizeof(INT), 1, fp);

    iNumChunk = (SWAP32(iNumChunk) - 4) / 4;
    return iNumChunk;
}

INT CPalApp::PAL_MKFReadChunk(
    LPBYTE          lpBuffer,
    UINT            uiBufferSize,
    UINT            uiChunkNum,
    FILE           *fp
)
/*++
  Purpose:

  Read a chunk from an MKF archive into lpBuffer.

  Parameters:

  [OUT] lpBuffer - pointer to the destination buffer.

  [IN]  uiBufferSize - size of the destination buffer.

  [IN]  uiChunkNum - the number of the chunk in the MKF archive to read.

  [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

  Integer value which indicates the size of the chunk.
  -1 if there are error in parameters.
  -2 if buffer size is not enough.

  --*/
{
    UINT     uiOffset = 0;
    UINT     uiNextOffset = 0;
    UINT     uiChunkCount = 0;
    UINT     uiChunkLen = 0;

    if (lpBuffer == NULL || fp == NULL || uiBufferSize == 0)
    {
        return -1;
    }

    //
    // Get the total number of chunks.
    //
    uiChunkCount = PAL_MKFGetChunkCount(fp);
    if (uiChunkNum >= uiChunkCount)
    {
        return -1;
    }

    //
    // Get the offset of the chunk.
    //
    fseek(fp, 4 * uiChunkNum, SEEK_SET);
    fread(&uiOffset, 4, 1, fp);
    fread(&uiNextOffset, 4, 1, fp);
    uiOffset = SWAP32(uiOffset);
    uiNextOffset = SWAP32(uiNextOffset);

    //
    // Get the length of the chunk.
    //
    uiChunkLen = uiNextOffset - uiOffset;

    if (uiChunkLen > uiBufferSize)
    {
        return -2;
    }

    if (uiChunkLen != 0)
    {
        fseek(fp, uiOffset, SEEK_SET);
        size_t nsize = fread(lpBuffer, uiChunkLen,1,  fp);
        assert(nsize == 1);
    }
    else
    {
        return -1;
    }

    return (INT)uiChunkLen;
}

INT CPalApp::PAL_MKFGetDecompressedSize(
    UINT    uiChunkNum,
    FILE   *fp
)
/*++
  Purpose:

  Get the decompressed size of a compressed chunk in an MKF archive.

  Parameters:

  [IN]  uiChunkNum - the number of the chunk in the MKF archive.

  [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

  Integer value which indicates the size of the chunk.
  -1 if the chunk does not exist.

  --*/
{
    DWORD         buf[2];
    UINT          uiOffset;
    UINT          uiChunkCount;

    if (fp == NULL)
    {
        return -1;
    }

    //
    // Get the total number of chunks.
    //
    uiChunkCount = PAL_MKFGetChunkCount(fp);
    if (uiChunkNum >= uiChunkCount)
    {
        return -1;
    }

    //
    // Get the offset of the chunk.
    //
    fseek(fp, 4 * uiChunkNum, SEEK_SET);
    fread(&uiOffset, 4, 1, fp);
    uiOffset = SWAP32(uiOffset);

    //
    // Read the header.
    //
    fseek(fp, uiOffset, SEEK_SET);

//#ifdef PAL_WIN95
    if (!gConfig->fisUSEYJ1DeCompress)
    {
        fread(buf, sizeof(DWORD), 1, fp);
        buf[0] = SWAP32(buf[0]);

        return (INT)buf[0];
    }
//#else
    else
    {
        fread(buf, sizeof(DWORD), 2, fp);
        buf[0] = SWAP32(buf[0]);
        buf[1] = SWAP32(buf[1]);
        //检测标识
        return (buf[0] != 0x315f4a59) ? -1 : (INT)buf[1];
    }
//#endif
}





INT CPalApp::PAL_MKFDecompressChunk(
    LPBYTE          lpBuffer,
    UINT            uiBufferSize,
    UINT            uiChunkNum,
    FILE           *fp
)
/*++
  Purpose:

  Decompress a compressed chunk from an MKF archive into lpBuffer.

  Parameters:

  [OUT] lpBuffer - pointer to the destination buffer.

  [IN]  uiBufferSize - size of the destination buffer.

  [IN]  uiChunkNum - the number of the chunk in the MKF archive to read.

  [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

  Integer value which indicates the size of the chunk.
  -1 if there are error in parameters, or buffer size is not enough.
  -3 if cannot allocate memory for decompression.

  --*/
{
    LPBYTE          buf;
    int             len;

    len = PAL_MKFGetChunkSize(uiChunkNum, fp);

    if (len <= 0)
    {
        return len;
    }

    buf = (LPBYTE)malloc(len);
    if (buf == NULL)
    {
        return -3;
    }

    PAL_MKFReadChunk(buf, len, uiChunkNum, fp);

    len = (PAL_DeCompress)(buf, lpBuffer, uiBufferSize);
    free(buf);

    return len;
}

VOID  CPalApp::PAL_LoadDefaultGame( VOID )
/*++
  Purpose:
  Load the default game data.
  Parameters:
  None.
  Return value:
  None.
  --*/
{
    GAMEDATA    *p = &gpGlobals->g;
    UINT32             i;

    //
    // Load the default data from the game data files.
    //
    LOAD_DATA(p->lprgEventObject, p->nEventObject * sizeof(EVENTOBJECT),
        0, gpGlobals->f.fpSSS);
    PAL_MKFReadChunk((LPBYTE)(p->rgScene), sizeof(p->rgScene), 1, gpGlobals->f.fpSSS);
    DO_BYTESWAP(p->rgScene, sizeof(p->rgScene));
    p->nScene = 300;
    if (gConfig->fIsWIN95)
    {
        PAL_MKFReadChunk((LPBYTE)(p->rgObject), sizeof(OBJECT) * MAX_OBJECTS, 2, gpGlobals->f.fpSSS);
        DO_BYTESWAP(p->rgObject, sizeof(OBJECT) * MAX_OBJECTS);
    }
    else
    {
        OBJECT_DOS objects[MAX_OBJECTS]{0};
        PAL_MKFReadChunk((LPBYTE)(objects), sizeof(objects), 2, gpGlobals->f.fpSSS);
        DO_BYTESWAP(objects, sizeof(objects));
        //
        // Convert the DOS-style data structure to WIN-style data structure
        //
        for (i = 0; i < MAX_OBJECTS; i++)
        {
            memcpy(&((LPOBJECT)p->rgObject)[i], &objects[i], sizeof(OBJECT_DOS));
            ((LPOBJECT)p->rgObject)[i].rgwData[6] = objects[i].rgwData[5];     // wFlags
            ((LPOBJECT)p->rgObject)[i].rgwData[5] = 0;                         // wScriptDesc or wReserved2
        }
    }

    PAL_MKFReadChunk((LPBYTE)(&(p->PlayerRoles)), sizeof(PLAYERROLES),
        3, gpGlobals->f.fpDATA);
    DO_BYTESWAP(&(p->PlayerRoles), sizeof(PLAYERROLES));

    //
    // Set some other default data.
    //
    gpGlobals->dwCash = 0;
    gpGlobals->wNumMusic = 0;
    gpGlobals->wNumPalette = 0;
    gpGlobals->wNumScene = 1;
    gpGlobals->wCollectValue = 0;
    gpGlobals->fNightPalette = FALSE;
    gpGlobals->wMaxPartyMemberIndex = 0;
    gpGlobals->viewport = PAL_XY(0, 0);
    gpGlobals->wLayer = 0;
    gpGlobals->wChaseRange = 1;

#ifdef FINISH_GAME_MORE_ONE_TIME
    gpGlobals->bFinishGameTime = 0;
#endif
    memset(gpGlobals->rgInventory, 0, sizeof(gpGlobals->rgInventory));
    memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
    memset(gpGlobals->rgParty, 0, sizeof(gpGlobals->rgParty));
    memset(gpGlobals->rgTrail, 0, sizeof(gpGlobals->rgTrail));
    memset(&(gpGlobals->Exp), 0, sizeof(gpGlobals->Exp));

    for (i = 0; i < MAX_PLAYER_ROLES; i++)
    {
        gpGlobals->Exp.rgPrimaryExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
        gpGlobals->Exp.rgHealthExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
        gpGlobals->Exp.rgMagicExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
        gpGlobals->Exp.rgAttackExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
        gpGlobals->Exp.rgMagicPowerExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
        gpGlobals->Exp.rgDefenseExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
        gpGlobals->Exp.rgDexterityExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
        gpGlobals->Exp.rgFleeExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
    }

    gpGlobals->fEnteringScene = TRUE;
}


VOID CPalApp::PAL_CompressInventory(
    VOID
)
/*++
  Purpose:    Remove all the items in inventory which has a number of zero.
  Parameters:    None.
  Return value:    None.
  --*/
{
    int i, j;

    j = 0;

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (gpGlobals->rgInventory[i].wItem == 0)
        {
            break;
        }

        if (gpGlobals->rgInventory[i].nAmount > 0)
        {
            gpGlobals->rgInventory[j] = gpGlobals->rgInventory[i];
            j++;
        }
    }

    for (; j < MAX_INVENTORY; j++)
    {
        gpGlobals->rgInventory[j].nAmount = 0;
        gpGlobals->rgInventory[j].nAmountInUse = 0;
        gpGlobals->rgInventory[j].wItem = 0;
    }
    PAL_New_SortInventory();
}

VOID CPalApp::PAL_New_SortInventory(
)
{
    int         i, j;
    WORD        ItemID1, ItemID2;
    WORD		ItemNum;
    INVENTORY   TempItem;
    INVENTORY	TempInventory[MAX_INVENTORY];

    memset(TempInventory, 0, sizeof(TempInventory));

    for (i = 0, j = 0; i < MAX_INVENTORY; i++)
    {
        TempItem = gpGlobals->rgInventory[i];
        if (TempItem.wItem != 0 && TempItem.nAmount != 0)
        {
            TempInventory[j] = TempItem;
            j++;
        }
    }
    ItemNum = j;

    for (i = 0; i < ItemNum; i++)
    {
        for (j = 0; j < ItemNum - i - 1; j++)
        {
            ItemID1 = TempInventory[j].wItem;
            ItemID2 = TempInventory[j + 1].wItem;


            if (ItemID1 > ItemID2)
            {
                TempItem = TempInventory[j];
                TempInventory[j] = TempInventory[j + 1];
                TempInventory[j + 1] = TempItem;
            }
        }
    }

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        gpGlobals->rgInventory[i] = TempInventory[i];
    }

    return;
}

INT
CPalApp::PAL_MKFSaveChunk(
    LPBYTE          lpBuffer,
    UINT            uiBufferSize,
    UINT            uiChunkNum,
    FILE           *fp
)
{
    UINT     uiOffset = 0;
    UINT     uiNextOffset = 0;
    UINT     uiChunkCount;
    UINT     uiChunkLen;

    if (lpBuffer == NULL || fp == NULL || uiBufferSize == 0)
    {
        return -1;
    }

    //
    // Get the total number of chunks.
    //
    uiChunkCount = PAL_MKFGetChunkCount(fp);
    if (uiChunkNum >= uiChunkCount)
    {
        return -1;
    }

    //
    // Get the offset of the chunk.
    //
    fseek(fp, 4 * uiChunkNum, SEEK_SET);
    fread(&uiOffset, 4, 1, fp);
    fread(&uiNextOffset, 4, 1, fp);
    uiOffset = SWAP32(uiOffset);
    uiNextOffset = SWAP32(uiNextOffset);

    //
    // Get the length of the chunk.
    //
    uiChunkLen = uiNextOffset - uiOffset;

    if (uiChunkLen > uiBufferSize)
    {
        return -2;
    }

    if (uiChunkLen != 0)
    {
        fseek(fp, uiOffset, SEEK_SET);
        fwrite(lpBuffer, uiChunkLen, 1, fp);
    }
    else
    {
        return -1;
    }
    return (INT)uiChunkLen;
}

INT CPalApp::PAL_MKFGetChunkSize(UINT uiChunkNum, FILE *fp)
/*++
  Purpose:

  Get the size of a chunk in an MKF archive.

  Parameters:

  [IN]  uiChunkNum - the number of the chunk in the MKF archive.

  [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

  Integer value which indicates the size of the chunk.
  -1 if the chunk does not exist.

  --*/
{
    UINT    uiOffset = 0;
    UINT    uiNextOffset = 0;
    UINT    uiChunkCount = 0;
    //
    // Get the total number of chunks.
    //
    uiChunkCount = PAL_MKFGetChunkCount(fp);
    if (uiChunkNum >= uiChunkCount)
    {
        return -1;
    }
    //
    // Get the offset of the specified chunk and the next chunk.
    //
    fseek(fp, 4 * uiChunkNum, SEEK_SET);
    fread(&uiOffset, sizeof(UINT), 1, fp);
    fread(&uiNextOffset, sizeof(UINT), 1, fp);
    //uiOffset = SWAP32(uiOffset);
    //uiNextOffset = SWAP32(uiNextOffset);
    // Return the length of the chunk.
    return uiNextOffset - uiOffset;
}

VOID CPalApp::UTIL_CloseFile(	FILE *fp )
/*++
  Purpose:

    Close a file.

  Parameters:

    [IN]  fp - file handle to be closed.

  Return value:

    None.

--*/
{
    if (fp != NULL)
    {
        fclose(fp);
    }
}


VOID CPalApp::PAL_FreeGlobals(VOID)
/*++
  Purpose:    Free global data.
  Parameters:    None.
  Return value:    None.
  --*/
{
    if (gpGlobals != NULL)
    {
        //
        // Close all opened files
        //
        UTIL_CloseFile(gpGlobals->f.fpFBP);
        UTIL_CloseFile(gpGlobals->f.fpMGO);
        UTIL_CloseFile(gpGlobals->f.fpBALL);
        UTIL_CloseFile(gpGlobals->f.fpDATA);
        UTIL_CloseFile(gpGlobals->f.fpF);
        UTIL_CloseFile(gpGlobals->f.fpFIRE);
        UTIL_CloseFile(gpGlobals->f.fpRGM);
        UTIL_CloseFile(gpGlobals->f.fpSSS);

        //
        // Free the game data
        //
        free(gpGlobals->g.lprgEventObject);
        free(gpGlobals->g.lprgScriptEntry);
        free(gpGlobals->g.lprgStore);
        free(gpGlobals->g.lprgEnemy);
        free(gpGlobals->g.lprgEnemyTeam);
        free(gpGlobals->g.lprgMagic);
        free(gpGlobals->g.lprgBattleField);
        free(gpGlobals->g.lprgLevelUpMagic);
        //
        free(gpGlobals->g.rgObject);
        //
        // Free the object description data
        //
        PAL_FreeObjectDesc(gpGlobals->lpObjectDesc);
        gpGlobals->lpObjectDesc = NULL;
        free(gpGlobals);
    }

    gpGlobals = NULL;
}

LPOBJECTDESC CPalApp::PAL_LoadObjectDesc( LPCSTR lpszFileName)
/*++
  Purpose:

    Load the object description strings from file.

  Parameters:

    [IN]  lpszFileName - the filename to be loaded.

  Return value:

    Pointer to loaded data, in linked list form. NULL if unable to load.

--*/
{
    FILE                      *fp;
    PAL_LARGE char             buf[512];
    LPSTR                      p;
    LPOBJECTDESC               lpDesc = NULL;
    unsigned int               i;
    
    fp = UTIL_OpenRequiredFile(lpszFileName);
    if (fp == NULL)
    {
        return NULL;
    }
    //
    // Load the description data

    lpDesc = (LPOBJECTDESC)UTIL_calloc(1, sizeof(OBJECTDESC));

    while (fgets(buf, 512, fp) != NULL)
    {
        p = strchr(buf, '=');
        if (p == NULL)
        {
            continue;
        }
        *p = '\0';
        p++;
        sscanf_s(buf, "%x", &i);
        char s[512];
        strcpy_s(s, 510,PAL_TextToUTF8(p));
        LPSTR p1 = new CHAR[strlen(s) + 2];
        strcpy_s(p1, strlen(s) + 2, s);
        lpDesc->p[i]= p1;
    }

    fclose(fp);
    return lpDesc;
}

VOID CPalApp::PAL_FreeText( )
/*++
  Purpose:

  Free the memory used by the texts.

  Parameters:

  None.

  Return value:

  None.

  --*/
{
    if (g_TextLib.lpMsgBuf != NULL)
    {
        free(g_TextLib.lpMsgBuf);
        g_TextLib.lpMsgBuf = NULL;
    }

    if (g_TextLib.lpMsgOffset != NULL)
    {
        free(g_TextLib.lpMsgOffset);
        g_TextLib.lpMsgOffset = NULL;
    }

    if (g_TextLib.lpWordBuf != NULL)
    {
        free(g_TextLib.lpWordBuf);
        g_TextLib.lpWordBuf = NULL;
    }
}


VOID CPalApp::PAL_FreeObjectDesc( LPOBJECTDESC   lpObjectDesc )
/*++
  Purpose:

    Free the object description data.

  Parameters:

    [IN]  lpObjectDesc - the description data to be freed.

  Return value:

    None.

--*/
{
    if (lpObjectDesc == NULL)
        return;
    int i;
    for (i = 0; i < MAX_OBJECTS; i++)
    {
        if (lpObjectDesc->p[i] != NULL)
            free(lpObjectDesc->p[i]);
        lpObjectDesc->p[i] = NULL;
    }
    free(lpObjectDesc);
}

LPCSTR CPalApp::PAL_GetObjectDesc(LPOBJECTDESC   lpObjectDesc,	WORD  wObjectID)
/*++
  Purpose:

    Get the object description string from the linked list.

  Parameters:

    [IN]  lpObjectDesc - the description data linked list.

    [IN]  wObjectID - the object ID.

  Return value:

    The description string. NULL if the specified object ID
    is not found.

--*/
{
    if (lpObjectDesc == NULL)
        return NULL;
    return lpObjectDesc->p[wObjectID];
}


LPCSTR CPalApp::PAL_GetWord(WORD wNumWord )
/*++
  Purpose:

    Get the specified word.

  Parameters:

    [IN]  wNumWord - the number of the requested word.

  Return value: for UTF8

    Pointer to the requested word. NULL if not found.

--*/
{
    static char buf[WORD_LENGTH + 10];
    static LPCSTR  cw_NewWord[] =
    {
    "信息",
    "情报",
    "灵抗",
    "毒抗",
    "可偷",
    "巫抗",
    "二次攻击",
    "附加攻击",
    "魔法",
    "物抗",
    "灵壶值",//20011
    "进度一",
    "进度二",
    "进度三",
    "进度四",
    "进度五",
    "进度六",
    "进度七",
    "进度八",
    "进度九",
    "进度十",
    };

    if (wNumWord >= PAL_ADDITIONAL_WORD_SECOND && wNumWord <
        PAL_ADDITIONAL_WORD_SECOND + sizeof(cw_NewWord)/sizeof(LPSTR))
    {
        //return cw_NewWord[wNumWord - PAL_ADDITIONAL_WORD_SECOND];
        //由于新加字是简体，需要转换
        if (gpGlobals->bIsBig5)
        {
            Cls_Iconv m;
            LPCSTR p = cw_NewWord[wNumWord - PAL_ADDITIONAL_WORD_SECOND];//UTF8
            LPSTR p1 = m.UTF8toGBK(p, 936);
            p1 = m.Gb2312ToBig5(p1);
            return PAL_TextToUTF8(p1);//返回繁体UTF8
        }
        else
            return cw_NewWord[wNumWord - PAL_ADDITIONAL_WORD_SECOND];//直接返回UTF8简体

    }
    else if (wNumWord >= g_TextLib.nWords)
    {
        return NULL;
    }

    SDL_zero(buf);
    memcpy(buf, &g_TextLib.lpWordBuf[wNumWord * WORD_LENGTH], WORD_LENGTH);
    buf[WORD_LENGTH] = '\0';

    //
    // Remove the trailing spaces
    //
    trim(buf);

    if ((strlen(buf) & 1) != 0 && buf[strlen(buf) - 1] == '1')
    {
        //buf[strlen(buf) - 1] = '\0';
    }
    return PAL_TextToUTF8( buf);
}

INT CPalApp::PAL_InitText( 	VOID )
/*++
  Purpose:

  Initialize the in-game texts.

  Parameters:

  None.

  Return value:

  0 = success.
  -1 = memory allocation error.

  --*/
{
    FILE       *fpMsg, *fpWord;
    int         i;

    //
    // Open the message and word data files.
    //
    fpMsg = UTIL_OpenRequiredFile("m.msg");
    fpWord = UTIL_OpenRequiredFile("word.dat");

    //
    // See how many words we have
    //
    fseek(fpWord, 0, SEEK_END);
    i = ftell(fpWord);
    //
    // Each word has 10 bytes
    //
    g_TextLib.nWords = (i + (WORD_LENGTH - 1)) / WORD_LENGTH;

    //
    // Read the words
    //
    g_TextLib.lpWordBuf = (LPBYTE)malloc(i);
    if (g_TextLib.lpWordBuf == NULL)
    {
        fclose(fpWord);
        fclose(fpMsg);
        return -1;
    }
    fseek(fpWord, 0, SEEK_SET);
    fread(g_TextLib.lpWordBuf, i, 1, fpWord);

    //
    // Close the words file
    //
    fclose(fpWord);

    //
    // Read the message offsets. The message offsets are in SSS.MKF #3
    //
    i = PAL_MKFGetChunkSize(3, gpGlobals->f.fpSSS) / sizeof(DWORD);
    g_TextLib.nMsgs = i - 1;

    g_TextLib.lpMsgOffset = (LPDWORD)UTIL_calloc(1,i * sizeof(DWORD));
    if (g_TextLib.lpMsgOffset == NULL)
    {
        free(g_TextLib.lpWordBuf);
        fclose(fpMsg);
        return -1;
    }

    PAL_MKFReadChunk((LPBYTE)(g_TextLib.lpMsgOffset), i * sizeof(DWORD), 3,
        gpGlobals->f.fpSSS);

    //
    // Read the messages.
    //
    fseek(fpMsg, 0, SEEK_END);
    i = ftell(fpMsg);

    g_TextLib.lpMsgBuf = (LPBYTE)malloc(i);
    if (g_TextLib.lpMsgBuf == NULL)
    {
        free(g_TextLib.lpMsgOffset);
        free(g_TextLib.lpWordBuf);
        fclose(fpMsg);
        return -1;
    }

    fseek(fpMsg, 0, SEEK_SET);
    fread(g_TextLib.lpMsgBuf, 1, i, fpMsg);

    fclose(fpMsg);

    g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
    g_TextLib.bIcon = 0;
    g_TextLib.posIcon = 0;
    g_TextLib.nCurrentDialogLine = 0;
    g_TextLib.iDelayTime = 3;
    g_TextLib.posDialogTitle = PAL_XY(12, 8);
    g_TextLib.posDialogText = PAL_XY(44, 26);
    g_TextLib.bDialogPosition = kDialogUpper;
    g_TextLib.fUserSkip = FALSE;

    PAL_MKFReadChunk(g_TextLib.bufDialogIcons, 282, 12, gpGlobals->f.fpDATA);

    return 0;
}
INT CPalApp:: PAL_InitUI(
    VOID
)
/*++
  Purpose:

    Initialze the UI subsystem.

  Parameters:

    None.

  Return value:

    0 = success, -1 = fail.

--*/
{
    int        iSize;

    //
    // Load the UI sprite.
    //
    iSize = PAL_MKFGetChunkSize(CHUNKNUM_SPRITEUI, gpGlobals->f.fpDATA);
    if (iSize < 0)
    {
        return -1;
    }

    gpSpriteUI = (LPSPRITE)calloc(1, iSize);
    if (gpSpriteUI == NULL)
    {
        return -1;
    }

    PAL_MKFReadChunk(gpSpriteUI, iSize, CHUNKNUM_SPRITEUI, gpGlobals->f.fpDATA);

    return 0;
}

VOID CPalApp::PAL_FreeUI(
    VOID
)
/*++
  Purpose:

    Shutdown the UI subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    if (gpSpriteUI != NULL)
    {
        free(gpSpriteUI);
        gpSpriteUI = NULL;
    }
}

INT  CPalApp::PAL_IsBig5(INT Default_Decision)
{
    /*
    * 功能：测试字符集是否是BIG5
    * 输入缺省定义 1 返回0 2 返回 1，其他测试
    * 返回：1 是，0 GB2312 //其他 -1;
    */
    if (Default_Decision == 1)return 0;
    if (Default_Decision == 2)return 1;
    int noGb2312 = 0, noBig5 = 0;
    LPBYTE buf{};
    FILE *fpWord =  UTIL_OpenRequiredFile("word.dat");
    fseek(fpWord, 0, SEEK_END);
    long i = ftell(fpWord);
    buf = (LPBYTE)malloc(i + 10);
    if (!buf)
        exit(2);
    memset(buf, 0, i + 10);
    fseek(fpWord, 0, SEEK_SET);
    fread(buf, i, 1, fpWord);

    for (int n = 0; n < i; n++)
    {
        BYTE c, c1;
        c = buf[n];
        c1 = buf[n + 1];
        if (c < 127)
            continue;//ascii
        if (c > 0xa0 && c <= 0xfe)
        {
            if (c1 >= 0x40 && c1 <= 0xfe)
            {
                if (c1 < 0xa1 )
                {
                    //gb2312编码范围之外
                    noGb2312 = 1;
                }
                //big5范围
                n++;
                continue;
            }
        }
        noBig5 = 1;
    }
    //end
    free(buf);
    fclose(fpWord);
    if (noBig5)
        return -1;
    if (!noGb2312)
        return 0;
    return  1;
}

INT CPalApp::PAL_IsPalWIN(VOID)
{
    //测试是否是Win_95系统
    //前提是gpGlobals 已经初始化
    long i;
    int ret = 0;
    i = PAL_MKFGetChunkSize(2, gpGlobals->f.fpSSS);
    FILE* fpWord = UTIL_OpenRequiredFile("word.dat");
    fseek(fpWord, 0, SEEK_END);
    long  wordlen = ftell(fpWord);
    fclose(fpWord);
   
    wordlen = (wordlen + WORD_LENGTH - 1) / WORD_LENGTH; //对象数上限
    if (i > 6.5 * wordlen * 2)
        return 1;
    return 0;

}

LPSTR CPalApp::PAL_GetMsg(WORD  wNumMsg
)
/*++
  Purpose:

  Get the specified message.

  Parameters:

  [IN]  wNumMsg - the number of the requested message.

  Return value:

  Pointer to the requested message. NULL if not found.

  --*/
{
    static char    buf[256];
    DWORD          dwOffset, dwSize;

    if (wNumMsg >= g_TextLib.nMsgs)
    {
        return NULL;
    }

    dwOffset = SWAP32(g_TextLib.lpMsgOffset[wNumMsg]);
    dwSize = SWAP32(g_TextLib.lpMsgOffset[wNumMsg + 1]) - dwOffset;
    assert(dwSize < 255);

    memcpy(buf, &g_TextLib.lpMsgBuf[dwOffset], dwSize);
    buf[dwSize] = '\0';

    return PAL_TextToUTF8( buf);
}

INT CPalApp::PAL_RNGReadFrame(
    LPBYTE          lpBuffer,
    UINT            uiBufferSize,
    UINT            uiRngNum,
    UINT            uiFrameNum,
    FILE           *fpRngMKF
)
/*++
  Purpose:

  Read a frame from a RNG animation.

  Parameters:

  [OUT] lpBuffer - pointer to the destination buffer.

  [IN]  uiBufferSize - size of the destination buffer.

  [IN]  uiRngNum - the number of the RNG animation in the MKF archive.

  [IN]  uiFrameNum - frame number in the RNG animation.

  [IN]  fpRngMKF - pointer to the fopen'ed MKF file.

  Return value:

  Integer value which indicates the size of the chunk.
  -1 if there are error in parameters.
  -2 if buffer size is not enough.

  --*/
{
    UINT         uiOffset = 0;
    UINT         uiSubOffset = 0;
    UINT         uiNextOffset = 0;
    UINT         uiChunkCount = 0;
    INT          iChunkLen = 0;

    if (lpBuffer == NULL || fpRngMKF == NULL || uiBufferSize == 0)
    {
        return -1;
    }

    //
    // Get the total number of chunks.
    //
    uiChunkCount = PAL_MKFGetChunkCount(fpRngMKF);
    if (uiRngNum >= uiChunkCount)
    {
        return -1;
    }

    //
    // Get the offset of the chunk.
    //
    fseek(fpRngMKF, 4 * uiRngNum, SEEK_SET);
    fread(&uiOffset, sizeof(UINT), 1, fpRngMKF);
    fread(&uiNextOffset, sizeof(UINT), 1, fpRngMKF);
    uiOffset = SWAP32(uiOffset);
    uiNextOffset = SWAP32(uiNextOffset);

    //
    // Get the length of the chunk.
    //
    iChunkLen = uiNextOffset - uiOffset;
    if (iChunkLen != 0)
    {
        fseek(fpRngMKF, uiOffset, SEEK_SET);
    }
    else
    {
        return -1;
    }

    //
    // Get the number of sub chunks.
    //
    fread(&uiChunkCount, sizeof(UINT), 1, fpRngMKF);
    uiChunkCount = (SWAP32(uiChunkCount) - 4) / 4;
    if (uiFrameNum >= uiChunkCount)
    {
        return -1;
    }

    //
    // Get the offset of the sub chunk.
    //
    fseek(fpRngMKF, uiOffset + 4 * uiFrameNum, SEEK_SET);
    fread(&uiSubOffset, sizeof(UINT), 1, fpRngMKF);
    fread(&uiNextOffset, sizeof(UINT), 1, fpRngMKF);
    uiSubOffset = SWAP32(uiSubOffset);
    uiNextOffset = SWAP32(uiNextOffset);

    //
    // Get the length of the sub chunk.
    //
    iChunkLen = uiNextOffset - uiSubOffset;
    if ((UINT)iChunkLen > uiBufferSize)
    {
        return -2;
    }

    if (iChunkLen != 0)
    {
        fseek(fpRngMKF, uiOffset + uiSubOffset, SEEK_SET);
        fread(lpBuffer, iChunkLen, 1, fpRngMKF);
    }
    else
    {
        return -1;
    }

    return iChunkLen;
}

LPSTR CPalApp::PAL_TextToUTF8(LPCSTR s)
{
    if (!s) return NULL;

    Cls_Iconv m{};
    LPSTR rs;
    if (gpGlobals->bIsBig5 && !gConfig->fIsUseBig5)
    {
        rs = m.GBKtoUTF8(m.Big5ToGb2312(s), 936);
    }
    else if (gpGlobals->bIsBig5 && gConfig->fIsUseBig5)
    {
        rs = m.GBKtoUTF8(s, 950);
    }
    else if (!gpGlobals->bIsBig5 && gConfig->fIsUseBig5)
    {
        rs = m.GBKtoUTF8(m.Gb2312ToBig5(s), 950);
    }
    else
    {
        rs = m.GBKtoUTF8(s, 936);
    }
    return rs;
}

