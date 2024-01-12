// cMap_Dlg.cpp: 实现文件
//
//#define _AFXDLL

#include "stdafx.h"
#include "s.h"
#include "afxdialogex.h"
#include "cMap_Dlg.h"
//#include "CMapList.h"
#include "../main/palgpgl.h"
#include "sDlg.h"
#include "cpalmapedit.h"
#include "CDeployOnTheMap.h"
#include "sdlg.h"
#include "CTestData.h"
// cMap_Dlg 对话框


IMPLEMENT_DYNAMIC(CMap_Dlg, CDialogEx)

#define Tile_B(d)( d & 0xff |((d >> 4) & 0x100))
#define Tile_T(d) ( ( (d >> 16) & 0xff)|( (d>>20) & 0x100) ) 

CMap_Dlg::CMap_Dlg(CWnd* pParent, int blogo)
	: CDialogEx(IDD_MAP_DLG, pParent)
	, pMapClass(nullptr)
	, m_Flags(0b0111)
	, m_nMap(0)
	, m_MapZoom(5.5)
	, m_MapRect({ 0 })
	, m_MapShowSize({ 0 })
	, m_OldPt(0, 0)
	, lyOrigin(0)
	, lxOrigin(0)
	, logo(blogo)
	, m_nCscene(0)
{
	//
	//if (blogo)m_Flags = 0b0111;
}

CMap_Dlg::~CMap_Dlg()
{
	if (pMapClass)
		delete pMapClass;
	pMapClass = nullptr;
}

void CMap_Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_LIST2, m_List);
	DDX_Control(pDX, IDC_STATIC2, m_Map);
	DDX_Control(pDX, IDC_STATIC_MINI_MAP, m_MiniMap);
	DDX_Control(pDX, IDC_SCROLLBAR1, m_vScrollBar);
	DDX_Control(pDX, IDC_SCROLLBAR2, m_hScrollBar);
	DDX_Control(pDX, IDC_STATIC3, m_TextStatic);
	
}


BEGIN_MESSAGE_MAP(CMap_Dlg, CDialogEx)
	ON_COMMAND_RANGE(WM_MAP_POPMENU,WM_MAP_POPMENU_END, &CMap_Dlg::MapPopMenuRetuen)
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
	ON_MESSAGE(WM_LIST_SELECTED_ROW, &CMap_Dlg::OnListSelectedRow)
END_MESSAGE_MAP()


// cMap_Dlg 消息处理程序


BOOL CMap_Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	
	RECT s_Rect;
	GetParent()->GetWindowRect(&s_Rect);

	MoveWindow(&s_Rect, TRUE);

	SetTimer(1, 100, NULL);
	//TestData &s = ((CsDlg*)GetParent())->pTestData[0];
	if (!logo)
		pMapClass = new CPalMapEdit(this);
	else
		pMapClass = new CDeployOnTheMap(this);

	if (pMapClass->PAL_LoadMap(m_nMap) == -1)
	{
		//失败返回
		PostMessage(WM_CLOSE, 0, 0);
		return FALSE;
	}
	CString s_Str;
	if (!logo)
		s_Str.Format(L"编辑地图  %3.3d", m_nMap);
	else
	{
		s_Str.Format(L"在地图上部署 场景 %3d,地图 %3d ", m_nCscene, m_nMap);
	}
	SetWindowText(s_Str.GetBuffer());

	//生成大图
	pMapClass->MakeMapImage(m_MapImage, m_Flags);
	//初始化tile列表
	m_ImageList.Create(IMAGE_X, IMAGE_Y, ILC_COLOR16 | ILC_MASK, 0, 0);

	pMapClass->Initializes_Tilelist(m_ImageList,m_List);

	SCROLLINFO scroll;
	m_vScrollBar.GetScrollInfo(&scroll);
	scroll.nMax = 1024;
	scroll.nPos = 1000;
	m_vScrollBar.SetScrollInfo(&scroll);
	m_hScrollBar.SetScrollInfo(&scroll);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CMap_Dlg::OnSize(UINT nType, int cx, int cy)
{

	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码

	if (m_List.GetSafeHwnd() == NULL)
		return;
	m_TextStatic.MoveWindow(4, cy - 30, cx - 8, 30);
	cy -= 30;
	m_List.MoveWindow(4, 4, 256, cy - 260);
	m_Map.MoveWindow(260, 4, cx - 260 - 30, cy - 8 - 30);
	m_Map.ShowWindow(TRUE);
	m_MiniMap.MoveWindow(4, cy - 260, 256, 256);
	m_vScrollBar.MoveWindow(cx - 30, 4, 28, cy - 30);
	//m_vScrollBar.ShowWindow(TRUE);
	m_hScrollBar.MoveWindow(260, cy - 28, cx - 290, 28);
	//m_hScrollBar.ShowWindow(TRUE);
}



