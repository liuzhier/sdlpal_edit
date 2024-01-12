#pragma once
#include "sdlg.h"

class CListScriptEntry :public CDialogEx
{
	DECLARE_DYNAMIC(CListScriptEntry)

public:
	CListScriptEntry(CWnd* pParent, WORD scriptEntry);

	virtual BOOL OnInitDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

public:
	DECLARE_MESSAGE_MAP()

public:
	CsListCtrl m_List;
	WORD   InScript{};
private:
	afx_msg void OnSize(UINT nType, int cx, int cy);

public:
	afx_msg void OnPaint();
};

