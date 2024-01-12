#pragma once
#include "../main/palgpgl.h"
#include <sdl.h>
#include <vector>
#define IMAGE_X 128
#define IMAGE_Y 60

#define MapWidth 2048
#define MapHight 2048

typedef struct ragUndo
{
	DWORD32 tPos;
	DWORD32 tOld;
	DWORD32 tNew;
}MAPUNDO;

typedef std::vector<MAPUNDO> MapUndoArray;

typedef std::vector<INT32>TestData;


class CPalMapEdit
{
public:
	DWORD          Tiles[128][64][2] = {0};
	LPVOID         pTileSprite{ 0 };
	INT            iMapNum{ 0 };//地图号
	INT			   iScene{ 0 };//场景号
	INT			   m_ImageCount{ 0 };//图片数量
	CImage* m_pImageList[512]{0};//tile列表
	class CGetPalData * Pal{nullptr};

	CPoint fixedpoint{ 0 };//不动点，在窗口中始终显示
	INT TriggerMode{ 0 };

protected:
	class CMap_Dlg* pPara{ nullptr };

private:
	MapUndoArray UndoArray;//保存修改的数据，用于撤销
public:

	CPalMapEdit(class CMap_Dlg* para=nullptr);
	virtual ~CPalMapEdit();

	INT PAL_LoadMap(int nMap);

	BOOL GetFrame(LPBYTE lpImageCode, DWORD dwImage, LPBYTE lpBuffer, DWORD dwBufferLen);

	BOOL DecodeRLE(LPBYTE lpImageCode, LPBYTE lpBuffer, DWORD dwBufferLen);
	virtual INT doUndo();
	// 更新Tile
	virtual VOID UpdateTile(DWORD flg);

	virtual VOID MakeUndo(const VOID *);
	//列表选择行返回对应值
	virtual INT  ListRowToReturn(INT );

	virtual DWORD GetUndoSize();
	// 画图块的边界
	BOOL BoundingLiteBox(CImage* image, int w, int h, int x = 0, int y = 0, COLORREF = RGB(255, 255, 255),int width = 1);
	// 设置m_pImageList
	void setTileImage();
	//生成地图
	virtual INT MakeMapImage(CImage& m,const DWORD flag);
	//画障碍
	VOID DrawObstacles(CImage* m, const SDL_Rect* lpSrcRect);
	//在SDl表面上画地图
	VOID PAL_MapBlitToSurface(CPalMapEdit* lpMap, SDL_Surface* lpSurface, const SDL_Rect* lpSrcRect, BYTE ucLayer);
	//获取指定层位置(x, y, h)上的平铺位图
	LPCBITMAPRLE PAL_MapGetTileBitmap(BYTE x, BYTE y, BYTE h, BYTE ucLayer, CPalMapEdit* lpMap);
	//从精灵中获取指向指定帧的指针
	LPCBITMAPRLE PAL_SpriteGetFrame(LPCSPRITE lpSprite, INT iFrameNum);
	//Blit一个rle压缩位图到SDL表面。
	//注意 : 假设表面已经锁定，并且表面是8位的。
	INT PAL_RLEBlitToSurface(LPCBITMAPRLE lpBitmapRLE, SDL_Surface* lpDstSurface, PAL_POS pos, BOOL bShadow = FALSE);

	//求dx,dy的tile坐标 x,y，h
	DWORD FindTileCoor(int dx, int dy, int* sx, int* sy ,int* h );
	//取tile的坐标
	BOOL GetMapTilePoint(WORD x, WORD y, WORD h, int& dx, int& dy);
	//退出时存储
	virtual INT SaveMapTiles();
	//初始化tile 列表
	virtual void Initializes_Tilelist(CImageList&,class CMapList&);
	// 取弹出菜单文字数组
	virtual LPCTSTR *get_Menu_Str()const;
	// 取显示信息字符串
	virtual VOID get_Msg_Str(CString& inStr, DWORD MapSelectedTPos, DWORD TileSelected,DWORD,DWORD);
	//处理弹出菜单返回值 //返回 0 取消 1 刷新 -1 正常 1
	virtual int MapPopMenuRetuen(int msg,DWORD& flags);

	LPRGBQUAD SDL_ColorsToRGBQUADS(SDL_Color *);
};

