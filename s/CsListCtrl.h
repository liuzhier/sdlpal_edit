#pragma once


// CsListCtrl

class CsListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CsListCtrl)

public:
	CsListCtrl();
	virtual ~CsListCtrl();
	VOID SetOwner(CWnd* p) { pOwner = p; };
private:
	CWnd* pOwner = 0;
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
};


