
//#define _AFXDLL

#include "stdafx.h"
#include "CPalMapedit.h"
#include "sdlg.h"
#include "cMap_Dlg.h"
#include "..\main\cscript.h"
static LPCWSTR pMap_Menu_Str[] =
{
	L"显示底层",
	L"显示上层",
	L"显示障碍",
	L"添加障碍",
	L"清除障碍",
	L"清除底层",
	L"清除上层",
	L"使用%3.3d替换底层",
	L"使用%3.3d替换上层",
	nullptr
};

enum tagMapFlags
{
	map_Bottom = 1 << 0,//下层标识
	map_Top = 1 << 1,//上层标识
	map_Barrier = 1 << 2,//障碍标识
	map_AddObstacle = 1 << 3,//添加障碍
	map_ClearObstacle = 1 << 4,//清除障碍
	map_ClearBottom = 1 << 5,//清除底层
	map_ClearTop = 1 << 6,//清除上层
	map_ReplaceBottom = 1 << 7, //	使用%3.3d替换底层
	map_ReplaceTop = 1 << 8, //	使用%3.3d替换上层
};


CPalMapEdit::CPalMapEdit(CMap_Dlg* para)
	:pTileSprite(nullptr)
	, iMapNum(-1)
	, m_ImageCount(0)
	, pPara(para)
{
	//SDL_Init((Uint32)(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE));

	Pal = ((CsDlg*)AfxGetApp()->m_pMainWnd)->Pal;
	Pal->gpPalette->colors = Pal->PAL_GetPalette(0, 0);

}

CPalMapEdit::~CPalMapEdit()
{
	if (pTileSprite)
		free(pTileSprite);
	for (size_t i = 0; i < m_ImageCount; i++)
	{
		if (m_pImageList[i]) delete m_pImageList[i];
	}
	//delete pScript;
	//SDL_Quit();
}

INT CPalMapEdit::PAL_LoadMap(int nMap)
{

	iMapNum = nMap;
	FILE* fpMAP = Pal->UTIL_OpenRequiredFile("map.mkf");
	FILE* fpGOP = Pal->UTIL_OpenRequiredFile("gop.mkf");
	int mLen = Pal->PAL_MKFGetChunkSize(nMap, fpMAP);
	int gLen = Pal->PAL_MKFGetChunkSize(nMap, fpGOP);
	if (mLen <= 0 || gLen <= 0)
	{
		fclose(fpMAP);
		fclose(fpGOP);
		return -1;
	}
	LPBYTE buf = (LPBYTE)malloc(mLen);
	pTileSprite = (LPBYTE)malloc(gLen);

	if (pTileSprite == nullptr || buf == nullptr)
	{
		fclose(fpMAP);
		fclose(fpGOP);
		return -1;
	}

	Pal->PAL_MKFReadChunk(buf, mLen, iMapNum, fpMAP);
	Pal->PAL_DeCompress(buf, (LPBYTE)(Tiles), sizeof(Tiles));
	free(buf);

	Pal->PAL_MKFReadChunk((LPBYTE)pTileSprite, gLen, iMapNum, fpGOP);

	fclose(fpMAP);
	fclose(fpGOP);

	return 0;
}

BOOL CPalMapEdit::GetFrame(LPBYTE lpImageCode, DWORD dwImage, LPBYTE lpBuffer, DWORD dwBufferLen)
{
	DWORD	dwImageCount = 0;
	DWORD	dwLen = 0;
	DWORD	dwOffset = 0;

	dwImageCount = ((WORD*)lpImageCode)[0] - 1;

	if (dwImage < dwImageCount)
	{
		dwOffset = ((WORD*)lpImageCode)[dwImage] << 1;
		return DecodeRLE(lpImageCode + dwOffset, lpBuffer, dwBufferLen);
	}

	return FALSE;
}

/********************************************************************************\
|
|	Name:
|		BOOL DeRLE(LPBYTE lpImageCode, LPBYTE lpBuffer, DWORD dwBufferLen)
|
|	Description:
|		解压RLE
|
\********************************************************************************/

