#ifndef CPALAPP_H
#define CPALAPP_H

#include "command.h"
#include "palgpgl.h"
#include "cpalevent.h"
#include <string>
#include <time.h>

//本类定义文件系统相关





static int glSeed;

#ifndef DO_BYTESWAP
#define DO_BYTESWAP(buf, size)									\
if(DO_SWAP)                                                     \
for (int i = 0; i < (size) / 2; i++)							\
{																\
	((LPWORD)(buf))[i] = SWAP16(((LPWORD)(buf))[i]);			\
}
#endif // !DO_BYTESWAP

#define WORD_LENGTH      10
#define   FONT_COLOR_DEFAULT        0x4F
#define   FONT_COLOR_YELLOW         0x2D
#define   FONT_COLOR_RED            0x1A
#define   FONT_COLOR_CYAN           0x8D
#define   FONT_COLOR_CYAN_ALT       0x8C
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


//基本磁盘IO类，不需要调用其他类
class CPalApp:public CPalEvent
{
public:

public:
    CPalApp();
    ~CPalApp();
    INT InitPalBase();
    VOID dstroyPalBase();
    //cpalapp.app 中定义
    LPCSTR getBasePath(){return BasePath.c_str();}
    VOID  setBasePath(LPCSTR path){BasePath = path;}
    BOOL IsFileExist(const std::string & csFile);
    BOOL IsDirExist(const std::string& csFile);
    BOOL isCorrectDir(const std::string &dir);
    const size_t getSaveFileOffset(int ob);
    VOID PAL_SaveGame(LPCSTR szFileName, WORD wSavedTimes);
    size_t getSaveFileLen();
    FILE* UTIL_OpenRequiredFile(LPCSTR  lpszFileName);
    INT PAL_MKFGetChunkSize(UINT uiChunkNum, FILE* fp);
    INT PAL_MKFGetChunkCount(FILE* fp);
    //申请内存
    VOID PAL_InitGlobalGameData(VOID);
    //打开公共数据文件
    VOID PAL_ReadGlobalGameData(VOID);

    void trim(char* str);

    //打开文件
    INT PAL_InitGlobals(VOID);
    //载入初始数据
    VOID  PAL_LoadDefaultGame(VOID);
    //载入档案
    INT PAL_LoadGame(LPCSTR szFileName);

    VOID PAL_CompressInventory(VOID);

    VOID PAL_New_SortInventory();

    INT PAL_MKFReadChunk(LPBYTE          lpBuffer,
        UINT            uiBufferSize,
        UINT            uiChunkNum,
        FILE* fp
    );
    INT PAL_MKFDecompressChunk(
        LPBYTE          lpBuffer,
        UINT            uiBufferSize,
        UINT            uiChunkNum,
        FILE* fp
    );
    INT PAL_MKFSaveChunk(
        LPBYTE          lpBuffer,
        UINT            uiBufferSize,
        UINT            uiChunkNum,
        FILE* fp
    );
    INT PAL_MKFGetDecompressedSize(
        UINT    uiChunkNum,
        FILE* fp
    );
    //压缩解压 
    INT is_Use_YJ1_Decompress();
    INT PAL_DeCompress(LPCVOID Source, LPVOID Destination, INT DestSize);
    int EncodeYJ1(const void* Source, UINT32 SourceLength, void*& Destination, UINT32& Length);
    int EncodeYJ2(const void* Source, UINT32 SourceLength, void*& Destination, UINT32& Length, bool bCompatible);
    int EnCompress(const void* Source, UINT32 SourceLength, void*& Destination, UINT32& Length, bool bCopatible);
    INT YJ1_Decompress(LPCVOID Source, LPVOID Destination, INT DestSize);
    INT YJ2_Decompress(LPCVOID Source, LPVOID Destination, INT DestSize);

    VOID PAL_FreeGlobals(VOID);
    VOID UTIL_CloseFile(FILE* fp);

    LPSTR PAL_GetMsg(WORD wNumMsg);
    INT PAL_RNGReadFrame(LPBYTE lpBuffer, UINT uiBufferSize, UINT uiRngNum, UINT uiFrameNum, FILE* fpRngMKF);
    LPSTR PAL_TextToUTF8(LPCSTR s);
    INT PAL_InitText(VOID);
    INT PAL_InitUI(VOID);
    VOID PAL_FreeUI(VOID);
    INT PAL_IsBig5(INT Default_Decision = 0);
    INT PAL_IsPalWIN(VOID);
    VOID PAL_FreeText();

    //读取说明
    LPCSTR PAL_GetObjectDesc(LPOBJECTDESC   lpObjectDesc, WORD  wObjectID);
    LPCSTR PAL_GetWord(WORD wNumWord);
    //载入说明
    LPOBJECTDESC PAL_LoadObjectDesc(LPCSTR lpszFileName);
    //释放说明
    VOID PAL_FreeObjectDesc(LPOBJECTDESC   lpObjectDesc);

    inline  void lsrand(unsigned int iInitialSeed)
    {
        glSeed = 1664525L * iInitialSeed + 1013904223L;
    }

    inline int	lrand(void)
    {
        if (glSeed == 0) // if the random seed isn't initialized...
            lsrand((unsigned int)time(NULL)); // initialize it first
        glSeed = 1664525L * glSeed + 1013904223L; // do some twisted math (infinite suite)
        return ((glSeed >> 1) + 1073741824L); // and return the result.
    }

    inline INT	RandomLong(int from, int to)
    {
        if (to <= from)
            return from;
        return from + lrand() / (INT_MAX / (to - from + 1));
    }

    inline float RandomFloat(float from, float to) {
        if (to <= from)
            return from;
        return from + (float)lrand() / (INT_MAX / (to - from));
    }

    inline FILE* UTIL_OpenFile(LPCSTR lpszFileName)
    {
        std::string  fstr = PalDir + lpszFileName;
        FILE* fp;
        fp = fopen(fstr.c_str(),"rb");
        return fp;
    }

public:
    //
};

#endif // CPALAPP_H
