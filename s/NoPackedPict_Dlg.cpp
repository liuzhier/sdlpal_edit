#include "NoPackedPict_Dlg.h"
#include "resource.h"
#include "sDlg.h"

static LPCWSTR pObject_Menu_Str[] =
{
	L"存储本图片到文件",
	L"载入并替换本图片",
	L"载入图片到末尾",
	nullptr,
};


IMPLEMENT_DYNAMIC(NoPackedPict_Dlg, CDialogEx)

BEGIN_MESSAGE_MAP(NoPackedPict_Dlg, CDialogEx)
	ON_COMMAND_RANGE(WM_OBJECT_POPMENU, WM_OBJECT_POPMENU_END, &NoPackedPict_Dlg::ObjectPopMenuRetuen)
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
	ON_MESSAGE(WM_LIST_SELECTED_ROW, &NoPackedPict_Dlg::OnListSelectedRow)
END_MESSAGE_MAP()

void NoPackedPict_Dlg::DoDataExchange(CDataExchange* pDX)
{
	PackedPict_Dlg::DoDataExchange(pDX);
}


LRESULT NoPackedPict_Dlg::OnListSelectedRow(WPARAM wParam, LPARAM lParam)
{
	//列表被选择后触发
	m_ListSelectRow = wParam;
	//以下生成图片列表和动画
	//m_Pict;

	m_MapZoom = 1.0;
	m_MapImage.Destroy();
	INT(CPalApp:: * getFileChunk)(LPBYTE lpBuffer, UINT uiBufferSize, UINT  uiChunkNum, FILE * fp);
	INT(CPalApp:: * getFileChunkSize)(UINT uiChunkNum, FILE * fp);

	if (CString(OpenFileName->lpstrFileTitle).MakeUpper() == CString(L"FBP.MKF"))
	{
		getFileChunk = &CPalApp::PAL_MKFDecompressChunk;
		getFileChunkSize = &CPalApp::PAL_MKFGetDecompressedSize;

		int chunklen = (Pal->*getFileChunkSize)(m_ListSelectRow, f);
		if (chunklen != 64000)
			return 0;
		m_MapImage.Create(320, -200, 8);
		//建立临时表
		CImage &m = m_MapImage;
		m.SetColorTable(0, 256, SDL_ColorsToRGBQUADS(Pal->gpPalette->colors));
		SDL_Color* pcolor = &Pal->gpPalette->colors[0];
		COLORREF tColor = RGB(pcolor->r, pcolor->g, pcolor->b);
		m.SetTransparentColor(tColor);
		LPBYTE chunk = (LPBYTE)m.GetBits();
		(Pal->*getFileChunk)(chunk, 64000, m_ListSelectRow, f);
	}
	else
	{
		getFileChunk = &CPalApp::PAL_MKFReadChunk;
		getFileChunkSize = &CPalApp::PAL_MKFGetChunkSize;
		int chunklen = (Pal->*getFileChunkSize)(m_ListSelectRow, f);
		if (chunklen <= 0)
			return 0;
		std::vector<BYTE> vChunt;
		vChunt.resize(chunklen);
		LPBYTE chunk = &vChunt[0];
		if ((Pal->*getFileChunk)(chunk, chunklen, m_ListSelectRow, f) <= 0)
			return 0;

		int width{};
		int height{};
		
		width = Pal->PAL_RLEGetWidth(chunk);
		height = Pal->PAL_RLEGetHeight(chunk);
		m_MapImage.Create(width, -height, 8);
		//临时表
		CImage &m = m_MapImage;
		m.SetColorTable(0, 256, SDL_ColorsToRGBQUADS(Pal->gpPalette->colors));
		SDL_Color* pcolor = &Pal->gpPalette->colors[0];
		COLORREF tColor = RGB(pcolor->r, pcolor->g, pcolor->b);
		m.SetTransparentColor(tColor);
		//生成SDL临时表面
		SDL_Surface mSuf{};
		mSuf.w = m.GetWidth();
		mSuf.h = m.GetHeight();
		mSuf.pixels = m.GetBits();
		mSuf.pitch = m.GetPitch();
		Pal->PAL_RLEBlitToSurface(chunk, &mSuf, 0);
	}

	return 0;
}


