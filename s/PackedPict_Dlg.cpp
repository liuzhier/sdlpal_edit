#include "PackedPict_Dlg.h"
#include "resource.h"
#include "sDlg.h"
#include <SDL.h>

static LPCWSTR pObject_Menu_Str[] =
{
	L"存储本组图片到目录",
	L"存储本组图片到RLE",
	L"载入并替换本组图片",
	L"载入图片到尾部",
	nullptr,
};


IMPLEMENT_DYNAMIC(PackedPict_Dlg, CDialogEx)

BEGIN_MESSAGE_MAP(PackedPict_Dlg, CDialogEx)
	ON_COMMAND_RANGE(WM_OBJECT_POPMENU, WM_OBJECT_POPMENU_END, &PackedPict_Dlg::ObjectPopMenuRetuen)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
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
	ON_MESSAGE(WM_LIST_SELECTED_ROW, &PackedPict_Dlg::OnListSelectedRow)
END_MESSAGE_MAP()

void PackedPict_Dlg::DoDataExchange(CDataExchange* pDX)
{
	CMap_Dlg::DoDataExchange(pDX);
}


LRESULT PackedPict_Dlg::OnListSelectedRow(WPARAM wParam, LPARAM lParam)
{
	//列表被选择后触发
	m_ListSelectRow = wParam;
	//以下生成图片列表和动画
	//m_Pict;
	INT len = Pal->PAL_MKFGetDecompressedSize(m_ListSelectRow, f);
	//INT m_itemCont{};
	if (len == 0)
		return 0;

	m_MapZoom = 1.0;
	
	LPSPRITE lpSprite = (LPBYTE)malloc(static_cast<size_t>(len + 2));

	if (Pal->PAL_MKFDecompressChunk(lpSprite, len, m_ListSelectRow, f) > 0)
	{
		m_itemCont = Pal->PAL_SpriteGetNumFrames(lpSprite);
	}
	int magnification;
	if (m_itemCont)
	{
		int maxwidth{}, maxheight{};
		for (int n = 0; n < m_itemCont; n++)
		{
			maxwidth = max(maxwidth, Pal->PAL_RLEGetWidth(Pal->PAL_SpriteGetFrame(lpSprite, n)));
			maxheight = max(maxheight, Pal->PAL_RLEGetHeight(Pal->PAL_SpriteGetFrame(lpSprite, n)));
		}
		if ((UINT)maxwidth > 1024 || (UINT)maxheight > 1024)
			return 0;
		//生成列表
		//初始化图像列表
		if (maxwidth < 80 && maxheight < 80)
			magnification = 0;
		else if(maxwidth < 160 && maxheight < 160)
			magnification = 1;
		else 
			magnification = 2;
		m_MapImage.Destroy();
		if (m_itemCont == 1)
			m_colCont = 1;
		else if (m_itemCont < 16)
			m_colCont = 4;//4*4
		else
			m_colCont = ((m_itemCont + 3) >> 2);//4*n
		m_ItemW = 80 << magnification;
		m_ItemH = 80 << magnification;

		lOriginZoom = m_ItemW * m_colCont / 2048.0;
		
		m_MapImage.Create(m_ItemW << (m_itemCont > 1 ? 2 : 0), -m_ItemH * m_colCont, 24);
		//建立临时表
		CImage m{};
		m.Create(m_MapImage.GetWidth(), m_MapImage.GetHeight(), 8);
		m.SetColorTable(0, 256, SDL_ColorsToRGBQUADS(Pal->gpPalette->colors));
		SDL_Color* pcolor = &Pal->gpPalette->colors[0];
		COLORREF tColor = RGB(pcolor->r, pcolor->g, pcolor->b);
		m.SetTransparentColor(tColor);

		//写临时表数据
		//生成临时表面
		SDL_Surface mSuf{};
		mSuf.w = m.GetWidth();
		mSuf.h = m.GetHeight();
		mSuf.pixels = m.GetBits();
		mSuf.pitch = m.GetPitch();
		for (int n = 0; n < m_itemCont; n++)
		{
			LPCBITMAPRLE lpFrame = Pal->PAL_SpriteGetFrame(lpSprite, n);
			if (!lpFrame)
				continue;
			//完成写入临时表
			DWORD pos = PAL_XY((m_ItemW >> 1) - (Pal->PAL_RLEGetWidth(lpFrame) >> 1) + m_ItemW * (n & 3),
				(m_ItemH >> 1) - (Pal->PAL_RLEGetHeight(lpFrame) >> 1) + m_ItemH * (n >> 2));
			Pal->PAL_RLEBlitToSurface(lpFrame, &mSuf, pos);
		}
		//完成写入主表
		HDC sDC = m_MapImage.GetDC();
		m.Draw(sDC, 0, 0, m.GetWidth(), m.GetHeight());
		//清理
		m_MapImage.ReleaseDC();
		m.Destroy();
	}
	free (lpSprite);
	return 0;
}