BOOL CPalMapEdit::DecodeRLE(LPBYTE lpImageCode, LPBYTE lpBuffer, DWORD dwBufferLen)
{
	DWORD i = 0;
	DWORD dwLen = 0;
	DWORD dwWidth = 0;
	DWORD dwHeight = 0;
	BYTE* pDestData = 0;

	if (lpImageCode == NULL || lpBuffer == NULL)
	{
		return FALSE;
	}

	// Gop.mkf中的图像数据格式开头是没有02 00 00 00的
	if (((DWORD*)lpImageCode)[0] == 0x00000002)
	{
		lpImageCode += 4;
	}

	dwWidth = ((WORD*)lpImageCode)[0];
	dwHeight = ((WORD*)lpImageCode)[1];
	dwLen = dwWidth * dwHeight;

	if (dwLen > dwBufferLen)
	{
		return FALSE;
	}

	::memset(lpBuffer, 0x0, dwLen);         // 全设为关键色

	// 解
	lpImageCode += 4;
	BYTE T;
	for (i = 0; i < dwLen;)
	{
		T = *lpImageCode++;
		if (0x80 < T && T <= 0x80 + dwWidth)
		{
			i = i + (T - 0x80);
		}
		else
		{
			::memcpy(lpBuffer + i, lpImageCode, T);
			lpImageCode += T;
			i = i + T;
		}
	}

	return TRUE;
}


INT CPalMapEdit::doUndo()
{
	if (UndoArray.size() == 0)
		return 0;
	MAPUNDO undo;
	undo = UndoArray[UndoArray.size() - 1];
	UndoArray.pop_back();
	int x = PAL_X(undo.tPos) >> 1, y = PAL_Y(undo.tPos), h = (undo.tPos & 1);
	Tiles[y & 0x7f][x & 0x3f][h & 1] = undo.tOld;
	return 1;
}
//更新Tile
VOID CPalMapEdit::UpdateTile(DWORD flg)
{
	DWORD pos = pPara->m_MapSelectedPos;
	DWORD row = pPara->m_ListSelectRow;
	DWORD TileSelected = pPara->m_TileSelected;

	flg ^= 0x7;
	int x, y, h;
	x = PAL_X(pos);
	y = PAL_Y(pos);
	h = x & 1;
	x >>= 1;
	MAPUNDO undo{ 0 };
	undo.tPos = pos;
	undo.tOld = TileSelected;
	WORD tTop = PAL_Y(TileSelected), tBottom = PAL_X(TileSelected);
	switch (flg)
	{
	case		map_AddObstacle://添加障碍
		tBottom |= 0x2000;
		break;
	case 		map_ClearObstacle://清除障碍
		if (tBottom & 0x2000)
			tBottom ^= 0x2000;
		break;
	case		map_ClearBottom://清除底层
		tBottom &= 0x2000;
		break;
	case		map_ClearTop://清除上层
		tTop = 0;
		break;
	case		map_ReplaceBottom: //	使用%3.3d替换底层
		tBottom &= 0xffff2000;
		tBottom |= row & 0xff;
		tBottom |= (row & 0x100) << 4;
		break;
	case		map_ReplaceTop: //	使用%3.3d替换上层
		tTop |= row & 0xff;
		tTop |= (row & 0x100) << 4;
		break;

	default:
		break;
	}
	undo.tNew = PAL_XY(tBottom, tTop);
	//检查修改
	if (undo.tOld == undo.tNew)
		return;
	//实现修改
	Tiles[y & 0x7f][x & 0x3f][h & 1] = undo.tNew;
	MakeUndo(&undo);

	//重新生成大地图
	MakeMapImage(pPara->m_MapImage, pPara->m_Flags);
}

VOID CPalMapEdit::MakeUndo(const VOID* pUndo)
{
	UndoArray.push_back(*(const MAPUNDO*)pUndo);
}

INT CPalMapEdit::ListRowToReturn(INT row)
{
	return row;
}

DWORD CPalMapEdit::GetUndoSize()
{
	return UndoArray.size();
}