void NoPackedPict_Dlg::ObjectPopMenuRetuen(UINT msg)
{
	msg -= WM_OBJECT_POPMENU;
	CString dir(Pal->PalDir.c_str());
	dir += L"image\\";

	if (TestSaverDir(dir))
		return;
	BOOL isUpdata{};
	switch (msg)
	{
	case 0:
	{
		//L"存储本图片到文件",
		CString filename;
		filename.Format(L"%s%s%3.3d%s", dir.GetBuffer(),
			CString(OpenFileName->lpstrFileTitle).Left(CString(OpenFileName->lpstrFileTitle).GetLength() - 4),
			m_ListSelectRow, L".bmp");
		string sDir = CStringA(filename).GetString();
		if (Pal->IsFileExist(sDir))
		{
			if (::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd,
				_T("\t继续吗？"), _T("文件已存在"), MB_OKCANCEL) == IDCANCEL)
				return;
		}
		SDL_Surface* m{};
		if (CString(OpenFileName->lpstrFileTitle).MakeUpper() == CString(L"FBP.MKF"))
		{
			m = SDL_CreateRGBSurface(0, 320, 200, 8, 0, 0, 0, 0);
			if (m == NULL)
				return;
			SDL_SetSurfacePalette(m, Pal->gpPalette);
			Pal->PAL_MKFDecompressChunk((LPBYTE)m->pixels, 320 * 200, m_ListSelectRow, f);
			SDL_SaveBMP(m,sDir.c_str());
			SDL_FreeSurface(m);
		}
		else
		{
			int chunklen = (Pal->PAL_MKFGetChunkSize(m_ListSelectRow, f));
			if (chunklen <= 0)
				return ;
			std::vector<BYTE> vChunt;
			vChunt.resize(chunklen);
			LPBYTE chunk = vChunt.data();
			Pal->PAL_MKFReadChunk(chunk, chunklen, m_ListSelectRow, f);
			int width{};
			int height{};

			width = Pal->PAL_RLEGetWidth(chunk);
			height = Pal->PAL_RLEGetHeight(chunk);
			SDL_Surface* m{};
			m = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
			SDL_SetSurfacePalette(m, Pal->gpPalette);
			Pal->PAL_RLEBlitToSurface(chunk, m, 0);
			SDL_SaveBMP(m, sDir.c_str());
			SDL_FreeSurface(m);
		}
		CString* ps = new CString;
		ps->Format(L"已经建立文件 %s,", filename.GetBuffer());
		AfxGetMainWnd()->PostMessage(WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
		break;
	}
	case 1:
	{
		//L"载入图片替换本图片"
		vector<WORD> vChunt;
		if (makeChunkfromAll(vChunt))
			return;

		Pal->replaceMKFOne(CStringA(OpenFileName->lpstrFile).GetString(),
			m_ListSelectRow, vChunt.data(), vChunt.size() << 1);
		
		//PostMessage(WM_CLOSE);
		isUpdata = TRUE;
		break;
	}
	case 2:
	{
		//L"载入图片追加图片"
		vector<WORD> vChunt;
		if (makeChunkfromAll(vChunt))
			return;

		Pal->replaceMKFOne(CStringA(OpenFileName->lpstrFile).GetString(),
			65536, vChunt.data(), vChunt.size() << 1);
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


//生成列表，成功返回 0
BOOL NoPackedPict_Dlg::makeList()
{
	INT i = Pal->PAL_MKFGetChunkCount(f);
	if (i <= 0)
	{
		PostMessage(WM_CLOSE);
		CString* ps = new CString(L"没有图像，返回");
		AfxGetMainWnd()->PostMessage(WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
		return TRUE;
	}

	m_List.DeleteAllItems();
	m_List.SetExtendedStyle(//m_ListCtrl.GetExtendedStyle() |
		LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_NOSORTHEADER);
	m_List.ModifyStyle(0, LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS);
	INT(CPalApp:: * getFileChunk)(LPBYTE lpBuffer, UINT uiBufferSize, UINT  uiChunkNum, FILE * fp);
	INT(CPalApp:: * getFileChunkSize)(UINT uiChunkNum, FILE * fp);
	if (CString(OpenFileName->lpstrFileTitle).MakeUpper() == CString(L"FBP.MKF"))
	{
		getFileChunk = &CPalApp::PAL_MKFDecompressChunk;
		getFileChunkSize = &CPalApp::PAL_MKFGetDecompressedSize;
	}
	else
	{
		getFileChunk = &CPalApp::PAL_MKFReadChunk;
		getFileChunkSize = &CPalApp::PAL_MKFGetChunkSize;
	}

	//生成列表
	m_List.InsertColumn(0, L"", 0, 0);
	m_List.InsertColumn(1, L"序号", 0, 60);
	m_List.InsertColumn(2, L"宽", 0, 60);
	m_List.InsertColumn(3, L"高", 0, 60);
	for (int n = 0; n < i; n++)
	{
		int len = (Pal->*getFileChunkSize)(n, f);
		if (len < 0)
		{
			PostMessage(WM_CLOSE);
			CString* ps = new CString(L"没有图像，返回");
			AfxGetMainWnd()->PostMessage(WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
			return FALSE;
		}
		LPBYTE buf = (LPBYTE)malloc(static_cast<size_t>(len));

		int width{};
		int height{};

		if ((Pal->*getFileChunk)(buf, len, n, f) > 0)
		{
			if (len < 64000)
			{
				width = Pal->PAL_RLEGetWidth(buf);
				height = Pal->PAL_RLEGetHeight(buf);
			}
			else
			{
				width = 320;
				height = 200;
			}
		}
		CString strText;
		m_List.InsertItem(n, L"");
		strText.Format(_T("%4.4d"), n);
		m_List.SetItemText(n, 1, strText);
		strText.Format(_T("%4d"), width);
		m_List.SetItemText(n, 2, strText);
		strText.Format(_T("%4d"), height);
		m_List.SetItemText(n, 3, strText);
		free(buf);
	}

	return 0;
}

NoPackedPict_Dlg::NoPackedPict_Dlg(CWnd* pParent, OPENFILENAME* filename)
	: PackedPict_Dlg(pParent,filename)
{
	Pal = ((CsDlg*)pParent)->Pal;
	if (Pal)
		OldCompressMode = Pal->gConfig->fisUSEYJ1DeCompress;
}

NoPackedPict_Dlg::~NoPackedPict_Dlg()
{
	if (f)
		fclose(f);
	f = nullptr;
	Pal->gConfig->fisUSEYJ1DeCompress = OldCompressMode;
	//Pal->PAL_ReloadPAL(FALSE);
}

void NoPackedPict_Dlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialogEx::OnPaint()

	if (!m_Map.GetSafeHwnd() || !m_MiniMap.GetSafeHwnd())
		return;

	RECT dRect;
	m_Map.GetWindowRect(&dRect);
	m_Map.ScreenToClient(&dRect);

	CDC* pDC = m_Map.GetDC();
	HDC  dDC = pDC->GetSafeHdc();

	FillRect(dDC, &dRect, CBrush(RGB(1, 1, 1)));

	if (m_MapImage.IsNull())
	{
		ReleaseDC(pDC);
		return;
	}

	//大图

	RECT sRect{ 0,0,m_MapImage.GetWidth(),m_MapImage.GetHeight() };
	RECT dsRect{ 0,0,dRect.right / m_MapZoom,dRect.bottom / m_MapZoom };

	Gdiplus::Graphics graphics(dDC);

	m_MapImage.Draw(dDC, dsRect, sRect);
	ReleaseDC(pDC);
	//小图
	pDC = m_MiniMap.GetDC();
	dDC = pDC->GetSafeHdc();
	m_MiniMap.GetWindowRect(&dRect);
	m_MiniMap.ScreenToClient(&dRect);
	m_MapImage.Draw(dDC, dRect, sRect);
	ReleaseDC(pDC);
}


void NoPackedPict_Dlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	if (isMousePtIn(point, (CWnd*)&m_Map))
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