void CMap_Dlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	ClientToScreen(&point);
	if (isMousePtIn(point, (CWnd*)&m_Map))
	{
		MapSetPoint(point);
	}
	CDialogEx::OnLButtonDown(nFlags, point);
}


void CMap_Dlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CDialogEx::OnPaint()

	if (!m_Map.GetSafeHwnd() || !m_MiniMap.GetSafeHwnd())
		return;
	if (m_MapImage.IsNull())
		return;

	CDC* pDC = m_Map.GetDC();
	HDC  dDC = pDC->GetSafeHdc();
	HDC  sDC = m_MapImage.GetDC();
	RECT dRect;
	//大地图
	m_Map.GetWindowRect(&dRect);
	m_Map.ScreenToClient(&dRect);
	//中间表
	CImage midimage;
	midimage.Create(dRect.right, dRect.bottom, 24);
	HDC middc = midimage.GetDC();
	RECT sRect = { lxOrigin , lyOrigin , m_MapImage.GetWidth() / m_MapZoom + lxOrigin,
		m_MapImage.GetHeight() / m_MapZoom + lyOrigin };
	//大地图表向中间表写
	m_MapImage.Draw(middc, dRect, sRect);
	if ((pMapClass->fixedpoint.x || pMapClass->fixedpoint.y) )
	{
		//检查固定点是否在显示范围内
		CRect cr = sRect;

		int wy = cr.Height()/4,wx = cr.Width()/4;
		cr -= CRect(wx, wy, wx , wy );

		if (!cr.PtInRect(pMapClass->fixedpoint))
		{
			INT w = (sRect.right - sRect.left);
			INT xpos = pMapClass->fixedpoint.x - w / 2;
			OnHScroll(SB_THUMBPOSITION, 1000 - xpos / 2, &m_hScrollBar);
			INT h = (sRect.bottom - sRect.top);
			INT ypos = pMapClass->fixedpoint.y - h / 2;
			OnVScroll(SB_THUMBPOSITION, 1000 - ypos / 2, &m_vScrollBar);
		}
	}

	if (!m_NoDrawPoint && m_Times & 4 ) {
		//在上次点击处画棱型
		DOUBLE bx = m_MapImage.GetWidth() / (double)dRect.right / m_MapZoom,
			by = m_MapImage.GetHeight() / (double)dRect.bottom / m_MapZoom;
		pMapClass->BoundingLiteBox(&midimage, m_MapSelectRect.Width() / bx
			, m_MapSelectRect.Height() / by
			, m_MapSelectRect.left / bx - lxOrigin / bx
			, m_MapSelectRect.top / by - lyOrigin / by
			, RGB(255, 255, 0) , 4);
	}
	if ((pMapClass->fixedpoint.x ) && (m_Times & 4))
	{
		//标注固定点
		DOUBLE bx = m_MapImage.GetWidth() / (double)dRect.right / m_MapZoom,
			by = m_MapImage.GetHeight() / (double)dRect.bottom / m_MapZoom;
		CPoint p = { pMapClass->fixedpoint.x - 16, pMapClass->fixedpoint.y - 8 };
		pMapClass->BoundingLiteBox(&midimage, 32.0 / bx
			, 16.0 / by
			, p.x / bx - lxOrigin / bx
			, p.y / by - lyOrigin / by
			, RGB(255, 255, 255));
	}
	if (pMapClass->TriggerMode > 0 && m_Times & 4)
	{
		//标注触发区域
		DOUBLE bx = m_MapImage.GetWidth() / (double)dRect.right / m_MapZoom,
			by = m_MapImage.GetHeight() / (double)dRect.bottom / m_MapZoom;
		int dx = pMapClass->fixedpoint.x, dy = pMapClass->fixedpoint.y;
		int w = ((pMapClass->TriggerMode & 3) << 6) + 32;
		int h = ((pMapClass->TriggerMode & 3) << 5) + 16;
		dx -= w >> 1;
		dy -= h >> 1;
		pMapClass->BoundingLiteBox(&midimage, w / bx
			, h / by
			, dx / bx - lxOrigin / bx
			, dy / by - lyOrigin / by
			, pMapClass->TriggerMode & 4 ? RGB(0, 0, 255) : RGB(0, 255, 0), 3);
	}
	//将中间表画到屏幕上
	::BitBlt(dDC, 0, 0, dRect.right, dRect.bottom, middc, 0, 0, SRCCOPY);
	midimage.ReleaseDC();
	midimage.Destroy();
	ReleaseDC(pDC);

	//小地图
	pDC = m_MiniMap.GetDC();
	dDC = pDC->GetSafeHdc();
	m_MiniMap.GetWindowRect(&dRect);
	m_MiniMap.ScreenToClient(&dRect);
	RECT svRect{ 0,0,2048,2048 };
	m_MapImage.Draw(dDC, dRect, svRect);
	//::StretchBlt(dDC, 4, 4, dRect.right-8, dRect.bottom-8, sDC, 0, 0, 
		//m_MapImage.GetWidth(), m_MapImage.GetHeight(), SRCCOPY);
	//在小地图上画大地图显示范围
	{
		
		sRect = { sRect.left >> 3,sRect.top >> 3,sRect.right >> 3,sRect.bottom >> 3 };
		sRect.right = min(sRect.right, 250);
		POINT pt[] = { {sRect.left,sRect.top},{sRect.right,sRect.top},
			{sRect.right,sRect.bottom},{sRect.left,sRect.bottom} };
		pDC->MoveTo(pt[3]);
		//画线
		CPen newpen;
		newpen.CreatePen(PS_SOLID, 1, RGB(255,255,255));
		CPen* oldpen = (pDC->SelectObject(&newpen));
		//画线
		for (int n = 0; n < 4; n++)
			pDC->LineTo(pt[n].x, pt[n].y);
		newpen.DeleteObject();
		pDC->SelectObject(&oldpen);
	}

	m_MapImage.ReleaseDC();
	ReleaseDC(pDC);

	{
		//将有关信息输出到信息栏
		CString s, s1;
		pMapClass->get_Msg_Str(s, m_MapSelectedPos, m_TileSelected, m_ListSelectRow,m_Flags);
		m_TextStatic.GetWindowText(s1);
		if (s != s1)
			m_TextStatic.SetWindowText(s);
	}
}