// 画菱形图块的边界，菱形起点为矩形的边界线段中点
//输入：图像结构指针，矩形宽度，矩形高度，起始x=0，起始y=0，线颜色=白色，线宽度=1
//返回，0 失败，非零成功
BOOL CPalMapEdit::BoundingLiteBox(CImage* image, int w,int h,int x,int y,COLORREF colo,int width)
{
	// TODO: 在此处添加实现代码.+
	if (!image)return 0;

	HDC dc = image->GetDC();

	POINT s[4] = { {x,y + (h >> 1)},{x + (w >> 1),y},{x + w ,y + (h >> 1)}, {x + (w >> 1),y + h} };
	MoveToEx(dc, s[3].x, s[3].y, 0);

	CDC* pdc = CDC::FromHandle(dc);
	CPen newpen;
	newpen.CreatePen(PS_SOLID, width,colo);
	CPen* oldpen = (pdc->SelectObject(&newpen));
	//画线
	for (int n = 0; n < 4; n++)
		LineTo(dc, s[n].x, s[n].y);
	pdc->SelectObject(&oldpen);

	image->ReleaseDC();
	return 1;
}


// 设置m_pImageList
void CPalMapEdit::setTileImage()
{
	// TODO: 在此处添加实现代码.
	BYTE imge[480];
	for (int nTile = 0; nTile < m_ImageCount; nTile++)
	{
		//取解压前字节
		GetFrame((LPBYTE)pTileSprite, nTile, imge, 480);
		//新建
		CImage *pMap = new CImage;
		
		pMap->Create(32, -15, 8);
		//设置调色板
		pMap->SetColorTable(0, 256, (const RGBQUAD *) Pal->gpPalette->colors);
		//拷贝图像数据
		memcpy(pMap->GetBits(), imge, 32 * 15);
		m_pImageList[nTile] = pMap;
	}
}

//
// 
//
INT  CPalMapEdit::MakeMapImage(CImage& mm,const DWORD flag)
{
	if (mm.IsNull())
		mm.Create(MapWidth, -MapHight, 24);//建立表
	CImage m;//中间表
	m.Create(MapWidth, -MapHight, 8);
	m.SetColorTable(0, 256, SDL_ColorsToRGBQUADS( Pal->gpPalette->colors));
	SDL_Surface mSuf{};
	mSuf.w = MapWidth;
	mSuf.h = MapHight;
	mSuf.pixels = m.GetBits();
	mSuf.pitch = m.GetPitch();
	SDL_Rect rect = { -0,-0,MapWidth,MapHight };
	//画底层
	if (flag & map_Bottom)
		PAL_MapBlitToSurface(this, &mSuf, &rect, 0);
	//画顶层
	if (flag & map_Top)
		PAL_MapBlitToSurface(this, &mSuf, &rect, 1);
	//画障碍
	if (flag & map_Barrier)
		DrawObstacles(&m, &rect);
	HDC ddc = mm.GetDC();
	m.Draw(ddc, 0, 0, MapWidth, MapHight);
	mm.ReleaseDC();
	return 0;
}

//画障碍
VOID CPalMapEdit::DrawObstacles(CImage* m, const SDL_Rect* lpSrcRect)
{
	int              sx, sy, dx, dy, x, y, h, xPos, yPos;
	//
	// Convert the coordinate
	//
	sy = lpSrcRect->y / 16 - 1;
	dy = (lpSrcRect->y + lpSrcRect->h) / 16 + 2;
	sx = lpSrcRect->x / 32 - 1;
	dx = (lpSrcRect->x + lpSrcRect->w) / 32 + 2;
	yPos = sy * 16 - 8 - lpSrcRect->y;
	for (y = sy; y < dy; y++)
	{
		for (h = 0; h < 2; h++, yPos += 8)
		{
			xPos = sx * 32 + h * 16 - 16 - lpSrcRect->x;
			for (x = sx; x < dx; x++, xPos += 32)
			{
				//
				if (x >= 64 || y >= 128 || h > 1 || x < 0 || y < 0)
					continue;
				DWORD32 l = Tiles[y][x][h];
				if (l & 0x2000)
				{
					//用白笔画
					BoundingLiteBox(m, 32, 16, xPos, yPos, RGB(255, 255, 255), pPara->m_MapZoom > 1.4 ? 1 : 3);
				}
			}
		}
	}
}



