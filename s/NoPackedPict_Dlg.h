#ifndef NOPACKEDPICT_DLG_H
#define NOPACKEDPICT_DLG_H

#pragma once
#include "stdafx.h"
#include "afxdialogex.h"
#include "CsListCtrl.h"
#include <vector>
#include "CMapList.h"
#include "cMap_Dlg.h"
#include "PackedPict_Dlg.h"

class NoPackedPict_Dlg : public PackedPict_Dlg
{
	DECLARE_DYNAMIC(NoPackedPict_Dlg)

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

protected:
	virtual BOOL makeList();
public:
	NoPackedPict_Dlg(CWnd* pParent, OPENFILENAME* openfilename);
	virtual ~NoPackedPict_Dlg();

	afx_msg void ObjectPopMenuRetuen(UINT msg);
	afx_msg LRESULT OnListSelectedRow(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};

#endif// NOPACKEDPICT_DLG_H

