#include "MapPict_Dlg.h"
#include "CGetPalData.h"
#include "CPalMapEdit.h"

IMPLEMENT_DYNAMIC(MapPict_Dlg, CDialogEx)

BEGIN_MESSAGE_MAP(MapPict_Dlg, CDialogEx)
	ON_COMMAND_RANGE(WM_OBJECT_POPMENU, WM_OBJECT_POPMENU_END, &MapPict_Dlg::ObjectPopMenuRetuen)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_GETMINMAXINFO()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_MESSAGE(WM_LIST_SELECTED_ROW, &MapPict_Dlg::OnListSelectedRow)
END_MESSAGE_MAP()

static LPCWSTR pMap_Menu_Str[] =
{
	L"显示底层",
	L"显示上层",
	L"显示障碍",
	L"存储本组图片到目录",
	L"存储本组图片到RLE",
	L"载入并替换本组图片",
	L"载入图片到尾部",
	nullptr
};

void MapPict_Dlg::DoDataExchange(CDataExchange* pDX)
{
	PackedPict_Dlg::DoDataExchange(pDX);
}

BOOL MapPict_Dlg::makeList()
{
	if (fpMAP == NULL || fpGOP == NULL)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}
	INT i = Pal->PAL_MKFGetChunkCount(fpMAP);
	m_itemCont = i;
	if (i <= 0)
	{
		PostMessage(WM_CLOSE);
		CString* ps = new CString(L"没有图像，返回");
		AfxGetMainWnd()->PostMessage(WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
		return FALSE;
	}
	m_List.DeleteAllItems();
	m_List.SetExtendedStyle(//m_ListCtrl.GetExtendedStyle() |
		LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_NOSORTHEADER);
	m_List.ModifyStyle(0, LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS);
	//生成列表
	m_List.InsertColumn(0, L"", 0, 0);
	m_List.InsertColumn(1, L"地图号", 0, 90);
	m_List.InsertColumn(2, L"有效", 0, 90);

	for (int n = 0; n < i; n++)
	{
		CString strText;
		m_List.InsertItem(n, L"");
		strText.Format(_T("%4d"), n);
		m_List.SetItemText(n, 1, strText);
		int len = Pal->PAL_MKFGetChunkSize(n, fpMAP);
		strText.Format(_T("%s"), len ? L"Yes" : L"No");
		m_List.SetItemText(n, 2, strText);
	}
	//以下建立相关数据结构
	pMapClass = new CPalMapEdit(this);

	return LRESULT();;
}

MapPict_Dlg::MapPict_Dlg(CWnd* pParent, OPENFILENAME* openfilename)
	: PackedPict_Dlg(pParent, openfilename)
{
	//打开地图文件
	fopen_s(&fpMAP,CStringA(openfilename->lpstrFile).GetString(), "rb");
	CString gopFN(openfilename->lpstrFile);
	gopFN = gopFN.Left(gopFN.GetLength() - 7) + L"gop.mkf";
	fopen_s(&fpGOP, CStringA(gopFN).GetString(), "rb");
	m_MapZoom = 1.05;
	m_Flags = 0b0111;
}

MapPict_Dlg::~MapPict_Dlg()
{
	if (fpMAP)
		fclose(fpMAP);
	if (fpGOP)
		fclose(fpGOP);
	if (pMapClass)
		delete pMapClass;
}