VOID CPalMapEdit::PAL_MapBlitToSurface(CPalMapEdit * lpMap, SDL_Surface* lpSurface, const SDL_Rect* lpSrcRect, BYTE ucLayer)

/*++
  Purpose:

  Blit the specified map area to a SDL Surface.

  Parameters:

  [IN]  lpMap - Pointer to the map.

  [OUT] lpSurface - Pointer to the destination surface.

  [IN]  lpSrcRect - Pointer to the source area.

  [IN]  ucLayer - The layer. 0 for bottom, 1 for top.

  Return value:

  None.

  --*/
{
	int              sx, sy, dx, dy, x, y, h, xPos, yPos;
	LPCBITMAPRLE     lpBitmap = NULL;

	//
	// Convert the coordinate
	//
	sy = lpSrcRect->y / 16 - 1;
	dy = (lpSrcRect->y + lpSrcRect->h) / 16 + 2;
	sx = lpSrcRect->x / 32 - 1;
	dx = (lpSrcRect->x + lpSrcRect->w) / 32 + 2;

	//
	// Do the drawing.
	//
	yPos = sy * 16 - 8 - lpSrcRect->y;
	for (y = sy; y < dy; y++)
	{
		for (h = 0; h < 2; h++, yPos += 8)
		{
			xPos = sx * 32 + h * 16 - 16 - lpSrcRect->x;
			for (x = sx; x < dx; x++, xPos += 32)
			{
				lpBitmap = PAL_MapGetTileBitmap((BYTE)x, (BYTE)y, (BYTE)h, ucLayer, lpMap);
				if (lpBitmap == NULL)
				{
					if (ucLayer)
					{
						continue;
					}
					lpBitmap = PAL_MapGetTileBitmap(0, 0, 0, ucLayer, lpMap);
				}
				PAL_RLEBlitToSurface(lpBitmap, lpSurface, PAL_XY(xPos, yPos));
			}
		}
	}
}


LPCBITMAPRLE CPalMapEdit::PAL_MapGetTileBitmap(BYTE x, BYTE y, BYTE h, BYTE ucLayer, CPalMapEdit * lpMap)

/*++
  Purpose:

  Get the tile bitmap on the specified layer at the location (x, y, h).

  Parameters:

  [IN]  x - Column number of the tile.

  [IN]  y - Line number in the map.

  [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
  (See map.h for details.)

  [IN]  ucLayer - The layer. 0 for bottom, 1 for top.

  [IN]  lpMap - Pointer to the loaded map.

  Return value:

  Pointer to the bitmap. NULL if failed.

  --*/
{
	DWORD d;

	//
	// Check for invalid parameters.
	//
	if (x >= 64 || y >= 128 || h > 1 || lpMap == NULL)
	{
		return NULL;
	}

	//
	// Get the tile data of the specified location.
	//
	d = lpMap->Tiles[y][x][h];

	if (ucLayer == 0)
	{
		//
		// Bottom layer
		//
		return PAL_SpriteGetFrame((LPBYTE)lpMap->pTileSprite, (d & 0xFF) | ((d >> 4) & 0x100));
	}
	else
	{
		//
		// Top layer
		//
		d >>= 16;
		return PAL_SpriteGetFrame((LPBYTE)lpMap->pTileSprite, ((d & 0xFF) | ((d >> 4) & 0x100)) - 1);
	}
}

