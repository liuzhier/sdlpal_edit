#pragma once
#ifndef CMAPLIST
#define CMAPLIST


class CMapList :public CListCtrl
{
	DECLARE_DYNAMIC(CMapList)
protected:
	DECLARE_MESSAGE_MAP()

public:

	afx_msg void OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
};



#endif // !CMAPLIST