void CMap_Dlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	//重画
	CRect rect;
	GetClientRect(&rect);
	PostMessage(WM_SIZE, (WPARAM)SIZE_RESTORED, MAKELPARAM(rect.Width(), rect.Height()));
	PostMessage(WM_PAINT);
	m_Times++;
	CDialogEx::OnTimer(nIDEvent);
}


void CMap_Dlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//return;
	int ret;
	if (isChanged())
	{
		ret = ::MessageBox(AfxGetApp()->m_pMainWnd->m_hWnd, _T("保存吗？(Y) 保存，(N)不保存,取消 返回"),
			_T("地图已经修改！"), MB_YESNOCANCEL);
		if (ret == IDCANCEL)
		{
			m_Map.SetFocus();
			return;
		}
		if (ret == IDYES)
		{
			//保存地图
			pMapClass->SaveMapTiles();
		}
	}
	KillTimer(1);
	CDialogEx::OnClose();
}


void CMap_Dlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	SCROLLINFO scroll;
	m_vScrollBar.GetScrollInfo(&scroll);
	scroll.nMin = 0;
	scroll.nMax = 1024;
	scroll.nPage = 20;
	switch (nSBCode)
	{
	case SB_LINEUP:
		scroll.nPos -= 2;
		break;
	case SB_LINEDOWN:
		scroll.nPos += 2;
		break;
	case SB_PAGEUP:
		scroll.nPos -= 20;
		break;
	case SB_PAGEDOWN:
		scroll.nPos += 20;
		break;
	case SB_THUMBPOSITION   ://4拖动后滚动条滑块被释放
		scroll.nPos = nPos;
		break;
	case SB_THUMBTRACK   :   //5滚动条滑块被拖动
		scroll.nPos = nPos;
		break;
	case SB_ENDSCROLL:
	default:
		break;
	}
	lyOrigin = (scroll.nMax - scroll.nPos) * 2 * lOriginZoom - 40;
	m_vScrollBar.SetScrollInfo(&scroll);
	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CMap_Dlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	SCROLLINFO scroll;
	m_hScrollBar.GetScrollInfo(&scroll);
	scroll.nMin = 0;
	scroll.nMax = 1024;
	scroll.nPage = 20;
	switch (nSBCode)
	{
	case SB_LINEUP:
		scroll.nPos -= 2;
		break;
	case SB_LINEDOWN:
		scroll.nPos += 2;
		break;
	case SB_PAGEUP:
		scroll.nPos -= 20;
		break;
	case SB_PAGEDOWN:
		scroll.nPos += 20;
		break;
	case SB_THUMBPOSITION://4拖动后滚动条滑块被释放
		scroll.nPos = nPos;
		break;
	case SB_THUMBTRACK:   //5滚动条滑块被拖动
		scroll.nPos = nPos;
		break;
	case SB_ENDSCROLL:
	default:
		break;
	}
	if (scroll.nPos < scroll.nMin)scroll.nPos = scroll.nMin;
	if (scroll.nPos > scroll.nMax)scroll.nPos = scroll.nMax;
	lxOrigin = (scroll.nMax - scroll.nPos) * 2 * lOriginZoom - 40;
	m_hScrollBar.SetScrollInfo(&scroll);

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}