void PackedPict_Dlg::ObjectPopMenuRetuen(UINT msg)
{
	msg -= WM_OBJECT_POPMENU;
	CString dir(Pal->PalDir.c_str());
	dir += L"image\\";

	if (TestSaverDir(dir))
		return;

	CString fileName = CString(OpenFileName->lpstrFileTitle).Left(CString(OpenFileName->lpstrFileTitle).GetLength() - 4);
	BOOL isUpdata{};//是否更新
	switch (msg)
	{
	case 0:
	{
		//L"存储本组图片到目录",
		//建立目录
		CString sDir;

		sDir.Format(L"%s%s%3.3d\\", dir.GetString(), fileName.GetString(), 
			m_ListSelectRow);
		if (TestSaverDir(sDir))
			return;

		std::vector<BYTE> vChunk;
		int chunkLen = Pal->PAL_MKFGetDecompressedSize(m_ListSelectRow, f);
		vChunk.resize(chunkLen);
		LPBYTE lpSprite = &vChunk[0];
		Pal->PAL_MKFDecompressChunk(lpSprite, chunkLen, m_ListSelectRow, f);
		for (int n = 0; n < m_itemCont; n++)
		{
			int width = Pal->PAL_RLEGetWidth(Pal->PAL_SpriteGetFrame(lpSprite, n));
			int height = Pal->PAL_RLEGetHeight(Pal->PAL_SpriteGetFrame(lpSprite, n));
			CImage m;
			m.Create(width, height, 8);
			m.SetColorTable(0, 256, SDL_ColorsToRGBQUADS(Pal->gpPalette->colors));
			
		//写临时表数据
		//生成临时表面
			SDL_Surface mSuf{};
			mSuf.w = m.GetWidth();
			mSuf.h = m.GetHeight();
			mSuf.pixels = m.GetBits();
			mSuf.pitch = m.GetPitch();

			LPCBITMAPRLE lpFrame = Pal->PAL_SpriteGetFrame(lpSprite, n);
			Pal->PAL_RLEBlitToSurface(lpFrame, &mSuf, 0);

			CString savename;
			savename.Format(L"%s%s%3.3d.bmp", sDir.GetString(), fileName.GetString(), n);
			m.Save(savename);
		}
		break;
	}
	case 1:
	{
		//L"存储本组图片到RLE"
		std::vector<BYTE> vChunk;
		int chunkLen = Pal->PAL_MKFGetDecompressedSize(m_ListSelectRow, f);
		vChunk.resize(chunkLen);
		LPBYTE lpSprite = &vChunk[0];
		Pal->PAL_MKFDecompressChunk(lpSprite, chunkLen, m_ListSelectRow, f);
		//生成存储文件名
		CString savename;
		savename.Format(L"%s%s%3.3d.RLE",dir.GetString(),fileName.GetString(),m_ListSelectRow);
		string svName = CStringA(savename).GetString();
		if (Pal->IsFileExist(svName.c_str()))
		{
			if (::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd,
				_T("\t继续吗？"), _T("文件已存在"), MB_OKCANCEL) == IDCANCEL)
				return;
		}

		FILE* saveF = fopen(svName.c_str(), "wb");
		fwrite(vChunk.data(), chunkLen, 1, saveF);
		fclose(saveF);
		break;
	}
	case 2:
	{
		//L"载入并替换本组图片"	
		std::vector<WORD> vChunk;

		if(makeChunkfromAll(vChunk))
			return;

		Pal->replaceMKFOne(CStringA(OpenFileName->lpstrFile).GetString(),
			m_ListSelectRow, vChunk.data(), vChunk.size() << 1);
		//PostMessage(WM_CLOSE);
		isUpdata = TRUE;
		break;
	}
	case 3:
	{
		//L"载入图片到尾部"
		std::vector<WORD> vChunk;

		if (makeChunkfromAll(vChunk))
			return;

		Pal->replaceMKFOne(CStringA(OpenFileName->lpstrFile).GetString(),
			65536, vChunk.data(), vChunk.size() << 1);
		//PostMessage(WM_CLOSE);
		isUpdata = TRUE;
		break;
	}
	default:
		break;
	}
	if (isUpdata)
	{
		fclose(f);
		Pal->PAL_ReloadPAL(FALSE);
		f = fopen(CStringA(OpenFileName->lpstrFile).GetString(), "rb");
		makeList();
		PostMessage(WM_LIST_SELECTED_ROW, m_ListSelectRow, 0);
	}
}