LRESULT MapPict_Dlg::OnListSelectedRow(WPARAM wParam, LPARAM lParam)
{
	//列表被选择后触发
	m_ListSelectRow = wParam;
	//以下生成图片列表和动画
	m_MapImage.Destroy();
	m_nMap = wParam;

	for (size_t i = 0; i < 512; i++)
	{
		if (pMapClass->m_pImageList[i]) delete pMapClass-> m_pImageList[i];
		pMapClass->m_pImageList[i] = nullptr;
	}

	int mLen = Pal->PAL_MKFGetChunkSize(m_nMap, fpMAP);
	int gLen = Pal->PAL_MKFGetChunkSize(m_nMap, fpGOP);
	if (mLen <= 0 || gLen <= 0)
	{
		return -1;
	}

	if (pMapClass-> pTileSprite)
		free(pMapClass->pTileSprite);
	pMapClass-> pTileSprite = NULL;
	LPBYTE buf = (LPBYTE)malloc(mLen);
	pMapClass->pTileSprite = (LPBYTE)malloc(gLen);

	if (pMapClass-> pTileSprite == nullptr || buf == nullptr)
	{
		return -1;
	}


	Pal->PAL_MKFReadChunk(buf, mLen, m_ListSelectRow, fpMAP);
	Pal->PAL_DeCompress(buf, (LPBYTE)(pMapClass->Tiles), sizeof(pMapClass->Tiles));
	free(buf);

	Pal->PAL_MKFReadChunk((LPBYTE)pMapClass->pTileSprite, gLen, m_ListSelectRow, fpGOP);

	//pMapClass->setTileImage();

	pMapClass->MakeMapImage(m_MapImage, m_Flags);

	return 0;
}

void MapPict_Dlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	if (isMousePtIn(point, (CWnd*)&m_Map) && m_itemCont)
	{
		//生成菜单
		CMenu pMenu;
		pMenu.CreatePopupMenu();
		for (UINT_PTR n = 0; pMap_Menu_Str[n]; n++)
		{
			//UINT32 flag{ MF_STRING };
			CString s;
			s.Format(pMap_Menu_Str[n]);
			UINT32 flag = MF_STRING | ((m_Flags & (1 << n)) ? MF_CHECKED : MF_UNCHECKED);
			pMenu.AppendMenu(flag, WM_OBJECT_POPMENU + n, s);
		}
		SetForegroundWindow();
		pMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
	}
	CDialogEx::OnRButtonDown(nFlags, point);

}