BOOL CMap_Dlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//放大缩小地图
	if (isMousePtIn(pt, &m_Map))
	{
		while (zDelta > 0 && m_MapZoom < 12.0)
		{
			m_MapZoom += 0.1;
			zDelta -= 120;
		}
		while (zDelta < 0 && m_MapZoom > 1.0)
		{
			m_MapZoom -= 0.1;
			zDelta += 120;
		}
	}
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}




BOOL CMap_Dlg::isMousePtIn(const CPoint pt,CWnd * wp)
{
	// TODO: 在此处添加实现代码.
	if (!wp || !wp->GetSafeHwnd())
		return FALSE;
	CRect rt;
	wp->GetWindowRect(&rt);
	return rt.PtInRect(pt);
}


void CMap_Dlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CDialogEx::OnLButtonUp(nFlags, point);
}


void CMap_Dlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nFlags == MK_LBUTTON)
	{
		if (int mt = (m_OldPt.x - point.x))
		{
			SendMessage(WM_HSCROLL, mt < 0 ? SB_LINERIGHT : SB_LINELEFT, 0);
		}
		if (int mt = (m_OldPt.y - point.y))
		{
			SendMessage(WM_VSCROLL, mt < 0 ? SB_LINERIGHT : SB_LINELEFT, 0);
		}
		m_OldPt = point;
	}
	CDialogEx::OnMouseMove(nFlags, point);
}


void CMap_Dlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
    //调整最小高度与宽度,如果需要的话
	lpMMI->ptMinTrackSize.x = 1300;
	lpMMI->ptMinTrackSize.y = 800;

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}



void CMap_Dlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_OldPt = point;
	ClientToScreen(&point);
	if (isMousePtIn(point, (CWnd*)&m_Map))
	{
		MapSetPoint(point);
		point = m_OldPt;
		ClientToScreen(&point);
		//生成菜单
		CMenu pMenu;
		pMenu.CreatePopupMenu();
		for (int n = 0; pMapClass->get_Menu_Str()[n]; n++)
		{
			UINT32 flag = MF_STRING |(( m_Flags & (1 << n)) ? MF_CHECKED : MF_UNCHECKED);
			CString s;
			s.Format(pMapClass->get_Menu_Str()[n],	
				pMapClass->ListRowToReturn(m_ListSelectRow));
			pMenu.AppendMenu(flag, WM_MAP_POPMENU + n, s);
		}
		SetForegroundWindow();
		pMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON , point.x, point.y, this);
	}
	CDialogEx::OnRButtonDown(nFlags, point);
}