LPCBITMAPRLE CPalMapEdit::PAL_SpriteGetFrame(
	LPCSPRITE       lpSprite,
	INT             iFrameNum
)
/*++
  Purpose:

	Get the pointer to the specified frame from a sprite.

  Parameters:

	[IN]  lpSprite - pointer to the sprite.

	[IN]  iFrameNum - number of the frame.

  Return value:

	Pointer to the specified frame. NULL if the frame does not exist.

--*/
{
	int imagecount, offset;

	if (lpSprite == NULL)
	{
		return NULL;
	}

	//
	// Hack for broken sprites like the Bloody-Mouth Bug
	//
 //   imagecount = (lpSprite[0] | (lpSprite[1] << 8)) - 1;
	imagecount = (lpSprite[0] | (lpSprite[1] << 8));

	if (iFrameNum < 0 || iFrameNum >= imagecount)
	{
		//
		// The frame does not exist
		//
		return NULL;
	}

	//
	// Get the offset of the frame
	//
	iFrameNum <<= 1;
	offset = ((lpSprite[iFrameNum] | (lpSprite[iFrameNum + 1] << 8)) << 1);
	if (offset == 0x18444) offset = (WORD)offset;
	return &lpSprite[offset];
}




BYTE
PAL_CalcShadowColor(
	BYTE bSourceColor
);
//{
	//return ((bSourceColor & 0xF0) | ((bSourceColor & 0x0F) >> 1));
//}


INT CPalMapEdit::PAL_RLEBlitToSurface(
	LPCBITMAPRLE      lpBitmapRLE,
	SDL_Surface* lpDstSurface,
	PAL_POS           pos,
	BOOL              bShadow
)
/*++
  Purpose:

	Blit an RLE-compressed bitmap to an SDL surface.
	NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

	[IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

	[OUT] lpDstSurface - pointer to the destination SDL surface.

	[IN]  pos - position of the destination area.

	[IN]  bShadow - flag to mention whether blit source color or just shadow.

  Return value:

	0 = success, -1 = error.

--*/
{
	UINT          i, j, k, sx;
	INT           x, y;
	UINT          uiLen = 0;
	UINT          uiWidth = 0;
	UINT          uiHeight = 0;
	UINT          uiSrcX = 0;
	BYTE          T;
	INT           dx = PAL_X(pos);
	INT           dy = PAL_Y(pos);
	LPBYTE        p;

	//
	// Check for NULL pointer.
	//
	if (lpBitmapRLE == NULL || lpDstSurface == NULL)
	{
		return -1;
	}

	//
	// Skip the 0x00000002 in the file header.
	//
	if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
		lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
	{
		lpBitmapRLE += 4;
	}

	//
	// Get the width and height of the bitmap.
	//
	uiWidth = lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
	uiHeight = lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);

	//
	// Check whether bitmap intersects the surface.
	//
	if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
		uiHeight + dy <= 0 || dy >= lpDstSurface->h)
	{
		goto end;
	}

	//
	// Calculate the total length of the bitmap.
	// The bitmap is 8-bpp, each pixel will use 1 byte.
	//
	uiLen = uiWidth * uiHeight;

	//
	// Start decoding and blitting the bitmap.
	//
	lpBitmapRLE += 4;
	for (i = 0; i < uiLen;)
	{
		T = *lpBitmapRLE++;
		if ((T & 0x80) && T <= 0x80 + uiWidth)
		{
			i += T - 0x80;
			uiSrcX += T - 0x80;
			if (uiSrcX >= uiWidth)
			{
				uiSrcX -= uiWidth;
				dy++;
			}
		}
		else
		{
			//
			// Prepare coordinates.
			//
			j = 0;
			sx = uiSrcX;
			x = dx + uiSrcX;
			y = dy;

			//
			// Skip the points which are out of the surface.
			//
			if (y < 0)
			{
				j += -y * uiWidth;
				y = 0;
			}
			else if (y >= lpDstSurface->h)
			{
				goto end; // No more pixels needed, break out
			}

			while (j < T)
			{
				//
				// Skip the points which are out of the surface.
				//
				if (x < 0)
				{
					j += -x;
					if (j >= T) break;
					sx += -x;
					x = 0;
				}
				else if (x >= lpDstSurface->w)
				{
					j += uiWidth - sx;
					x -= sx;
					sx = 0;
					y++;
					if (y >= lpDstSurface->h)
					{
						goto end; // No more pixels needed, break out
					}
					continue;
				}

				//
				// Put the pixels in row onto the surface
				//
				k = T - j;
				if (lpDstSurface->w - x < k) k = lpDstSurface->w - x;
				if (uiWidth - sx < k) k = uiWidth - sx;
				sx += k;
				p = ((LPBYTE)lpDstSurface->pixels) + y * lpDstSurface->pitch;
				if (bShadow)
				{
					j += k;
					for (; k != 0; k--)
					{
						p[x] = PAL_CalcShadowColor(p[x]);
						/////
						if (p[x] == 0)p[x] = 1;
						x++;
					}
				}
				else
				{
					for (; k != 0; k--)
					{
						p[x] = lpBitmapRLE[j];
						/////
						if (p[x] == 0)p[x] = 1;
						j++;
						x++;
					}
				}

				if (sx >= uiWidth)
				{
					sx -= uiWidth;
					x -= uiWidth;
					y++;
					if (y >= lpDstSurface->h)
					{
						goto end; // No more pixels needed, break out
					}
				}
			}
			lpBitmapRLE += T;
			i += T;
			uiSrcX += T;
			while (uiSrcX >= uiWidth)
			{
				uiSrcX -= uiWidth;
				dy++;
			}
		}
	}