void PackedPict_Dlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CDialogEx::OnLButtonDblClk(nFlags, point);
}

PackedPict_Dlg::PackedPict_Dlg(CWnd* pParent,OPENFILENAME * filename)
	: CMap_Dlg( pParent),
	OpenFileName(filename),
	m_Parent(pParent)
{
	Pal = ((CsDlg*)pParent)->Pal;
	if (Pal)
		OldCompressMode = Pal->gConfig->fisUSEYJ1DeCompress;
	//打开文件
	string opfn = CStringA(OpenFileName->lpstrFile).GetString();
	f = fopen(opfn.c_str(), "rb");
}

PackedPict_Dlg::~PackedPict_Dlg()
{
	//关闭文件
	if (f)
		fclose(f);
	f = nullptr;
	Pal->gConfig->fisUSEYJ1DeCompress = OldCompressMode;
	//Pal->PAL_ReloadPAL(FALSE);
}

void PackedPict_Dlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialogEx::OnPaint()

	if (!m_Map.GetSafeHwnd() || !m_MiniMap.GetSafeHwnd())
		return;
	if (m_MapImage.IsNull())
		return;
	//生成中间表
	CDC* pDC = m_Map.GetDC();
	HDC  dDC = pDC->GetSafeHdc();
	HDC  sDC = m_MapImage.GetDC();
	RECT dRect;
	//大图
	m_Map.GetWindowRect(&dRect);
	m_Map.ScreenToClient(&dRect);
	//中间表
	CImage midImage;
	midImage.Create(dRect.right, dRect.bottom, 24);
	HDC midDC = midImage.GetDC();
	INT xOrigin = lxOrigin * m_MapImage.GetWidth() / 1000;
	INT yOrigin = lyOrigin * m_MapImage.GetWidth() / 1000;
	RECT sRect = { xOrigin , yOrigin , m_MapImage.GetWidth() / m_MapZoom + xOrigin,
	m_MapImage.GetHeight() / m_MapZoom + yOrigin };
	//大地图表向中间表写
	m_MapImage.Draw(midDC, dRect, sRect);
	//将中间表画到屏幕上
	::BitBlt(dDC, 0, 0, dRect.right, dRect.bottom, midDC, 0, 0, SRCCOPY);
	midImage.ReleaseDC();
	midImage.Destroy();
	ReleaseDC(pDC);
	//生成小图
	int itemN = (m_Times >> 2) % m_itemCont;

	sRect = { (itemN & 0x3) * m_ItemW,(itemN >> 2) * m_ItemH,
		m_ItemW,m_ItemH };

	pDC = m_MiniMap.GetDC();
	dDC = pDC->GetSafeHdc();
	m_MiniMap.GetWindowRect(&dRect);
	m_MiniMap.ScreenToClient(&dRect);
	::StretchBlt(dDC, 4, 4, dRect.right - 8, dRect.bottom - 8,
		sDC, sRect.left, sRect.top,
		sRect.right, sRect.bottom, SRCCOPY);

	m_MapImage.ReleaseDC();
	ReleaseDC(pDC);
}