//处理弹出菜单返回值
void CMap_Dlg::MapPopMenuRetuen(UINT msg)
{
	DWORD oldflag = m_Flags;
	msg -= WM_MAP_POPMENU;
	int ret = pMapClass->MapPopMenuRetuen(msg, m_Flags);
	if (ret == 0)
	{
		MessageBox(L"没有指定的显示图层，返回");
		m_Flags = oldflag;
		return;
	};
	if (ret == -1)
		//重新生成大地图
		pMapClass->MakeMapImage(m_MapImage, m_Flags);
}




// 使用鼠标点，生成相关操作
//输入：鼠标位置，返回的dx dy 大地图中的点
VOID CMap_Dlg::MapSetPoint(CPoint &pt)
{
	// TODO: 在此处添加实现代码.
	int dx, dy;
	RECT sRect;
	m_Map.SetFocus();
	m_Map.GetWindowRect(&sRect);
	m_Map.ScreenToClient(&sRect);
	m_Map.ScreenToClient(&pt);
	DOUBLE bx = m_MapImage.GetWidth() / (double)sRect.right,
		by = m_MapImage.GetHeight() / (double)sRect.bottom;
	dx = (pt.x * bx) / m_MapZoom + lxOrigin;
	dy = (pt.y * by) / m_MapZoom + lyOrigin;
	SIZE size = { 32,16 };
	int x, y, h;
	m_TileSelected = pMapClass->FindTileCoor(dx, dy, &x, &y, &h);
	m_MapSelectedPos = PAL_XY((x << 1) + h, y);
	pMapClass->GetMapTilePoint(x, y, h, dx, dy);
	CPoint s_point(dx, dy);
	m_MapSelectRect = CRect(s_point, size);

	return VOID();
}


void CMap_Dlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//双击进行编辑
	ClientToScreen(&point);
	if (isMousePtIn(point, (CWnd*)&m_Map) )
	{
		pMapClass-> UpdateTile(m_Flags);
		MapSetPoint(point);
	}

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CMap_Dlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nChar == VK_ESCAPE || nChar == VK_RETURN)
	{
		PostMessage(WM_CLOSE);
	}
	if (GetKeyState(VK_CONTROL) && (nChar == 'Z'))
	{
		//处理撤销事件
		if (pMapClass->doUndo())
		{
			pMapClass->MakeMapImage(m_MapImage,m_Flags);
		};
	}

	CDialogEx::OnKeyDown(nChar, nRepCnt, nFlags);
}


BOOL CMap_Dlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类

	if (pMsg->message == WM_KEYDOWN)
	{
		//只处理键盘信息
		SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
		return FALSE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

BOOL CMap_Dlg::isChanged() 
	{ return pMapClass && pMapClass->GetUndoSize() != 0; }

LRESULT CMap_Dlg::OnListSelectedRow(WPARAM wParam, LPARAM lParam)
{
	m_ListSelectRow = wParam;
	pMapClass->MakeMapImage(m_MapImage, m_Flags);
	return 0;
}

//检测压缩文件是否使用的是YJ_1
//不是，返回0 出错 - 1 是返回1
INT  CMap_Dlg::isUseYJ_1(FILE* f) 
{
	//以下检测压缩属性
		//
		// Find the first non-empty sub-file
		//
	//int	data_size{};
	uint8_t* data{};
	int count = Pal->PAL_MKFGetChunkCount(f), j = 0, size{};
	while (j < count && (size = Pal->PAL_MKFGetChunkSize(j, f)) < 4) j++;
	if (j >= count)
	{
		//出错了 ，空的文件 
		PostMessage(WM_CLOSE);
		CString* ps = new CString(L"文件打开错，返回");
		AfxGetMainWnd()->PostMessage(WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
		return -1;
	}
	//
	// Read the content and check the compression signature
	// Note that this check is not 100% correct, however in incorrect situations,
	// the sub-file will be over 784MB if uncompressed, which is highly unlikely.
	//
	data = (uint8_t*)malloc(size);
	if (data == nullptr)
		return -1;
	Pal->PAL_MKFReadChunk(data, size, j, f);

	INT isYJ_1{};
	if (data[0] == 'Y' && data[1] == 'J' && data[2] == '_' && data[3] == '1')
		isYJ_1 = TRUE;
	free(data);
	return isYJ_1;
}