end:
	//
	// Success
	//
	return 0;
}

//求dx,dy的tile坐标 x,y，h
//输入：dx,dy点的坐标，三个指针分别是指向x,y,h
// 返回：tile解析值，0x2000 障碍，高位顶层，低位底层，掩码0x1fff 
//bx,by 组成矩形边32*16的中点构成棱型，此棱型将矩形分割成5部分， 左上，左下，右上，右下和中五部分
// 左上x-1 y-1 h=1 x< -2y+16
// 左下x-1,y,h=1   x<  2y-16
// 右上x,y-1,h=1   x>  2y+16
// 右下 x,y,h=1    x> -2y+48 
// 中 x,y,h=0
DWORD CPalMapEdit::FindTileCoor(int dx, int dy, int* sx, int* sy, int* sh)
{
	//dx,dy 除 32 和 16
	int x = (dx + 16) >> 5;
	int y = (dy + 8) >> 4;
	int h = 0;
	int bx = (dx + 16) & 0x1f;
	int by = (dy + 8) & 0xf;
	if (bx < -2 * by + 16)
		x--, y--, h = 1;
	else if (bx < 2 * by - 16)
		x--, h = 1;
	else if (bx > 2 * by + 16)
		y--, h = 1;
	else if (bx > -2 * by + 48)
		h = 1;
	if (sx && sy && sh)
		*sy = y, * sx = x, * sh = h;
	if (x >= 0 && y >= 0)
		return Tiles[y][x][h];
	return 0;
}

//取tile的坐标
//输入：x,y,h 指向坐标的指针
// 返回 1 成功，0 失败
BOOL  CPalMapEdit::GetMapTilePoint(WORD x,WORD y, WORD h, int &dx,int &dy)
{
	if (x > 64 || y > 128 || h > 1)
		return FALSE;
	dy = y * 16 - 8 + h * 8;
	dx = x * 32 - 16 + h * 16;
	return TRUE;
}

INT CPalMapEdit::SaveMapTiles()
{
	UINT32 len{};
	LPVOID s{};
	Pal->EnCompress(Tiles, sizeof(Tiles), s, len, 0);
	string filename = Pal->PalDir;
	filename += "map.mkf";
	Pal->replaceMKFOne(filename.c_str(), iMapNum, s, len);
	free(s);
	return 0;
}