void PackedPict_Dlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	if (isMousePtIn(point, (CWnd*)&m_Map))
	{
		//MapSetPoint(point);
	}
	CDialogEx::OnLButtonDown(nFlags, point);
}

void PackedPict_Dlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	if (isMousePtIn(point, (CWnd*)&m_Map) && m_itemCont)
	{
		//生成菜单
		CMenu pMenu;
		pMenu.CreatePopupMenu();
		for (UINT_PTR n = 0; pObject_Menu_Str[n]; n++)
		{
			UINT32 flag{ MF_STRING };
			CString s;
			s.Format(pObject_Menu_Str[n]);
			pMenu.AppendMenu(flag, WM_OBJECT_POPMENU + n, s);
		}
		SetForegroundWindow();
		pMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
	}
	CDialogEx::OnRButtonDown(nFlags, point);
}

void PackedPict_Dlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nChar == VK_ESCAPE || nChar == VK_RETURN)
	{
		PostMessage(WM_CLOSE);
	}
	CDialogEx::OnKeyDown(nChar,nRepCnt,nFlags);
}

BOOL PackedPict_Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CString s_Str;
	s_Str.Format(L"图像编辑  %s", OpenFileName->lpstrFile);
	SetWindowText(s_Str.GetBuffer());

	// TODO:  在此添加额外的初始化
	if (!f || !Pal)
	{
		//AfxGetApp;
		PostMessage(WM_CLOSE);
		CString* ps = new CString(L"文件打开错，返回");
		AfxGetMainWnd()->PostMessage(WM_POST_MESSAGE_CSTR,0,(LPARAM)ps);
		return FALSE;
	}
	//以下检测压缩属性
	{
		INT isYJ_1 = isUseYJ_1(f);
		if (isYJ_1 == -1)
			return FALSE;//错误返回
		Pal->gConfig->fisUSEYJ1DeCompress = isYJ_1;
	}

	m_TextStatic.SetWindowText(OpenFileName->lpstrFile);
	RECT s_Rect{};
	GetParent()->GetWindowRect(&s_Rect);

	//以下生成列表
	if (makeList())
		return 0;;

	MoveWindow(&s_Rect, TRUE);
	SetTimer(1, 100, NULL);
	Pal->gpPalette->colors = Pal->PAL_GetPalette(0, 0);

	SCROLLINFO scroll;
	m_vScrollBar.GetScrollInfo(&scroll);
	scroll.nMax = 1024;
	scroll.nPos = 1000;
	m_vScrollBar.SetScrollInfo(&scroll);
	m_hScrollBar.SetScrollInfo(&scroll);
	m_ListSelectRow = 1;
	PostMessage(WM_LIST_SELECTED_ROW, m_ListSelectRow, 0);
	return TRUE;
}
//检查存储图像文件目录dir，不存在建立之， 返回 0 成功 其他失败
BOOL PackedPict_Dlg::TestSaverDir(CString & dir)
{
	string sDir;
	sDir = CStringA(dir).GetString();
	if (!Pal->IsDirExist(sDir))
	{
		//建立目录
		if (CreateDirectory(dir, 0) == 0)
		{
			PostMessage(WM_CLOSE);
			CString* ps = new CString(L"无法建立目录，返回");
			AfxGetMainWnd()->PostMessage(WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
			return 1;
		}
	}
	return 0;
}

//功能生成图像压缩片断
// 输入：压缩片断存储结构引用，打开文件结构指针
// 返回 成功返回0 不成功返回其他 成功时返回值在chunk中。
BOOL PackedPict_Dlg::makeChunkfromAll(std::vector<WORD>& chunk, std::vector<WORD>* mapChunk)
{
	//取纯文件名-不包括目录
	CString openFileN(OpenFileName->lpstrFileTitle);
	openFileN = openFileN.Left(openFileN.GetLength() - 4).MakeUpper();

	INT flag{};//1位是否用RLE压缩，0位是是否多个文件用目录表示，2位是否使用yj1压缩
	if (openFileN == CString(L"FBP"))
		flag = 0b0100;//rj1 or rj2
	else if (openFileN == CString(L"MGO") || openFileN == CString(L"ABC")
		|| (openFileN == CString(L"F")) || (openFileN == CString(L"FIRE")))
		flag = 0b0111;
	else if (openFileN == CString(L"RGM") || (openFileN == CString(L"BALL")))
		flag = 0b0010;
	else if (openFileN == CString(L"GOP")||(openFileN == CString(L"MAP")))
	{
		flag = 0b011;
		openFileN = L"GOP";
		if (mapChunk == NULL)
			return 1;
	}
	//SDL_Surface* m{};
	CString mDir;
	mDir.Format(L"%simage\\", OpenFileName->lpstrInitialDir);

	CString saveFilter;
	saveFilter.Format(L" 图片%s |%s???.BMP| RLE文件 |%s???.RLE|||",
		openFileN.GetString(), openFileN.GetString(), openFileN.GetString());
	CFileDialog loadFile(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		saveFilter.GetString());
	loadFile.m_pOFN->lpstrInitialDir = mDir.GetString();
	if (loadFile.DoModal() != IDOK)
		return 1;
	CString mapDir;
	if (openFileN ==CString( L"GOP"))
	{
		CString d1 = loadFile.GetPathName();
		//提取目录
		d1 = d1.Left(d1.GetLength()-10);
		mapDir = CString(loadFile.GetPathName()).Right(17).Left(6);
		mapDir = d1 + mapDir + L".rle";
		string sPath = CStringA(mapDir).GetString();
		FILE* rlefile{};
		if (fopen_s(&rlefile,sPath.c_str(), "rb"))
			return 1;
		fseek(rlefile, 0, SEEK_END);
		size_t len = ftell(rlefile);
		fseek(rlefile, 0, SEEK_SET);
		//处理压缩
		vector<BYTE> vChunk(len);
		//读取
		fread(&vChunk[0], len, 1, rlefile);
		fclose(rlefile);
		UINT32 compressLen{};
		VOID* s{};
		if (Pal->EnCompress(vChunk.data(), vChunk.size(), s, compressLen, 0))
			return 1;
		mapChunk->resize(compressLen >> 1);
		memcpy(mapChunk->data(), s, compressLen);
		free(s);
	}
	if (loadFile.m_pOFN->nFilterIndex == 2)
	{
		//RLE file
		FILE* rlefile = fopen(CStringA(loadFile.m_pOFN->lpstrFile).GetString(), "rb");
		if (!rlefile)
			return 1;
		fseek(rlefile, 0, SEEK_END);
		size_t len = ftell(rlefile);
		fseek(rlefile, 0, SEEK_SET);
		if (flag & 0b0100)//RLE file
		{
			//处理压缩
			vector<BYTE> vChunk(len);
			//读取
			fread(&vChunk[0], len, 1, rlefile);
			fclose(rlefile);
			UINT32 compressLen{};
			VOID* s{};
			if (Pal->EnCompress(vChunk.data(), vChunk.size(), s, compressLen, 0))
				return 1;
			chunk.resize(compressLen >> 1);
			memcpy(chunk.data(), s, compressLen);
			free(s);
			return 0;
		}
		else
		{
			//直接读取文件返回
			chunk.resize(len);
			fread(&chunk[0], len, 1, rlefile);
			fclose(rlefile);
			return 0;
		}
	}

	vector<WORD> vSprite(0xffff);//可能容纳的最大值
	if (flag & 1)//处理多个图形文件
	{
		CString cFilename(loadFile.m_pOFN->lpstrFile);
		WCHAR wdir[256]{};
		_wsplitpath(cFilename.GetString(), NULL, wdir, NULL, NULL);
		mDir = cFilename.Left(cFilename.GetLength() - 7);
		CString filter = mDir + L"???.bmp";
		CFileDir find{};
		int iCount = find.getFileCount(filter);
		//int iCount = _wtoi(CString(wdir).TrimRight('\\').Right(3).GetString());
		//多个图形文件必然使用RLE压缩
		DWORD iOffset = ((size_t)(iCount + 1));
		SDL_Surface* m{};
		for (int n = 0; n < iCount; n++)
		{
			cFilename.Format(L"%s%3.3d.BMP", mDir.GetString(), n);
			string sFileName(CStringA(cFilename).GetString());
			if ((m = SDL_LoadBMP(sFileName.c_str())) == NULL)
			{
				return 1;
			}
			int len{};
			LPVOID buf{};
			if (Pal->EncodeRLE(m->pixels, 0, m->pitch, m->w, m->h, buf, len))
				return 1;
			memcpy(&vSprite[iOffset], buf, len);
			vSprite[n] = iOffset;

			iOffset += len >> 1;
			vSprite[n + 1] = iOffset;

			free(buf);
			SDL_FreeSurface(m);
		}

		vSprite.resize(iOffset);
	}

	else//处理单个图形文件
	{
		SDL_Surface* m{};
		CString cFilename(loadFile.m_pOFN->lpstrFile);
		m = SDL_LoadBMP(CStringA(cFilename).GetString());
		if (m == NULL)
			return 1;
		if (flag & 0b010)
		{
			//使用RLE压缩
			int len{};
			LPVOID buf{};
			if (Pal->EncodeRLE((LPCBYTE)m->pixels, 0, m->pitch, m->w, m->h, buf, len))
				return 1;
			vSprite.resize(len);
			memcpy(vSprite.data(), buf, len);
			free(buf);
		}
		else
		{
			//不使用RLE压缩
			DWORD len = m->h * m->pitch;
			vSprite.resize(len >> 1);
			memcpy(vSprite.data(), m->pixels, len);
		}
		SDL_FreeSurface(m);
		m = NULL;
	}

	//以下处理压缩或不压缩
	if (flag & 0b0100)
	{
		//压缩
		LPVOID buf{};
		UINT32 len{};
		if (Pal->EnCompress(vSprite.data(), vSprite.size() << 1, buf, len, 0))
			return 1;
		chunk.resize(len);
		memcpy(chunk.data(), buf, len);
		free(buf);
		return 0;
	}
	else
	{
		//不压缩
		chunk.swap(vSprite);
		return 0;
	}
	return 1;
}

//建立列表，成功返回0
BOOL PackedPict_Dlg::makeList()
{

	//以下生成列表
	INT i = Pal->PAL_MKFGetChunkCount(f);
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
	m_List.InsertColumn(1, L"序号", 0, 60);
	m_List.InsertColumn(2, L"图片数", 0, 60);
	m_List.InsertColumn(3, L"最大宽", 0, 60);
	m_List.InsertColumn(4, L"最大高", 0, 60);

	for (int n = 0; n < i; n++)
	{
		int len = Pal->PAL_MKFGetDecompressedSize(n, f);
		int cont{};
		int width{};
		int height{};
		if (len < 0)
		{
			PostMessage(WM_CLOSE);
			CString* ps = new CString(L"没有图像，返回");
			AfxGetMainWnd()->PostMessage(WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
			return 1;
		}
		LPBYTE buf = (LPBYTE)malloc(static_cast<size_t>(len + 2));

		if (Pal->PAL_MKFDecompressChunk(buf, len, n, f) > 0)
		{
			cont = Pal->PAL_SpriteGetNumFrames(buf);
			LPCBITMAPRLE lpBitmap = Pal->PAL_SpriteGetFrame(buf, 0);
			width = Pal->PAL_RLEGetWidth(lpBitmap);
			height = Pal->PAL_RLEGetHeight(lpBitmap);
			int maxwidth = width, maxheight = height;
			for (int i = 0; i < cont; i++)
			{
				maxwidth = max(maxwidth, Pal->PAL_RLEGetWidth(Pal->PAL_SpriteGetFrame(buf, i)));
				maxheight = max(maxheight, Pal->PAL_RLEGetHeight(Pal->PAL_SpriteGetFrame(buf, i)));
			}
			width = maxwidth;
			height = maxheight;
		}
		CString strText;
		m_List.InsertItem(n, L"");
		strText.Format(_T("%4.4d"), n);
		m_List.SetItemText(n, 1, strText);
		strText.Format(_T("%4d"), cont);
		m_List.SetItemText(n, 2, strText);
		strText.Format(_T("%4d"), width);
		m_List.SetItemText(n, 3, strText);
		strText.Format(_T("%4d"), height);
		m_List.SetItemText(n, 4, strText);
		free(buf);
	}
	return 0;
}


