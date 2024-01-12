#include "stdafx.h"
#include "CMapList.h"

IMPLEMENT_DYNAMIC(CMapList, CListCtrl)

BEGIN_MESSAGE_MAP(CMapList, CListCtrl)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CMapList::OnLvnItemchanged)
END_MESSAGE_MAP()

//消息处理程序

void CMapList::OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	if (pNMLV->uNewState == 3)
		//SelectRow = pNMLV->iItem;
		::PostMessage(GetParent()->m_hWnd, WM_LIST_SELECTED_ROW, pNMLV->iItem,0);
	*pResult = 0;
}