//初始化tile 列表
void CPalMapEdit::Initializes_Tilelist(CImageList& m_ImageList,class CMapList& m_List)
{
	m_ImageCount = *((WORD*)pTileSprite) - 1;

	//建立tile列表
	setTileImage();

	for (int n = 0; n < m_ImageCount; n++)
	{
		// 图列表中选择并拷贝放大
		CImage* pm = new CImage;
		pm->Create(IMAGE_X, IMAGE_Y, 24);

		//拷贝放大
		//放大并建立32位图像
		HDC destDC = pm->GetDC();

		(m_pImageList[n])->StretchBlt(destDC, 0, 0, IMAGE_X, IMAGE_Y);
		SDL_Color* pcolor = &Pal->gpPalette->colors[0];
		COLORREF tColor = RGB(pcolor->r, pcolor->g, pcolor->b);
		pm->SetTransparentColor(tColor);
		pm->ReleaseDC();


		//画边界
		BoundingLiteBox(pm, IMAGE_X, IMAGE_Y, 0, 0, RGB(255, 255, 255), 2);

		CBitmap cbm;
		HBITMAP hbm = pm->operator HBITMAP();
		cbm.Attach(hbm);
		int i = m_ImageList.Add(&cbm, tColor);
		ASSERT(i == n);
		delete pm;
	}
	m_List.ModifyStyle(0, LVS_SINGLESEL);
	m_List.SetBkColor(RGB(20, 20, 20));
	m_List.SetImageList(&m_ImageList, LVSIL_NORMAL);
	m_List.ModifyStyle(LVS_ALIGNLEFT, LVS_ALIGNTOP);//把水平滚动条换成垂直滚动条
	//生成列表
	for (int n = 0; n < m_ImageCount; n++)
	{
		CString strText;
		strText.Format(_T("%3.3d"), n); //item text
		int i = m_List.InsertItem(n, strText, n);//最后一个参数就是图片的Id
	}
	//设置焦点
	m_List.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	m_List.SetSelectionMark(0);
}


// 取弹出菜单文字数组
LPCTSTR* CPalMapEdit::get_Menu_Str() const
{
	return  pMap_Menu_Str;
}

#define Tile_B(d)( d & 0xff |((d >> 4) & 0x100))
#define Tile_T(d) ( ( (d >> 16) & 0xff)|( (d>>20) & 0x100) ) 

// 取显示信息字符串
VOID CPalMapEdit::get_Msg_Str(CString& inStr, DWORD MapSelectedTPos, 
	DWORD TileSelected,DWORD row,DWORD m_Flags)
{
	// TODO: 在此处添加实现代码.
	CString s1;
	inStr.Format(L"X:%3.3d Y:%3.3d 底层 %3.3d 顶层 %3.3d 障碍 %s   ",
		PAL_X(MapSelectedTPos), PAL_Y(MapSelectedTPos),
		Tile_B(TileSelected), Tile_T(TileSelected),
		TileSelected & 0x2000 ? L"√" : L"×");
	for (size_t n = 0; n < 3; n++)
	{
		s1.Format(L" %s%s ", get_Menu_Str()[n], m_Flags & 1 << n ? L"√" : L"×");
		inStr += s1;
	}
	if (UndoArray.size())
	{
		//历史不为空，显示历史数据
		s1.Format(L"已经修改 %d 处，Ctr+Z 撤销， ", UndoArray.size());
		inStr += s1;
	}
	for (size_t n = 3; get_Menu_Str()[n]; n++)
	{
		if (m_Flags & 1 << n)
		{
			inStr += L"  编辑(双击)：";
			s1.Format(get_Menu_Str()[n], row);
			inStr += s1;
		}
	}
	return ;
}


//处理弹出菜单返回值 //返回 0 取消 1 刷新 -1 正常 1
int CPalMapEdit::MapPopMenuRetuen(int msg,DWORD& m_Flags)
{
	// TODO: 在此处添加实现代码.
	if (msg < 3)
	{
		m_Flags ^= 1 << msg;
	}
	else if (m_Flags & 1 << msg)
		m_Flags ^= 1 << msg;
	else
	{
		m_Flags &= 0x7;
		m_Flags |= 1 << msg;
	}
	if (!(m_Flags & 0x3))
	{
		return 0;
	};
	if (msg < 3) return -1;
	return 1;
}

LPRGBQUAD CPalMapEdit::SDL_ColorsToRGBQUADS(SDL_Color* colo)
{
	static RGBQUAD wColor[256];
	for (int n = 0; n < 256; n++)
	{
		wColor[n].rgbRed = colo[n].r;
		wColor[n].rgbBlue = colo[n].b;
		wColor[n].rgbGreen = colo[n].g;
	}
	return wColor;
}
