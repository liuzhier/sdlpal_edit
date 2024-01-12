// CsListCtrl.cpp: 实现文件
//

//#define _AFXDLL

#include "stdafx.h"
//#include "pch.h"
#include "s.h"
#include "CsListCtrl.h"
#include <string>
#include "sDlg.h"


// CsListCtrl

IMPLEMENT_DYNAMIC(CsListCtrl, CListCtrl)

CsListCtrl::CsListCtrl()
{

}

CsListCtrl::~CsListCtrl()
{
}


BEGIN_MESSAGE_MAP(CsListCtrl, CListCtrl)
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_CHAR()
END_MESSAGE_MAP()



// CsListCtrl 消息处理程序




void CsListCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CListCtrl::OnKillFocus(pNewWnd);
	// TODO: 在此处添加消息处理程序代码
	ShowWindow(FALSE);
}


void CsListCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CListCtrl::OnSetFocus(pOldWnd);

	// TODO: 在此处添加消息处理程序代码
	SetWindowPos(this, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}


void CsListCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	
	//发送信息传递选择值
	
	ShowWindow(FALSE);
	CListCtrl::OnLButtonDown(nFlags, point);
	//
	int  nRow = GetNextItem(-1, LVIS_SELECTED);
	int item{0};
	CString Ls = GetItemText(nRow,2);
	std::string s = CStringA(Ls);
	sscanf_s(s.c_str(), "%x", &item);
	if (pOwner && IsWindow(*pOwner))
		::SendMessage(pOwner->m_hWnd, WM_GRID_CELL_CHARGE, item, 0);
}


void CsListCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nChar == 27)
		ShowWindow(FALSE);
	CListCtrl::OnChar(nChar, nRepCnt, nFlags);
}

void CsListCtrl::OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	if (pNMLV->uNewState == 3)
		::SendMessage(GetParent()->m_hWnd, WM_LIST_SELECTED_ROW, pNMLV->iItem, 0);
	*pResult = 0;
}