void MapPict_Dlg::ObjectPopMenuRetuen(UINT msg)
{
	msg -= WM_OBJECT_POPMENU;
	INT isUpdata{};
	CString dir(Pal->PalDir.c_str());
	dir += L"image\\";

	if (TestSaverDir(dir))
		return;
	DWORD oldFlags = m_Flags;

	CString fileName = CString(OpenFileName->lpstrFileTitle).Left(CString(OpenFileName->lpstrFileTitle).GetLength() - 4);

	switch (msg)
	{
	case 0:
	case 1:
	case 2:
		m_Flags ^= (1 << msg);
		if ((m_Flags ^ 0b0111) == 0b0111)
		{
			m_Flags = oldFlags;
		}
		//刷新
		PostMessage(WM_LIST_SELECTED_ROW, m_ListSelectRow, 0);
		break;
	case 3:
	{
		//L"存储本组图片到目录",
		INT m_ImageCount = *((WORD*)pMapClass->pTileSprite) - 1;
		string sDir = Pal->va("%simage\\map%3.3d\\",Pal->PalDir.c_str(),m_ListSelectRow);
		if (TestSaverDir(CString(sDir.c_str())))
			return;
		//存储地图数据
		string mapRLEname = sDir + Pal-> va("map%3.3d.rle", m_ListSelectRow);;
		FILE* f{};
		if (fopen_s(&f, mapRLEname.c_str(), "wb"))
			return;
		//数据已经读入相关结构
		fwrite(pMapClass->Tiles, sizeof(pMapClass->Tiles), 1, f);
		fclose(f);

		SDL_Surface* m = SDL_CreateRGBSurface(0, 32, 15, 8, 0, 0, 0, 0);
		if (m == NULL)
			return;
		SDL_SetSurfacePalette(m, Pal->gpPalette);
		auto lpSprite = (LPBYTE)pMapClass->pTileSprite;
		for (int n = 0; n < m_ImageCount; n++)
		{
			int width = Pal->PAL_RLEGetWidth(Pal->PAL_SpriteGetFrame(lpSprite, n));
			int height = Pal->PAL_RLEGetHeight(Pal->PAL_SpriteGetFrame(lpSprite, n));
			if (width != 32 || height != 15)
				return;
			//解压，写图像文件
			SDL_FillRect(m, 0, 0);//用0颜色覆盖
			Pal->PAL_RLEBlitToSurface(Pal->PAL_SpriteGetFrame(lpSprite, n), m, 0);
			string savename;
			savename = sDir + Pal->va("gop%3.3d.bmp", n);
			SDL_SaveBMP(m, savename.c_str());
		}
		SDL_FreeSurface(m);
		break;
	}
	case 4:
	{
		//L"存储本组图片到RLE",
		INT m_ImageCount = *((WORD*)pMapClass->pTileSprite) - 1;
		string sDir = Pal->va("%simage\\map%3.3d\\", Pal->PalDir.c_str(), m_ListSelectRow);
		if (TestSaverDir(CString(sDir.c_str())))
			return;
		//存储地图数据
		string mapRLEname = sDir + Pal->va("map%3.3d.rle", m_ListSelectRow);
		FILE* f{};
		if (fopen_s(&f, mapRLEname.c_str(), "wb"))
			return;
		//数据已经读入相关结构
		fwrite(pMapClass->Tiles, sizeof(pMapClass->Tiles), 1, f);
		fclose(f);

		mapRLEname = sDir + Pal->va("gop%3.3d.rle",m_ListSelectRow);
		if (fopen_s(&f, mapRLEname.c_str(), "wb"))
			return;
		//数据已经读入相关结构
		int gLen = Pal->PAL_MKFGetChunkSize(m_ListSelectRow, fpGOP);
		Pal->PAL_MKFReadChunk((LPBYTE)pMapClass->pTileSprite, gLen, m_ListSelectRow, fpGOP);

		fwrite(pMapClass->pTileSprite, gLen, 1, f);
		fclose(f);
		break;
	}
	case 5:
		//L"载入并替换本组图片",
	{
		//map数据
		vector<WORD> gopChunt;
		//取gop.数据
		vector<WORD> mapChunk;
		if (makeChunkfromAll(gopChunt,&mapChunk))
			return;
		//更新map文件
		Pal->replaceMKFOne(CStringA(OpenFileName->lpstrFile).GetString(),
			m_ListSelectRow, mapChunk.data(), mapChunk.size() << 1);
		//更新GOP文件
		string gopName = CStringA(OpenFileName->lpstrFile).GetString();
		gopName.replace(gopName.size() - 7, gopName.size(), "gop.mkf");
		Pal->replaceMKFOne(gopName.c_str(),
			m_ListSelectRow, gopChunt.data(), gopChunt.size() << 1);
		isUpdata = TRUE;
		break;
	}
	case 6:
		//L"载入图片到尾部",
	{
		//map数据
		vector<WORD> gopChunt;
		//取gop.数据
		vector<WORD> mapChunk;
		if (makeChunkfromAll(gopChunt, &mapChunk))
			return;
		//更新map文件
		Pal->replaceMKFOne(CStringA(OpenFileName->lpstrFile).GetString(),
			m_itemCont, mapChunk.data(), mapChunk.size() << 1);
		//更新GOP文件
		string gopName = CStringA(OpenFileName->lpstrFile).GetString();
		gopName.replace(gopName.size() - 7, gopName.size(), "gop.mkf");
		Pal->replaceMKFOne(gopName.c_str(),
			m_itemCont, gopChunt.data(), gopChunt.size() << 1);
		isUpdata = TRUE;
		break;
	}
	default:
		break;
	}
	if (isUpdata)
	{
		if (fpMAP)
			fclose(fpMAP);
		if (fpGOP)
			fclose(fpGOP);
		if (f)
			fclose(f);
		if (pMapClass)
			delete pMapClass;
		f = fpMAP = fpGOP = NULL;
		pMapClass = NULL;
		Pal->PAL_ReloadPAL(FALSE);
		PostMessage(WM_LIST_SELECTED_ROW, m_ListSelectRow, 0);

		fopen_s(&fpMAP, CStringA(OpenFileName->lpstrFile).GetString(), "rb");
		CString gopFN(OpenFileName->lpstrFile);
		gopFN = gopFN.Left(gopFN.GetLength() - 7) + L"gop.mkf";
		fopen_s(&fpGOP, CStringA(gopFN).GetString(), "rb");
		//更新列表
		makeList();
	}
}
